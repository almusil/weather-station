use crate::config::Discovery;
use async_std::sync::{Arc, Mutex};
use futures::channel::mpsc;

pub const SPI_DEV: &str = "/dev/spidev0.0";
pub const GPIO_CHIP: &str = "/dev/gpiochip0";
pub const CS_NAME: &str = "chip-select";
pub const INTERRUPT_NAME: &str = "interrupt";
pub const CS_PIN_NUM: u32 = 25;
pub const INTERRUPT_PIN_NUM: u32 = 24;
pub const PAYLOAD_ON: &str = "1";
pub const PAYLOAD_OFF: &str = "0";
pub const MQTT_URI: &str = "tcp://127.0.0.1:1883";
pub const CONF_PATH: &str = "/proxy/conf/config.yaml";
pub const LOG_PATH: &str = "/proxy/log/proxy.log";
pub const LOG_TIME_FORMAT: &str = "%d.%m.%Y %H:%M:%S.%f";
pub const LOG_MODULE_IGNORE: &str = "paho_mqtt";
pub const PACKET_CONFIG: u8 = 0x02;
pub const PACKET_DATA: u8 = 0x08;
pub const BATTERY_CONFIG_TOPIC: &str = "homeassistant/sensor/node/analog_bat/config";
pub const BATTERY_SENSOR: Discovery = Discovery::Sensor {
    name: "Battery",
    state_topic: "node/analog/bat/state",
    unit_of_measurement: "V",
    value_template: "{{ ((float(value) * 3.3 / (2**12 - 1)) / 0.8) | round(3) }}",
};
pub const WEATHER_SENSOR_TEMPLATE: &str = "{{ (float(value) / 100) | round(2) }}";
pub const TEMPERATURE_CONFIG_TOPIC: &str = "homeassistant/sensor/node/temperature/config";
pub const TEMPERATURE_SENSOR: Discovery = Discovery::Sensor {
    name: "Temperature",
    state_topic: "node/analog/temperature/state",
    unit_of_measurement: "°C",
    value_template: WEATHER_SENSOR_TEMPLATE,
};
pub const PRESSURE_CONFIG_TOPIC: &str = "homeassistant/sensor/node/pressure/config";
pub const PRESSURE_SENSOR: Discovery = Discovery::Sensor {
    name: "Pressure",
    state_topic: "node/analog/pressure/state",
    unit_of_measurement: "hPa",
    value_template: WEATHER_SENSOR_TEMPLATE,
};
pub const HUMIDITY_CONFIG_TOPIC: &str = "homeassistant/sensor/node/humidity/config";
pub const HUMIDITY_SENSOR: Discovery = Discovery::Sensor {
    name: "Humidity",
    state_topic: "node/analog/humidity/state",
    unit_of_measurement: "%",
    value_template: WEATHER_SENSOR_TEMPLATE,
};

pub type Shared<T> = Arc<Mutex<T>>;
pub type Receiver<T> = mpsc::UnboundedReceiver<T>;
pub type Sender<T> = mpsc::UnboundedSender<T>;

#[macro_export]
macro_rules! new_shared {
    ($data:expr) => {
        Arc::new(Mutex::new($data))
    };
}
