use tokio::{net::TcpListener, io::{AsyncReadExt, AsyncWriteExt}};


#[tokio::main]
async fn main() {
    let listner = TcpListener::bind("127.0.0.1:8080").await.unwrap();
    let (mut socket, _addr) = listner.accept().await.unwrap();

    loop {

        let mut buffer = [0u8; 1024];
    
        let bytes_read = socket.read(&mut buffer).await.unwrap();
    
        socket.write_all(&buffer[0..bytes_read]).await.unwrap();
    }

}
