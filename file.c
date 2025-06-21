#include "file.h"
#include "common.h"

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

