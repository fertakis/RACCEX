bin_PROGRAMS = server libscifapiwrapper.so 

AM_CFLAGS = -I$(PROTOBUF_C_CFLAGS) -L$(PROTOBUF_C_LIBDIR) -I@builddir@ -I/usr/include/google $(DEBUG_CFLAGS)

server_SOURCES = server.c process.c process.h common.h common.c communication.c
communication.h common.pb-c common.pc-c.h

libscifapiwrapper_so_SOURCES = libscifapiwrapper.c process.c process.h common.h
common.c communication.c communication.h client.c client.h common.pb-c.c common.pb-c.h


