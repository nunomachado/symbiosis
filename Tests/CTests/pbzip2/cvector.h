// C-based vector implementation begin

#include<stdlib.h>
#include<string.h>
#include <pthread.h>

typedef struct vector_ {
  void** data;
  int size;
  int count;
} cvector;

void vector_init(cvector*);
void vector_resize(cvector* v, int sz);
int vector_count(cvector*);
int vector_size(cvector*);
void vector_add(cvector*, void*);
void vector_set(cvector*, int, void*);
void *vector_get(cvector*, int);
void vector_delete(cvector*, int);
void vector_free(cvector*);
