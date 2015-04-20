#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include <unistd.h>
#include "klee.h"
#define MAXVAL 1000

struct wonk{
    int a;
} *shrdPtr, *symb_shrdPtr; //*symb_shrdPtr is necessary to mark operations on shrdPtr as symbolic

pthread_mutex_t lock;

struct wonk *getNewVal(struct wonk**old){
    //free(*old);
    *old = NULL;
    struct wonk *newval = (struct wonk*)malloc(sizeof(struct wonk));
    newval->a = 1;
    return newval;
}

void *updaterThread(void *arg){
    
    int i;
    for(i = 0; i < 10; i++){
        pthread_mutex_lock(&lock);
        struct wonk *newval = getNewVal(&symb_shrdPtr); //Nuno: replicate getNewVal on symb_shrdPtr to allow capture symb operations on shrdPtr
        pthread_mutex_unlock(&lock);
        usleep(20);
        pthread_mutex_lock(&lock);
        shrdPtr = newval; symb_shrdPtr = newval; //Nuno: replicate operation for symb_shrdPtr
        pthread_mutex_unlock(&lock);
    }
    
}

void swizzle(int *result){
    
    pthread_mutex_lock(&lock);//400a4e
    if(symb_shrdPtr != NULL)
    {
        pthread_mutex_unlock(&lock);
        //usleep(20);
        pthread_mutex_lock(&lock);//400a6e
        assert(symb_shrdPtr!=NULL);
        *result += shrdPtr->a;
        pthread_mutex_unlock(&lock);
        
    }else{
        pthread_mutex_unlock(&lock);
    }
    
}

void *accessorThread(void *arg){
    
    int *result = (int*)malloc(sizeof(int)); klee_make_symbolic(result, sizeof(int), "result");
    *result = 0;
    
    while(*result < MAXVAL){
        swizzle(result);
        usleep(10 + (rand() % 100) );
    }
    
    pthread_exit(result);
}

int main(int argc, char *argv[]){
    
    int res = 0;
    shrdPtr = (struct wonk*)malloc(sizeof(struct wonk)); symb_shrdPtr = shrdPtr; klee_make_symbolic(shrdPtr, sizeof(struct wonk), "shrdPtr->a"); klee_make_symbolic(&symb_shrdPtr, sizeof(symb_shrdPtr), "shrdPtr"); //Nuno
    shrdPtr->a = 1;
    
    pthread_mutex_init(&lock,NULL);
    
    pthread_t acc[4],upd;
    pthread_create(&acc[0],NULL,accessorThread,(void*)shrdPtr);
    pthread_create(&acc[1],NULL,accessorThread,(void*)shrdPtr);
    pthread_create(&acc[2],NULL,accessorThread,(void*)shrdPtr);
    pthread_create(&acc[3],NULL,accessorThread,(void*)shrdPtr);
    pthread_create(&upd,NULL,updaterThread,(void*)shrdPtr);
    
    pthread_join(upd,NULL);
    pthread_join(acc[0],(void**)&res);
    pthread_join(acc[1],(void**)&res);
    pthread_join(acc[2],(void**)&res);
    pthread_join(acc[3],(void**)&res);
    fprintf(stderr,"Final value of res was %d\n",res); 
}
