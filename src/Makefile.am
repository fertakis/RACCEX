bin_PROGRAMS = server libscifapiwrapper.so  #test_client hello_world 

AM_CFLAGS = -L$(PROTOBUF_C_LIBDIR) -I/usr/include/google -L/home/fertakis/repos/remotePhiExec/src/ #-DRSCIF_DEBUG -DRSCIF_COMM_DEBUG

BUILT_SOURCES =common.pb-c.c common.pb-c.h

server_CFLAGS = -DBREAKDOWN
server_SOURCES = server.c process.c common.c protocol.c
server_SOURCES += include/common.pb-c.c timer.c

libscifapiwrapper_so_CFLAGS = -fPIC -shared -DBREAKDOWN#-DDEEP_DEBUG#-DRSCIF_DEBUG #-DMEMDEBUG $(DEBUG_CFLAGS)
libscifapiwrapper_so_SOURCES = libscifapiwrapper.c process.c
libscifapiwrapper_so_SOURCES += common.c protocol.c client.c include/common.pb-c.c 

#test_server_SOURCES = test_server.c common.c common.h common.pb-c.c common.pb-c.h client.c client.h
#test_server_SOURCES += protocol.c protocol.h

#test_client_SOURCES = test_client.c
#test_client_LDADD = -lscif

#hello_world_SOURCES = hello_world.c
#hello_world_CFLAGS = -fopenmp

common.pb-c.c: @srcdir@/common.proto
	 $(PROTOC_C) --proto_path=@srcdir@ --c_out=include/ @srcdir@/common.proto

common.pb-c.h: @srcdir@/common.pb-c.c
	@if test ! -f $@; then \
	  rm -f $<; \
	  $(MAKE) $(AM_MAKEFLAGS) $<; \
	else :; fi

CLEANFILES = @builddir@/include/common.pb-c.c @builddir@/include/common.pb-c.h


server_LDADD = $(PROTOBUF_C_LIBS) -lpthread -L/home/fertakis/repos/remotePhiExec/src/ -lscif
libscifapiwrapper_so_LDADD = $(PROTOBUF_C_LIBS) $(SCIF_LIBS) -L/home/fertakis/repos/remotePhiExec/src/ -lscif -ldl

EXTRA_DIST = common.proto

AM_CFLAGS += -I@builddir@

