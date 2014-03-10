#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "object.h"

void * object_ninit(size_t size)
{
    object_t* new_object = malloc(size);
    if(new_object == NULL){
            perror("Error allocating memory.");
            return NULL;
    }
    return new_object;
}

void * object_init(size_t size)
{
    object_t* new_object = malloc(size);
    if(new_object == NULL){
            perror("Error allocating memory.");
            return NULL;
    }
    memset(new_object, '\0', size);
    return new_object;
}

void * object_delete(object_t* objected)
{
    if(objected == NULL)
            return NULL;
    free(objected);
    objected = NULL;
    return NULL;
}
