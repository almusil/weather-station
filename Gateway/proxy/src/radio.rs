use crate::config::Config;
use crate::error::Result;
use crate::util::{
    Receiver, Shared, CS_NAME, CS_PIN_NUM, GPIO_CHIP, PACKET_CONFIG, PACKET_DATA, SPI_DEV,
};
use async_std::sync::{Arc, Mutex};
use async_std::task::{block_on, spawn_blocking};
use futures::channel::mpsc;
use futures::SinkExt;
use hal::gpio_cdev::{Chip, LineRequestFlags};
use hal::spidev::{SpiModeFlags, SpidevOptions};
use hal::{Delay, Pin, Spidev};
use linux_embedded_hal as hal;
use rfm69::{low_power_lab_defaults, Rfm69};

struct RfmWrapper(Rfm69<Pin, Spidev, Delay>, u8);

impl RfmWrapper {
    fn new(network_id: u8, gateway_addr: u8) -> Result<Self> {
        let spi = configure_spi()?;
        let cs = configure_cs()?;

        let rfm = low_power_lab_defaults(Rfm69::new(spi, cs, Delay), network_id, 433_000_000.0)?;
        Ok(RfmWrapper(rfm, gateway_addr))
    }

    fn receive(&mut self) -> Result<Vec<u8>> {
        let mut buffer = [0; 64];
        self.0.recv(&mut buffer)?;
        let packet = Packet::from_bytes(&buffer);
        if packet.ack_requested() && packet.is_to(self.1) {
            let ack = Packet::ack_from(&packet);
            self.0.send(&mut ack.as_bytes())?;
        }
        Ok(packet.message())
    }

    fn send(&mut self, data: Vec<u8>, to: u8) -> Result<()> {
        let packet = Packet::new(self.1, to, data, false);
        self.0.send(&mut packet.as_bytes())?;
        Ok(())
    }

    fn send_config(&mut self, conf: &Shared<Config>) -> Result<()> {
        let mut conf = block_on(conf.lock());
        let node = conf.node();
        if node.is_config_dirty() {
            let mut buffer = node.to_bytes();
            buffer.insert(0, PACKET_CONFIG);
            println!("Sending new config");
            println!("{:?}", buffer);
            self.send(buffer, node.addr())?;
        }
        conf.node_mut().update_config_dirty(false);
        Ok(())
    }
}

pub struct Radio {
    rfm: Shared<RfmWrapper>,
    conf: Shared<Config>,
}

impl Radio {
    pub fn new(shared_conf: Shared<Config>) -> Result<Self> {
        let rfm: RfmWrapper;
        {
            let conf = block_on(shared_conf.lock());
            rfm = RfmWrapper::new(conf.network_id(), conf.gateway_addr())?;
        }
        Ok(Radio {
            rfm: new_shared!(rfm),
            conf: shared_conf,
        })
    }

    pub fn receiver_channel(&self) -> Receiver<Vec<u8>> {
        let (mut s, r) = mpsc::unbounded();
        let rfm_clone = self.rfm.clone();
        let config_clone = self.conf.clone();
        spawn_blocking(move || {
            let mut rfm = block_on(rfm_clone.lock());
            loop {
                let result = rfm.receive();
                if let Err(err) = result {
                    println!("{}", err);
                    continue;
                }
                let buffer = result.unwrap();
                if is_config_request(&buffer) {
                    let result = rfm.send_config(&config_clone);
                    if let Err(err) = result {
                        println!("{}", err);
                    }
                } else if is_data_update(&buffer) {
                    let result = block_on(s.send(buffer));
                    if let Err(err) = result {
                        if err.is_disconnected() {
                            println!("Disconnected, ending loop");
                            break;
                        }
                    }
                } else {
                    println!("Invalid data");
                }
            }
        });
        r
    }
}

#[derive(Debug)]
pub struct Packet {
    from: u8,
    to: u8,
    message: Vec<u8>,
    control: u8,
}

impl Packet {
    pub fn new(from: u8, to: u8, message: Vec<u8>, request_ack: bool) -> Self {
        let mut control = 0;
        if request_ack {
            control |= 0x40;
        }

        Packet {
            from,
            to,
            message,
            control,
        }
    }

    pub fn message(self) -> Vec<u8> {
        self.message
    }

    pub fn ack_from(packet: &Packet) -> Self {
        Packet {
            from: packet.to,
            to: packet.from,
            message: Vec::new(),
            control: 0x80,
        }
    }

    pub fn from_bytes(buffer: &[u8]) -> Self {
        let len = (buffer[0] - 3) as usize;
        let to = buffer[1];
        let from = buffer[2];
        let control = buffer[3];
        let message = if len > 0 {
            Vec::from(&buffer[4..4 + len])
        } else {
            Vec::new()
        };
        Packet {
            from,
            to,
            message,
            control,
        }
    }

    pub fn ack_requested(&self) -> bool {
        self.control & 0x40 != 0
    }

    pub fn is_to(&self, addr: u8) -> bool {
        self.to == addr
    }

    pub fn as_bytes(&self) -> Vec<u8> {
        let mut buffer = self.message.clone();
        let len = (buffer.len() + 3) as u8;
        buffer.insert(0, self.control);
        buffer.insert(0, self.from);
        buffer.insert(0, self.to);
        buffer.insert(0, len);
        buffer
    }
}

fn configure_cs() -> Result<Pin> {
    let mut chip = Chip::new(GPIO_CHIP)?;
    let output_pin = chip.get_line(CS_PIN_NUM)?;
    let handle = output_pin.request(LineRequestFlags::OUTPUT, 0, CS_NAME)?;
    Ok(Pin::new(handle)?)
}

fn configure_spi() -> Result<Spidev> {
    let mut spi = Spidev::open(SPI_DEV)?;
    let options: SpidevOptions = SpidevOptions::new()
        .bits_per_word(8)
        .max_speed_hz(1_000_000)
        .mode(SpiModeFlags::SPI_MODE_0)
        .build();
    spi.configure(&options)?;
    Ok(spi)
}

fn is_config_request(data: &[u8]) -> bool {
    data.len() == 1 && (data[0] == PACKET_CONFIG)
}

fn is_data_update(data: &[u8]) -> bool {
    data.len() == 10 && (data[0] == PACKET_DATA)
}
