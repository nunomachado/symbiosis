//
//  AvisoUtil.h
//
//
//  Created by Nuno Machado on 10/4/13.
//
//

#ifndef ____AvisoUtil_H__
#define ____AvisoUtil_H__

#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <string>
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>


struct AvisoEvent {
    std::string tid;
    int loc;
    std::string filename;
    
} ;

typedef std::vector<AvisoEvent> AvisoEventVector;
typedef std::map<std::string, AvisoEventVector > AvisoTrace;

extern AvisoTrace atrace; //** multimap: thread Id -> vector<avisoEvent>
extern AvisoEventVector fulltrace; //** sorted vector containing all avisoEvents
extern std::map<std::string, bool> threadHasFinished;    //** Nuno: to mark whether a thread has finished or not (the program should only terminate when all threads finish)

void printAvisoTrace();

//** searches for a file with extension .trace in current directory
char* findTraceFile();

//** from a path to file like a/b/c.txt, extracts the basename c.txt
std::string extractFileBasename(char* path);
std::string extractFileBasename(std::string path);

void print_state(const std::ios& stream);

//** generate a hash value for a given std::string
unsigned int generateHash(std::string str, size_t len);

AvisoTrace loadAvisoTrace();

AvisoEventVector getFullAvisoTrace();

#endif /* defined(____AvisoUtil_H__) */
