###################################################
#
# Makefile
# Remote Intel PHI Execution
#
# Konstantinos Fertakis <kfertak@cslab.ece.ntua.gr>
#
###################################################

CC = gcc
ICC = /various/common_tools/intel-icc-and-tools/bin/icc

CFLAGS = -Wall
#ICFLAGS = $(CFLAGS) -g -mmic -lscif
CFLAGS += -g
# CFLAGS += -O2 -fomit-frame-pointer -finline-functions

#set ICC license file
#export INTEL_LICENSE_FILE=/various/common_tools/intel-icc-and-tools/licenses/

LIBS = -lscif

BINS = server client

all: $(BINS)

server: server.c common.h
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)

client: client.c common.h
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)

clean:
	rm -f *.o *~ $(BINS)
