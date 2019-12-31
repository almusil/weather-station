use crate::proxy::Proxy;
use crate::util::{Sender, Shared};
use async_std::sync::{Arc, Mutex};
use async_std::task::block_on;
use ctrlc;
use futures::channel::mpsc;
use futures::SinkExt;

mod config;

#[macro_use]
mod util;
mod home_assistant;
mod proxy;
mod radio;

// TODO Proper error handling
// TODO Logging into file, probably https://docs.rs/crate/simplelog/0.7.4
// TODO Expression support in config for analog types, probably https://docs.rs/asciimath/0.8.8/asciimath/

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
