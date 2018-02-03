#ifndef SKIPLIST_H
#define SKIPLIST_H

//> A node of the priority queue
struct pq_node {
	long key;
	int height;
	thr_mng **next;
	thr_mng **prev; 	//FIXME This is a double-linked list. You can remove it!
	thr_mng *thr_str; //FIXME This is the value , it can only be just an int, like (int value)
};

//> The priority queue struct
typedef struct {
	struct pq_node *head, *tail;
	unsigned int levelmax;
	unsigned int curr_size;
	unsigned int nvertices;
	struct pq_node **where_in_pq;
} pq_t;



extern struct pq_node *pq_node_new(long key,int toplevel);
extern void pq_node_free(struct pq_node *node);
extern pq_t *pq_create(unsigned int nvertices);
extern void delete_pq(pq_t *pq);
extern int get_rand_level(int levelmax);
extern int pq_insert(pq_t *pq, int key, unsigned int vertex_id);
extern int pq_read_min(pq_t *pq, int index);
extern int pq_delete_min(pq_t *pq);
struct id_node *delete_node_with_vertex_id(pq_t *pq, unsigned int vertex_id);
extern void pq_decrease_key(pq_t *pq, unsigned int vertex_id, int new_key);
extern int print(pq_t *pq);
#endif


