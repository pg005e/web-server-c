#include "common.h"
#include <stddef.h>
#include <stdio.h>

typedef struct {
  FILE *fp;
  char *fbuffer;
  size_t fsize;
} FileInfo;

FileInfo *read_file(const char *file_name) {
  FileInfo *f = malloc(sizeof(FileInfo));

  f->fp = fopen(file_name, "rb");

  if (f->fp == NULL)
    error("ERROR opening file");

  if (fseek(f->fp, 0, SEEK_END) < 0)
    error("ERROR seeking file");

  long ret = ftell(f->fp);
  rewind(f->fp);

  f->fsize = ret + 1;

  f->fbuffer = malloc(f->fsize);
  if (f->fbuffer == NULL)
    error("ERROR allocating memory");

  size_t size = fread(f->fbuffer, 1, ret, f->fp);
  if (size <= 0) {
    error("ERROR on fread");
  }

  f->fbuffer[size] = '\0';

  fclose(f->fp);

  return f;
}

/* serve files one by one, one at a time */
void serve_file(int client_fd, const char *file_name) {
  FileInfo *f = read_file(file_name);

  size_t ret = write(client_fd, f->fbuffer, f->fsize);
  if (ret < 0) {
    error("ERROR writing file");
    free(f->fbuffer);
    free(f);
  }

  free(f->fbuffer);
  free(f);
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

  // serve a HTML file to the clients that try to connect with the server
  serve_file(client_fd, "index.html");

  printf("after serving file...");

  // close the connection
  shutdown(client_fd, SHUT_RDWR);
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

  return 0;
}
