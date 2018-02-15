CC=gcc
CFLAGS=-Wall -I.
LDFLAGS=-lscif

.PHONY: all
all: scif_write scif_read scif_register_write scif_register_read

scif_register_write: scif_register_write.o
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@

scif_register_read: scif_register_read.o
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@

scif_write: scif_write.o
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@

scif_read: scif_read.o
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@


clean:
	rm -v *.o
	rm scif_write scif_read
