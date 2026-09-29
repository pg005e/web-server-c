.PHONY: all clean test

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

test: all
	./server & SRV_PID=$$!; sleep 0.5; \
	curl -s http://127.0.0.1:6969/ | grep -q "Hello World" && echo "PASS: GET /" || echo "FAIL: GET /"; \
	curl -s http://127.0.0.1:6969/../../etc/passwd | grep -q "Hello World" && echo "FAIL: path traversal" || echo "PASS: path traversal blocked"; \
	kill $$SRV_PID 2>/dev/null; wait $$SRV_PID 2>/dev/null; true

clean:
	rm -f *.o server
