#include <string.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct http_header{
    char * name;
    char * value;
    struct http_header * next;
}http_header;


typedef void object_t;

typedef struct objectlist {
	object_t * object;
	struct objectlist * next;
} objectlist_t;

/* OBJECT.c */
void * object_init(int size){
	object_t* new_object = malloc(size);
	memset(new_object, '\0', size);
	return new_object;
}

void * object_delete(object_t* objected){
	if(objected != NULL)
		free(objected);
	objected = NULL;
	return NULL;
}
/* LIST.C */
objectlist_t 

int main() {
	http_header * header = object_init(sizeof(http_header));

	header->name = "hoi";
	printf("lol: %s\n", header->name);
	object_delete(header);
	return 0;
}