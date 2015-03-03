//
//  Util.h
//  symbiosisSolver
//
//  Created by Nuno Machado on 03/01/14.
//  Copyright (c) 2014 Nuno Machado. All rights reserved.
//

#ifndef snorlaxsolver_Util_h
#define snorlaxsolver_Util_h

#include <string>
#include <sys/types.h>

namespace util{
    
    std::string stringValueOf(int i);                           //transforms an int into a string
    int intValueOf(std::string i);                           //transforms a string into an int
    pid_t popen2(const char *command, int *infp, int *outfp);   //similar to popen, but allows for bidirectional communication
    void print_state (const std::ios& stream);                  //prints the error flags when an attempt to open a file fails
    
    std::string extractFileBasename(std::string name);          //from a path to file like a/b/c.txt, extracts the basename c.txt
    std::string extractFileBasename(char* path);                //from a path to file like a/b/c.txt, extracts the basename c.txt
    bool isClosedExpression(std::string expr);                  //returns true if an expression is closed, i.e. its number of '(' == number of ')'
    const std::string getcwd();                           //returns the path of the current working directory
    
    std::string parseThreadId(std::string operation);       //parses the threadId of an operation
    std::string parseVar(std::string operation);       //parses the variable of an operation
}


#endif
