/*
 * client.c
 * Client application for Remote Intel PHI Execution
 *
 * Konstantinos Fertakis <kfertak@cslab.ece.ntua.gr>
 */

#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <netdb.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#include "include/common.h"
#include "include/client.h"
#include "include/common.pb-c.h"
#include "include/protocol.h"

void initialise_thr_mng_list(struct thread_mng_list *list) {
	list->num_threads = 1;
	list->head = malloc_safe(sizeof(thr_mng));
	list->head->thread_id = pthread_self();
	list->head->sockfd = -1;
	list->head->ref_count = 0;
	list->head->next = NULL;
}

thr_mng * identify_thread(struct thread_mng_list *threads) 
{
	pthread_t curr = pthread_self();
	thr_mng *ret = NULL;

	thr_mng *it = threads->head;
	while (it->next != NULL) {
		if((pthread_equal(curr, it->thread_id) != 0 )) {
			//found it
			ret = it;
			break;
		}
		it = it->next;
	}
	if(ret == NULL) {
		//not found, check last entry
		if((pthread_equal(curr, it->thread_id) != 0))
			//found, was the last entry
			ret = it;
		else {
			//not found, create new entry
			ret = malloc_safe(sizeof(thr_mng));
			ret->thread_id = curr;
			ret->sockfd = -1;
			ret->ref_count = 0;
			ret->next = NULL;
			it->next = ret;
			threads->num_threads++;
		}
	}
	return ret; 
}

int init_client_connection(const char *s_ip, const char *s_port)
{
	int sd, port;
	struct hostent *hp;
	struct sockaddr_in sa;

	/* Create TCP/IP socket, used as main chat channel */
	if ((sd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		exit(1);
	}
	rcdprintf("Created TCP socket\n");

	/* Look up remote hostname on DNS */
	if ( !(hp = gethostbyname(s_ip))) {
		printf("DNS lookup failed for host %s\n", s_ip);
		exit(1);
	}

	/* Connect to remote TCP port */
	sa.sin_family = AF_INET;
	sa.sin_port = htons(atoi(s_port));
	memcpy(&sa.sin_addr.s_addr, hp->h_addr, sizeof(struct in_addr));
	rcdprintf("Connecting to remote host...\n"); 
	fflush(stderr);
	if (connect(sd, (struct sockaddr *) &sa, sizeof(sa)) < 0) {
		perror("connect");
		exit(1);
	}
	rcdprintf("Connected.\n");

	return sd;
}

void establish_connection(thr_mng *uow) {
	char *server, *server_port;

	get_server_connection_config(&server, &server_port);

	uow->sockfd = init_client_connection(server, server_port);
	rcdprintf("Connected to server %s on port %s...\n", server, server_port);
}

int send_phi_cmd(int socket_fd, PhiCmd *cmd)
{
	void *buf = NULL, *payload = NULL;
	size_t len;

	ddprintf("Preparing and sending Phi cmd %d by thread %d\n", cmd->type, (int)pthread_self());

	//pack_phi_cmd(&payload, args, arg_cnt, cmd_type);
	TIMER_START(&ser);
	len = serialise_message(&buf, PHI_CMD, cmd);
	TIMER_STOP(&ser);
	if(buf == NULL)
		return -1;
	TIMER_START(&smsg);
	send_message(socket_fd, buf, len);
	TIMER_STOP(&smsg);

	free(buf);
	free(payload);
	return 0;
}

int get_phi_cmd_result(PhiCmd **result, Cookie **cookie, int socket_fd)
{
	size_t len;
	void *buf = NULL;
	PhiCmd *res;
	Cookie *ck;
	int res_code;

	ddprintf("Waiting result from PHI Server...\n");
	len = receive_message(&buf, socket_fd);

	TIMER_STOP(&call);
	TIMER_START(&after);
	if(len > 0)
		deserialise_message(&ck, &res, buf, len);
	else {
		fprintf(stderr, "Problem receiving server response.\n");
		exit(EXIT_FAILURE);
	}

	if(res == NULL) {
		fprintf(stderr, "Problem deserialising message.\n");
		exit(EXIT_FAILURE);
	} else {
		*result = res;
		res_code = res->phi_result_code;
		ddprintf("Server responded: \n| result code: %d\n", res_code);
		if(res_code != 0)
			ddprintf("error detected ! errno=%d\n", res->phi_errorno);

		*cookie = ck;
	}

	if(buf != NULL)
		free(buf);
	return res_code;
}

void free_arg_data(var **args, int len) {
	int i;
	for(i=0; i < len ; i++) {
		free(args[i]->data);
	}
}
