#include "sys/socket.h"
#include "netinet/in.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include <stddef.h>
#include <stdint.h>
#include <unistd.h>
#include "arpa/inet.h"

#define BUFSIZE 1024
#define DEBUG 1

// Struct to store parsed HTTP message
typedef struct {
  char method[8];
  char uri[128];
  char version[16];
  char headers[1024];
  char body[1024];
} http_request_t;


http_request_t *parse_http_request(char *request, int bytes_received) {

    http_request_t *req = malloc(sizeof(http_request_t));
    char *line = strtok(request, "\r\n");
    sscanf(line, "%s %s %s", req->method, req->uri, req->version);
    strcpy(req->headers, line + strlen(req->method) + strlen(req->uri) + strlen(req->version) + 3);
    // Check for message body
    int content_length = 0;
    char *content_length_str = strstr(req->headers, "Content-Length: ");
    if (content_length_str != NULL) {
        sscanf(content_length_str, "Content-Length: %d", &content_length);
    }

    // Receive message body
    if (content_length > 0) {
        char *body_ptr = request + bytes_received - content_length;
        strncpy(req->body, body_ptr, content_length);
    }

    printf("HTTP request:\n");
    printf("Method: %s\n", req->method);
    printf("URI: %s\n", req->uri);
    printf("Version: %s\n", req->version);
    printf("Headers:\n%s", req->headers);
    printf("Body:\n%s\n", req->body);

    return req;
}



void checkNetError(int status) {
    if (status < 0) {
        perror("Socket");
        exit(EXIT_FAILURE);
    }
}

struct sockaddr_in create_sockaddr_in(const char* interface, uint16_t port) {
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    // convert ip (network interface) string to binary repr. 
    if (interface == NULL) {
        addr.sin_addr.s_addr = htonl(IN_LOOPBACKNET);
    }else {
       if (inet_pton(AF_INET, interface, &addr.sin_addr) <= 0) {
            perror("inet_pton");
            exit(EXIT_FAILURE);
       }
    }
    return addr;
} 


int listen_tcp_local(struct sockaddr_in serv_addr){

    // create a stream based (SOCK_STREAM) network fd with the address family of ipv4.
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    checkNetError(sock_fd);
    // bind socket to the address
    checkNetError(bind(sock_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)));
    // start listening
    checkNetError(listen(sock_fd, SOMAXCONN));
    return sock_fd;
}


int main() {
    const char * interface = "127.0.0.1";
    uint16_t port = 8080;
    struct sockaddr_in server_address = create_sockaddr_in(interface,port);
    int sock_fd = listen_tcp_local(server_address);

    while (1) {

        int client_fd;
        struct sockaddr_in client_addr;

        char buff[BUFSIZE];

        socklen_t client_addr_len = sizeof(client_addr);
        client_fd = accept(sock_fd, (struct sockaddr *) &client_addr, &client_addr_len);
        int n_bytes = recv(client_fd, buff, BUFSIZE, 0);
        if (n_bytes <= 0) {
            close(client_fd);
            fprintf(stdout, "closing connection!\n");
            exit(EXIT_FAILURE);
        }
        http_request_t* req =  parse_http_request(buff, n_bytes);
        free(req);
        char response[] = "HTTP/1.1 200 OK\r\nContent-Length: 11\r\n\r\nHello, world";
        send(client_fd, response, sizeof(response), 0);
        close(client_fd);
    }
    close(sock_fd);
}
