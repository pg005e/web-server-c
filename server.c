#include "common.h"
#include "file.h"
#include "httprequest.h"

/* receive a HTTP request */
void receive_request(int client_fd) {
  char request[BUFSIZ];
  int ret;

  ret = read(client_fd, request, BUFSIZ);
  if (ret < 0)
    error("ERROR reading request");

  // if the header size if greater
  // than 8Kb, send back an error code
  // store the value returned by read

  // parse the request
  HttpRequest req = parse_request(request);

  serve_file(client_fd, "index.html", 140);
}

/* Configure Server Socket */
void server_init(const int *server_fd, const struct sockaddr_in *server_addr) {
  int ret;

  // bind socket with address properties
  ret = bind(*server_fd, (struct sockaddr *)server_addr, sizeof *server_addr);
  if (ret < 0)
    error("ERROR binding the socket");

  // listen for connections, max 5 at a time
  ret = listen(*server_fd, 1);
  if (ret < 0)
    error("ERROR listening for connections");
}

/* Accept connections */
void server_accept(int server_fd) {
  char buf[BUFSIZ];
  int ret, client_fd;
  struct sockaddr_in client_addr;
  socklen_t client_addr_len = sizeof(client_addr);

  // start accepting connections
  client_fd =
      accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
  if (client_fd < 0)
    error("ERROR accepting connections");

  // accept the request and check for errors
  receive_request(client_fd);

  // close the connection
  shutdown(client_fd, SHUT_RDWR);
  close(client_fd);
}

int main() {
  int server_fd;
  struct sockaddr_in server_addr;

  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0)
    error("ERROR creating a socket");

  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) <
      0) {
    error("setsockopt(SO_REUSEADDR) failed");
  }

  // need to set to zero for padding and proper init
  memset(&server_addr, 0, sizeof server_addr);

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(PORT);
  inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

  server_init(&server_fd, &server_addr);
  printf("Server listening on %d...\n", PORT);

  server_accept(server_fd);

  shutdown(server_fd, SHUT_RDWR);
  close(server_fd);

  return 0;
}
