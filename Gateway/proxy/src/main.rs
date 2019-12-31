use crate::home_assistant::HomeAssistant;
use crate::radio::Radio;
use crate::util::{Receiver, Sender, Shared, PAYLOAD_ON};
use async_std::sync::{Arc, Mutex};
use async_std::task::block_on;
use config::{read_conf, Config};
use ctrlc;
use futures::channel::mpsc;
use futures::{select, FutureExt, SinkExt, StreamExt};
use paho_mqtt::Message;

mod config;

#[macro_use]
mod util;
mod home_assistant;
mod radio;

#[async_std::main]
async fn main() {
    let (s, r) = mpsc::unbounded();
    let sender = new_shared!(s);
    configure_ctrlc_handler(sender);

    let mut proxy = Proxy::new("conf/config.yaml", r).await;
    proxy.main_loop().await;

    println!("Exiting");
}

fn configure_ctrlc_handler(shared_sender: Shared<Sender<bool>>) {
    ctrlc::set_handler(move || {
        println!("Got CTRL+C");
        let mut sender = block_on(shared_sender.lock());
        block_on(sender.send(true)).unwrap();
    })
    .unwrap();
}

struct Proxy {
    conf: Shared<Config>,
    shutdown: Receiver<bool>,
    radio: Radio,
    mqtt: HomeAssistant,
}

impl Proxy {
    async fn new(conf_path: &str, shutdown: Receiver<bool>) -> Self {
        let conf = new_shared!(read_conf(conf_path).await);
        let radio = Radio::new(conf.clone());
        let mqtt = HomeAssistant::new(conf.clone()).await;
        Proxy {
            conf,
            shutdown,
            radio,
            mqtt,
        }
    }

    async fn main_loop(&mut self) {
        let mut shutdown = (&mut self.shutdown).fuse();
        let mut receiver = self.radio.receiver_channel();
        let mut mqtt_receiver = self.mqtt.stream().fuse();
        loop {
            select! {
                opt = receiver.next().fuse() => match opt {
                    Some(buffer) => self.mqtt.update_state(&buffer).await,
                    None => println!("Radio channel is closed"),
                },
                option = mqtt_receiver.next().fuse() => {
                    helper_mqtt_config(self.conf.clone(), option).await
                },
                sig =  shutdown.next().fuse() => match sig {
                    Some(_) => break,
                    None => println!("Shutdown channel is closed, should not happen")
                },
            }
        }
        receiver.close();
        println!("Main loop exit");
    }
}

async fn helper_mqtt_config(
    shared_conf: Shared<Config>,
    wrapper: Option<Result<Option<Message>, ()>>,
) {
    match wrapper {
        Some(result) => {
            if result.is_err() {
                println!("MQTT stream error");
                return;
            }
            let message = result.unwrap().unwrap();
            let mut conf = shared_conf.lock().await;
            let payload = message.payload_str();
            if payload.len() == 1 {
                conf.node_mut()
                    .update_output(message.topic(), payload == PAYLOAD_ON)
            }
            println!("Message {:?}", message);
        }
        None => println!("MQTT channel is closed"),
    }
}
