INSTALL = install
INSTALL_d = $(INSTALL) -d
INSTALL_x = $(INSTALL)
INSTALL_f = $(INSTALL) -m644

prefix ?= /usr/local
exec_prefix ?= $(prefix)
bindir ?= $(exec_prefix)/bin/

SRCDIR = .
TUTESRC = $(SRCDIR)
TUTEINSTALL = /usr/share/doc/scif/tutorials

CXXFLAGS += -g -I. -I../../include -Iusr/include

BOTHTESTS = \
	scif_connect \
	scif_accept \
	scif_connect_poll \
	scif_accept_poll \
	scif_connect_multiple \
	scif_accept_multiple \
	scif_connect_launch \
	scif_accept_launch \
	scif_connect_register \
	scif_accept_register \
	scif_connect_rma_mmap \
	scif_accept_rma_mmap \
	scif_connect_rma_fork \
	scif_accept_rma_fork \
	scif_connect_ringbuffer \
	scif_accept_ringbuffer \
	scif_connect_readwrite_p1 \
	scif_connect_readwrite_p2 \
	scif_accept_readwrite \
	scif_connect_vreadwrite_p1 \
	scif_connect_vreadwrite_p2 \
	scif_accept_vreadwrite \
	scif_connect_rma_register_p1 \
	scif_connect_rma_register_p2 \
	scif_accept_rma_register \
	scif_connect_rma_fence \
	scif_accept_rma_fence

define destfile
$$(DESTDIR)/$(notdir $(1)): $(1) | destdir
	$(3) $$< $(DESTDIR)$(2)
DESTFILES += $$(DESTDIR)/$(notdir $(1))
endef

.PHONY: all install

.DEFAULT_GOAL := all

target_arch=$(shell $(CC) -dumpmachine | sed 's:-.*::')
ifeq ($(target_arch),x86_64)
TESTS = $(BOTHTESTS:=_host)
%_host: %.c
	$(CC) $(CFLAGS) $(LDFLAGS)  -Wall -DHOST $< -g -lscif -lpthread -o $@
else ifeq ($(target_arch),k1om)
TESTS = $(BOTHTESTS:=_mic)
%_mic: %.c
	$(CC) $(CFLAGS) $(LDFLAGS)  -Wall $< -lscif -lpthread -o $@
else
        $(error Architecture '$(target_arch)' is not supported)
endif

$(foreach dest,$(TESTS),$(eval $(call destfile,$(dest),$(bindir), $(INSTALL_x))))

destdir:
	$(INSTALL_d) $(DESTDIR)$(bindir)

install: destdir $(DESTFILES)
	$(INSTALL_d) $(DESTDIR)$(TUTEINSTALL)
	$(INSTALL_f) $(TUTESRC)/*.* $(DESTDIR)$(TUTEINSTALL)
	$(INSTALL_f) $(TUTESRC)/Makefile $(DESTDIR)$(TUTEINSTALL)

all: $(TESTS)

clean:
	rm -rf bin/*
	rm -rf *_mic
	rm -rf *_host
