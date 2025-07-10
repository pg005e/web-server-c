#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

typedef enum {
  GET,
  POST,
  PUT,
  PATCH,
  DELETE,
} HttpMethod;

typedef struct {
  HttpMethod method;
  char *resource;
  float version;
  char *body_payload;
  int content_length;
} HttpRequest;

HttpRequest parse_request(char *request);

#endif
