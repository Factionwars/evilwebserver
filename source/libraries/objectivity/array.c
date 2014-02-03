#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "object.h"

void * array_init(size_t osize, size_t nmembers)
{
	void ** array = calloc(osize, nmembers);
	if(array == NULL){
		perror("Error allocating memory.");
		return NULL;
	}
}