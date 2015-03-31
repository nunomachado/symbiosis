//
//  Parameters.cpp
//  symbiosisSolver
//
//  Created by Nuno Machado on 21/05/14.
//  Copyright (c) 2014 Nuno Machado. All rights reserved.
//

#include "Parameters.h"


const int MAX_LINE_SIZE = 512;
bool debug = false;
bool bugFixMode = false;
bool jpfMode = false;
std::string symbFolderPath = "";
std::string avisoFilePath = "";
std::string solverPath = "";
std::string formulaFile = "";
std::string solutionFile = "";
std::string assertThread = "";
std::string sourceFilePath = "";
bool failedExec = false;

std::map<std::string,std::string> solutionValues;
std::vector<Operation*> failScheduleOrd;
std::vector<std::string> altScheduleOrd;
std::map<std::string, std::vector<Operation*> > operationsByThread;    //map thread id -> vector with thread's operations



std::vector<int> unsatCore;
std::vector<std::string> bugCondOps;

time_t startTime;
time_t endTime;

int numEventsDifDebug = 0;
int numDepFull = 0;
int numDepDifDebug = 0;      