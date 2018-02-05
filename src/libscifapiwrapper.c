/*
 * libscifapiwrapper.c
 * SCIF Api Library Wrapper for Remote Intel PHI Execution
 *
 * Konstantinos Fertakis <kfertak@cslab.ece.ntua.gr>
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <string.h>
#include "stdio.h"
#include <linux/errno.h>
#include <stdint.h>
#include <errno.h>

#include "scif.h"
#include "scif_ioctl.h"
#include "common.h"
#include "common.pb-c.h"
#include "client.h"
#include "protocol.h"

//unitofwork uow = { .socket_fd = -1, .endp = -1, .ref_count = 0};
struct thread_mng_list threads;
static uint8_t scif_version_mismatch;

	static	int
scif_get_driver_version(void)
{
	//TODO:Needs revision to support multithreading
	/*int res_code, version = -1;
	PhiCmd *result = NULL;
	void *des_msg = NULL;
	
	//initialise socket & establish connection
	establish_connection(&uow);

	printf("send cookie\n");
	//prepare & send cmd	
	if(send_phi_cmd(<uow->sockfd>, NULL, 0, GET_VERSION) == -1) 
	{
		fprintf(stderr, "Error sending PHI cmd!\n");
		exit(EXIT_FAILURE);
	}

	//receive resutls
	get_phi_cmd_result(&result, &des_msg, uow->sockfd);

	if(res_code == PHI_SUCCESS) {
		version = (int)result->int_args[0];
	}

	printf(" scif_get_driver_version executed \n");
	free_deserialised_message(des_msg);

	//close connection
	close(uow->sockfd);*/

	return -1;
}

	scif_epd_t
scif_open(void)
{
	int res_code;
	scif_epd_t fd;
	PhiCmd *result;
	void *des_msg = NULL;
	thr_mng *uow;

	printf("scif_open...\n");

	uow = identify_thread(&threads);

	if(uow->sockfd < 0)
		establish_connection(uow);
	uow->ref_count++;
	printf("ref_count=%d\n", uow->ref_count);

	if(send_phi_cmd(uow->sockfd, NULL, 0, OPEN) < 0 )
	{
		fprintf(stderr, "Problem sending PHI cmd!\n");
		exit(EXIT_FAILURE);	
	}

	res_code = get_phi_cmd_result(&result, &des_msg, uow->sockfd);
	if(res_code == PHI_SUCCESS) {
		fd = (scif_epd_t)result->int_args[0];
	}
	else { 
		fd = -1;
		errno  = (int)result->phi_errorno;
	}

	free_deserialised_message(des_msg);
	return fd;
}

	int
scif_close(scif_epd_t epd)
{
	int res_code, ret = 0;
	var arg = { .elements = 1}, *args[] = { &arg };
	PhiCmd *result;
	void *des_msg = NULL;
	thr_mng *uow;

	printf("scif_close(epd=%d)\n",epd);

	uow = identify_thread(&threads);

	if(uow->sockfd < 0)
		establish_connection(uow);

	arg.type = INT;
	arg.length = sizeof(int);
	arg.data = &epd;

	if(send_phi_cmd(uow->sockfd, args, 1, CLOSE) < 0)
	{
		fprintf(stderr, "Problem sending PHI cmd!\n");
		exit(EXIT_FAILURE);	
	}	

	res_code = get_phi_cmd_result(&result, &des_msg,  uow->sockfd);

	if(res_code != PHI_SUCCESS) {
		ret = -1;
		errno  = (int)result->phi_errorno;
	}

	free_deserialised_message(des_msg);

	uow->ref_count--;
	if(uow->ref_count == 0) {
		close(uow->sockfd);
		uow->sockfd = -1;
	}

	return ret;
}

	int
scif_bind(scif_epd_t epd, uint16_t pn)
{
	int pni = pn, res_code, ret = -1;
	var arg_int = { .elements =1 }, arg_uint = { .elements = 1}, *args[] = { &arg_int, &arg_uint };
	PhiCmd *result;  
	void *des_msg = NULL;
	thr_mng *uow;

	printf("scif_bind(epd=%d, pn %d)\n", epd, pn);

	uow = identify_thread(&threads);

	if(uow->sockfd < 0)
		establish_connection(uow);

	arg_int.type = INT;
	arg_int.length = sizeof(int);
	arg_int.data = &epd;

	arg_uint.type = UINT;
	arg_uint.length = sizeof(uint16_t);
	arg_uint.data = &pn;

	if(send_phi_cmd(uow->sockfd, args, 2, BIND) < 0)
	{
		fprintf(stderr, "Problem sending PHI cmd!\n");
		exit(EXIT_FAILURE);	
	}	

	res_code = get_phi_cmd_result(&result, &des_msg, uow->sockfd);

	if(res_code == PHI_SUCCESS) {
		ret = (uint16_t)result->int_args[0];
	}
	else {
		ret = -1;
		errno = (int)result->phi_errorno;
	}

	free_deserialised_message(des_msg);

	return ret;
}

	int
scif_listen(scif_epd_t epd, int backlog)
{
	int res_code, ret = 0;
	var arg = { .elements = 2 }, *args[] = { &arg };
	PhiCmd *result;  
	void *des_msg = NULL;
	thr_mng *uow;

	printf("scif_listen(epd=%d, backlog= %d)\n", epd, backlog);

	uow = identify_thread(&threads);

	if(uow->sockfd < 0)
		establish_connection(uow);

	arg.type = INT;
	arg.length = sizeof(int) * arg.elements;
	int *data = (int *)malloc_safe(arg.length);
	data[0] = epd;
	data[1] = backlog;
	arg.data = data;

	if(send_phi_cmd(uow->sockfd, args, 1, LISTEN) < 0)
	{
		fprintf(stderr, "Problem sending PHI cmd!\n");
		exit(EXIT_FAILURE);	
	}	

	res_code = get_phi_cmd_result(&result, &des_msg, uow->sockfd);

	if(res_code != PHI_SUCCESS) {
		ret = -1;
		errno = (int)result->phi_errorno;
	}

	free_deserialised_message(des_msg);

	return ret;
}

	int
scif_connect(scif_epd_t epd, struct scif_portID *dst)
{
	int res_code, ret =-1;
	var arg_int = { .elements = 1}, arg_bytes = { .elements = 1}, *args[] = { &arg_int, &arg_bytes};
	PhiCmd *result; 
	void *des_msg = NULL;
	thr_mng *uow;

	printf("executing scif_connect(epd=%d,dst->node=%d, dst->port=%d\n",epd, dst->node, dst->port);

	uow = identify_thread(&threads);

	if(uow->sockfd < 0)
		establish_connection(uow);

	arg_int.type = INT;
	arg_int.length = sizeof(int);
	arg_int.data = &epd;

	arg_bytes.type = BYTES;
	arg_bytes.length = sizeof(struct scif_portID);
	arg_bytes.data = dst;

	if(send_phi_cmd(uow->sockfd, args, 2, CONNECT) < 0)
	{
		fprintf(stderr, "Problem sending PHI cmd!\n");
		exit(EXIT_FAILURE);	
	}	

	res_code = get_phi_cmd_result(&result, &des_msg, uow->sockfd);
	if(res_code == SCIF_SUCCESS) 
		ret = (int)result->int_args[0];
	else {
		ret = -1;
		errno = (int)result->phi_errorno;
		printf("Error detected : %s\n", strerror(errno));
	}

	free_deserialised_message(des_msg);

	printf("ret =%d\n", ret);

	return ret;

}

	int
scif_accept(scif_epd_t epd, struct scif_portID *peer, scif_epd_t *newepd, int flags)
{
	int res_code, ret = -1 ;
	var arg_int = { .elements =2 }, arg_bytes = { .elements = 1}, *args[]= { &arg_int, &arg_bytes};
	PhiCmd *result = NULL;
	void *des_msg = NULL;
	thr_mng *uow;

	printf("scif_accept(epd=%d...\n", epd);

	uow = identify_thread(&threads);

	if(uow->sockfd < 0)
		establish_connection(uow);

	arg_int.type = INT;
	arg_int.length = sizeof(int)*arg_int.elements;
	int *data = malloc_safe(arg_int.length);
	data[0] = epd;
	data[1] = flags;
	arg_int.data = data;

	arg_bytes.type = BYTES;
	arg_bytes.length = sizeof(struct scif_portID);
	arg_bytes.data = peer;

	if(send_phi_cmd(uow->sockfd, args, 2, ACCEPT) < 0)
	{
		fprintf(stderr, "Problem sending PHI cmd!\n");
		exit(EXIT_FAILURE);	
	}	

	res_code = get_phi_cmd_result(&result, &des_msg, uow->sockfd);
	if(res_code == SCIF_SUCCESS) {
		newepd = (scif_epd_t *)&result->int_args[0];
		ret = 0;
	}
	else {
		ret = -1;
		errno = (int)result->phi_errorno;	
	}

	free_deserialised_message(des_msg);

	return ret;

}

	int
scif_send(scif_epd_t epd, void *msg, int len, int flags)
{
	int res_code, ret = -1; 
	var arg_int = { .elements = 3 }, arg_bytes = { .elements = 1}, *args[] = { &arg_int, &arg_bytes}; 
	PhiCmd *result = NULL;
	void *des_msg = NULL;
	thr_mng *uow;

	printf("executing scif_send(endp=%d, len=%d, flags=%d\n", epd, len, flags);

	uow = identify_thread(&threads);

	if(uow->sockfd < 0)
		establish_connection(uow);

	arg_int.type = INT;
	arg_int.length = sizeof(int)*arg_int.elements;
	int *data = malloc_safe(arg_int.length);
	data[0] = epd;
	data[1] = len;
	data[2] = flags;
	arg_int.data = data;

	arg_bytes.type = BYTES;
	arg_bytes.length = len;
	arg_bytes.data = msg;

	if(send_phi_cmd(uow->sockfd, args, 2, SEND) < 0)
	{
		fprintf(stderr, "Problem sending PHI cmd!\n");
		exit(EXIT_FAILURE);
	}

	res_code = get_phi_cmd_result(&result, &des_msg, uow->sockfd);
	if(res_code == SCIF_SUCCESS) {
		ret = (int)result->int_args[0];
	}
	else {
		ret = -1 ;
		errno = (int)result->phi_errorno;
	}

	free_deserialised_message(des_msg);
	
	printf("scif_send has completed\n");

	return ret; 
}

	int
scif_recv(scif_epd_t epd, void *msg, int len, int flags)
{
	int res_code, ret = -1; 
	var arg_int = { .elements = 3 }, *args[] = { &arg_int}; 
	PhiCmd *result = NULL;
	void *des_msg = NULL;
	thr_mng *uow;

	uow = identify_thread(&threads);

	if(uow->sockfd < 0)
		establish_connection(uow);

	arg_int.type = INT;
	arg_int.length = sizeof(int)*arg_int.elements;
	int *data = malloc_safe(arg_int.length);
	data[0] = epd;
	data[1] = len;
	data[2] = flags;
	arg_int.data = data;

	if(send_phi_cmd(uow->sockfd, args, 1, RECV) < 0)
	{
		fprintf(stderr, "Problem sending PHI cmd!\n");
		exit(EXIT_FAILURE);
	}

	res_code = get_phi_cmd_result(&result, &des_msg, uow->sockfd);
	if(res_code == SCIF_SUCCESS) {
		memcpy(msg, result->extra_args[0].data, result->extra_args[0].len);
		ret = (int)result->int_args[0];
	}
	else {
		ret = -1;
		errno = (int)result->phi_errorno;
	}

	free_deserialised_message(des_msg);

	return ret;
}

	off_t
scif_register(scif_epd_t epd, void *addr, size_t len, off_t offset,
		int prot, int flags)
{
	int res_code;
	off_t ret;
	var arg_int = { .elements = 3 }, arg_uint = {.elements = 1},
			arg_bytes = { .elements = 1 }, 
			*args[] = { &arg_int, &arg_uint, &arg_bytes}; 
	PhiCmd *result = NULL;
	void *des_msg = NULL;
	thr_mng *uow;
	
	printf("executing scif_register with epd %d, addr %d, len %d, offset %d, prot %d, flags %d\n", epd, addr, len, offset, prot, flags);

	uow = identify_thread(&threads);

	if(uow->sockfd < 0)
		establish_connection(uow);

	arg_int.type = INT;
	arg_int.length = sizeof(int)*arg_int.elements;
	int *data = malloc_safe(arg_int.length);
	data[0] = epd;
	data[1] = prot;
	data[2] = flags;
	arg_int.data = data;

	arg_uint.type = UINT;
	arg_uint.length = sizeof(uint32_t)*arg_uint.elements;
	arg_uint.data = &len;

	arg_bytes.type = BYTES;
	arg_bytes.length = sizeof(void *) + sizeof(off_t);
	arg_bytes.data = malloc_safe(arg_bytes.length);
	memcpy(arg_bytes.data, &addr, sizeof(void *));
	memcpy(arg_bytes.data + sizeof(void *), &offset, sizeof(off_t));

	if(send_phi_cmd(uow->sockfd, args, 3, REGISTER) < 0)
	{
		fprintf(stderr, "Problem sending PHI cmd!\n");
		exit(EXIT_FAILURE);
	}

	res_code = get_phi_cmd_result(&result, &des_msg, uow->sockfd);
	if(res_code == SCIF_SUCCESS) {
		memcpy(&ret, result->extra_args[0].data, sizeof(off_t));
	}
	else {
		ret = -1;
		errno = (int)result->phi_errorno; 
	}

	free_deserialised_message(des_msg);

	return ret;
}

	int
scif_unregister(scif_epd_t epd, off_t offset, size_t len)
{
	int res_code, ret = -1;
	var arg_int = { .elements = 1 }, arg_uint = { .elements = 1}, 
			arg_bytes = { .elements = 1 }, 
			*args[] = { &arg_int, &arg_uint, &arg_bytes}; 
	PhiCmd *result = NULL;
	void *des_msg = NULL;
	thr_mng *uow;

	uow = identify_thread(&threads);

	if(uow->sockfd < 0)
		establish_connection(uow);

	arg_int.type = INT;
	arg_int.length = sizeof(int)*arg_int.elements;
	int *data = malloc_safe(arg_int.length);
	data[0] = epd;
	arg_int.data = data;

	arg_uint.type = UINT;
	arg_uint.length = sizeof(uint32_t)*arg_uint.elements;
	arg_uint.data = &len;

	arg_bytes.type = BYTES;
	arg_bytes.length = sizeof(off_t);
	arg_bytes.data = &offset;

	if(send_phi_cmd(uow->sockfd, args, 3, UNREGISTER) < 0)
	{
		fprintf(stderr, "Problem sending PHI cmd!\n");
		exit(EXIT_FAILURE);
	}

	res_code = get_phi_cmd_result(&result, &des_msg, uow->sockfd);
	if(res_code == SCIF_SUCCESS)
		ret = (int)result->int_args[0];
	else {
		ret = -1;
		errno = (int)result->phi_errorno;
	}

	free_deserialised_message(des_msg);

	return ret;
}

	void*
scif_mmap(void *addr, size_t len, int prot, int flags, scif_epd_t epd, off_t offset)
{
	//TODO: To be implemented;
	return (void *)-1;
}

	int
scif_munmap(void *addr, size_t len)
{
	//TODO: To be implemented;
	return -1;
}

	int
scif_readfrom(scif_epd_t epd, off_t loffset, size_t len, off_t roffset, int flags)
{
	int res_code, ret = -1;
	var arg_int = { .elements = 2 }, arg_uint = { .elements = 1}, 
			arg_bytes = { .elements = 2 }, 
			*args[] = { &arg_int, &arg_uint, &arg_bytes}; 
	PhiCmd *result = NULL;
	void *des_msg = NULL;
	thr_mng *uow;

	uow = identify_thread(&threads);

	if(uow->sockfd < 0)
		establish_connection(uow);

	arg_int.type = INT;
	arg_int.length = sizeof(int)*arg_int.elements;
	int *data = malloc_safe(arg_int.length);
	data[0] = epd;
	data[1] = flags;
	arg_int.data = data;
	
	arg_uint.type = UINT;
	arg_uint.length = sizeof(uint32_t)*arg_uint.elements;
	arg_uint.data = &len;

	arg_bytes.type = BYTES;
	arg_bytes.length = sizeof(off_t)*2;
	arg_bytes.data = (off_t *)malloc_safe(arg_bytes.length);
	((off_t *)arg_bytes.data)[0] = loffset;
	((off_t *)arg_bytes.data)[1] = roffset;

	if(send_phi_cmd(uow->sockfd, args, 3, READ_FROM) < 0)
	{
		fprintf(stderr, "Problem sending PHI cmd!\n");
		exit(EXIT_FAILURE);
	}

	res_code = get_phi_cmd_result(&result, &des_msg, uow->sockfd);
	if(res_code == SCIF_SUCCESS)
		ret = (int)result->int_args[0];
	else {
		ret = -1;
		errno = (int)result->phi_errorno;
	}

	free_deserialised_message(des_msg);

	return ret;
}
	int
scif_writeto(scif_epd_t epd, off_t loffset, size_t len, off_t roffset, int flags)
{
	int res_code, ret = -1;
	var arg_int = { .elements = 2 }, arg_uint = { .elements = 1 }, 
		arg_bytes = { .elements = 2 }, 
		*args[] = { &arg_int, &arg_uint, &arg_bytes}; 
	PhiCmd *result = NULL;
	void *des_msg = NULL;
	thr_mng *uow;

	uow = identify_thread(&threads);

	if(uow->sockfd < 0)
		establish_connection(uow);

	arg_int.type = INT;
	arg_int.length = sizeof(int)*arg_int.elements;
	int *data = malloc_safe(arg_int.length);
	data[0] = epd;
	data[1] = flags;
	arg_int.data = data;
		
	arg_uint.type = UINT;
	arg_uint.length = sizeof(uint32_t)*arg_uint.elements;
	arg_uint.data = &len;

	arg_bytes.type = BYTES;
	arg_bytes.length = sizeof(off_t)*2;
	arg_bytes.data = (off_t *)malloc_safe(arg_bytes.length);
	((off_t *)arg_bytes.data)[0] = loffset;
	((off_t *)arg_bytes.data)[1] = roffset;

	if(send_phi_cmd(uow->sockfd, args, 3, WRITE_TO) < 0)
	{
		fprintf(stderr, "Problem sending PHI cmd!\n");
		exit(EXIT_FAILURE);
	}

	res_code = get_phi_cmd_result(&result, &des_msg, uow->sockfd);
	if(res_code == SCIF_SUCCESS)	
		ret = (int)result->int_args[0];
	else {
		ret = -1;
		errno = (int)result->phi_errorno;
	}

	free_deserialised_message(des_msg);

	return ret;
}

	int
scif_vreadfrom(scif_epd_t epd, void *addr, size_t len, off_t offset, int flags)
{
	int res_code, ret = -1;
	var arg_int = { .elements = 2 }, arg_uint = { .elements = 1}, 
			arg_bytes = { .elements = 1 }, 
			*args[] = { &arg_int, &arg_uint, &arg_bytes}; 
	PhiCmd *result = NULL;
	void *des_msg = NULL;
	thr_mng *uow;
	
	uow = identify_thread(&threads);

	if(uow->sockfd < 0)
		establish_connection(uow);

	arg_int.type = INT;
	arg_int.length = sizeof(int)*arg_int.elements;
	int *data = malloc_safe(arg_int.length);
	data[0] = epd;
	data[1] = flags;
	arg_int.data = data;

	arg_uint.type = UINT;
	arg_uint.length = sizeof(uint32_t)*arg_uint.elements;
	arg_uint.data = &len;

	arg_bytes.type = BYTES;
	arg_bytes.length = sizeof(off_t);
	arg_bytes.data = &offset;

	if(send_phi_cmd(uow->sockfd, args, 3, VREAD_FROM) < 0)
	{
		fprintf(stderr, "Problem sending PHI cmd!\n");
		exit(EXIT_FAILURE);
	}

	res_code = get_phi_cmd_result(&result, &des_msg, uow->sockfd);
	if(res_code == SCIF_SUCCESS){
		ret = (int)result->int_args[0];
		memcpy(addr, result->extra_args[0].data, result->extra_args[0].len);
	}
	else {
		ret = -1;
		errno = (int)result->phi_errorno;
	}

	free_deserialised_message(des_msg);

	return ret;
}

	int
scif_vwriteto(scif_epd_t epd, void *addr, size_t len, off_t offset, int flags)
{
	int res_code, ret = -1;
	var arg_int = { .elements = 2 }, arg_uint = { .elements = 1},
		arg_bytes = { .elements = 1 }, 
		*args[] = { &arg_int, &arg_uint, &arg_bytes}; 
	PhiCmd *result = NULL;
	void *des_msg = NULL;
	thr_mng *uow;
	
	uow = identify_thread(&threads);

	if(uow->sockfd < 0)
		establish_connection(uow);

	arg_int.type = INT;
	arg_int.length = sizeof(int)*arg_int.elements;
	int *data = malloc_safe(arg_int.length);
	data[0] = epd;
	data[1] = flags;
	arg_int.data = data;

	arg_uint.type = UINT;
	arg_uint.length = sizeof(size_t)*arg_uint.elements;
	arg_uint.data = &len;

	arg_bytes.type = BYTES;
	arg_bytes.length = sizeof(off_t) + len;
	arg_bytes.data = malloc_safe(arg_bytes.length);
	memcpy(arg_bytes.data, addr, len);
	memcpy(arg_bytes.data + len, &offset, sizeof(off_t));

	if(send_phi_cmd(uow->sockfd, args, 3, VWRITE_TO) < 0)
	{
		fprintf(stderr, "Problem sending PHI cmd!\n");
		exit(EXIT_FAILURE);
	}

	res_code = get_phi_cmd_result(&result, &des_msg, uow->sockfd);
	if(res_code == SCIF_SUCCESS)	
		ret = (int)result->int_args[0];
	else {
		ret = -1;
		errno = (int)result->phi_errorno;
	}

	free_deserialised_message(des_msg);

	return ret;
}

	int
scif_fence_mark(scif_epd_t epd, int flags, int *mark)
{
	int res_code, ret = -1;
	var arg_int = { .elements = 2 }, *args[] = { &arg_int }; 
	PhiCmd *result = NULL;
	void *des_msg = NULL;
	thr_mng *uow;

	uow = identify_thread(&threads);

	if(uow->sockfd < 0)
		establish_connection(uow);

	arg_int.type = INT;
	arg_int.length = sizeof(int)*arg_int.elements;
	int *data = malloc_safe(arg_int.length);
	data[0] = epd;
	data[1] = flags;
	arg_int.data = data;

	if(send_phi_cmd(uow->sockfd, args, 1, FENCE_MARK) < 0)
	{
		fprintf(stderr, "Problem sending PHI cmd!\n");
		exit(EXIT_FAILURE);
	}

	res_code = get_phi_cmd_result(&result, &des_msg, uow->sockfd);
	if(res_code == SCIF_SUCCESS){
		memcpy(mark, &result->int_args[0], sizeof(int));	
		ret = (int)result->int_args[1];
	}
	else {
		ret = -1;
		errno = (int)result->phi_errorno;
	}

	free_deserialised_message(des_msg);

	return ret;
}

	int
scif_fence_wait(scif_epd_t epd, int mark)
{
	int res_code, ret = -1;
	var arg_int = { .elements = 2 }, *args[] = { &arg_int}; 
	PhiCmd *result = NULL;
	void *des_msg = NULL;
	thr_mng *uow;

	uow = identify_thread(&threads);

	if(uow->sockfd < 0)
		establish_connection(uow);
	arg_int.type = INT;
	arg_int.length = sizeof(int)*arg_int.elements;
	int *data = malloc_safe(arg_int.length);
	data[0] = epd;
	data[1] = mark;
	arg_int.data = data;

	if(send_phi_cmd(uow->sockfd, args, 1, FENCE_WAIT) < 0)
	{
		fprintf(stderr, "Problem sending PHI cmd!\n");
		exit(EXIT_FAILURE);
	}

	res_code = get_phi_cmd_result(&result, &des_msg, uow->sockfd);
	if(res_code == SCIF_SUCCESS)
		ret = (int)result->int_args[0];
	else {
		ret = -1;
		errno = (int)result->phi_errorno;
	}

	free_deserialised_message(des_msg);

	return ret;
}

	int
scif_fence_signal(scif_epd_t epd, off_t loff, uint64_t lval,
		off_t roff, uint64_t rval, int flags)
{
	int res_code, ret = -1;
	var arg_int = { .elements = 2 }, arg_u64int = { .elements = 2},
			 arg_bytes = { .elements = 2},
	    		*args[] = { &arg_int, &arg_u64int, &arg_bytes}; 
	PhiCmd *result = NULL;
	void *des_msg = NULL;
	thr_mng *uow;

	uow = identify_thread(&threads);

	if(uow->sockfd < 0)
		establish_connection(uow);

	arg_int.type = INT;
	arg_int.length = sizeof(int)*arg_int.elements;
	int *data = malloc_safe(arg_int.length);
	data[0] = epd;
	data[1] = flags;
	arg_int.data = data;

	arg_u64int.type = U64INT;
	arg_u64int.length = sizeof(uint64_t)*arg_u64int.elements;
	uint64_t *u64ints  = malloc_safe(arg_u64int.length);
	u64ints[0] = lval;
	u64ints[1] = rval;
	arg_u64int.data = u64ints;

	arg_bytes.type = BYTES;
	arg_bytes.length = sizeof(off_t)*2;
	arg_bytes.data = (off_t *)malloc_safe(arg_bytes.length);
	((off_t *)arg_bytes.data)[0] = loff;
	((off_t *)arg_bytes.data)[1] = roff;

	if(send_phi_cmd(uow->sockfd, args, 3, FENCE_SIGNAL) < 0)
	{
		fprintf(stderr, "Problem sending PHI cmd!\n");
		exit(EXIT_FAILURE);
	}

	res_code = get_phi_cmd_result(&result, &des_msg, uow->sockfd);
	if(res_code == SCIF_SUCCESS)
		ret = (int)result->int_args[0];
	else {
		ret = -1;
		errno = (int)result->phi_errorno;
	}

	free_deserialised_message(des_msg);

	return ret;
}

	int
scif_get_nodeIDs(uint16_t *nodes, int len, uint16_t *self)
{
	int res_code, ret = 1; 
	var arg = { .elements = 1 }, *args[] = { &arg };
	PhiCmd *result;
	void *des_msg = NULL;
	thr_mng *uow;

	printf("get_scif_nodes\n");	
	uow = identify_thread(&threads);

	if(uow->sockfd < 0)
		establish_connection(uow);

	arg.type = INT;
	arg.length = sizeof(int);
	arg.data = &len;

	if(send_phi_cmd(uow->sockfd, args, 1, GET_NODE_IDS) < 0)
	{
		fprintf(stderr, "Problem sending PHI cmd!\n");
		exit(EXIT_FAILURE);
	} 	

	res_code = get_phi_cmd_result(&result, &des_msg, uow->sockfd);
	if(res_code == SCIF_SUCCESS) {
		ret = result->int_args[0];
		if (ret > len)
			memcpy(nodes, result->extra_args[0].data, len*sizeof(uint16_t));
		else
			memcpy(nodes, result->extra_args[0].data, ret*sizeof(uint16_t));
		memcpy(self, result->extra_args[0].data+sizeof(uint16_t)*ret, sizeof(uint16_t));

		printf("scif_get_nodeIDs() completed succesfuly with #nodes=%d and node[0]=%d and node[1]=%d \n", ret, nodes[0], nodes[1]);
	}
	else {
		ret = 1;
		errno = (int)result->phi_errorno;
	}

	free_deserialised_message(des_msg);

	/*if(uow.ref_count == 0) {
	  close(uow->sockfd);
	  uow->sockfd = -1;
	  }*/

	return ret;
}

	int
scif_poll(struct scif_pollepd *ufds, unsigned int nfds, long timeout_msecs)
{
	int res_code, ret = -1;
	var arg_uint = { .elements = 2}, arg_bytes = { .elements = 1},
	    *args[] = { &arg_uint, &arg_bytes}; 
	PhiCmd *result = NULL;
	void *des_msg = NULL;
	thr_mng *uow;
	
	printf("executing scif_poll( nfds=%d timeout=%ld)\n", nfds, timeout_msecs);

	uow = identify_thread(&threads);

	if(uow->sockfd < 0)
		establish_connection(uow);

	arg_uint.type = UINT;
	arg_uint.length = sizeof(uint32_t)*arg_uint.elements;
	uint32_t *uints  = malloc_safe(arg_uint.length);
	uints[0] = nfds;
	uints[1] = timeout_msecs;
	arg_uint.data = uints;

	arg_bytes.type = BYTES;
	arg_bytes.length = sizeof(struct scif_pollepd);
	arg_bytes.data = ufds;

	if(send_phi_cmd(uow->sockfd, args, 2, POLL) < 0)
	{
		fprintf(stderr, "Problem sending PHI cmd!\n");
		exit(EXIT_FAILURE);
	}

	res_code = get_phi_cmd_result(&result, &des_msg, uow->sockfd);
	if(res_code == SCIF_SUCCESS){
		ret = result->int_args[0];
	}
	else {
		ret = -1;
		errno = (int)result->phi_errorno;
	}

	free_deserialised_message(des_msg);

	return ret;
}

__attribute__ ((constructor)) static void scif_lib_init(void)
{
	/*int scif_driver_ver = scif_get_driver_version();
	  if ((scif_driver_ver > 0) && (scif_driver_ver != SCIF_VERSION))
	  scif_version_mismatch = 1;*/
	initialise_thr_mng_list(&threads);
}
