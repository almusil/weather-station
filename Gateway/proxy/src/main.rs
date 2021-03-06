#![recursion_limit = "256"]
#[macro_use]
extern crate failure;
#[macro_use]
extern crate log;

use crate::error::Result;
use crate::proxy::Proxy;
use crate::util::{Sender, Shared, CONF_PATH, LOG_MODULE_IGNORE, LOG_PATH, LOG_TIME_FORMAT};
use async_std::sync::{Arc, Mutex};
use async_std::task::block_on;
use futures::channel::mpsc;
use futures::SinkExt;
use simplelog::{ConfigBuilder, LevelFilter, WriteLogger};
use std::fs::File;

mod config;
mod data;
mod error;

#[macro_use]
mod util;
mod home_assistant;
mod proxy;
mod radio;
mod vutbr;

#[async_std::main]
async fn main() {
    let result = run_main().await;
    if let Err(err) = result {
        eprintln!("{}", err);
        error!("{:?}", err);
    }
}

async fn run_main() -> Result<()> {
    let config = ConfigBuilder::new()
        .set_location_level(LevelFilter::Error)
        .set_time_format_str(LOG_TIME_FORMAT)
        .add_filter_ignore_str(LOG_MODULE_IGNORE)
        .build();
    let _ = WriteLogger::init(LevelFilter::Debug, config, File::create(LOG_PATH)?);
    let (s, r) = mpsc::unbounded();
    let sender = new_shared!(s);
    configure_ctrlc_handler(sender);

    let mut proxy = Proxy::new(CONF_PATH, r).await?;
    proxy.main_loop().await?;

    info!("Exiting");
    Ok(())
}

fn configure_ctrlc_handler(shared_sender: Shared<Sender<bool>>) {
    ctrlc::set_handler(move || {
        info!("Got CTRL+C");
        let mut sender = block_on(shared_sender.lock());
        block_on(sender.send(true)).unwrap();
    })
    .unwrap();
}
