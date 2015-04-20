// This file is used to mimic the StringBuffer bug in JDK1.4
// Author: Jie Yu (jieyu@umich.edu)

#include "stringbuffer.hpp"
#include "klee.h"

#include <cassert>
#include <cstdio>
#include <cstring>
#include <pthread.h>

//StringBuffer *StringBuffer::null_buffer = new StringBuffer("null");
int sbcount;    //symb var for sb->count
int buffcount;  //symb var for buffer->count

StringBuffer::StringBuffer() {
    value = new char[16];
    value_length = 16;
    count = 0;
    klee_make_symbolic(&sbcount, sizeof(sbcount), "sbcount"); //Nuno
    sbcount = 0; //Nuno
    pthread_mutex_init(&mutex_lock, NULL);
}

StringBuffer::StringBuffer(int length) {
    value = new char[length];
    value_length = length;
    count = 0;
    pthread_mutex_init(&mutex_lock, NULL);
}

StringBuffer::StringBuffer(char *str) {
    int length = strlen(str) + 16;
    value = new char[length];
    value_length = length;
    count = 0;
    klee_make_symbolic(&buffcount, sizeof(buffcount), "buffcount"); //Nuno
     buffcount = 0; //Nuno
    pthread_mutex_init(&mutex_lock, NULL);
    append(str);
}

StringBuffer::~StringBuffer() {
    delete[] value;
}

int StringBuffer::length() {
    pthread_mutex_lock(&mutex_lock);
    int ret = buffcount; //int ret = count; //Nuno
    pthread_mutex_unlock(&mutex_lock);
    return ret;
}

void StringBuffer::getChars(int srcBegin, int srcEnd,
                            char *dst, int dstBegin) {
    pthread_mutex_lock(&mutex_lock);
    /*if (srcBegin < 0) {
     assert(0);
     }
     if ((srcEnd < 0) || (srcEnd > count)) {
     assert(0);
     }
     if (srcBegin > srcEnd) {
     assert(0);
     }*/
    //klee_make_symbolic(&srcEnd, sizeof(srcEnd), "len");
    assert(srcBegin >= 0);
    assert(srcEnd >= 0);
    assert(srcEnd <= buffcount);//assert(srcEnd <= count);
    assert(srcBegin <= srcEnd);
    
    //memcpy(dst + dstBegin, value + srcBegin, srcEnd - srcBegin);
    pthread_mutex_unlock(&mutex_lock);
}

StringBuffer *StringBuffer::append(StringBuffer *sb) {
    pthread_mutex_lock(&mutex_lock);
    if (sb == NULL) {
        sb = new StringBuffer("null");
    }
    int len, newcount;
    klee_make_symbolic(&len, sizeof(len), "len");
    len = sb->length();
    newcount = sbcount + len; //newcount = count + len;  //Nuno
    if (newcount > value_length)
        expandCapacity(newcount);
    sb->getChars(0, len, value, sbcount); //Nuno
   // count = newcount;
    sbcount = newcount;  //Nuno
    pthread_mutex_unlock(&mutex_lock);
    return this;
}

StringBuffer *StringBuffer::append(char *str) {
    pthread_mutex_lock(&mutex_lock);
    if (str == NULL) {
        str = "null";
    }
    int len, newcount;
    klee_make_symbolic(&len, sizeof(len), "len");
	len = strlen(str);
	newcount = buffcount + len;//newcount = count + len;  //Nuno
	if (newcount > value_length)
	    expandCapacity(newcount);
    //memcpy(value + count, str, len);
	// count = newcount;
    buffcount = newcount;  //Nuno
	pthread_mutex_unlock(&mutex_lock);
	return this;
}

StringBuffer *StringBuffer::erase(int start, int end) {
    pthread_mutex_lock(&mutex_lock);
    if (start < 0)
        assert(0);
    /*if (end > count)
        end = count;*/
    if(end > buffcount)
        end = buffcount;
    if (start > end)
        assert(0);
    int len;
    klee_make_symbolic(&len, sizeof(len), "len");
    len = end - start;
    if (len > 0) {
        //memcpy(value + start, value + start + len, count - end);
        //count -= len;
        buffcount -= len;
    }
    pthread_mutex_unlock(&mutex_lock);
    return this;
}

void StringBuffer::print() {
    for (int i = 0; i < count; i++) {
        printf("%c", *(value + i));
    }
    printf("\n");
}

void StringBuffer::expandCapacity(int minimumCapacity) {
    int newCapacity = (value_length + 1) * 2;
    if (newCapacity < 0) {
        newCapacity = INTEGER_MAX_VALUE;
    } else if (minimumCapacity > newCapacity) {
        newCapacity = minimumCapacity;
    }
    
    char *newValue = new char[newCapacity];
    //memcpy(newValue, value, count);
    delete[] value;
    value = newValue;
    value_length = newCapacity;
}

