use tokio::{
    io::{AsyncBufReadExt, AsyncReadExt, AsyncWriteExt, BufReader},
    net::TcpListener,
};

#[tokio::main]
async fn main() {
    let listner = TcpListener::bind("127.0.0.1:8080").await.unwrap();
    loop {
        let (mut socket, _addr) = listner.accept().await.unwrap();
        tokio::spawn(async move {
            let (red, mut wrt) = socket.split();
            let mut reader = BufReader::new(red);
            let mut line = String::new();

            loop {
                let bytes_read = reader.read_line(&mut line).await.unwrap();
                if bytes_read == 0 {
                    break;
                }
                wrt.write_all(&line.as_bytes()).await.unwrap();
                line.clear();
            }
        });
    }
}
