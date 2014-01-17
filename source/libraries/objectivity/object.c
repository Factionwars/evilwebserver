#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "object.h"


/* OBJECT.c */
void * object_init(int size){
	object_t* new_object = malloc(size);
	if(new_object == NULL){
		perror("Error allocating memory.");
		return NULL;
	}
	memset(new_object, '\0', size);
	return new_object;
}

void * object_delete(object_t* objected){
	if(objected == NULL)
		return NULL;
	free(objected);
	objected = NULL;
	return NULL;
}

/*

int main() {
	http_header * header = object_init(sizeof(http_header));

	header->name = "hoi";
	printf("lol: %s\n", header->name);
	object_delete(header);
	return 0;
} 
*/