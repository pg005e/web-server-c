.PHONY: all clean

CC = gcc
CFLAGS = -Wall -Wextra -Werror -fsanitize=address 
OBJ = common.o file.o server.o httprequest.o

all: program

program: $(OBJ)
	$(CC) $(CFLAGS) -o server $(OBJ)

server.o: server.c
	$(CC) $(CFLAGS) -c server.c -o server.o

common.o: common.c
	$(CC) $(CFLAGS) -c common.c -o common.o

file.o: file.c
	$(CC) $(CFLAGS) -c file.c -o file.o

httprequest.o: httprequest.c
	$(CC) $(CFLAGS) -c httprequest.c -o httprequest.o

client: client.c
	$(CC) -o client client.c

clean:
	rm -f *.o
