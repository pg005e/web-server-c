#include "common.h"

int main() {
  int server_fd, ret;
  char buf[BUFSIZ];
  struct sockaddr_in server_addr;

  // specify server address
  server_addr.sin_port = htons(PORT);
  server_addr.sin_family = AF_INET;
  inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

  // create socket for connection
  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
    error("ERROR creating a socket");
  }

  // connect to the server
  ret = connect(server_fd, (struct sockaddr *)&server_addr, sizeof server_addr);
  if (ret < 0)
    error("ERROR connecting to socket");

  // read block-wise because server can send files
  bzero(&buf, BUFSIZ);
  ret = read(server_fd, buf, BUFSIZ - 1);
  if (ret < 0)
    error("ERROR reading from socket");

  printf("%s", buf);

  close(server_fd);

  return 0;
}
