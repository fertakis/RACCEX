CC = gcc
CFLAGS = -I/usr/local/include
LIBS = -L/usr/local/lib -lprotobuf-c

all: example_serializer example_deserializer

example_serializer: proto_example.o amessage.pb-c.o
	$(CC) -o example_serializer proto_example.o amessage.pb-c.o $(LIBS)

example_deserializer: example_deserializer.o amessage.pb-c.o
	$(CC) -o example_deserializer example_deserializer.o amessage.pb-c.o $(LIBS)

example.o: 
	$(CC) -c $(CFLAGS) proto_example.c
example_deserializer.o:
	$(CC) -c $(CFLAGS) example_deserializer.c
amessage.pb-c.o:
	$(CC) -c $(CFLAGS) amessage.pb-c.c

.PHONY: clean
clean:
	rm -f *.o example_serializer example_desearializer
