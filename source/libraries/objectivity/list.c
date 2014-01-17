#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "object.h"

#include "list.h"

indexnode_t * list_init()
{
	indexnode_t * indexnode = object_init(sizeof(indexnode_t));
	return indexnode;
}


indexnode_t * list_add(indexnode_t * indexnode)
{
	indexnode->last = list_add(indexnode->last);
}

objectnode_t * list_add(objectnode_t * objectnode)
{
	while(objectnode->next != NULL){
		objectnode = objectnode->next;
	}
	objectnode->next = object_init(sizeof(objectnode_t));
	return objectnode;
}

void list_delete(indexnode_t * indexnode)
{
	if(indexnode == NULL)
		return;
	list_delete(indexnode->first);
	free(indexnode);
}

void list_delete(objectnode_t * objectnode)
{
	while(objectnode != NULL){
		objectnode_t * previous = objectnode;
		objectnode = objectnode->next;
		free(previous);
	}
}