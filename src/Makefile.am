bin_PROGRAMS = server libscifapiwrapper.so test_server 

AM_CFLAGS = -L$(PROTOBUF_C_LIBDIR) -I/usr/include/google -L$(SCIF_INSTALL_PATH)/lib64  $(DEBUG_CFLAGS)

BUILT_SOURCES = @srcdir@/common.pb-c.c @srcdir@/common.pb-c.h

server_SOURCES = server.c process.c process.h common.h common.c protocol.c
server_SOURCES += protocol.h common.pb-c.c common.pc-c.h

libscifapiwrapper_so_CFLAGS = -fPIC -shared $(DEBUG_CFLAGS)
libscifapiwrapper_so_SOURCES = libscifapiwrapper.c process.c process.h common.h
libscifapiwrapper_so_SOURCES += common.c protocol.c protocol.h client.c client.h common.pb-c.c common.pb-c.h

test_server_SOURCES = test_server.c common.c common.h common.pb-c.c common.pb-c.h client.c client.h
test_server_SOURCES += protocol.c protocol.h

common.pb-c.c: @srcdir@/common.proto
	 $(PROTOC_C) --proto_path=@srcdir@ --c_out=. @srcdir@/common.proto

common.pb-c.h: @srcdir@/common.pb-c.c
	@if test ! -f $@; then \
	  rm -f $<; \
	  $(MAKE) $(AM_MAKEFLAGS) $<; \
	else :; fi

CLEANFILES = @builddir@/common.pb-c.c @builddir@/common.pb-c.h


server_LDADD = $(PROTOBUF_C_LIBS) $(SCIF_LIBS) -lscif
libscifapiwrapper_so_LDADD = $(PROTOBUF_C_LIBS) $(SCIF_LIBS) -lscif -ldl

EXTRA_DIST = common.proto

AM_CFLAGS += -I@builddir@

