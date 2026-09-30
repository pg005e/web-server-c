#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

typedef struct {
  char *method;
  char *resource;
  char *version;
  int content_length;
  char *body_payload;
  char *connection;
  int keep_alive;
} HttpRequest;

HttpRequest parse_request(char *request);

#endif
