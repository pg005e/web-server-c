#include "httprequest.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

HttpRequest parse_request(char *request) {
  HttpRequest req;
  char buffer[BUFSIZ];
  char line[120];

  const char *header_delim = "\r\n\r\n";
  const char *line_delim = "\r\n";
  const char *req_start = request;
  const char *req_end;

  // get the entire header
  req_end = strstr(request, header_delim);
  memcpy(buffer, req_start, (int)(req_end - req_start));
  req_start = req_end + 1;

  // get each line in the header
  // identify the content
  while ((req_end = strstr(request, line_delim)) != NULL) {

    req_start = req_end + 1;
  }

  return req;
}
