use crate::error::Error;
use bincode::deserialize;
use serde::Deserialize;
use std::convert::TryFrom;

#[derive(Deserialize, Debug)]
pub struct Data {
    pub packet_type: u8,
    pub gpio_value: u8,
    pub adc_value: [u16; 3],
    pub bat_value: u16,
    pub temperature: i16,
    pub pressure: u32,
    pub humidity: u16,
}

impl TryFrom<&[u8]> for Data {
    type Error = Error;

    fn try_from(bytes: &[u8]) -> Result<Self, Self::Error> {
        deserialize(bytes).map_err(|err| err.into())
    }
}
