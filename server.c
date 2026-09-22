#include "common.h"
#include "file.h"
#include "httprequest.h"
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_CHILDREN 128

static int active_children = 0;

static void sigchld_handler(int sig) {
  (void)sig;
  int saved_errno = errno;
  while (waitpid(-1, NULL, WNOHANG) > 0)
    active_children--;
  errno = saved_errno;
}

/* receive a HTTP request */
void receive_request(int client_fd) {
  char buffer[BUFSIZ + 1];
  int total_read = 0;
  int ret;

  while (1) {
    total_read = 0;

    while (total_read < BUFSIZ) {
      ret = read(client_fd, buffer + total_read, BUFSIZ - total_read);
      if (ret < 0)
        error("ERROR reading buffer");
      if (ret == 0)
        break;

      total_read += ret;
      buffer[total_read] = '\0';

      if (strstr(buffer, "\r\n\r\n"))
        break;
    }

    if (total_read >= BUFSIZ || total_read == 0)
      break;

    HttpRequest req = parse_request(buffer);

    int keep_alive = req.connection && strstr(req.connection, "keep-alive");

    serve_file(client_fd, req);

    free(req.method);
    free(req.resource);
    free(req.version);
    free(req.connection);

    if (!keep_alive)
      break;
  }
}

/* Configure Server Socket */
void server_init(const int *server_fd, const struct sockaddr_in *server_addr) {
  int ret;

  // bind socket with address properties
  ret = bind(*server_fd, (struct sockaddr *)server_addr, sizeof *server_addr);
  if (ret < 0)
    error("ERROR binding the socket");

  ret = listen(*server_fd, 128);
  if (ret < 0)
    error("ERROR listening for connections");
}

/* Accept connections */
void server_accept(int server_fd) {
  struct sigaction sa;
  sa.sa_handler = sigchld_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  sigaction(SIGCHLD, &sa, NULL);

  while (1) {
    if (active_children >= MAX_CHILDREN) {
      usleep(10000);
      continue;
    }

    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int client_fd =
        accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
    if (client_fd < 0) {
      if (errno == EMFILE || errno == ENFILE) {
        usleep(10000);
        continue;
      }
      if (errno == EINTR) continue;
      error("ERROR accepting connections");
    }

    active_children++;

    pid_t pid = fork();
    if (pid < 0) {
      active_children--;
      error("ERROR forking");
    }
    if (pid == 0) {
      close(server_fd);
      receive_request(client_fd);
      shutdown(client_fd, SHUT_RDWR);
      close(client_fd);
      exit(0);
    }
    close(client_fd);
  }
}

int main() {
  int server_fd;
  struct sockaddr_in server_addr;

  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0)
    error("ERROR creating a socket");

  // set options for the socket to have a reusable address immediately after a socket dies
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
