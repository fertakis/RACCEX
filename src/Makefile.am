bin_PROGRAMS = server libscifapiwrapper.so 

AM_CFLAGS = -I$(PROTOBUF_C_CFLAGS) -L$(PROTOBUF_C_LIBDIR) -I@builddir@ -I/usr/include/google $(DEBUG_CFLAGS)

BUILT_SOURCES = @srcdir@/common.pb-c.c @srcdir@/common.pb-c.h

server_SOURCES = server.c process.c process.h common.h common.c communication.c
server_SOURCES += ommunication.h common.pb-c common.pc-c.h

libscifapiwrapper_so_SOURCES = libscifapiwrapper.c process.c process.h common.h
libscifapiwrapper_so_SOURCES += ommon.c communication.c communication.h client.c client.h common.pb-c.c common.pb-c.h

common.pb-c.c: @srcdir@/common.proto
	 $(PROTOC_C) --proto_path=@srcdir@ --c_out=. @srcdir@/common.proto

common.pb-c.h: @srcdir@/common.pb-c.c
	@if test ! -f $@; then \
	  rm -f $<; \
	  $(MAKE) $(AM_MAKEFLAGS) $<; \
	else :; fi

CLEANFILES = @builddir@/common.pb-c.c @builddir@/common.pb-c.h
EXTRA_DIST = common.proto

AM_CFLAGS += -I@builddir@


