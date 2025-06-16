#include "common.h"
#include <stdio.h>

void error(const char *msg) {
  perror(msg);
  exit(1);
}

/* Address Info */
void address() {
  struct addrinfo hints;
  struct addrinfo *info;
  int ret;
  char ipstr[14];

  memset(&hints, 0, sizeof hints);

  hints.ai_protocol = 0;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_family = AF_INET;

  ret = getaddrinfo("127.0.0.1", "621", &hints, &info);
  if (ret != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
    exit(1);
  }

  // get the first address returned by getaddrinfo()
  struct sockaddr_in *ipv4 = (struct sockaddr_in *)info->ai_addr;
  inet_ntop(AF_INET, &ipv4->sin_addr.s_addr, ipstr, sizeof(ipstr));

  // print IP address of client
  printf("%s\n", ipstr);

  freeaddrinfo(info);
}

/*Communicate with a client connection */
void chat_client(int client_fd) {
  char buf[BUFSIZ];
  int ret;

  // send rules to client
  bzero(buf, BUFSIZ);
  char msg[] = "Press 'Ctrl + ]' to end the conversation\r\n";
  ret = write(client_fd, msg, sizeof(msg));
  if (ret < 0)
    error("client did not get the rules");

  while (1) {
    int pos = 0;

    // read from client
    bzero(buf, BUFSIZ);
    while (pos < BUFSIZ - 1) {
      ret = read(client_fd, &buf[pos], 1);
      if (ret < 0)
        error("ERROR reading from socket");
      if (buf[pos] == '\0') // newline
        break;
      ++pos;
    }

    // exit condition
    if (pos == 2 && buf[0] == '\035') {
      printf("terminating conversation...\n");
      return;
    }

    // output client message
    printf("prayer: %s", buf);

    // send a reply back to the client
    bzero(buf, BUFSIZ);
    strcpy(buf, "hello darkness my old friend\r\n");
    ret = write(client_fd, buf, strlen(buf) + 1);
    if (ret < 0)
      error("ERROR writing to client");
  }

  return;
}
