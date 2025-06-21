#include "common.h"

typedef struct {
  FILE *fp;
  char *fbuffer;
  size_t fsize;
} FileInfo;

/* read the contents of a file */
FileInfo *read_file(const char *file_name);

/* serve files one by one, one at a time */
void serve_file(int client_fd, const char *file_name);
