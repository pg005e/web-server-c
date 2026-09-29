#include "file.h"
#include "httprequest.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

char *sanitize_resource(const char *resource) {
  if (!resource || !*resource) return NULL;
  char *path = strdup(resource);
  if (!path) return NULL;

  // Strip query strings
  char *qm = strchr(path, '?');
  if (qm) *qm = '\0';

  // Map / to index.html under docroot
  if (strcmp(path, "/") == 0) {
    free(path);
    path = strdup("index.html");
    if (!path) return NULL;
  }

  // Reject paths containing ..
  if (strstr(path, "..")) {
    free(path);
    return NULL;
  }

  // Prepend docroot
  char *full = malloc(strlen(DOCROOT) + strlen(path) + 2);
  if (!full) { free(path); return NULL; }
  strcpy(full, DOCROOT);
  strcat(full, "/");
  strcat(full, path);
  free(path);
  return full;
}

void generate_response_header(HttpRequest req, char *response_header, long file_size) {
  snprintf(response_header, 128,
    "HTTP/%s 200 OK\r\n"
    "Content-Length: %ld\r\n"
    "Content-Type: text/html\r\n"
    "Connection: keep-alive\r\n"
    "\r\n",
    req.version, file_size);
}

FileInfo *read_file(const char *file_name, char *response_header, HttpRequest req) {
  FileInfo *f = malloc(sizeof(FileInfo));
  if (!f) return NULL;

  f->fp = fopen(file_name, "rb");
  if (!f->fp) { free(f); return NULL; }

  if (fseek(f->fp, 0, SEEK_END) < 0) { fclose(f->fp); free(f); return NULL; }

  long ret = ftell(f->fp);
  if (ret < 0) { fclose(f->fp); free(f); return NULL; }
  rewind(f->fp);

  generate_response_header(req, response_header, ret);
  ssize_t header_len = strlen(response_header);

  f->fbuffer = malloc(ret + header_len);
  if (!f->fbuffer) { fclose(f->fp); free(f); return NULL; }

  memcpy(f->fbuffer, response_header, header_len);

  size_t read_size = fread(f->fbuffer + header_len, 1, ret, f->fp);
  fclose(f->fp);

  if (read_size <= 0) { free(f->fbuffer); free(f); return NULL; }

  f->fsize = header_len + read_size;
  return f;
}

void serve_file(int client_fd, HttpRequest req) {
  if (!req.resource || !*req.resource) {
    const char *err = "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
    write(client_fd, err, strlen(err));
    return;
  }
  char *safe_path = sanitize_resource(req.resource);

  if (!safe_path) {
    // 400 Bad Request or 404 - send error response
    const char *err = "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
    write(client_fd, err, strlen(err));
    return;
  }

  char response_header[128];
  FileInfo *f = read_file(safe_path, response_header, req);
  size_t total_written = 0;

  if (f) {
    while (total_written < f->fsize) {
      ssize_t n = write(client_fd, f->fbuffer + total_written, f->fsize - total_written);

      if (n < 0) {
        if (errno == EINTR) continue;
        error("ERROR writing file");
        free(f->fbuffer);
        free(f);
      }
      total_written += n;
    }
    free(f->fbuffer);
    free(f);
  } else {
    // 404 Not Found - send error response
    const char *err = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
    write(client_fd, err, strlen(err));
  }

  free(safe_path);
}
