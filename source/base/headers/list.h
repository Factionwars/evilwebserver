#ifndef LIST_HEADER
#define LIST_HEADER

//WARNING, always keep the indexnode and objectnode in different sizes
// Index node is a information holder for a objectnode list

enum NODE_TYPE {
	NODE_INDEX,
	NODE_OBJECT
};

typedef struct typenode {
	int type;
} typenode_t;

typedef struct objectnode {
	int type;
	object_t * object;
	struct objectnode * next;
} objectnode_t;

typedef struct indexnode {
	int type;
	objectnode_t * first; /* First node */
	objectnode_t * last;  /* Last node */
	unsigned int amount;  /* Number of nodes */
} indexnode_t;

#endif