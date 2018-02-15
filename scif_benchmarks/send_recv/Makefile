CC=gcc
CFLAGS=-Wall -I.
LDFLAGS=-lscif

.PHONY: all
all: scif_send scif_recv

scif_send: scif_send.o
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

scif_recv: scif_recv.o
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)


clean:
	rm -fv *.o
	rm -f scif_send scif_recv
