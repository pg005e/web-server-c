#include "common.h"
#include "file.h"
#include "httprequest.h"
#include <errno.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_THREADS 128

static pthread_mutex_t active_mutex = PTHREAD_MUTEX_INITIALIZER;
static int active_connections = 0;

typedef struct {
  int client_fd;
} thread_arg_t;

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

/* Thread function: handles one client connection */
void *handle_client(void *arg) {
  int client_fd = ((thread_arg_t *)arg)->client_fd;
  free(arg);

  receive_request(client_fd);

  shutdown(client_fd, SHUT_RDWR);
  close(client_fd);

  pthread_mutex_lock(&active_mutex);
  active_connections--;
  pthread_mutex_unlock(&active_mutex);

  return NULL;
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

/* Accept connections with pthread-per-connection */
void server_accept(int server_fd) {
  while (1) {
    pthread_mutex_lock(&active_mutex);
    if (active_connections >= MAX_THREADS) {
      pthread_mutex_unlock(&active_mutex);
      usleep(10000);
      continue;
    }
    active_connections++;
    pthread_mutex_unlock(&active_mutex);

    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int client_fd =
        accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
    if (client_fd < 0) {
      pthread_mutex_lock(&active_mutex);
      active_connections--;
      pthread_mutex_unlock(&active_mutex);
      if (errno == EMFILE || errno == ENFILE) {
        usleep(10000);
        continue;
      }
      if (errno == EINTR) continue;
      error("ERROR accepting connections");
    }

    thread_arg_t *arg = malloc(sizeof(thread_arg_t));
    if (!arg) {
      pthread_mutex_lock(&active_mutex);
      active_connections--;
      pthread_mutex_unlock(&active_mutex);
      close(client_fd);
      continue;
    }
    arg->client_fd = client_fd;

    pthread_t tid;
    if (pthread_create(&tid, NULL, handle_client, arg) != 0) {
      pthread_mutex_lock(&active_mutex);
      active_connections--;
      pthread_mutex_unlock(&active_mutex);
      free(arg);
      close(client_fd);
    }
    pthread_detach(tid);
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
