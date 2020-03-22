use crate::config::Config;
use crate::error::{Error, Result};
use crate::util::{
    Receiver, Shared, CS_NAME, CS_PIN_NUM, GPIO_CHIP, INTERRUPT_NAME, INTERRUPT_PIN_NUM,
    PACKET_CONFIG, PACKET_DATA, SPI_DEV,
};
use async_std::sync::{Arc, Mutex};
use async_std::task::{block_on, spawn_blocking};
use futures::channel::mpsc;
use futures::SinkExt;
use hal::gpio_cdev::{Chip, EventRequestFlags, LineEventHandle, LineRequestFlags};
use hal::spidev::{SpiModeFlags, SpidevOptions};
use hal::{CdevPin, Delay, Spidev};
use linux_embedded_hal as hal;
use rfm69::registers::{DioMapping, DioMode, DioPin, DioType, Mode};
use rfm69::{low_power_lab_defaults, Rfm69};

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
                    eprintln!("{}", err);
                    error!("{:?}", err);
                    continue;
                }
                let buffer = result.unwrap();
                if is_config_request(&buffer) {
                    let result = rfm.send_config(&config_clone);
                    if let Err(err) = result {
                        eprintln!("{}", err);
                        error!("{:?}", err);
                    }
                } else if is_data_update(&buffer) {
                    let result = block_on(s.send(buffer));
                    if let Err(err) = result {
                        if err.is_disconnected() {
                            info!("Disconnected, ending loop");
                            break;
                        }
                    }
                } else {
                    error!("Invalid data");
                }
            }
        });
        r
    }
}

struct RfmWrapper {
    rfm: Rfm69<CdevPin, Spidev, Delay>,
    gateway_addr: u8,
    interrupt: LineEventHandle,
}

impl RfmWrapper {
    fn new(network_id: u8, gateway_addr: u8) -> Result<Self> {
        let mut chip = Chip::new(GPIO_CHIP)?;
        let spi = configure_spi()?;
        let cs = configure_cs(&mut chip)?;
        let interrupt = configure_interrupt_pin(&mut chip)?;

        let mut rfm =
            low_power_lab_defaults(Rfm69::new(spi, cs, Delay), network_id, 433_000_000.0)?;
        rfm.dio_mapping(DioMapping {
            pin: DioPin::Dio0,
            dio_type: DioType::Dio01,
            dio_mode: DioMode::Rx,
        })?;
        Ok(RfmWrapper {
            rfm,
            gateway_addr,
            interrupt,
        })
    }

    fn receive(&mut self) -> Result<Vec<u8>> {
        let mut buffer = [0; 64];
        self.wait_packet_ready()?;
        self.rfm.recv(&mut buffer)?;
        let packet = Packet::from_bytes(&buffer)?;
        if packet.ack_requested() && packet.is_to(self.gateway_addr) {
            let ack = Packet::ack_from(&packet);
            self.rfm.send(&mut ack.as_bytes())?;
        }
        Ok(packet.message())
    }

    fn send(&mut self, data: Vec<u8>, to: u8) -> Result<()> {
        let packet = Packet::new(self.gateway_addr, to, data, false);
        self.rfm.send(&mut packet.as_bytes())?;
        Ok(())
    }

    fn send_config(&mut self, conf: &Shared<Config>) -> Result<()> {
        let mut conf = block_on(conf.lock());
        let node = conf.node();
        if node.is_config_dirty() {
            let mut buffer = node.to_bytes();
            buffer.insert(0, PACKET_CONFIG);
            info!("Sending new config");
            debug!("Config: {:?}", buffer);
            self.send(buffer, node.addr())?;
        }
        conf.node_mut().update_config_dirty(false);
        Ok(())
    }

    fn wait_packet_ready(&mut self) -> Result<()> {
        self.rfm.mode(Mode::Receiver)?;
        while !self.rfm.is_packet_ready()? {
            self.interrupt.get_event()?;
        }
        Ok(())
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

    pub fn from_bytes(buffer: &[u8]) -> Result<Self> {
        let len = buffer[0] as usize;
        let to = buffer[1];
        let from = buffer[2];
        let control = buffer[3];
        if len >= buffer.len() {
            return Err(Error::new_index_out_of_range(buffer.len(), len));
        }
        let message = if len > 0 {
            Vec::from(&buffer[4..=len])
        } else {
            Vec::new()
        };
        Ok(Packet {
            from,
            to,
            message,
            control,
        })
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

fn configure_cs(chip: &mut Chip) -> Result<CdevPin> {
    let output_pin = chip.get_line(CS_PIN_NUM)?;
    let handle = output_pin.request(LineRequestFlags::OUTPUT, 0, CS_NAME)?;
    Ok(CdevPin::new(handle)?)
}

fn configure_interrupt_pin(chip: &mut Chip) -> Result<LineEventHandle> {
    let input_pin = chip.get_line(INTERRUPT_PIN_NUM)?;
    Ok(input_pin.events(
        LineRequestFlags::INPUT,
        EventRequestFlags::RISING_EDGE,
        INTERRUPT_NAME,
    )?)
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
    data.len() == 18 && (data[0] == PACKET_DATA)
}
