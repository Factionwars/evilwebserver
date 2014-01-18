#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "object.h"

#include "list.h"

indexnode_t * list_init()
{
	indexnode_t * indexnode = object_init(sizeof(indexnode_t));
	indexnode->type = NODE_INDEX;
	return indexnode;
}

void * list_add(void * node)
{
	typenode_t * typenode = node;
	if(typenode->type == NODE_INDEX){
		indexnode_t * indexnode = node;
		indexnode->last = list_add(indexnode->last);
		return indexnode;
	} else if(typenode->type == NODE_OBJECT){
		objectnode_t * objectnode = node;
		while(objectnode->next != NULL){
			objectnode = objectnode->next;
		}
		objectnode->next = object_init(sizeof(objectnode_t));
		objectnode->next->type = NODE_OBJECT;
		return objectnode;		
	}

}

void list_delete(void * node)
{	
	typenode_t * typenode = node;
	if(typenode->type == NODE_INDEX){
		indexnode_t * indexnode = node;
		if(indexnode == NULL)
			return;
		list_delete(indexnode->first);
		free(indexnode);
	} else if(typenode->type == NODE_OBJECT){
		objectnode_t * objectnode = node;
		while(objectnode != NULL){
			objectnode_t * previous = objectnode;
			objectnode = objectnode->next;
			free(previous);
		}
	}

}