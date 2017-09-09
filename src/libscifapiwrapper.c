#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include "stdio.h"
#include <linux/errno.h>
#include <stdint.h>
#include <errno.h>

#include "scif.h"
#include "scif_ioctl.h"


#define DEVICE_NODE "/dev/mic/scif"

#define __symver_tag SCIF

#define __symver(S, at, M, N) \
	__asm__ (".symver " __str(V(S, M, N)) "," __V(S, at, M, N));
#define __V(symbol, at, major, minor) \
	__str(symbol) at __str(__symver_tag) "_" __str(major) "." __str(minor)
#define __str(s) ___str(s)
#define ___str(s) #s

#ifndef GENMAP_PARSING_PASS
#define V(symbol, major, minor) \
	__ ## symbol ## _ ## major ## _ ## minor
#define compatible_version(symbol, major, minor) \
	__symver(symbol, "@", major, minor)
#define default_version(symbol, major, minor) \
	__symver(symbol, "@@", major, minor)
#define only_version(symbol, major, minor)
#endif


