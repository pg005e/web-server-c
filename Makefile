CC = gcc
OBJ = common.o server.o

all: program

program: $(OBJ)
	$(CC) -o server $(OBJ)

server.o: server.c
	$(CC) -c server.c -o server.o

common.o: common.c
	$(CC) -c common.c -o common.o 

client: client.c
	$(CC) -o client client.c

test: test.c
	$(CC) -o test test.c

clean:
	rm -f *.o
