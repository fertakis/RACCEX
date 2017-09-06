CC = gcc
CFLAGS = -I/usr/local/include
LIBS = -L/usr/local/lib -lprotobuf-c

all: client server

client: client_proto_example.o amessage.pb-c.o
	$(CC) -o client client_proto_example.o amessage.pb-c.o $(LIBS)

server: server.o amessage.pb-c.o
	$(CC) -o server server_protobuf.o amessage.pb-c.o $(LIBS)

client.o: 
	$(CC) -c $(CFLAGS) client_proto_example.c
server.o:
	$(CC) -c $(CFLAGS) server_protobuf.c
amessage.pb-c.o:
	$(CC) -c $(CFLAGS) amessage.pb-c.c

.PHONY: clean
clean:
	rm -f *.o client server
