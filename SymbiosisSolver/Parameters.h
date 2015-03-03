//
//  Parameters.h
//  symbiosisSolver
//
//  Created by Nuno Machado on 21/05/14.
//  Copyright (c) 2014 Nuno Machado. All rights reserved.
//

#ifndef __symbiosisSolver__Parameters__
#define __symbiosisSolver__Parameters__

#include <iostream>
#include <vector>

    //program inputs
    extern const int MAX_LINE_SIZE;
    extern bool debug;                  //prints debug info
    extern bool bugFixMode;             //run in bug fixing mode
    extern bool jpfMode;                //parse symbolic traces with Java Path Finder syntax
    extern std::string symbFolderPath;  //path to the folder containing the symbolic event traces
    extern std::string avisoFilePath;   //path to the aviso event trace
    extern std::string solverPath;      //path to the solver executable
    extern std::string formulaFile;     //path to the output file of the generated constraint formula
    extern std::string solutionFile;    //path to the output file of the solution
    extern std::string assertThread;    //id of the thread that contains the assertion
    extern bool failedExec;             //indicates whether the traces correspond to a failing or successful execution

    //global vars
    extern std::vector<int> unsatCore;  //vector to store the core (i.e. the constraints) of an unsat model (this is only used in the bug-fixing mode, to store which events of the failing schedule cause the non-bug condition to be unsat)
    extern std::vector<std::string> bugCondOps; //operations/events that appear in the bug condition

    //vars to measure solving time
    extern time_t startTime;
    extern time_t endTime;

    //vars for statistics of differential debugging
    extern int numEventsDifDebug;   //number of events in the root-cause
    extern int numDepFull;         //number of data-dependencies in the full failing schedule
    extern int numDepDifDebug;      //number of data-dependencies in the differential debugging schedule

#endif /* defined(__symbiosisSolver__Parameters__) */
