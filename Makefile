CC = gcc
CFLAGS = -Wall
TARGETS = client server

all: $(TARGETS)

err.o: err.c err.h

message.o: message.c message.h

client.o: client.c err.h message.h

client: client.o err.o message.o

server.o: server.c err.h message.h

server: server.o err.o message.o

clean:
	rm -f *.o $(TARGETS)