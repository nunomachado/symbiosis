//
//  AvisoUtil.cpp
//
//
//  Created by Nuno Machado on 10/4/13.
//
//

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

#include <AvisoUtil.h>

using namespace std;

const int MAX_LINE_SIZE = 512;

AvisoTrace atrace; //** multimap: thread Id -> vector<avisoEvent>
AvisoEventVector fulltrace; //** sorted vector containing all avisoEvents
std::map<string,bool> threadHasFinished;    //** Nuno: to mark whether a thread has finished or not (the program should only terminate when all threads finish)

void printAvisoTrace()
{
    for(AvisoTrace::iterator iter = atrace.begin(); iter != atrace.end(); ++iter ) {
        AvisoEventVector tempVec = (*iter).second;
        string Key = (*iter).first;
        llvm::errs() << " T" << Key << ": ";
        for (unsigned i = 0; i < tempVec.size(); i++) {
            llvm::errs() << tempVec[i].loc << " ";
        }
        llvm::errs() << "\n";
    }
}

//** searches for a file with extension .trace in current directory
char* findTraceFile()
{
    char path[256];
    getcwd(path,256);
    char traceFile[256];
    //llvm::errs() << "PATH: "<<path << "\n";
    DIR* dirFile = opendir(path);
    if ( dirFile )
    {
        struct dirent* hFile;
        while (( hFile = readdir( dirFile )) != NULL )
        {
            if ( !strcmp( hFile->d_name, "."  )) continue;
            if ( !strcmp( hFile->d_name, ".." )) continue;
            
            // in linux hidden files all start with '.'
            if (hFile->d_name[0] == '.' ) continue;
            
            if ( strstr( hFile->d_name, ".trace" ))
            {
                llvm::errs() << "found trace file: " << hFile->d_name << "\n";
                strcpy(traceFile, hFile->d_name);
            }
        }
        closedir( dirFile );
    }
    return traceFile;
}

//** from a path to file like a/b/c.txt, extracts the basename c.txt
string extractFileBasename(char* path)
{
    string name = path;
    // Remove directory if present.
    // Do this before extension removal incase directory has a period character.
    const size_t last_slash_idx = name.find_last_of("\\/");
    if (std::string::npos != last_slash_idx)
    {
        name.erase(0, last_slash_idx + 1);
    }
    
    // Remove extension if present.
    /*const size_t period_idx = filename.rfind('.');
    if (std::string::npos != period_idx)
    {
        filename.erase(period_idx);
    }*/
    
    return name;
}

//** from a path to file like a/b/c.txt, extracts the basename c.txt
string extractFileBasename(string name)
{
    // Remove directory if present.
    // Do this before extension removal incase directory has a period character.
    const size_t last_slash_idx = name.find_last_of("\\/");
    if (std::string::npos != last_slash_idx)
    {
        name.erase(0, last_slash_idx + 1);
    }
    
    // Remove extension if present.
    /*const size_t period_idx = filename.rfind('.');
     if (std::string::npos != period_idx)
     {
     filename.erase(period_idx);
     }*/
    
    return name;
}


void print_state (const std::ios& stream) {
    llvm::errs() << " good()=" << stream.good();
    llvm::errs() << " eof()=" << stream.eof();
    llvm::errs() << " fail()=" << stream.fail();
    llvm::errs() << " bad()=" << stream.bad();
}


unsigned int generateHash(string str, size_t len) {
    
    unsigned int hash = 0;
    for(size_t i = 0; i < len; ++i)
        hash = 65599 * hash + str[i];
    return hash ^ (hash >> 16);
}


AvisoTrace loadAvisoTrace()
{
    char file[256];
    char* filetmp = findTraceFile();
    strcpy(file, filetmp);
    ifstream fin;
    fin.open(file);
    if (!fin.good())
    {
        print_state(fin);
        llvm::errs() << " -> Error opening file "<< file <<".\n";
        fin.close();
        exit(0);
    }
    
    // read each line of the file
    while (!fin.eof())
    {
        // read an entire line into memory
        char buf[MAX_LINE_SIZE];
        fin.getline(buf, MAX_LINE_SIZE);
        
        if(buf[0])
        {
            char* token;
            token = strtok (buf," :"); //token == thread id
            
            AvisoEvent aetmp;
            aetmp.tid = token;
            
            token = strtok (NULL," :"); //token == filename
            aetmp.filename = extractFileBasename(token);
            
            token = strtok (NULL," :"); //token == line of code
            aetmp.loc = atoi(token);
            
            //llvm::errs() << "TID: " << aetmp.tid << " Filename: " << aetmp.filename << " Loc: "<< aetmp.loc << "\n";
            
            atrace[aetmp.tid].push_back(aetmp);
            fulltrace.push_back(aetmp);
        }
    }
    fin.close();
    return atrace;
    
}

AvisoEventVector getFullAvisoTrace()
{
    return fulltrace;
}


