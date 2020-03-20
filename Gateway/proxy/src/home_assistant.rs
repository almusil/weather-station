use crate::config::{Config, Pin};
use crate::data::Data;
use crate::error::Result;
use crate::util::{
    Shared, BATTERY_SENSOR, HUMIDITY_SENSOR, MQTT_URI, PAYLOAD_OFF, PAYLOAD_ON, PRESSURE_SENSOR,
    TEMPERATURE_SENSOR,
};
use async_std::task::block_on;
use futures::compat::{Future01CompatExt, Stream01CompatExt};
use futures::StreamExt;
use paho_mqtt::{AsyncClient, AsyncClientBuilder, ConnectOptions, Message};
use std::result::Result as StdResult;

macro_rules! mqtt_publish {
    ($mqtt:expr,$topic:expr, $payload:expr) => {
        let msg = Message::new($topic, $payload, 0);
        $mqtt.publish(msg).compat().await?;
    };
}

macro_rules! convert_digital {
    ($num:ident,$digital:ident) => {
        if (1 << $num) & $digital != 0 {
            PAYLOAD_ON
        } else {
            PAYLOAD_OFF
        };
    };
}

pub struct HomeAssistant {
    mqtt: AsyncClient,
    conf: Shared<Config>,
}

impl HomeAssistant {
    pub async fn new(conf: Shared<Config>) -> Result<Self> {
        let mqtt = AsyncClientBuilder::new().server_uri(MQTT_URI).finalize();
        mqtt.connect(ConnectOptions::new()).compat().await?;
        let result = HomeAssistant { mqtt, conf };
        result.init_topics().await?;
        Ok(result)
    }

    pub async fn update_state(&self, data: &Data) -> Result<()> {
        debug!("Data for MQTT {:?}", data);
        let conf = self.conf.lock().await;
        let digital = data.gpio_value;
        for pin in conf.node().digital() {
            let (topic, num) = pin.topic_number_tuple();
            mqtt_publish!(self.mqtt, topic, convert_digital!(num, digital));
        }
        for pin in conf.node().analog() {
            if !pin.enabled {
                continue;
            }
            let (topic, num) = pin.topic_number_tuple();
            mqtt_publish!(self.mqtt, topic, data.adc_value[num as usize].to_string());
        }
        mqtt_publish!(
            self.mqtt,
            BATTERY_SENSOR.state_topic(),
            data.bat_value.to_string()
        );
        mqtt_publish!(
            self.mqtt,
            TEMPERATURE_SENSOR.state_topic(),
            data.temperature.to_string()
        );
        mqtt_publish!(
            self.mqtt,
            PRESSURE_SENSOR.state_topic(),
            data.pressure.to_string()
        );
        mqtt_publish!(
            self.mqtt,
            HUMIDITY_SENSOR.state_topic(),
            data.humidity.to_string()
        );
        Ok(())
    }

    pub fn stream(&mut self) -> impl StreamExt<Item = StdResult<Option<Message>, ()>> {
        self.mqtt.get_stream(50).compat()
    }

    async fn init_topics(&self) -> Result<()> {
        let conf = self.conf.lock().await;
        for (topic, discovery) in &conf.node().discovery() {
            let json = serde_json::to_string(discovery)?;
            debug!("Trying to configure {}, {}", topic, json);
            self.mqtt
                .publish(Message::new(topic, json, 0))
                .compat()
                .await?;
        }
        for topic in conf.node().subscribe_topics() {
            self.mqtt.subscribe(topic, 0).compat().await?;
        }
        Ok(())
    }

    async fn drop_topics(&self) -> Result<()> {
        let conf = self.conf.lock().await;
        for (topic, _) in &conf.node().discovery() {
            debug!("Trying to deconfigure {}", topic);
            self.mqtt
                .publish(Message::new(topic, Vec::new(), 0))
                .compat()
                .await?;
        }
        for topic in conf.node().subscribe_topics() {
            self.mqtt.unsubscribe(topic).compat().await?;
        }
        Ok(())
    }
}

impl Drop for HomeAssistant {
    fn drop(&mut self) {
        let result = block_on(self.drop_topics());
        if let Err(err) = result {
            eprintln!("{}", err);
            error!("{:?}", err);
        }
    }
}
