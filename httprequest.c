#include "httprequest.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

void match_request_headers(char *headers_bulk, HttpRequest *req) {
  const char *start = headers_bulk;
  const char *end;
  char lineBuf[120]; // to store each Http Header for processing
  int counter = 1;

  // get each line
  while ((end = strstr(start, "\r\n")) != NULL) {
    int num_bytes = (int)(end - start);
    memcpy(lineBuf, start, num_bytes);
    start = end + 2;

    // HTTP status line
    if (counter == 1) {
      const char *ptr = lineBuf;

      // http method
      end = strchr(ptr, ' ');
      num_bytes = (int)(end - ptr);
      req->method = strndup(ptr, num_bytes);

      ptr = end + 1;

      // http resource
      end = strchr(ptr, '/');
      end = end + 1;
      ptr = strchr(end, ' ');

      num_bytes = (int)(ptr - end);
      req->resource = strndup(end, num_bytes);

      // http version
      end = strchr(ptr, '.');
      end = end - 1;
      req->version = (char *)end;
    }

    // return if the method is GET
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
  const char *line_delim = "\r\n";
  const char *start = request;
  const char *end;

  // get the entire request header (apart from the body payload)
  end = strstr(request, header_delim);
  memcpy(buffer, start, (int)(end - start));
  start = end + 4;

  match_request_headers(buffer, &req);
  req.body_payload = (char *)start;

  return req;
}
