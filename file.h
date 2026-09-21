#ifndef FILE_SERVE_H
#define FILE_SERVE_H

#include "common.h"
#include "httprequest.h"

#define DOCROOT "./www"

typedef struct {
  FILE *fp;
  char *fbuffer;
  size_t fsize;
} FileInfo;

/* sanitize resource: reject .., strip query strings, map / to index.html */
char *sanitize_resource(const char *resource);

/* read the contents of a file */
FileInfo *read_file(const char *file_name, char *response_header);

/* serve files one by one, one at a time */
void serve_file(int client_fd, HttpRequest req);

#endif
