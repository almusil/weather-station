use crate::config::{Config, Pin};
use crate::error::Result;
use crate::util::{Shared, BATTERY_STATE_TOPIC, MQTT_URI, PAYLOAD_OFF, PAYLOAD_ON};
use async_std::task::block_on;
use futures::compat::{Future01CompatExt, Stream01CompatExt};
use futures::StreamExt;
use paho_mqtt::{AsyncClient, AsyncClientBuilder, ConnectOptions, Message};
use std::result::Result as StdResult;

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

    pub async fn update_state(&self, buffer: &[u8]) -> Result<()> {
        println!("Data for MQTT {:?}", buffer);
        let conf = self.conf.lock().await;
        let digital = buffer[1];
        for pin in conf.node().digital() {
            let (topic, num) = pin.topic_number_tuple();
            let payload = if (1 << num) & digital != 0 {
                PAYLOAD_ON
            } else {
                PAYLOAD_OFF
            };

            let msg = Message::new(topic, payload, 0);
            println!("Update status {}", msg);
            self.mqtt.publish(msg).compat().await?;
        }
        for pin in conf.node().analog() {
            if !pin.enabled {
                continue;
            }
            let (topic, num) = pin.topic_number_tuple();
            let payload =
                u16::from_be_bytes([buffer[2 + 2 * num as usize], buffer[3 + 2 * num as usize]])
                    .to_string();
            let msg = Message::new(topic, payload, 0);
            println!("Update status {}", msg);
            self.mqtt.publish(msg).compat().await?;
        }
        let msg = Message::new(
            BATTERY_STATE_TOPIC,
            u16::from_be_bytes([buffer[8], buffer[9]]).to_string(),
            0,
        );
        println!("Update bat {}", msg);
        self.mqtt.publish(msg).compat().await?;
        Ok(())
    }

    pub fn stream(&mut self) -> impl StreamExt<Item = StdResult<Option<Message>, ()>> {
        self.mqtt.get_stream(50).compat()
    }

    async fn init_topics(&self) -> Result<()> {
        let conf = self.conf.lock().await;
        for (topic, discovery) in &conf.node().discovery() {
            let json = serde_json::to_string(discovery)?;
            println!("Trying to configure {}, {}", topic, json);
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
            println!("Trying to deconfigure {}", topic);
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
            println!("{}", err);
        }
    }
}
