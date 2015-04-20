// This file is used to mimic the StringBuffer bug in JDK1.4
// Author: Jie Yu (jieyu@umich.edu)

#include "stringbuffer.hpp"
#define MAX 10

StringBuffer *buffer;

void *thread_func(void *args) {
    int i = 0;
    while (i < MAX) {
        buffer->erase(0, i); //change this to random length
        buffer->append("abcefghijk");
        i++;
    }
}

int main(int argc, char *argv[]) {
    pthread_t thd;
    int rc;

    buffer = new StringBuffer("abcefghijk");
    rc = pthread_create(&thd, NULL, thread_func, NULL);
    
    int i = 0;
    while (i < MAX) {
        StringBuffer *sb = new StringBuffer();
        sb->append(buffer);
        i++;
    }
    
    return 0;
}
