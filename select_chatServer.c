#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define PORT "8080"

void *get_in_addr(struct sockaddr *sa);
int get_listener_socket(void);

int main()
{

    // master file descriptor list
    fd_set master;
    // temp file descriptor list for select() (because select modifies the set)
    fd_set read_fds;
    // the maximum file descriptor number
    int fdmax;
    // the listening socket descriptor
    int listener;
    // newly accept()ed socket descriptor
    int newfd;
    // client address
    struct sockaddr_storage remoteaddr;
    socklen_t addrlen;
    // buffer for client data
    char buf[256];
    // number of bytes read
    int nbytes;
    // create a tcp socket and return it
    listener = get_listener_socket();

    // add the listener to the master set
    FD_SET(listener, &master);
    // keep track of the biggest file descriptor
    fdmax = listener; // so far, it's this one

    char remoteIP[INET6_ADDRSTRLEN];

    for (;;)
    {
        // we copy the list of fds to use its for select
        read_fds = master;

        // select will block until at least one fd in the read_fds set is ready for read
        // at the beginning the set of read_fds only contain the listening socket file descriptor
        // which means at the beginning it will unblock when someone tries to connect to the server
        if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1)
        {
            perror("select() error");
        }

        for (int i = 0; i <= fdmax; i++)
        {
            // which sockets are ready
            if (FD_ISSET(i, &read_fds))
            {
                // incomming new connection
                if (i == listener)
                {
                    // handle new connection

                    addrlen = sizeof remoteaddr;
                    newfd = accept(listener, (struct sockaddr *)&remoteaddr, &addrlen);
                    // handle connection error
                    if (newfd == -1)
                    {
                        perror("accept error");
                    }
                    else
                    {
                        FD_SET(newfd, &master);
                        if (newfd > fdmax)
                        {
                            fdmax = newfd;
                        }

                        const char *iaddr = inet_ntop(remoteaddr.ss_family, get_in_addr((struct sockaddr *)&remoteaddr), remoteIP, INET6_ADDRSTRLEN);

                        printf("Established new connection from %s, on socket %d\n", iaddr, newfd);

                    }
                }
                else
                {
                    // otherwise it is a message from an already established connection

                    if ((nbytes = recv(i, buf, sizeof buf, 0)) <= 0)
                    {
                        // got error or connection closed by client
                        if (nbytes == 0)
                        {
                            // connection closed
                            printf("Socket %d hung up\n", i);
                        }
                        else
                        {
                            perror("recv error");
                        }
                        close(i);
                        // remove from master set
                        FD_CLR(i, &master);
                    }
                    else
                    // we got data from client
                    {
                        // we broadcast the message to all active connections
                        for (int activeSocket = 0; activeSocket <= fdmax; activeSocket++)
                        {
                            if (FD_ISSET(activeSocket, &master))
                            {
                                // we skip the sender and ourselves
                                if (activeSocket == listener || activeSocket == i)
                                {
                                    continue;
                                }
                                // write the data
                                send(activeSocket, buf, nbytes, 0);
                            }
                        }
                    }
                }
            }
        }
    }

    printf("Exiting");
    return 0;
}

// Get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

// Return a listening socket
int get_listener_socket(void)
{
    int listener; // Listening socket descriptor
    int yes = 1;  // For setsockopt() SO_REUSEADDR, below
    int rv;

    struct addrinfo hints, *ai, *p;

    // Get us a socket and bind it
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0)
    {
        fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
        exit(1);
    }

    for (p = ai; p != NULL; p = p->ai_next)
    {
        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listener < 0)
        {
            continue;
        }

        // Lose the pesky "address already in use" error message
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

        if (bind(listener, p->ai_addr, p->ai_addrlen) < 0)
        {
            close(listener);
            continue;
        }

        break;
    }

    // If we got here, it means we didn't get bound
    if (p == NULL)
    {
        return -1;
    }

    freeaddrinfo(ai); // All done with this

    // Listen
    if (listen(listener, 10) == -1)
    {
        return -1;
    }

    return listener;
}
