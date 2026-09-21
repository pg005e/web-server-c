#include "httprequest.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

void match_request_headers(char *headers_bulk, HttpRequest *req) {
  const char *start = headers_bulk;
  const char *end;
  char lineBuf[120];
  int counter = 1;

  while ((end = strstr(start, "\r\n")) != NULL) {
    int num_bytes = (int)(end - start);
    memcpy(lineBuf, start, num_bytes);
    lineBuf[num_bytes] = '\0';
    start = end + 2;

    if (counter == 1) {
      const char *ptr = lineBuf;

      end = strchr(ptr, ' ');
      if (!end) return;
      num_bytes = (int)(end - ptr);
      req->method = strndup(ptr, num_bytes);

      ptr = end + 1;

      end = strchr(ptr, '/');
      if (!end) return;
      ptr = strchr(end, ' ');
      if (!ptr) return;

      num_bytes = (int)(ptr - end);
      req->resource = strndup(end, num_bytes);

      end = strchr(ptr, '/');
      if (!end) return;
      end = end + 1;
      req->version = strdup(end);
    }

    if (!strcmp(req->method, "GET")) {
      return;
    }

    memset(lineBuf, 0, sizeof lineBuf);
    ++counter;
  }

  if (*start != '\0') {
    if (strstr(start, "Content-Length")) {
      end = strchr(start, ':');
      end = end + 1;
      int content_length = atoi(end);
      req->content_length = content_length;
    }
  }
}

HttpRequest parse_request(char *request) {
  HttpRequest req;
  char buffer[BUFSIZ];

  const char *header_delim = "\r\n\r\n";
  const char *start = request;
  const char *end;

  end = strstr(request, header_delim);
  if (!end) {
    memset(&req, 0, sizeof req);
    return req;
  }
  memcpy(buffer, start, (int)(end - start));
  buffer[(int)(end - start)] = '\0';
  start = end + 4;

  match_request_headers(buffer, &req);
  req.body_payload = (char *)start;

  return req;
}
