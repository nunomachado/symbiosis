#include "cvector.h"

pthread_mutex_t cvector_mutex;

void vector_init(cvector *v)
{
  pthread_mutex_init (&cvector_mutex, 0);
  pthread_mutex_lock(&cvector_mutex);
  v->data = NULL;
  v->size = 0;
  v->count = 0;
  pthread_mutex_unlock(&cvector_mutex);
}

void vector_resize(cvector* v, int sz){
  pthread_mutex_lock(&cvector_mutex);
  int i = 0;
  if(sz == v->size){
    pthread_mutex_unlock(&cvector_mutex);
    return;
  }
  else{
    v->size = sz;
    v->data = (void**)realloc(v->data, sizeof(void*) * v->size);    
  }
  pthread_mutex_unlock(&cvector_mutex);
}

int vector_size(cvector *v){
  pthread_mutex_lock(&cvector_mutex);
  int size = v->size;
  pthread_mutex_unlock(&cvector_mutex);
  return size;
}

int vector_count(cvector *v)
{
  pthread_mutex_lock(&cvector_mutex);
  int size = v->size;
  pthread_mutex_unlock(&cvector_mutex);
  return size;
}

void vector_add(cvector *v, void *e)
{
  pthread_mutex_lock(&cvector_mutex);
  if (v->size == 0) {
    v->size = 10;
    v->data = (void**)malloc(sizeof(void*) * v->size);
    memset(v->data, '\0', sizeof(void) * v->size);
  }

  // condition to increase v->data:
  // last slot exhausted
  if (v->size == v->count) {
    v->size *= 2;
    v->data = (void**)realloc(v->data, sizeof(void*) * v->size);
  }
  v->data[v->count] = e;
  v->count++;
  pthread_mutex_unlock(&cvector_mutex);
}

void vector_set(cvector *v, int index, void *e)
{
  pthread_mutex_lock(&cvector_mutex);
  if (index >= v->count) {
    pthread_mutex_unlock(&cvector_mutex);
    return;
  }

  v->data[index] = e;
  pthread_mutex_unlock(&cvector_mutex);
}

void *vector_get(cvector *v, int index)
{
  pthread_mutex_lock(&cvector_mutex);
  if (index >= v->count) {
    pthread_mutex_unlock(&cvector_mutex);
    return 0;
  }
  void* retVal = v->data[index];; 
  pthread_mutex_unlock(&cvector_mutex);
  return retVal;
}

void vector_delete(cvector *v, int index)
{
  pthread_mutex_lock(&cvector_mutex);
  if (index >= v->count) {
  pthread_mutex_unlock(&cvector_mutex);
    return;
  }

  v->data[index] = NULL;

  int i, j;
  void **newarr = (void**)malloc(sizeof(void*) * v->count * 2);
  for (i = 0, j = 0; i < v->count; i++) {
    if (v->data[i] != NULL) {
      newarr[j] = v->data[i];
      j++;
    }
  }

  free(v->data);

  v->data = newarr;
  v->count--;
  pthread_mutex_unlock(&cvector_mutex);
}

void vector_free(cvector *v)
{
  pthread_mutex_lock(&cvector_mutex);
  free(v->data);
  pthread_mutex_unlock(&cvector_mutex);
}

// C-based vector implementation end
