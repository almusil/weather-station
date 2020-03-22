use bincode::Error as BincodeError;
use failure::{Backtrace, Fail, SyncFailure};
use linux_embedded_hal::gpio_cdev::errors::Error as CdevError;
use paho_mqtt::errors::MqttError;
use rfm69::Error as RfmError;
use serde_json::Error as SerdeJsonError;
use serde_yaml::Error as SerdeYamlError;
use std::env::VarError;
use std::io::Error as IoError;
use std::result;

pub type Result<T> = result::Result<T, Error>;

#[derive(Debug, Fail)]
pub enum Error {
    #[fail(display = "Radio error: {}", _0)]
    RadioError(&'static str, Backtrace),
    #[fail(display = "Character device GPIO error: {}", _0)]
    CdevGpioError(#[fail(cause)] SyncFailure<CdevError>, Backtrace),
    #[fail(display = "IO error: {}", _0)]
    IoError(#[fail(cause)] IoError, Backtrace),
    #[fail(display = "Serde YAML error: {}", _0)]
    SerdeYamlError(#[fail(cause)] SerdeYamlError, Backtrace),
    #[fail(display = "Option error: {}", _0)]
    OptionError(&'static str, Backtrace),
    #[fail(display = "MQTT error: {}", _0)]
    MqttError(#[fail(cause)] MqttError, Backtrace),
    #[fail(display = "Serde JSON error: {}", _0)]
    SerdeJsonError(#[fail(cause)] SerdeJsonError, Backtrace),
    #[fail(display = "Bincode error: {}", _0)]
    BincodeError(#[fail(cause)] BincodeError, Backtrace),
    #[fail(display = "Result (Err=()) error: {}", _0)]
    ResultError(&'static str, Backtrace),
    #[fail(display = "IndexOutOfRange, buffer size {}, index {}", size, index)]
    IndexOutOfRange {
        size: usize,
        index: usize,
        backtrace: Backtrace,
    },
    #[fail(display = "Env variable error: {}", _0)]
    VarError(#[fail(cause)] VarError, Backtrace),
}

impl Error {
    pub fn new_option(msg: &'static str) -> Self {
        Error::OptionError(msg, Backtrace::new())
    }

    pub fn new_result(msg: &'static str) -> Self {
        Error::ResultError(msg, Backtrace::new())
    }

    pub fn new_index_out_of_range(size: usize, index: usize) -> Self {
        Error::IndexOutOfRange {
            size,
            index,
            backtrace: Backtrace::new(),
        }
    }
}

impl From<RfmError> for Error {
    fn from(err: RfmError) -> Self {
        Error::RadioError(err.0, Backtrace::new())
    }
}

impl From<CdevError> for Error {
    fn from(err: CdevError) -> Self {
        Error::CdevGpioError(SyncFailure::new(err), Backtrace::new())
    }
}

impl From<IoError> for Error {
    fn from(err: IoError) -> Self {
        Error::IoError(err, Backtrace::new())
    }
}

impl From<SerdeYamlError> for Error {
    fn from(err: SerdeYamlError) -> Self {
        Error::SerdeYamlError(err, Backtrace::new())
    }
}

impl From<MqttError> for Error {
    fn from(err: MqttError) -> Self {
        Error::MqttError(err, Backtrace::new())
    }
}

impl From<SerdeJsonError> for Error {
    fn from(err: SerdeJsonError) -> Self {
        Error::SerdeJsonError(err, Backtrace::new())
    }
}

impl From<BincodeError> for Error {
    fn from(err: BincodeError) -> Self {
        Error::BincodeError(err, Backtrace::new())
    }
}

impl From<VarError> for Error {
    fn from(err: VarError) -> Self {
        Error::VarError(err, Backtrace::new())
    }
}
