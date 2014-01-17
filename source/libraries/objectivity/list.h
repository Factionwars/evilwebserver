#ifndef LIST_HEADER
#define LIST_HEADER

// Index node is a information holder for a objectnode list
typedef struct indexnode {
	objectnode_t * first; /* First node */
	objectnode_t * last;  /* Last node */
	unsigned int amount;  /* Number of nodes */
} indexnode_t;

typedef struct objectnode {
	object_t * object;
	struct objectnode * next;
} objectnode_t;

#endif