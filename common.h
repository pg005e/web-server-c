#ifndef COMMON_H
#define COMMON_H

#include <netdb.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <asm-generic/socket.h>

#define PORT 6969

void error(const char *msg);

#endif
