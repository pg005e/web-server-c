#include "file.h"
#include "httprequest.h"
#include <stdio.h>

void generate_response_header(HttpRequest req, char *response_header) {
  strcpy(response_header, "HTTP/");
  strcat(response_header, req.version);
  strcat(response_header, " 200 OK\r\n\r\n");
}

FileInfo *read_file(const char *file_name, char *response_header) {
  FileInfo *f = malloc(sizeof(FileInfo));

  f->fp = fopen(file_name, "rb");

  if (f->fp == NULL)
    error("ERROR opening file");

  if (fseek(f->fp, 0, SEEK_END) < 0)
    error("ERROR seeking file");

  long ret = ftell(f->fp);
  rewind(f->fp);

  ssize_t header_len = strlen(response_header);

  f->fsize = ret + header_len + 1;

  f->fbuffer = malloc(f->fsize);
  if (f->fbuffer == NULL)
    error("ERROR allocating memory");

  // copy response header to buffer
  memcpy(f->fbuffer, response_header, header_len);

  // copy the file to buffer stream
  size_t read_size = fread(f->fbuffer + header_len, 1, ret, f->fp);
  if (read_size <= 0) {
    error("ERROR on fread");
  }

  f->fbuffer[header_len + read_size] = '\0';

  fclose(f->fp);

  return f;
}

void serve_file(int client_fd, HttpRequest req) {
  char response_header[20];
  generate_response_header(req, response_header);

  FileInfo *f = read_file(req.resource, response_header);
  size_t ret = 0;

  while (ret < f->fsize) {
    ret = write(client_fd, f->fbuffer, f->fsize);

    if (ret < 0) {
      error("ERROR writing file");
      free(f->fbuffer);
      free(f);
    }
  }

  free(f->fbuffer);
  free(f);
}
