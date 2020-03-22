use crate::config::{read_conf, Config};
use crate::data::Data;
use crate::error::{Error, Result};
use crate::home_assistant::HomeAssistant;
use crate::radio::Radio;
use crate::util::{Receiver, Shared, PAYLOAD_ON};
use crate::vutbr::VutBr;
use async_std::sync::{Arc, Mutex};
use futures::{select, FutureExt, StreamExt};
use paho_mqtt::Message;
use std::convert::TryFrom;
use std::result::Result as StdResult;

pub struct Proxy {
    conf: Shared<Config>,
    shutdown: Receiver<bool>,
    radio: Radio,
    mqtt: HomeAssistant,
    vutbr: VutBr,
}

impl Proxy {
    pub async fn new(conf_path: &str, shutdown: Receiver<bool>) -> Result<Self> {
        let conf = new_shared!(read_conf(conf_path).await?);
        let radio = Radio::new(conf.clone())?;
        let mqtt = HomeAssistant::new(conf.clone()).await?;
        let vutbr = VutBr::new().await?;
        Ok(Proxy {
            conf,
            shutdown,
            radio,
            mqtt,
            vutbr,
        })
    }

    pub async fn main_loop(&mut self) -> Result<()> {
        let mut shutdown = (&mut self.shutdown).fuse();
        let mut receiver = self.radio.receiver_channel();
        let mut mqtt_receiver = self.mqtt.stream().fuse();
        loop {
            select! {
                opt = receiver.next().fuse() => match opt {
                    Some(buffer) => {
                        let data = Data::try_from(&buffer[..])?;
                        self.mqtt.update_state(&data).await?;
                        self.vutbr.update_state(&data).await?;
                    },
                    None => error!("Radio channel is closed"),
                },
                option = mqtt_receiver.next().fuse() => {
                    helper_mqtt_config(self.conf.clone(), option).await?
                },
                sig =  shutdown.next().fuse() => match sig {
                    Some(_) => break,
                    None => error!("Shutdown channel is closed, should not happen")
                },
            }
        }
        receiver.close();
        info!("Main loop exit");
        Ok(())
    }
}

async fn helper_mqtt_config(
    shared_conf: Shared<Config>,
    wrapper: Option<StdResult<Option<Message>, ()>>,
) -> Result<()> {
    match wrapper {
        Some(result) => {
            if result.is_err() {
                error!("MQTT stream error");
                return Ok(());
            }
            let message = result
                .map_err(|()| Error::new_result("MQTT message error, hint Result"))?
                .ok_or_else(|| Error::new_option("MQTT message error"))?;
            let mut conf = shared_conf.lock().await;
            let payload = message.payload_str();
            if payload.len() == 1 {
                conf.node_mut()
                    .update_output(message.topic(), payload == PAYLOAD_ON)?;
            }
            debug!("Message {:?}", message);
        }
        None => error!("MQTT channel is closed"),
    }
    Ok(())
}
