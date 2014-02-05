#ifndef OBJECT_HEADER
#define OBJECT_HEADER

typedef void object_t;

void * object_ninit(size_t size);
void * object_init(size_t size);
void * object_delete(object_t* objected);
#endif
