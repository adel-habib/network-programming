use std::{os::unix::prelude::IntoRawFd, future::Future};


fn main() {
    println!("Hello, world!");
}



// foo1 and foo2 are the same
async fn foo1() -> usize{
    0
}
fn foo2() -> impl Future<Output = usize> {
    async {
        0
    }
}