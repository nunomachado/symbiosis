/*
** pqueue.c - FIFO queue management routines.
**
** Copyright (c) 1997-2002 Peter Eriksson <pen@lysator.liu.se>
**
** This program is free software; you can redistribute it and/or
** modify it as you wish - as long as you don't claim that you wrote
** it.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/


#include <stdlib.h>
#include <pthread.h>

#include "pqueue.h"
#include <assert.h>
#include <unistd.h>

int
pqueue_init(PQUEUE *qp,
	   int qsize)
{
    qp->buf = calloc(sizeof(void *), qsize);
    if (qp->buf == NULL)
	return NULL;

    qp->qsize = qsize;
    qp->occupied = 0;
    qp->nextin = 0;
    qp->nextout = 0;
    qp->closed = 0;

    pthread_mutex_init(&qp->mtx, NULL);
    pthread_cond_init(&qp->more, NULL);
    pthread_cond_init(&qp->less, NULL);

    return 0;
}



void
pqueue_close(PQUEUE *qp)
{
    pthread_mutex_lock(&qp->mtx);

    qp->closed = 1;

    pthread_mutex_unlock(&qp->mtx);
    pthread_cond_broadcast(&qp->more);
}


int
pqueue_put(PQUEUE *qp,
	  void *item)
{
    pthread_mutex_lock(&qp->mtx); //(0x4069d7)

    if (qp->closed)
	return 0;
    
    while (qp->occupied >= qp->qsize)
	pthread_cond_wait(&qp->less, &qp->mtx);

    qp->buf[qp->nextin++] = item;

    qp->nextin %= qp->qsize;
    qp->occupied++;

    pthread_mutex_unlock(&qp->mtx);
    pthread_cond_signal(&qp->more);

    return 1;
}



int
pqueue_get(PQUEUE *qp,
	   void **item)
{
    int got = 0;
    int * j = NULL;
    
    pthread_mutex_lock(&qp->mtx); //(0x406d01)
    
    while (qp->occupied <= 0 && !qp->closed)
	pthread_cond_wait(&qp->more, &qp->mtx);

    if (qp->occupied > 0)
    {

        pthread_mutex_unlock(&qp->mtx);
        usleep(20);
        pthread_mutex_lock(&qp->mtx);
        assert(qp->occupied > 0); //bug condition

        *item = qp->buf[qp->nextout++];
	qp->nextout %= qp->qsize;
	qp->occupied = qp->occupied - 1;
	got = 1;

	pthread_mutex_unlock(&qp->mtx);
	pthread_cond_signal(&qp->less);
    }
    else
	pthread_mutex_unlock(&qp->mtx);

    return got;
}



void
pqueue_destroy(PQUEUE *qp)
{
    pthread_mutex_destroy(&qp->mtx);
    pthread_cond_destroy(&qp->more);
    pthread_cond_destroy(&qp->less);
    free(qp->buf);
}
