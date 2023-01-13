package main

import (
	"fmt"
	"log"
	"net"
	"os"
	"time"
)

const MAX_ROUTINES = 40

func main() {
	ip := os.Getenv("IP")
	start := time.Now()
	sem := make(chan int, MAX_ROUTINES)
	d := net.Dialer{Timeout: time.Millisecond * 300}
	for i := 60000; i <= 7000; i++ {
		sem <- 1
		go func(i int) {
			c, err := d.Dial("tcp", fmt.Sprintf("%s:%d", ip, i))
			if err == nil {
				fmt.Println(fmt.Sprintf("port %d is opened", i))
				c.Close()
			}
			<-sem
		}(i)

	}
	elapsed := time.Since(start)
	log.Printf("Scanning took %s\n", elapsed)
}
