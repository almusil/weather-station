use crate::config::Discovery;
use async_std::sync::{Arc, Mutex};
use futures::channel::mpsc;

pub const SPI_DEV: &str = "/dev/spidev0.0";
pub const GPIO_CHIP: &str = "/dev/gpiochip0";
pub const CS_NAME: &str = "chip-select";
pub const CS_PIN_NUM: u32 = 25;
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
pub const BATTERY_STATE_TOPIC: &str = "node/analog/bat/state";
pub const BATTERY_UNIT: &str = "V";
pub const BATTERY_EXPR: &str = "{{ (float(value) * 3.3 / (2**12 - 1)) | round(3) }}";
pub const BATTERY_SENSOR: Discovery = Discovery::Sensor {
    name: "Battery",
    state_topic: BATTERY_STATE_TOPIC,
    unit_of_measurement: BATTERY_UNIT,
    value_template: BATTERY_EXPR,
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
