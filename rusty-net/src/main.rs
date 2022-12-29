use std::net::SocketAddr;

use tokio::{
    io::{AsyncBufReadExt, AsyncWriteExt, BufReader},
    net::TcpListener, sync::broadcast,
};

#[tokio::main]
async fn main() {
    let listner = TcpListener::bind("127.0.0.1:8080").await.unwrap();
    let (tx, _rx) = broadcast::channel::<(String, SocketAddr)>(10);
    
    loop {
        let (mut socket, addr) = listner.accept().await.unwrap();
        let tx = tx.clone();
        let mut rx = tx.subscribe();

        tokio::spawn(async move {
            let (red, mut wrt) = socket.split();
            let mut reader = BufReader::new(red);
            let mut line = String::new();

            loop {
                tokio::select! {
                    result = reader.read_line(&mut line) => {
                        if result.unwrap() == 0 {
                            let hang_up_msg = String::from("[ Server -> client ") + &addr.to_string() + " left the chat ] \n";
                            tx.send((hang_up_msg, addr)).unwrap();
                            break;
                        }
                        let msg_with_addr = addr.to_string() + "-> " + &line;
                        tx.send((msg_with_addr.clone(), addr)).unwrap();
                        line.clear();
                    }
                    result = rx.recv() => {
                        let (msg,other_addr) = result.unwrap();
                        if other_addr != addr {
                            wrt.write_all(&msg.as_bytes()).await.unwrap();
                        }
                    }
                }

            }
        });
    }
}
