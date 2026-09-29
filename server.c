#include "common.h"
#include "file.h"
#include "httprequest.h"
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>

#define MAX_CLIENTS 128

static int server_fd;
static struct pollfd fds[MAX_CLIENTS];
static int nfds = 1;

static void drop_client(int i) {
  close(fds[i].fd);
  for (int j = i; j < nfds - 1; j++) {
    fds[j] = fds[j + 1];
  }
  nfds--;
  fds[nfds].fd = -1;
}

/* receive a HTTP request. returns 0 if the connection must be closed */
int receive_request(int client_fd) {
  char buffer[BUFSIZ + 1];
  int total_read = 0;
  int ret;

  while (total_read < BUFSIZ) {
    ret = read(client_fd, buffer + total_read, BUFSIZ - total_read);
    if (ret < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) break;
    }
    if (ret == 0)
      return 0;

    total_read += ret;
    buffer[total_read] = '\0';

    if (strstr(buffer, "\r\n\r\n"))
      break;
  }

  if (total_read >= BUFSIZ || total_read == 0)
    return 0;

  HttpRequest req = parse_request(buffer);

  int keep_alive = req.connection && strstr(req.connection, "keep-alive");

  serve_file(client_fd, req);

  free(req.method);
  free(req.resource);
  free(req.version);
  free(req.connection);

  return keep_alive ? 1 : 0;
}

/* Configure Server Socket */
void server_init(const struct sockaddr_in *server_addr) {
  int ret;

  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0)
    error("ERROR creating a socket");

  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) <
      0) {
    error("setsockopt(SO_REUSEADDR) failed");
  }

  int flags = fcntl(server_fd, F_GETFL, 0);
  fcntl(server_fd, F_SETFL, flags | O_NONBLOCK);

  ret = bind(server_fd, (struct sockaddr *)server_addr, sizeof *server_addr);
  if (ret < 0)
    error("ERROR binding the socket");

  ret = listen(server_fd, 128);
  if (ret < 0)
    error("ERROR listening for connections");
}

/* Event-driven main loop */
void server_loop(void) {
  while (1) {
    int ret = poll(fds, nfds, -1);
    if (ret < 0 && errno != EINTR)
      error("ERROR in poll");

    // wake on new connections (always readable atp for listening sockets)
    if (fds[0].revents & POLLIN) {
      struct sockaddr_in client_addr;
      socklen_t client_addr_len = sizeof(client_addr);
      int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
      if (client_fd >= 0 && nfds < MAX_CLIENTS) {
        int flags = fcntl(client_fd, F_GETFL, 0);
        fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);
        fds[nfds].fd = client_fd;
        fds[nfds].events = POLLIN;
        fds[nfds].revents = 0;
        nfds++;
      }
    }

    for (int i = 1; i < nfds; i++) {
      if (fds[i].revents & (POLLHUP | POLLERR)) {
        drop_client(i);
        i--;
        continue;
      }
      if (fds[i].revents & POLLIN) {
        if (!receive_request(fds[i].fd)) {
          drop_client(i);
          i--;
        }
      }
    }
  }
}

int main() {
  struct sockaddr_in server_addr;

  memset(&server_addr, 0, sizeof server_addr);
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(PORT);
  inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

  server_init(&server_addr);

  fds[0].fd = server_fd;
  fds[0].events = POLLIN;

  printf("Server listening on %d...\n", PORT);

  server_loop();

  shutdown(server_fd, SHUT_RDWR);
  close(server_fd);

  return 0;
}
