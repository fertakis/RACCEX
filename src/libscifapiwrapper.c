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
#include <pthread.h>
#include "scif.h"
#include "scif_ioctl.h"
#include "include/common.h"
#include "include/common.pb-c.h"
#include "include/client.h"
#include "include/protocol.h"

struct thread_mng_list threads;
static uint8_t scif_version_mismatch;

	static	int
scif_get_driver_version(void)
{
	//TODO:Needs revision to support multithreading
/*	int res_code, version = -1;
	PhiCmd *result = NULL;
	void *des_msg = NULL;
	
	//initialise socket & establish connection
	printf("scif_open... by thread=%d\n", pthread_self());
	
	uow = identify_thread(&threads);

	if(uow->sockfd < 0)
		establish_connection(uow);

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
	PhiCmd *cmd, *result;
	void *des_msg = NULL;
	thr_mng *uow;
	
	uow = identify_thread(&threads);

	if(uow->sockfd < 0)
		establish_connection(uow);
	uow->ref_count++;
	ddprintf("ref_count=%d\n", uow->ref_count);

	cmd = malloc_safe(sizeof(PhiCmd));
	phi_cmd__init(cmd);
	cmd->type = OPEN;
	cmd->arg_count = 0;

	if(send_phi_cmd(uow->sockfd, cmd) < 0 )
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
	PhiCmd *cmd,*result;
	void *des_msg = NULL;
	thr_mng *uow;

	uow = identify_thread(&threads);

	if(uow->sockfd < 0)
		establish_connection(uow);

	cmd = malloc_safe(sizeof(PhiCmd));	
	phi_cmd__init(cmd);
	cmd->type = CLOSE;
	cmd->arg_count = 1;
	cmd->n_int_args = 1;
	cmd->int_args = &epd;

	if(send_phi_cmd(uow->sockfd, cmd) < 0)
	{
		fprintf(stderr, "Problem sending PHI cmd!\n");
		exit(EXIT_FAILURE);	
	}

	free(cmd);	

	res_code = get_phi_cmd_result(&result, &des_msg,  uow->sockfd);

	if(res_code != PHI_SUCCESS) {
		ret = -1;
		errno  = (int)result->phi_errorno;
	}

	free_deserialised_message(des_msg);

	return ret;
}

	int
scif_bind(scif_epd_t epd, uint16_t pn)
{
	int pni = pn, res_code, ret = -1;
	PhiCmd *cmd, *result;  
	void *des_msg = NULL;
	thr_mng *uow;

	uow = identify_thread(&threads);

	if(uow->sockfd < 0)
		establish_connection(uow);

	cmd = malloc_safe(sizeof(PhiCmd));
	phi_cmd__init(cmd);
	cmd->type = BIND;
	cmd->arg_count = 2;
	cmd->n_int_args = 1 ;
	cmd->int_args = &epd;
	cmd->n_uint_args = 1;
	cmd->uint_args = &pn;

	if(send_phi_cmd(uow->sockfd, cmd) < 0)
	{
		fprintf(stderr, "Problem sending PHI cmd!\n");
		exit(EXIT_FAILURE);	
	}	

	free(cmd);

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
	PhiCmd *cmd, *result;  
	void *des_msg = NULL;
	thr_mng *uow;

	uow = identify_thread(&threads);

	if(uow->sockfd < 0)
		establish_connection(uow);

	cmd = malloc_safe(sizeof(PhiCmd));
	phi_cmd__init(cmd);
	cmd->type = LISTEN;
	cmd->arg_count = 1;
	cmd->n_int_args = 2;
	int *data = (int *)malloc_safe(sizeof(int)*2);
	data[0] = epd;
	data[1] = backlog;
	cmd->int_args = data;

	if(send_phi_cmd(uow->sockfd, cmd) < 0)
	{
		fprintf(stderr, "Problem sending PHI cmd!\n");
		exit(EXIT_FAILURE);	
	}	

	//free arg allocated memory
	free(data);
	free(cmd);

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
	PhiCmd *cmd,*result; 
	void *des_msg = NULL;
	thr_mng *uow;

	uow = identify_thread(&threads);

	if(uow->sockfd < 0)
		establish_connection(uow);
	
	cmd = malloc_safe(sizeof(PhiCmd));
	phi_cmd__init(cmd);
	cmd->type = CONNECT;
	cmd->arg_count = 2;
	cmd->n_int_args = 1;
	cmd->int_args = &epd;
	
	cmd->n_extra_args = 1;
	cmd->extra_args = malloc_safe(sizeof(ProtobufCBinaryData));
	cmd->extra_args[0].len = sizeof(struct scif_portID);
	cmd->extra_args[0].data = (uint8_t *)dst;

	if(send_phi_cmd(uow->sockfd, cmd) < 0)
	{
		fprintf(stderr, "Problem sending PHI cmd!\n");
		exit(EXIT_FAILURE);	
	}	

	free(cmd->extra_args);
	free(cmd);

	res_code = get_phi_cmd_result(&result, &des_msg, uow->sockfd);
	if(res_code == SCIF_SUCCESS) 
		ret = (int)result->int_args[0];
	else {
		ret = -1;
		errno = (int)result->phi_errorno;
		ddprintf("Error detected : %s\n", strerror(errno));
	}

	free_deserialised_message(des_msg);

	return ret;
}

	int
scif_accept(scif_epd_t epd, struct scif_portID *peer, scif_epd_t *newepd, int flags)
{
	int res_code, ret = -1 ;
	PhiCmd *cmd,*result = NULL;
	void *des_msg = NULL;
	thr_mng *uow;

	uow = identify_thread(&threads);

	if(uow->sockfd < 0)
		establish_connection(uow);

	cmd = malloc_safe(sizeof(PhiCmd));
	phi_cmd__init(cmd);
	cmd->type = ACCEPT;
	cmd->arg_count = 2;
	cmd->n_int_args = 2;
	int *data = malloc_safe(sizeof(int)*2);
	data[0] = epd;
	data[1] = flags;
	cmd->int_args = data;

	cmd->n_extra_args = 1;
	cmd->extra_args = malloc_safe(sizeof(ProtobufCBinaryData));
	cmd->extra_args[0].len = sizeof(struct scif_portID);
	cmd->extra_args[0].data = (uint8_t *)peer;

	if(send_phi_cmd(uow->sockfd, cmd) < 0)
	{
		fprintf(stderr, "Problem sending PHI cmd!\n");
		exit(EXIT_FAILURE);	
	}	

	//free arg allocated memory
	free(data);
	free(cmd->extra_args);
	free(cmd);

	res_code = get_phi_cmd_result(&result, &des_msg, uow->sockfd);
	if(res_code == SCIF_SUCCESS) {
		memcpy(newepd, &result->int_args[0], sizeof(scif_epd_t));
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
	PhiCmd *cmd, *result = NULL;
	void *des_msg = NULL;
	thr_mng *uow;

	uow = identify_thread(&threads);
	
	if(uow->sockfd < 0)
		establish_connection(uow);

	cmd = malloc_safe(sizeof(PhiCmd));
	phi_cmd__init(cmd);
	cmd->type = SEND;
	cmd->arg_count = 2;
	cmd->n_int_args = 3;
	int *data = malloc_safe(sizeof(int));
	data[0] = epd;
	data[1] = len;
	data[2] = flags;
	cmd->int_args = data;

	cmd->n_extra_args = 1;
	cmd->extra_args = malloc_safe(sizeof(ProtobufCBinaryData));
	cmd->extra_args[0].len = len;
	cmd->extra_args[0].data = msg;

	if(send_phi_cmd(uow->sockfd, cmd) < 0)
	{
		fprintf(stderr, "Problem sending PHI cmd!\n");
		exit(EXIT_FAILURE);
	}

	free(data);
	free(cmd->extra_args);
	free(cmd);

	res_code = get_phi_cmd_result(&result, &des_msg, uow->sockfd);
	if(res_code == SCIF_SUCCESS) {
		ret = (int)result->int_args[0];
	}
	else {
		ret = -1 ;
		errno = (int)result->phi_errorno;
	}

	free_deserialised_message(des_msg);
	
	return ret; 
}

	int
scif_recv(scif_epd_t epd, void *msg, int len, int flags)
{
	int res_code, ret = -1; 
	PhiCmd *cmd,*result = NULL;
	void *des_msg = NULL;
	thr_mng *uow;
	
	uow = identify_thread(&threads);

	if(uow->sockfd < 0)
		establish_connection(uow);

	cmd = malloc_safe(sizeof(PhiCmd));
	phi_cmd__init(cmd);
	cmd->type = RECV;
	cmd->arg_count = 1;
	cmd->n_int_args = 3;
	int *data = malloc_safe(sizeof(int)*3);
	data[0] = epd;
	data[1] = len;
	data[2] = flags;
	cmd->int_args = data;

	if(send_phi_cmd(uow->sockfd, cmd) < 0)
	{
		fprintf(stderr, "Problem sending PHI cmd!\n");
		exit(EXIT_FAILURE);
	}

	free(data);
	free(cmd);

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
	PhiCmd *cmd, *result = NULL;
	void *des_msg = NULL;
	thr_mng *uow;
	pid_t pid = getpid();
	
	uow = identify_thread(&threads);

	if(uow->sockfd < 0)
		establish_connection(uow);
	
	cmd = malloc_safe(sizeof(PhiCmd));
	phi_cmd__init(cmd);
	cmd->type = REGISTER;
	cmd->arg_count = 3;
	cmd->n_int_args = 3;
	int *data = malloc_safe(sizeof(int)*3);
	data[0] = epd;
	data[1] = prot;
	data[2] = flags;
	cmd->int_args = data;

	cmd->n_uint_args = 1;
	cmd->uint_args = &len;

	cmd->n_extra_args = 3;
	cmd->extra_args = malloc_safe(sizeof(ProtobufCBinaryData)*3);
	cmd->extra_args[0].len = sizeof(void *);
	cmd->extra_args[0].data = (uint8_t *)&addr; 
	cmd->extra_args[1].len = sizeof(off_t);
	cmd->extra_args[1].data = (uint8_t *)&offset;
	cmd->extra_args[2].len = sizeof(pid_t);
	cmd->extra_args[2].data = (uint8_t *)&pid;

	if(send_phi_cmd(uow->sockfd, cmd) < 0)
	{
		fprintf(stderr, "Problem sending PHI cmd!\n");
		exit(EXIT_FAILURE);
	}

	free(data);
	free(cmd->extra_args);
	free(cmd);

	res_code = get_phi_cmd_result(&result, &des_msg, uow->sockfd);
	if(res_code == SCIF_SUCCESS) {
		memcpy(&ret, result->extra_args[0].data, sizeof(off_t));
		addr_map *entry = identify_map(pid, addr, NULL, len, ret); 
		if(entry == NULL)
			rdprintf(out_fd, "error entring data for registered address\n");
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
	PhiCmd *cmd, *result = NULL;
	void *des_msg = NULL;
	thr_mng *uow;
	pid_t pid = getpid();

	uow = identify_thread(&threads);

	if(uow->sockfd < 0)
		establish_connection(uow);

	cmd = malloc_safe(sizeof(PhiCmd));
	phi_cmd__init(cmd);
	cmd->type = UNREGISTER;
	cmd->arg_count = 3;
	cmd->n_int_args = 1;
	cmd->int_args = &epd;
	
	cmd->n_uint_args = 1;
	cmd->uint_args = &len;
	cmd->n_extra_args = 2;
	cmd->extra_args = malloc_safe(sizeof(ProtobufCBinaryData)*2);
	cmd->extra_args[0].len = sizeof(off_t);
	cmd->extra_args[0].data = (uint8_t *)&offset;
	cmd->extra_args[1].len = sizeof(pid_t);
	cmd->extra_args[1].data = (uint8_t *)&pid;

	if(send_phi_cmd(uow->sockfd, cmd) < 0)
	{
		fprintf(stderr, "Problem sending PHI cmd!\n");
		exit(EXIT_FAILURE);
	}

	free(cmd->extra_args);
	free(cmd);

	res_code = get_phi_cmd_result(&result, &des_msg, uow->sockfd);
	if(res_code == SCIF_SUCCESS){
		ret = (int)result->int_args[0];
		if(remove_mapping(pid, offset) < 0)
			rdprintf(out_fd, "error unregistering address space\n"); 
	}
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
	printf("scif_mmap() was called\n");
	return NULL;
}

	int
scif_munmap(void *addr, size_t len)
{
	//TODO: To be implemented;
	printf("scif_munmap() was called\n");
	return -1;
}

	int
scif_readfrom(scif_epd_t epd, off_t loffset, size_t len, off_t roffset, int flags)
{
	int res_code, ret = -1;
	PhiCmd *cmd,*result = NULL;
	void *des_msg = NULL;
	thr_mng *uow;
	pid_t pid = getpid();
	addr_map *mp = get_map(pid, loffset);
	
	if(mp == NULL) {
		printf("scif_readfrom: error while acquiring map\n. Exiting.\n");
		ret = -1;
		goto end;
	}	

	uow = identify_thread(&threads);

	if(uow->sockfd < 0)
		establish_connection(uow);

	//RMA_SYNC
	flags = flags | SCIF_RMA_SYNC;

	cmd = malloc_safe(sizeof(PhiCmd));
	phi_cmd__init(cmd);
	cmd->type = READ_FROM;
	cmd->arg_count = 3;
	cmd->n_int_args = 2;
	int *data = malloc_safe(sizeof(int)*2);
	data[0] = epd;
	data[1] = flags;
	cmd->int_args = data;

	cmd->n_uint_args = 1;
	cmd->uint_args = &len;	

	cmd->n_extra_args = 3;
	cmd->extra_args  = malloc_safe(sizeof(ProtobufCBinaryData)*3);
	cmd->extra_args[0].len = sizeof(off_t);
	cmd->extra_args[0].data = (uint8_t *)&loffset;
	cmd->extra_args[1].len = sizeof(off_t);
	cmd->extra_args[1].data = (uint8_t *)&roffset;
	cmd->extra_args[2].len = sizeof(pid_t);
	cmd->extra_args[2].data = (uint8_t *)&pid;

	if(send_phi_cmd(uow->sockfd, cmd) < 0)
	{
		fprintf(stderr, "Problem sending PHI cmd!\n");
		exit(EXIT_FAILURE);
	}
	
	free(data);
	free(cmd->extra_args);
	free(cmd);

	res_code = get_phi_cmd_result(&result, &des_msg, uow->sockfd);
	if(res_code == SCIF_SUCCESS) {
		ret = (int)result->int_args[0];
		void *addr_to_write = mp->client_addr + (loffset - mp->offset);
		memcpy(addr_to_write, result->extra_args[0].data, len);
	}	
	else {
		ret = -1;
		errno = (int)result->phi_errorno;
	}

	free_deserialised_message(des_msg);

end:
	return ret;
}
	int
scif_writeto(scif_epd_t epd, off_t loffset, size_t len, off_t roffset, int flags)
{
	int res_code, ret = -1;
	PhiCmd *cmd, *result = NULL;
	void *des_msg = NULL;
	thr_mng *uow;
	pid_t pid = getpid();
	addr_map *mp = get_map(pid, loffset);

	//breakdown analysis

	TIMER_RESET(&b_fd);
	TIMER_RESET(&b_cp);
	TIMER_RESET(&snd);
	TIMER_RESET(&pack);
	TIMER_RESET(&ser);
	TIMER_RESET(&smsg);
	TIMER_RESET(&call);
	TIMER_RESET(&after);

	TIMER_START(&b_fd);	

	if(mp == NULL) {
		printf("scif_writeto: error while acquiring map\n. Exiting.\n");
		ret = -1;
		goto end;
	}

	uow = identify_thread(&threads);

	if(uow->sockfd < 0)
		establish_connection(uow);
	
	TIMER_STOP(&b_fd);
	TIMER_START(&b_cp);
	//RMA_SYNC
	flags = flags | SCIF_RMA_SYNC;

	cmd = malloc_safe(sizeof(PhiCmd));
	phi_cmd__init(cmd);
	cmd->type = WRITE_TO;
	cmd->arg_count = 3;
	cmd->n_int_args = 2;
	int *data = malloc_safe(sizeof(int)*2);
	data[0] = epd;
	data[1] = flags;
	cmd->int_args = data;
		
	cmd->n_uint_args = 1;
	cmd->uint_args = &len;

	cmd->n_extra_args = 4;
	cmd->extra_args  = malloc_safe(sizeof(ProtobufCBinaryData)*4);
	cmd->extra_args[0].len = sizeof(off_t);
	cmd->extra_args[0].data = (uint8_t *)&loffset;
	cmd->extra_args[1].len = sizeof(off_t);
	cmd->extra_args[1].data = (uint8_t *)&roffset;
	cmd->extra_args[2].len = sizeof(pid_t);
	cmd->extra_args[2].data = (uint8_t *)&pid;
	cmd->extra_args[3].len = len;

	void *addr_to_copy_from = mp->client_addr + (loffset - mp->offset);
	cmd->extra_args[3].data = (uint8_t *)addr_to_copy_from;
	TIMER_STOP(&b_cp);

	TIMER_START(&snd);
	if(send_phi_cmd(uow->sockfd, cmd) < 0)
	{
		fprintf(stderr, "Problem sending PHI cmd!\n");
		exit(EXIT_FAILURE);
	}

	free(data);
	free(cmd->extra_args);
	free(cmd);
	TIMER_STOP(&snd);
	TIMER_START(&call);

	res_code = get_phi_cmd_result(&result, &des_msg, uow->sockfd);

	if(res_code == SCIF_SUCCESS)	
		ret = (int)result->int_args[0];
	else {
		ret = -1;
		errno = (int)result->phi_errorno;
	}

	free_deserialised_message(des_msg);

end:
	TIMER_STOP(&after);

	printf("TIME IDENTIFY: %llu us %lf sec\n", TIMER_TOTAL(&b_fd), TIMER_TOTAL(&b_fd)/1000000.0);
	printf("TIME COPY: %llu us %lf sec\n", TIMER_TOTAL(&b_cp), TIMER_TOTAL(&b_cp)/1000000.0);
	printf("TIME SEND: %llu us %lf sec\n", TIMER_TOTAL(&snd), TIMER_TOTAL(&snd)/1000000.0);
	printf("TIME PACK: %llu us %lf sec\n", TIMER_TOTAL(&pack), TIMER_TOTAL(&pack)/1000000.0);
	printf("TIME SERIALIZE: %llu us %lf sec\n", TIMER_TOTAL(&ser), TIMER_TOTAL(&ser)/1000000.0);
	printf("TIME SEND_MESSAGE: %llu us %lf sec\n", TIMER_TOTAL(&smsg), TIMER_TOTAL(&smsg)/1000000.0);
	printf("TIME DURING CALL: %llu us %lf sec\n", TIMER_TOTAL(&call), TIMER_TOTAL(&call)/1000000.0);
	printf("TIME AFTER: %llu us %lf sec\n", TIMER_TOTAL(&after), TIMER_TOTAL(&after)/1000000.0);
	return ret;
}

	int
scif_vreadfrom(scif_epd_t epd, void *addr, size_t len, off_t offset, int flags)
{
	int res_code, ret = -1;
	PhiCmd *cmd,*result = NULL;
	void *des_msg = NULL;
	thr_mng *uow;
	
	uow = identify_thread(&threads);

	if(uow->sockfd < 0)
		establish_connection(uow);
	//RMA_SYNC
	flags = flags | SCIF_RMA_SYNC;

	cmd = malloc_safe(sizeof(PhiCmd));
	phi_cmd__init(cmd);
	cmd->type = VREAD_FROM;
	cmd->arg_count = 3;
	cmd->n_int_args = 2;
	int *data = malloc_safe(sizeof(int)*2);
	data[0] = epd;
	data[1] = flags;
	cmd->int_args = data;

	cmd->n_uint_args = 1;
	cmd->uint_args = &len;

	cmd->n_extra_args = 1;
	cmd->extra_args = malloc_safe(sizeof(ProtobufCBinaryData));
	cmd->extra_args[0].len = sizeof(off_t);
	cmd->extra_args[0].data = (uint8_t *)&offset;

	if(send_phi_cmd(uow->sockfd, cmd) < 0)
	{
		fprintf(stderr, "Problem sending PHI cmd!\n");
		exit(EXIT_FAILURE);
	}

	free(data);
	free(cmd->extra_args);
	free(cmd);

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
	PhiCmd *cmd,*result = NULL;
	void *des_msg = NULL;
	thr_mng *uow;
	
	uow = identify_thread(&threads);

	if(uow->sockfd < 0)
		establish_connection(uow);
	//RMA_SYNC
	flags = flags | SCIF_RMA_SYNC;

	cmd = malloc_safe(sizeof(PhiCmd));
	phi_cmd__init(cmd);
	cmd->type = VWRITE_TO;
	cmd->arg_count = 3;
	cmd->n_int_args = 2;
	int *data = malloc_safe(sizeof(int)*2);
	data[0] = epd;
	data[1] = flags;
	cmd->int_args = data;

	cmd->n_uint_args = 1;
	cmd->uint_args = &len;

	cmd->n_extra_args = 2;
	cmd->extra_args = malloc_safe(sizeof(ProtobufCBinaryData)*2);
	cmd->extra_args[0].len = sizeof(off_t);
	cmd->extra_args[0].data = (uint8_t *)offset;
	cmd->extra_args[1].len = len;
	cmd->extra_args[1].data = (uint8_t *)addr;

 	if(send_phi_cmd(uow->sockfd, cmd) < 0)
	{
		fprintf(stderr, "Problem sending PHI cmd!\n");
		exit(EXIT_FAILURE);
	}

	free(data);
	free(cmd->extra_args);
	free(cmd);

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
	PhiCmd *cmd,*result = NULL;
	void *des_msg = NULL;
	thr_mng *uow;

	uow = identify_thread(&threads);

	if(uow->sockfd < 0)
		establish_connection(uow);

	cmd = malloc_safe(sizeof(PhiCmd));
	phi_cmd__init(cmd);
	cmd->type = FENCE_MARK;
	cmd->arg_count = 1;
	cmd->n_int_args = 2;
	int *data = malloc_safe(sizeof(int)*2);
	data[0] = epd;
	data[1] = flags;
	cmd->int_args = data;

	if(send_phi_cmd(uow->sockfd, cmd) < 0)
	{
		fprintf(stderr, "Problem sending PHI cmd!\n");
		exit(EXIT_FAILURE);
	}

	free(data);
	free(cmd);
	
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
	PhiCmd *cmd,*result = NULL;
	void *des_msg = NULL;
	thr_mng *uow;

	uow = identify_thread(&threads);

	if(uow->sockfd < 0)
		establish_connection(uow);

	cmd = malloc_safe(sizeof(PhiCmd));
	phi_cmd__init(cmd);

	cmd->type = FENCE_WAIT;
	cmd->arg_count = 1;
	cmd->n_int_args = 2;
	int *data = malloc_safe(sizeof(int)*2);
	data[0] = epd;
	data[1] = mark;
	cmd->int_args = data;

	if(send_phi_cmd(uow->sockfd, cmd) < 0)
	{
		fprintf(stderr, "Problem sending PHI cmd!\n");
		exit(EXIT_FAILURE);
	}

	free(data);
	free(cmd);

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
	PhiCmd *cmd,*result = NULL;
	void *des_msg = NULL;
	thr_mng *uow;
	
	uow = identify_thread(&threads);

	if(uow->sockfd < 0)
		establish_connection(uow);

	cmd = malloc_safe(sizeof(PhiCmd));
	phi_cmd__init(cmd);
	cmd->type = FENCE_SIGNAL;
	cmd->arg_count = 3;
	cmd->n_int_args = 2;
	int *data = malloc_safe(sizeof(int)*2);
	data[0] = epd;
	data[1] = flags;
	cmd->int_args = data;

	cmd->n_u64int_args = 2;
	uint64_t *u64ints  = malloc_safe(sizeof(uint64_t)*2);
	u64ints[0] = lval;
	u64ints[1] = rval;
	cmd->u64int_args= u64ints;

	cmd->n_extra_args = 2; 
	cmd->extra_args = malloc_safe(sizeof(ProtobufCBinaryData)*2);
	cmd->extra_args[0].len = sizeof(off_t);
	cmd->extra_args[0].data = (uint8_t * )&loff;
	cmd->extra_args[1].len = sizeof(off_t);
	cmd->extra_args[1].data = (uint8_t * )&roff;

	if(send_phi_cmd(uow->sockfd, cmd) < 0)
	{
		fprintf(stderr, "Problem sending PHI cmd!\n");
		exit(EXIT_FAILURE);
	}

	free(data);
	free(u64ints);
	free(cmd->extra_args);
	free(cmd);

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
	PhiCmd *cmd, *result;
	void *des_msg = NULL;
	thr_mng *uow;

	uow = identify_thread(&threads);

	if(uow->sockfd < 0)
		establish_connection(uow);

	cmd = malloc_safe(sizeof(PhiCmd));
	phi_cmd__init(cmd);
	cmd->type = GET_NODE_IDS;
	cmd->arg_count = 1;
	cmd->n_int_args = 1;
	cmd->int_args = &len;

	if(send_phi_cmd(uow->sockfd, cmd) < 0)
	{
		fprintf(stderr, "Problem sending PHI cmd!\n");
		exit(EXIT_FAILURE);
	} 	

	free(cmd);

	res_code = get_phi_cmd_result(&result, &des_msg, uow->sockfd);
	if(res_code == SCIF_SUCCESS) {
		ret = result->int_args[0];
		if (ret > len)
			memcpy(nodes, result->extra_args[0].data, len*sizeof(uint16_t));
		else
			memcpy(nodes, result->extra_args[0].data, ret*sizeof(uint16_t));
		memcpy(self, result->extra_args[0].data+sizeof(uint16_t)*ret, sizeof(uint16_t));
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
	PhiCmd *cmd,*result = NULL;
	void *des_msg = NULL;
	thr_mng *uow;
	
	uow = identify_thread(&threads);

	if(uow->sockfd < 0)
		establish_connection(uow);

	cmd = malloc_safe(sizeof(PhiCmd));
	phi_cmd__init(cmd);
	cmd->type = POLL;
	cmd->arg_count = 2;
	cmd->n_uint_args = 2;
	uint32_t *uints  = malloc_safe(sizeof(uint32_t)*2);
	uints[0] = nfds;
	uints[1] = timeout_msecs;
	cmd->uint_args = uints;

	cmd->n_extra_args = 1;
	cmd->extra_args = malloc_safe(sizeof(ProtobufCBinaryData));
	cmd->extra_args[0].len = sizeof(struct scif_pollepd)*nfds;
	cmd->extra_args[0].data= (uint8_t *)ufds;

	if(send_phi_cmd(uow->sockfd, cmd) < 0)
	{
		fprintf(stderr, "Problem sending PHI cmd!\n");
		exit(EXIT_FAILURE);
	}

	free(uints);
	free(cmd->extra_args);
	free(cmd);

	res_code = get_phi_cmd_result(&result, &des_msg, uow->sockfd);
	if(res_code == SCIF_SUCCESS){
		ret = result->int_args[0];
		memcpy(ufds, result->extra_args[0].data, sizeof(struct scif_pollepd)*nfds);
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
	initialise_addr_map_list();
}
