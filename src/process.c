/*
 * process.c
 * Server Infrastructure for Remote Intel PHI Execution
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

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#include "common.h"

int process_phi_cmd(void **result, void *cmd_ptr, void *free_list, void *busy_list, void **client_list, void **client_handle) {
	int phi_result = 0, arg_count = 0;
	PhiCmd *cmd = cmd_ptr;
	uint64_t uint_res = 0, tmp_ptr = 0;
	void *extra_args = NULL, *res_data = NULL;
	size_t extra_args_size = 0, res_length = 0;
	uow **res = NULL;
	var_type res_type;

	if (*client_handle == NULL && cmd->type != INIT) {
		fprintf(stderr, "process_phi_cmd: Invalid client handle\n");
		return -1;
	}

	gdprintf("Processing PHI_CMD\n");
	switch(cmd->type) {
		case INIT:
			/*gdprintf("Executing cuInit...\n");
			get_client_handle(client_handle, client_list, cmd->int_args[0]);
			uint_res = ((client_node *) *client_handle)->id;
			// cuInit() should have already been executed by the server 
			// by that point...
			//cuda_result = cuda_err_print(cuInit(cmd->uint_args[0]), 0);
			cuda_result = CUDA_SUCCESS;
			res_type = UINT;*/
			break;
		case DEVICE_GET:
			/*gdprintf("Executing cuDeviceGet...\n");
			if (update_device_of_client(&uint_res, free_list, cmd->int_args[0], *client_handle) < 0)
				cuda_result = CUDA_ERROR_INVALID_DEVICE;
			else
				cuda_result = CUDA_SUCCESS;

			res_type = UINT;*/
			break;
		case DEVICE_GET_COUNT:
			/*gdprintf("Executing cuDeviceGetCount...\n");
			cuda_result = get_device_count_for_client(&uint_res);
			res_type = UINT;*/
			break;
		case DEVICE_GET_NAME:
			/*gdprintf("Executing cuDeviceGetName...\n");
			cuda_result = get_device_name_for_client(&extra_args, &extra_args_size, cmd->int_args[0], cmd->uint_args[0]);*/
			break;
		/*case CONTEXT_CREATE:
			gdprintf("Executing cuCtxCreate...\n");
			cuda_result = assign_device_to_client(cmd->uint_args[1], free_list, busy_list, *client_handle);
			if (cuda_result	< 0)
				break; // Handle appropriately in client.

			cuda_result = create_context_of_client(&uint_res, cmd->uint_args[0], cmd->uint_args[1], *client_handle);
			res_type = UINT;
			break;
		case CONTEXT_DESTROY:
			gdprintf("Executing cuCtxDestroy...\n");
			// We assume that only one context per device is created
			cuda_result = destroy_context_of_client(&tmp_ptr, cmd->uint_args[0], *client_handle);
			if (cuda_result == CUDA_SUCCESS) {
				free_device_from_client(tmp_ptr, free_list, busy_list, *client_handle);
				if (cmd->n_uint_args > 1 && cmd->uint_args[1] == 1) {
					del_client_of_list(*client_handle);
					*client_handle = NULL;
					//((client_node *) *client_handle)->status = 0;
				}
			}
			break;
		case MODULE_LOAD:
			gdprintf("Executing cuModuleLoad...\n");
			//print_file_as_hex(cmd->extra_args[0].data, cmd->extra_args[0].len);
			cuda_result = load_module_of_client(&uint_res, &(cmd->extra_args[0]), *client_handle);
			res_type = UINT;
			break;
		case MODULE_GET_FUNCTION:
			gdprintf("Executing cuModuleGetFuction...\n");
			cuda_result = get_module_function_of_client(&uint_res, cmd->uint_args[0], cmd->str_args[0], *client_handle);
			res_type = UINT;
			break;
		case MEMORY_ALLOCATE:
			gdprintf("Executing cuMemAlloc...\n");
			cuda_result = memory_allocate_for_client(&uint_res, cmd->uint_args[0]);
			res_type = UINT;
			break;
		case MEMORY_FREE:
			gdprintf("Executing cuMemFree...\n");
			cuda_result = memory_free_for_client(cmd->uint_args[0]);
			break;
		case MEMCPY_HOST_TO_DEV:
			gdprintf("Executing cuMemcpyHtoD...\n");
			cuda_result = memcpy_host_to_dev_for_client(cmd->uint_args[0], cmd->extra_args[0].data, cmd->extra_args[0].len);
			break;
		case MEMCPY_DEV_TO_HOST:
			gdprintf("Executing cuMemcpyDtoH...\n");
			cuda_result = memcpy_dev_to_host_for_client(&extra_args, &extra_args_size, cmd->uint_args[0], cmd->uint_args[1]);
			break;
		case LAUNCH_KERNEL:
			gdprintf("Executing cuLaunchKernel...\n");
			cuda_result = launch_kernel_of_client(cmd->uint_args, cmd->n_uint_args, cmd->extra_args, cmd->n_extra_args);
			break;*/
	}

	if (res_type == UINT) {
		res_length = sizeof(uint64_t);
		res_data = &uint_res;
	} else if (extra_args_size != 0) {	
		res_type = BYTES;
		res_length = extra_args_size;
		res_data = extra_args;
	}


	if (res_length > 0) {
		res = malloc_safe(sizeof(*res) * 2);
		res[1] = malloc_safe(sizeof(**res));
		res[1]->type = res_type;
		res[1]->elements = 1;
		res[1]->length = res_length;
		res[1]->data = malloc_safe(res_length);
		memcpy(res[1]->data, res_data, res_length);
		arg_count = 2;
	} else {
		res = malloc_safe(sizeof(*res));
		arg_count = 1;
	}
	res[0] = malloc_safe(sizeof(**res));
	res[0]->type = INT;
	res[0]->elements = 1;
	res[0]->length = sizeof(int);
	res[0]->data = malloc_safe(res[0]->length);
	memcpy(res[0]->data, &cuda_result, res[0]->length);

	*result = res;

	if (extra_args != NULL)
		free(extra_args);

	return arg_count;
}

