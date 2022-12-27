#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#define ISVALIDSOCKET(s) ((s) >= 0)
#define CLOSESOCKET(s) close(s)
#define SOCKET int
#define GETSOCKETERRNO() (errno)
#include <stdio.h>
#include <string.h>

int main(){

    // create socket and bind to address

    printf("Configuring local address...\n");

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    struct addrinfo *bind_address;
    getaddrinfo(0, "8080", &hints, &bind_address);

    printf("Creating socket...\n");

    SOCKET socket_listen;
    socket_listen = socket(bind_address->ai_family, bind_address->ai_socktype, bind_address->ai_protocol);

    if (!ISVALIDSOCKET(socket_listen))
    {
        fprintf(stderr, "socket() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }

    printf("Binding socket to local address...\n");
    if (bind(socket_listen, bind_address->ai_addr, bind_address->ai_addrlen))
    {
        fprintf(stderr, "bind() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }
    freeaddrinfo(bind_address);
    printf("Listening...\n");
    if (listen(socket_listen, 10) < 0)
    {
        fprintf(stderr, "listen() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }

    // select approach
    // create a set of fds -> for now we only have our listening socket which is also the max socket
    fd_set master;
    FD_ZERO(&master);
    FD_SET(socket_listen, &master);
    SOCKET max_socket = socket_listen;

    // listen to connections

    printf("Waiting for connections...\n");

    while (1)
    {
        fd_set reads;
        reads = master;
        if (select(max_socket + 1, &reads, 0, 0, 0) < 0)
        {
            fprintf(stderr, "select() failed. (%d)\n", GETSOCKETERRNO());
            return 1;
        }
        
        SOCKET i;
        // iterate over the set of fds 
        for(i = 1; i <= max_socket; ++i) {
            // is any fds ready for read? 
            if (FD_ISSET(i, &reads)) {

                // are we getting a new connection? 
                if (i == socket_listen) {
                    struct sockaddr_storage client_address;
                    socklen_t client_len = sizeof(client_address);
                    // accept connection
                    SOCKET socket_client = accept(socket_listen,(struct sockaddr*) &client_address,&client_len);
                    // handle errors
                    if (!ISVALIDSOCKET(socket_client)) {
                        fprintf(stderr, "accept() failed. (%d)\n",GETSOCKETERRNO());
                        return 1;
                    }
                    // add the new connection's fd to the set of all fds 
                    FD_SET(socket_client, &master);
                    // inc the max fd num
                    if (socket_client > max_socket)
                        max_socket = socket_client;

                    char address_buffer[100];
                    // get client address
                    getnameinfo((struct sockaddr*)&client_address,client_len,address_buffer, sizeof(address_buffer), 0, 0,NI_NUMERICHOST);
                    printf("New connection from %s\n", address_buffer);

                } // are we getting a message from an already established connection?  
                else {
                    // buffer to read message
                    char read[1024];
                    int bytes_received = recv(i, read, 1024, 0);

                    // handle client closing connection
                    if (bytes_received < 1) {
                        FD_CLR(i, &master);
                        CLOSESOCKET(i);
                        continue;
                    }
                    // echo the message back after convcerting it to upper case
                    for (int j = 0; j < bytes_received; ++j)
                        read[j] = toupper(read[j]);
                    send(i, read, bytes_received, 0);
                }

            }
        }
    }

    printf("Closing the socket...\n");
    CLOSESOCKET(socket_listen);
}