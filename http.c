#include "arpa/inet.h"
#include "netinet/in.h"
#include "stdbool.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "sys/socket.h"
#include <stddef.h>
#include <stdint.h>
#include <unistd.h>

#define BUFSIZE 1024
#define DEBUG 1
#define MAX_HEADERS 50
#define MAX_CONTENT_LENGTH 50

// Struct to store parsed HTTP message
typedef struct {
  char method[8];
  char uri[128];
  char version[16];
  char headers[1024];
  char body[1024];
} http_request_t;

typedef struct {
  int status;
  char *headers[MAX_HEADERS];
  char *body;
} http_response_t;

bool is_valid_http_method(const char *method) {
  // List of valid HTTP methods
  const char *valid_methods[] = {"GET",     "HEAD",   "POST",
                                 "PUT",     "DELETE", "CONNECT",
                                 "OPTIONS", "TRACE",  "PATCH"};
  int num_valid_methods = sizeof(valid_methods) / sizeof(valid_methods[0]);

  // Check if the method is in the list of valid methods
  for (int i = 0; i < num_valid_methods; i++) {
    if (strcmp(method, valid_methods[i]) == 0) {
      return true;
    }
  }

  // If we reach here, the method is not in the list of valid methods
  return false;
}

http_request_t *parse_http_request(int client_fd) {
  char request[BUFSIZE];
  bool valid = true;
  int bytes_received = recv(client_fd, request, BUFSIZE, 0);
  if (bytes_received <= 0) {
    perror("recv failure");
    close(client_fd);
  }
  printf("parsed %d bytes\n", bytes_received);
  http_request_t *req = malloc(sizeof(http_request_t));
  char *line = strtok(request, "\r\n");
  sscanf(line, "%s %s %s", req->method, req->uri, req->version);

  valid = is_valid_http_method(req->method);
  if (strcmp("HTTP/1.1", req->version) != 0) {
    valid = false;
  }
  if (!valid) {
    perror("invalid http request");
    close(client_fd);
    return NULL;
  }
  strcpy(req->headers, line + strlen(req->method) + strlen(req->uri) +
                           strlen(req->version) + 3);
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
  return req;
}

http_response_t *create_http_response(int status, char *headers[], char *body) {
  http_response_t *response = malloc(sizeof(http_response_t));
  response->status = status;
  for (int i = 0; i < MAX_HEADERS; i++) {
    response->headers[i] = headers[i];
  }
  response->body = body;
  return response;
}

char *create_http_response_string(http_response_t *res) {
  static char response[1024];
  char *status_string;
  char content_length_header[50];
  int content_len_provided = -1;
  int status_code = res->status;
  char **headers = res->headers;
  char *body = res->body;
  // Reset the response buffer to an empty string
  response[0] = '\0';

  switch (status_code) {
  case 200:
    status_string = "200 OK";
    break;
  case 201:
    status_string = "201 OK";
    break;
  case 400:
    status_string = "400 BAD REQUEST";
    break;
  case 404:
    status_string = "404 Not Found";
    break;
  default:
    status_string = "500 Internal Server Error";
    break;
  }

  // Start building the response
  sprintf(response, "HTTP/1.1 %s\r\n", status_string);

  // Update the Content-Length header to use the actual length of the body
  sprintf(content_length_header, "Content-Length: %zu", strlen(body));

  // Add the headers
  for (int i = 0; headers[i] != NULL; i++) {
    // Check if the current header is the Content-Length header
    if (strncmp(headers[i], "Content-Length:", 15) == 0) {
      // Use the updated Content-Length header
      content_len_provided = 1;
      strcat(response, content_length_header);
    } else {
      // Use the original header
      strcat(response, headers[i]);
    }
    strcat(response, "\r\n");
  }
  if (content_len_provided == -1) {
    strcat(response, content_length_header);
    strcat(response, "\r\n");
  }

  // Add a blank line to indicate the end of the headers
  strcat(response, "\r\n");

  // Add the body
  strcat(response, body);

  return response;
}

int send_http_response(int client_fd, char *response) {
  printf("%s\n", response);
  int n = send(client_fd, response, strlen(response), 0);
  printf("sent %d bytes", n);
  if (n < 0) {
    perror("send failed");
    return -1;
  } else {
    return n;
  }
}

void checkNetError(int status) {
  if (status < 0) {
    perror("Socket");
    exit(EXIT_FAILURE);
  }
}

struct sockaddr_in create_sockaddr_in(const char *interface, uint16_t port) {
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  // convert ip (network interface) string to binary repr.
  if (interface == NULL) {
    addr.sin_addr.s_addr = htonl(IN_LOOPBACKNET);
  } else {
    if (inet_pton(AF_INET, interface, &addr.sin_addr) <= 0) {
      perror("inet_pton");
      exit(EXIT_FAILURE);
    }
  }
  return addr;
}

int listen_tcp_local(struct sockaddr_in serv_addr) {

  // create a stream based (SOCK_STREAM) network fd with the address family of
  // ipv4.
  int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
  checkNetError(sock_fd);
  // bind socket to the address
  checkNetError(
      bind(sock_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)));
  // start listening
  checkNetError(listen(sock_fd, SOMAXCONN));
  return sock_fd;
}

int main() {
  struct sockaddr_in server_address = create_sockaddr_in("127.0.0.1", 8080);
  int sock_fd = listen_tcp_local(server_address);

  while (1) {
    // accept connection from client
    int client_fd;
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    client_fd =
        accept(sock_fd, (struct sockaddr *)&client_addr, &client_addr_len);
    // parse the http request
    http_request_t *req = parse_http_request(client_fd);
    free(req);
    // write resposne
    char *body = "HELLO WORLD FROM C!";
    char *headers[] = {"Content-type: text/plain", NULL};
    http_response_t *resp = create_http_response(200, headers, body);
    char *resp_string = create_http_response_string(resp);
    send_http_response(client_fd, resp_string);
    free(resp);
    close(client_fd);
  }
  close(sock_fd);
}
