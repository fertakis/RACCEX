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
#include "common.h"
#include "common.pb-c.h"
#include "client.h"


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

unitofwork uow;

static uint8_t scif_version_mismatch;

	int
scif_get_driver_version(void)
{
	int res_code, version;
	void *result = NULL;
	var arg = { .elements = 1}; 

	//initialise parameters
/*	init_params(&uow);

	//initialise socket & establish connection
	establish_connection(&uow);
	//prepare & send cmd
	if(send_phi_cmd(uow.socket_fd, NULL, 0, PHI_CMD) == -1) 
	{
		fprintf(stderr, "Error sending PHI cmd!\n");
		exit(EXIT_FAILURE);
	}
	//receive resutls
	get_phi_cmd_result(&result, uow.socket_fd);*/
	/*if(res_code == PHI_SUCCESS) {
	  version = *(int *) result;
	  free(result);
	  }*/

	return 1;
}

	scif_epd_t
scif_open(void)
{
	int res_code;
	scif_epd_t fd;
	var arg = {.elements = 1}, *args[] = { &arg };
	void *result;

	init_params(&uow);

	establish_connection(&uow);

	if(send_phi_cmd(uow.socket_fd, NULL, 0, OPEN) < 0 )
	{
		fprintf(stderr, "Problem sending PHI cmd!\n");
		exit(EXIT_FAILURE);	
	}

	res_code = get_phi_cmd_result(&result, uow.socket_fd);
	if(res_code == PHI_SUCCESS) {
		fd = *(scif_epd_t *)result;
		uow.endp = fd;
		free(result);
	}

	return fd;
}
only_version(scif_open, 0, 0)

	int
scif_close(scif_epd_t epd)
{
	int res_code;
	var arg = { .elements = 1}, *args[] = { &arg };
	void *result;

	arg.type = INT;
	arg.length = sizeof(int);
	arg.data = &epd;

	if(send_phi_cmd(uow.socket_fd, args, 1, CLOSE) < 0)
	{
		fprintf(stderr, "Problem sending PHI cmd!\n");
		exit(EXIT_FAILURE);	
	}	

	res_code = get_phi_cmd_result(&result, uow.socket_fd);

	free(result);

	return res_code;
}
only_version(scif_close, 0, 0)

	int
scif_bind(scif_epd_t epd, uint16_t pn)
{
	int pni = pn, res_code, ret = -1;
	var arg_int = { .elements =1 }, arg_uint = { .elements = 1}, *args[] = { &arg_int, &arg_uint };
	void *result;  

	arg_int.type = INT;
	arg_int.length = sizeof(int);
	arg_int.data = &epd;

	arg_uint.type = UINT;
	arg_uint.length = sizeof(uint16_t);
	arg_uint.data = &pn;

	if(send_phi_cmd(uow.socket_fd, args, 2, BIND) < 0)
	{
		fprintf(stderr, "Problem sending PHI cmd!\n");
		exit(EXIT_FAILURE);	
	}	

	res_code = get_phi_cmd_result(&result, uow.socket_fd);

	if(res_code == PHI_SUCCESS) {
		ret = *(uint16_t *)result;
	}

	return ret;
}
only_version(scif_bind, 0, 0)

	int
scif_listen(scif_epd_t epd, int backlog)
{
	int res_code, ret = -1;
	var arg = { .elements = 2 }, *args[] = { &arg };
	void *result;  

	arg.type = INT;
	arg.length = sizeof(int) * arg.elements;
	int *data = (int *)malloc_safe(arg.length);
	data[0] = epd;
	data[1] = backlog;
	arg.data = data;
	
	if(send_phi_cmd(uow.socket_fd, args, 1, LISTEN) < 0)
	{
		fprintf(stderr, "Problem sending PHI cmd!\n");
		exit(EXIT_FAILURE);	
	}	

	res_code = get_phi_cmd_result(&result, uow.socket_fd);
	
	ret = *(int *) result ;

	return ret;
}
only_version(scif_listen, 0, 0)

	int
scif_connect(scif_epd_t epd, struct scif_portID *dst)
{
	int res_code, ret =-1;
	var arg_int = { .elements = 1}, arg_bytes = { .elements = 1}, *args[] = { &arg_int, &arg_bytes};
	void *result; 

	arg_int.type = INT;
	arg_int.length = sizeof(int);
	arg_int.data = &epd;
	
	arg_bytes.type = BYTES;
	arg_bytes.length = sizeof(struct scif_portID);
	arg_bytes.data = dst;
	
	if(send_phi_cmd(uow.socket_fd, args, 2, CONNECT) < 0)
	{
		fprintf(stderr, "Problem sending PHI cmd!\n");
		exit(EXIT_FAILURE);	
	}	

	res_code = get_phi_cmd_result(&result, uow.socket_fd);
	if(res_code == SCIF_SUCCESS) 
		ret = *(int *) result ;

	return ret;

}
only_version(scif_connect, 0, 0)

	int
scif_accept(scif_epd_t epd, struct scif_portID *peer, scif_epd_t *newepd, int flags)
{
	int res_code, ret = -1 ;
	var arg_int = { .elements =2 }, arg_bytes = { .elements = 1}, *args[]= { &arg_int, &arg_bytes};
	void *result = NULL;

	arg_int.type = INT;
	arg_int.length = sizeof(int)*arg_int.elements;
	int *data = malloc_safe(arg_int.length);
	data[0] = epd;
	data[1] = flags;
	arg_int.data = data;
	
	arg_bytes.type = BYTES;
	arg_bytes.length = sizeof(struct scif_portID);
	arg_bytes.data = peer;

	if(send_phi_cmd(uow.socket_fd, args, 2, ACCEPT) < 0)
	{
		fprintf(stderr, "Problem sending PHI cmd!\n");
		exit(EXIT_FAILURE);	
	}	

	res_code = get_phi_cmd_result(&result, uow.socket_fd);
	if(res_code == SCIF_SUCCESS) {
		newepd = (scif_epd_t *) result;
		ret = 0;
	}

	return ret;

}
only_version(scif_accept, 0, 0)

	int
scif_send(scif_epd_t epd, void *msg, int len, int flags)
{
	struct scifioctl_msg send_msg =
	{ .msg = msg, .len = len, .flags = flags };
	if (ioctl(epd, SCIF_SEND, &send_msg) < 0)
		return -1;
	return send_msg.out_len;
}
only_version(scif_send, 0, 0)

	int
scif_recv(scif_epd_t epd, void *msg, int len, int flags)
{
	struct scifioctl_msg recv_msg =
	{ .msg = msg, .len = len, .flags = flags };

	if (ioctl(epd, SCIF_RECV, &recv_msg) < 0)
		return -1;
	return recv_msg.out_len;
}
only_version(scif_recv, 0, 0)

	off_t
scif_register(scif_epd_t epd, void *addr, size_t len, off_t offset,
		int prot, int flags)
{
	struct scifioctl_reg reg =
	{ .addr = addr, .len = len, .offset = offset,
		.prot = prot, .flags = flags };

	if (ioctl(epd, SCIF_REG, &reg) < 0)
		return -1;
	return reg.out_offset;
}
only_version(scif_register, 0, 0)

	int
scif_unregister(scif_epd_t epd, off_t offset, size_t len)
{
	struct scifioctl_unreg unreg =
	{ .len = len, .offset = offset};

	if (ioctl(epd, SCIF_UNREG, &unreg) < 0)
		return -1;
	return 0;
}
only_version(scif_unregister, 0, 0)

	void*
scif_mmap(void *addr, size_t len, int prot, int flags, scif_epd_t epd, off_t offset)
{
	return mmap(addr, len, prot, (flags | MAP_SHARED), (int)epd, offset);
}
only_version(scif_mmap, 0, 0)

	int
scif_munmap(void *addr, size_t len)
{
	return munmap(addr, len);
}
only_version(scif_munmap, 0, 0)

	int
scif_readfrom(scif_epd_t epd, off_t loffset, size_t len, off_t roffset, int flags)
{
	struct scifioctl_copy copy =
	{.loffset = loffset, .len = len, .roffset = roffset, .flags = flags };

	if (ioctl(epd, SCIF_READFROM, &copy) < 0)
		return -1;
	return 0;
}
only_version(scif_readfrom, 0, 0)

	int
scif_writeto(scif_epd_t epd, off_t loffset, size_t len, off_t roffset, int flags)
{
	struct scifioctl_copy copy =
	{.loffset = loffset, .len = len, .roffset = roffset, .flags = flags };

	if (ioctl(epd, SCIF_WRITETO, &copy) < 0)
		return -1;
	return 0;
}
only_version(scif_writeto, 0, 0)

	int
scif_vreadfrom(scif_epd_t epd, void *addr, size_t len, off_t offset, int flags)
{
	struct scifioctl_copy copy =
	{.addr = addr, .len = len, .roffset = offset, .flags = flags };

	if (ioctl(epd, SCIF_VREADFROM, &copy) < 0)
		return -1;
	return 0;
}
only_version(scif_vreadfrom, 0, 0)

	int
scif_vwriteto(scif_epd_t epd, void *addr, size_t len, off_t offset, int flags)
{
	struct scifioctl_copy copy =
	{.addr = addr, .len = len, .roffset = offset, .flags = flags };

	if (ioctl(epd, SCIF_VWRITETO, &copy) < 0)
		return -1;
	return 0;
}
only_version(scif_vwriteto, 0, 0)

	int
scif_fence_mark(scif_epd_t epd, int flags, int *mark)
{
	struct scifioctl_fence_mark fence_mark =
	{.flags = flags, .mark = mark};

	if (ioctl(epd, SCIF_FENCE_MARK, &fence_mark) < 0)
		return -1;
	return 0;
}
only_version(scif_fence_mark, 0, 0)

	int
scif_fence_wait(scif_epd_t epd, int mark)
{
	if (ioctl(epd, SCIF_FENCE_WAIT, mark) < 0)
		return -1;
	return 0;
}
only_version(scif_fence_wait, 0, 0)

	int
scif_fence_signal(scif_epd_t epd, off_t loff, uint64_t lval,
		off_t roff, uint64_t rval, int flags)
{
	struct scifioctl_fence_signal signal =
	{.loff = loff, .lval = lval, .roff = roff,
		.rval = rval, .flags = flags};

	if (ioctl(epd, SCIF_FENCE_SIGNAL, &signal) < 0)
		return -1;

	return 0;
}
only_version(scif_fence_signal, 0, 0)

	int
scif_get_nodeIDs(uint16_t *nodes, int len, uint16_t *self)
{
	scif_epd_t fd;
	struct scifioctl_nodeIDs nodeIDs = {.nodes = nodes, .len = len, .self = self};

	if ((fd = open(DEVICE_NODE, O_RDWR)) < 0)
		return -1;

	if (ioctl(fd, SCIF_GET_NODEIDS, &nodeIDs) < 0) {
		close(fd);
		return -1;
	}

	close(fd);

	return nodeIDs.len;
}
only_version(scif_get_nodeIDs, 0, 0)

	int
scif_poll(struct scif_pollepd *ufds, unsigned int nfds, long timeout_msecs)
{
	return poll((struct pollfd*)ufds, nfds, timeout_msecs);
}
only_version(scif_poll, 0, 0)

__attribute__ ((constructor)) static void scif_lib_init(void)
{
	int scif_driver_ver = scif_get_driver_version();
	if ((scif_driver_ver > 0) && (scif_driver_ver != SCIF_VERSION))
		scif_version_mismatch = 1;
}
