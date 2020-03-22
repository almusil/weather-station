use crate::data::Data;
use crate::error::Result;
use crate::util::{PAYLOAD_OFF, PAYLOAD_ON, VUTBR_TOPIC, VUTBR_URI_ENV};
use futures::compat::Future01CompatExt;
use paho_mqtt::{AsyncClient, AsyncClientBuilder, ConnectOptions, Message};
use serde::Serialize;

macro_rules! create_sensor {
    ($name:expr,$value:expr) => {
        Sensor {
            name: $name,
            value: $value.to_string(),
        }
    };
}

#[derive(Serialize, Debug)]
struct Sensor {
    name: String,
    value: String,
}

pub struct VutBr {
    mqtt: AsyncClient,
}

impl VutBr {
    pub async fn new() -> Result<Self> {
        let server = std::env::var(VUTBR_URI_ENV)?;
        let mqtt = AsyncClientBuilder::new().server_uri(&server).finalize();
        mqtt.connect(ConnectOptions::new()).compat().await?;
        let result = VutBr { mqtt };
        Ok(result)
    }

    pub async fn update_state(&self, data: &Data) -> Result<()> {
        let mut vec = Vec::<Sensor>::with_capacity(15);
        let digital = data.gpio_value;
        for num in 0..8 {
            vec.push(create_sensor!(
                format!("Digital{}", num),
                convert_digital!(num, digital)
            ));
        }
        for num in 0usize..3 {
            vec.push(create_sensor!(
                format!("Analog{}", num),
                data.adc_value[num]
            ));
        }
        let battery = (data.bat_value as f64 * 3.3) / (2u32.pow(12) - 1) as f64 / 0.8;
        vec.push(create_sensor!("Baterie".to_string(), battery));

        let temperature = data.temperature as f64 / 100.;
        vec.push(create_sensor!("Teplota".to_string(), temperature));

        let pressure = data.pressure as f64 / 100.;
        vec.push(create_sensor!("Tlak".to_string(), pressure));

        let humidity = data.humidity as f64 / 100.;
        vec.push(create_sensor!("Vlhkost".to_string(), humidity));

        let payload = serde_json::to_string(&vec)?;
        let msg = Message::new(VUTBR_TOPIC, payload, 0);
        self.mqtt.publish(msg).compat().await?;
        Ok(())
    }
}
