
#include <stdio.h>        /* perror */
#include <stdlib.h>       /* malloc, free, exit */
#include <limits.h>       /* INT_MAX ... */
#include <string.h>
#include <time.h>
#include <math.h>	/* INFINITY */

#include <unistd.h>

#include "alloc.h"
#include "sl_pq.h"



//> Create a new node when key does not exist in the skip list without setting its next fields.
struct pq_node *pq_node_new(long key, int toplevel)
{

	struct pq_node *node;

	XMALLOC(node, 1);
	XMALLOC(node->next, toplevel);
	XMALLOC(node->prev, toplevel);
	XMALLOC(node->thr_str, 1);

	node->key = key;
	node->height = toplevel;

	return node;
}

/*//> Add a vertex_id to a pq_node
struct id_node *pq_node_add_id(struct pq_node *node, unsigned int vertex_id)
{
	struct id_node *node_id;

	XMALLOC(node_id, 1);

	node_id->vertex_id = vertex_id;
	node_id->prev = NULL;
	node_id->next = node->id_list;
	node->id_list->prev = node_id;
	node->id_list = node_id;
	return node_id;
}*/

//> Delete a node
void pq_node_free(struct pq_node *node)
{
	free(node->thr_str);
    	free(node->next);
    	free(node->prev);
	free(node);
  	return;
}

/*//> Delete a vertex_id from a pq_node
struct id_node *pq_node_delete_vertex_id(pq_t *pq, struct pq_node *node, unsigned int vertex_id)
{
	struct id_node *cur = pq->where_in_id_list[vertex_id];

	if(cur->prev != NULL){
		cur->prev->next = cur->next;
		if(cur->next != NULL)
			cur->next->prev = cur->prev;
	}else{
		if(cur->next != NULL)
			cur->next->prev = NULL;
		node->id_list = cur->next;
	}


	//cur->next = NULL;
	//cur->prev = NULL;
	//free(cur);

  	return cur;
}*/

pq_t *pq_create(unsigned int nvertices)
{
	unsigned int i;
	pq_t *ret;

	XMALLOC(ret, 1);
	ret->nvertices = nvertices;
	ret->levelmax = (int) round(log(nvertices)/log(2)); // FIXME
	ret->curr_size = 0;

	XMALLOC(ret->where_in_pq, nvertices);
	memset(ret->where_in_pq, 0, nvertices * sizeof(*ret->where_in_pq));

	ret->head = pq_node_new(LONG_MIN, ret->levelmax);
	ret->tail = pq_node_new(LONG_MAX,  ret->levelmax);

	for (i=0; i < ret->levelmax; i++) {
		ret->head->next[i] = ret->tail;
		ret->head->prev[i] = NULL;
		ret->tail->next[i] = NULL;
		ret->tail->prev[i] = ret->head;
	}

	return ret;
}

void delete_pq(pq_t *pq)
{
	struct pq_node *cur, *prev;
	struct id_node *cur_node, *prev_node;
	int count = 0;
	int count_ids = 0;

	for (cur = pq->head->next[0]; cur != pq->tail;){
		cur_node = cur->id_list;
		while(cur_node->next != NULL){
			prev_node = cur_node;
			cur_node = cur_node->next;
			free(prev_node);
		}
		prev = cur;	
	 	cur = cur->next[0];
		free(prev->next);
		free(prev->prev);
		free(prev);
	}		
	free(pq->head->next);
	free(pq->head->prev);
	free(pq->head);

	free(pq->tail->next);
	free(pq->tail->prev);
	free(pq->tail);

	return count;
}


int get_rand_level(int levelmax)
{
    int i, level = 1, ret;

    for (i=0; i < levelmax - 1; i++) {
		ret = rand() % 100;
        if (ret < 50)
            level++;
        else
            break;
    }

    return level;
}

//> Insert a node if does not exist or add a vertex_id to an existing node
int pq_insert(pq_t *pq, int key, unsigned int vertex_id)
{ 
	int cmpKey;
	int h;
	struct pq_node **updateArr, *cur;
	
	cur = pq->head;
	XMALLOC(updateArr, pq->levelmax);

	// find where we should put the new node
	for (h = pq->levelmax - 1; h >= 0; h--) {
		cmpKey = cur->next[h]->key;
		while (cmpKey < key) {
			cur = cur->next[h];
			cmpKey = (cur->next[h])->key;
		}
		if(cmpKey == key)
			break;
		updateArr[h] = cur;
	}

	if(cmpKey != key){	// if the key does not exist
		int nodeHeight = get_rand_level(pq->levelmax); // find the height of the new node
		struct pq_node *node = pq_node_new(key, vertex_id, nodeHeight);

		for(h=0; h < nodeHeight; h++) {
			node->prev[h] = updateArr[h];
			node->next[h] = updateArr[h]->next[h];
			updateArr[h]->next[h]->prev[h] = node;
			updateArr[h]->next[h] = node;
		}
		
		pq->where_in_pq[vertex_id] = node;
		pq->where_in_id_list[vertex_id] = node->id_list;
	}else{
		struct id_node *node_id = pq_node_add_id(cur->next[h], vertex_id);
		pq->where_in_pq[vertex_id] = cur->next[h];	
		pq->where_in_id_list[vertex_id] = node_id;
	}

	pq->curr_size++;
	return 1;
}

/*
int print(pq_t *pq)
{
	struct pq_node *cur;
	struct id_node *cur_node;
	int count = 0;
	int count_ids = 0;

	for (cur = pq->head->next[0]; cur != pq->tail; cur = cur->next[0]){
		cur_node = cur->id_list;
		count++;
		count_ids += cur->id_size;
		while(cur_node->next != NULL){
			cur_node = cur_node->next;
			count++;
		}
	}		

	//printf("count %d size %d ids  %d\n",count, pq->curr_size, count_ids);
	if(count != pq->curr_size)
		printf("different size");
	return count;
}
*/
int pq_read_min(pq_t *pq, int index)
{
	int i=0, j;
	struct pq_node *cur = pq->head->next[0];
	struct id_node *cur_id = cur->id_list;

	i=0;
	
	while(cur != pq->tail) {
		while(i < index && cur_id != NULL && cur_id->next != NULL){
			cur_id = cur_id->next;
			i++;
		}
		if(i == index) 
			break;
		cur = cur->next[0];
		cur_id = cur->id_list;
		i++;
	}


	//> If we reached tail return NULL
	if(cur == pq->tail)
		return -1;

	if(cur_id == NULL){ 
		printf("Error!! %d ", pq->curr_size);
		return -1;
	}
	return cur_id->vertex_id;

}

int pq_delete_min(pq_t *pq)
{
	int h;
	struct pq_node *cur = pq->head->next[0];
	int ret;

	if (cur == pq->tail){ 
		//fprintf(stderr, "ERROR %d\n", pq->curr_size);	
		return -1;
	}
	 
	if(cur->id_list->next == NULL){
		for (h=0; h < cur->height; h++) {
			cur->next[h]->prev[h] = pq->head;
			pq->head->next[h] = cur->next[h];
		}
		ret = cur->id_list->vertex_id;
		pq->where_in_pq[ret] = NULL;
		pq->where_in_id_list[ret] = NULL;
		pq->curr_size--;
		// free node !!!
		return ret;
	}
		
	ret = cur->id_list->vertex_id;
	pq->where_in_pq[ret] = NULL;
	pq->where_in_id_list[ret] = NULL;
	
	struct id_node *prev = cur->id_list;
	cur->id_list = cur->id_list->next;
	cur->id_list->prev = NULL;
	prev->next = NULL;
	//free(prev);

	pq->curr_size--;
	return ret;
}

//> Remove a node with a certain vertex_id
struct id_node *delete_node_with_vertex_id(pq_t *pq, unsigned int vertex_id)
{
	struct pq_node *node = pq->where_in_pq[vertex_id];
	int h;

	if(node->id_list->next == NULL){
		for (h=0; h < node->height; h++) {
			node->prev[h]->next[h] = node->next[h];
			node->next[h]->prev[h] = node->prev[h];
		}
		//free node !!!
		return node->id_list;
	}else {
		return pq_node_delete_vertex_id(pq, node, vertex_id);
	}

	return NULL;
}

//> Insert an already allocated node
int pq_insert_id_node(pq_t *pq, int key, struct id_node *node_id)
{ 
	int cmpKey;
	int h;
	struct pq_node *updateArr[20], *cur;
	unsigned int vertex_id = node_id->vertex_id;
	
	cur = pq->head;

	// find where we should put the new node
	for (h = pq->levelmax - 1; h >= 0; h--) {
		cmpKey = cur->next[h]->key;
		while (cmpKey < key) {
			cur = cur->next[h];
			cmpKey = (cur->next[h])->key;
		}
		if(cmpKey == key)
			break;
		updateArr[h] = cur;
	}

	if(cmpKey != key){	// if the key does not exist
		int nodeHeight = get_rand_level(pq->levelmax); // find the height of the new node
		struct pq_node *node = pq_node_new(key, vertex_id, nodeHeight);

		for(h=0; h < nodeHeight; h++) {
			node->prev[h] = updateArr[h];
			node->next[h] = updateArr[h]->next[h];
			updateArr[h]->next[h]->prev[h] = node;
			updateArr[h]->next[h] = node;
		}
		
		pq->where_in_pq[vertex_id] = node;
		pq->where_in_id_list[vertex_id] = node->id_list;
	}else{
		node_id->next = cur->next[h]->id_list;
		cur->next[h]->id_list->prev = node_id;
		cur->next[h]->id_list = node_id;
		node_id->prev = NULL;
		pq->where_in_pq[vertex_id] = cur->next[h];	
		//pq->where_in_id_list[vertex_id] = node_id;
	}


	return 1;
}


void pq_decrease_key(pq_t *pq, unsigned int vertex_id, int new_key)
{
	struct pq_node *curr = pq->where_in_pq[vertex_id];
	if(curr != NULL){ 
		if(curr->key == new_key)
		    return;
		struct id_node *node_id = delete_node_with_vertex_id(pq, vertex_id); 
		pq_insert_id_node(pq, new_key, node_id); 
	}else{
		pq_insert(pq, new_key, vertex_id); 
	}

	return;
}

