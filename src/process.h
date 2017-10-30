#ifndef PROCESS_H
#define PROCESS_H

int process_phi_cmd(void **result, void *cmd_ptr);

int pack_phi_cmd(void **payload, var **args, size_t arg_count,int type);

#endif /* PROCESS_H */
