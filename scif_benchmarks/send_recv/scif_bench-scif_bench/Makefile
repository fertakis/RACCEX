#prefix ?= /home/users/kpapazaf/intel-mpss/scif_bench 
exec_prefix ?= $(prefix)
bindir ?= $(exec_prefix)/bin/

SRCDIR = .

CXXFLAGS += -g -I. -I./include -Iusr/include
CFLAGS += -I./include

BOTHTESTS = \
	scif-bench

.PHONY: all

.DEFAULT_GOAL := all

target_arch=$(shell $(CC) -dumpmachine | sed 's:-.*::')
ifeq ($(target_arch),x86_64)
TESTS = $(BOTHTESTS:=_host)
%_host: %.o common.o
	$(CC) $(CFLAGS) $(LDFLAGS) -Wall -DHOST $^ -lscif -lpthread -lrt -o $@
else ifeq ($(target_arch),k1om)
TESTS = $(BOTHTESTS:=_mic)
%_mic: %.c
	$(CC) $(CFLAGS) $(LDFLAGS) -Wall $< -lscif -lpthread -lrt -o $@
else
        $(error Architecture '$(target_arch)' is not supported)
endif

%.o: %.c
	$(CC) $(CFLAGS) $(LDFLAGS) -Wall -c $< -lscif -lpthread -lrt -o $@

all: $(TESTS)

clean:
	rm -rf *_mic
	rm -rf *_host
