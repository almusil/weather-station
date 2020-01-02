use crate::error::{Error, Result};
use crate::util::{BATTERY_CONFIG_TOPIC, BATTERY_SENSOR, PAYLOAD_OFF, PAYLOAD_ON};
use async_std::fs::File;
use futures::AsyncReadExt;
use serde::{Deserialize, Serialize};
use std::collections::HashMap;

#[derive(Debug, Serialize, Deserialize)]
pub struct Config {
    gateway_addr: u8,
    network_id: u8,
    node: Node,
}

impl Config {
    pub fn gateway_addr(&self) -> u8 {
        self.gateway_addr
    }

    pub fn network_id(&self) -> u8 {
        self.network_id
    }

    pub fn node(&self) -> &Node {
        &self.node
    }

    pub fn node_mut(&mut self) -> &mut Node {
        &mut self.node
    }
}

#[derive(Debug, Serialize, Deserialize)]
pub struct Node {
    sleep_time: u16,
    node_addr: u8,
    digital: HashMap<String, DigitalPin>,
    analog: HashMap<String, AnalogPin>,
    #[serde(skip)]
    config_dirty: bool,
}

impl Node {
    pub fn addr(&self) -> u8 {
        self.node_addr
    }

    pub fn is_config_dirty(&self) -> bool {
        self.config_dirty
    }

    pub fn update_output(&mut self, topic: &str, new_state: bool) -> Result<()> {
        let pin = self
            .digital
            .values_mut()
            .find(|digital| {
                if let DigitalPin::Output { command_topic, .. } = digital {
                    return command_topic == topic;
                }
                false
            })
            .ok_or_else(|| Error::new_option("Cannot find pin to update."))?;

        if let DigitalPin::Output { state, .. } = pin {
            *state = new_state;
        }
        self.update_config_dirty(true);
        Ok(())
    }

    pub fn update_config_dirty(&mut self, dirty: bool) {
        self.config_dirty = dirty;
    }

    pub fn to_bytes(&self) -> Vec<u8> {
        let mut bytes = Vec::with_capacity(5);
        bytes.extend_from_slice(&self.sleep_time.to_be_bytes());

        let mut digital_direction = 0;
        let mut digital_value = 0;
        for pin in self.digital.values() {
            let (pin_direction, pin_value) = pin.as_tuple();
            digital_direction |= pin_direction;
            digital_value |= pin_value;
        }
        bytes.push(digital_direction);
        bytes.push(digital_value);

        let mut analog = 0;
        for pin in self.analog.values() {
            analog |= pin.as_byte();
        }
        bytes.push(analog);
        bytes
    }

    pub fn discovery(&self) -> Vec<(String, Discovery)> {
        let mut result = Vec::with_capacity(12);
        for (name, pin) in &self.digital {
            result.push((pin.discovery_topic(), pin.as_discovery(name)));
        }
        for (name, pin) in &self.analog {
            if pin.enabled {
                result.push((pin.discovery_topic(), pin.as_discovery(name)));
            }
        }
        result.push((BATTERY_CONFIG_TOPIC.to_string(), BATTERY_SENSOR));
        result
    }

    pub fn subscribe_topics(&self) -> Vec<&str> {
        self.digital
            .values()
            .map(|pin| match pin {
                DigitalPin::Output { command_topic, .. } => Some(command_topic.as_str()),
                _ => None,
            })
            .filter(Option::is_some)
            .map(Option::unwrap)
            .collect()
    }

    pub fn digital(&self) -> impl Iterator<Item = &DigitalPin> {
        self.digital.values()
    }

    pub fn analog(&self) -> impl Iterator<Item = &AnalogPin> {
        self.analog.values()
    }

    fn init_mqtt_topic(&mut self) {
        for pin in self.digital.values_mut() {
            match pin {
                DigitalPin::Input {
                    number,
                    state_topic,
                } => {
                    *state_topic = format!("node/digital/{}/state", number);
                }
                DigitalPin::Output {
                    number,
                    state_topic,
                    command_topic,
                    ..
                } => {
                    *state_topic = format!("node/digital/{}/state", number);
                    *command_topic = format!("node/digital/{}/set", number);
                }
            }
        }
        for pin in self.analog.values_mut() {
            pin.state_topic = format!("node/analog/{}/state", pin.number);
        }
    }
}

pub trait Pin {
    fn as_discovery<'str, 'pin>(&'pin self, name: &'str str) -> Discovery<'str, 'pin>;
    fn discovery_topic(&self) -> String;
    fn topic_number_tuple(&self) -> (&str, u8);
}

#[derive(Debug, Serialize, Deserialize)]
#[serde(tag = "type")]
pub enum DigitalPin {
    Output {
        number: u8,
        state: bool,
        #[serde(skip)]
        state_topic: String,
        #[serde(skip)]
        command_topic: String,
    },
    Input {
        number: u8,
        #[serde(skip)]
        state_topic: String,
    },
}

impl Pin for DigitalPin {
    fn as_discovery<'str, 'pin>(&'pin self, name: &'str str) -> Discovery<'str, 'pin> {
        match self {
            DigitalPin::Output {
                state_topic,
                command_topic,
                ..
            } => Discovery::Switch {
                name,
                state_topic: &state_topic,
                command_topic: &command_topic,
                payload_on: PAYLOAD_ON,
                payload_off: PAYLOAD_OFF,
            },
            DigitalPin::Input { state_topic, .. } => Discovery::BinarySensor {
                name,
                state_topic: &state_topic,
                payload_on: PAYLOAD_ON,
                payload_off: PAYLOAD_OFF,
            },
        }
    }

    fn discovery_topic(&self) -> String {
        match self {
            DigitalPin::Output { number, .. } => {
                format!("homeassistant/switch/node/digital_{}/config", *number)
            }

            DigitalPin::Input { number, .. } => format!(
                "homeassistant/binary_sensor/node/digital_{}/config",
                *number
            ),
        }
    }

    fn topic_number_tuple(&self) -> (&str, u8) {
        match self {
            DigitalPin::Output {
                number,
                state_topic,
                ..
            } => (&state_topic, *number),
            DigitalPin::Input {
                number,
                state_topic,
                ..
            } => (&state_topic, *number),
        }
    }
}

impl DigitalPin {
    pub fn as_tuple(&self) -> (u8, u8) {
        let mut direction = 0;
        let mut value = 0;
        if let DigitalPin::Output { number, state, .. } = self {
            direction |= 1u8 << number;
            value |= (*state as u8) << number;
        }
        (direction, value)
    }
}

#[derive(Debug, Serialize, Deserialize)]
pub struct AnalogPin {
    pub number: u8,
    pub enabled: bool,
    #[serde(skip)]
    pub state_topic: String,
}

impl Pin for AnalogPin {
    fn as_discovery<'str, 'pin>(&'pin self, name: &'str str) -> Discovery<'str, 'pin> {
        Discovery::Sensor {
            name,
            state_topic: &self.state_topic,
        }
    }

    fn discovery_topic(&self) -> String {
        format!("homeassistant/sensor/node/analog_{}/config", self.number)
    }

    fn topic_number_tuple(&self) -> (&str, u8) {
        (&self.state_topic, self.number)
    }
}

impl AnalogPin {
    pub fn as_byte(&self) -> u8 {
        (self.enabled as u8) << self.number
    }
}

#[derive(Serialize)]
#[serde(untagged)]
pub enum Discovery<'a, 'b> {
    Switch {
        name: &'a str,
        state_topic: &'b str,
        command_topic: &'b str,
        payload_on: &'static str,
        payload_off: &'static str,
    },
    BinarySensor {
        name: &'a str,
        state_topic: &'b str,
        payload_on: &'static str,
        payload_off: &'static str,
    },
    Sensor {
        name: &'a str,
        state_topic: &'b str,
    },
}

pub async fn read_conf(path: &str) -> Result<Config> {
    let mut conf_file = File::open(path).await?;
    let mut conf_content = String::new();
    conf_file.read_to_string(&mut conf_content).await?;
    let mut config = serde_yaml::from_str::<Config>(&conf_content)?;
    config.node.update_config_dirty(true);
    config.node.init_mqtt_topic();
    info!("{:?}", config);
    Ok(config)
}
