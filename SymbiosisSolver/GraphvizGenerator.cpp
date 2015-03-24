//
//  GraphvizGenerator.cpp
//  symbiosisSolver
//
//  Created by Nuno Machado on 30/07/14.
//  Copyright (c) 2014 Nuno Machado. All rights reserved.
//

#include <iostream>
#include <fstream>
#include <sstream>
#include <set>
#include <algorithm>
#include <unistd.h>

#include <fcntl.h> //for read
#include <iostream>
#include <fstream>
#include <vector>
#include "Z3Solver.h"
#include "GraphvizGenerator.h"
#include "Util.h"
#include "Parameters.h"
#include <string>
#include <iostream>
#include <string>
#include <algorithm>

#define LINEMAX 256


using namespace std;

map<string,string> threadColors; //map: thread id -> banner color (used to distinguish threads in the graph)

map<string,string> readDependFail;  //[failing schedule] map: R operation -> last W operation on the same var (this means that R is data dependent on W)
map<string,string> readDependAlt;   //[alternate schedule] map: R operation -> last W operation on the same var

map<string,vector<string>> writeDependFail; //[failing schedule] map: W operation -> R operations on the same var affected by this W
map<string,vector<string>> writeDependAlt; //[alternate schedule] map: W operation -> R operations on the same var affected by this W

map<string,string> exclusiveFail;   //same as above, but only for the unique dependencies on failing schedule
map<string,string> exclusiveAlt;    //same as above, but only for the unique dependencies on failing schedule

map<string,int> dependIdsFail; //map: operation -> position the failing schedule
map<string,int> dependIdsAlt;  //map: operation -> position the alt schedule

map<int,string> lockVariables; //map: lockID -> lockVariable

set<string> relevantThreads; //set containing the threads that are relevant (i.e. that have operations with exclusive data-dependencies)

int altCounter = 0; //counts the number of alternate schedules (used to name the output files)
pid_t lineCode_pid;                          //id of the process running getCodeLine()
int procR, procW;                           //pipes to read from and write to the process running getCodeLine()
std::vector<std::string> operationsVars;    //vector containing the operation variables
std::vector<std::string> orderVars;         //vector containing the operation order variables
int numOps;                             //number of operations to ordered

/*
 * Returns a port name if the operation is involved in a dependence
 * in the failing schedule. Otherwise, returns empty string.
 */
string getFailDependencePort(string op)
{
    // if(exclusiveFail.count(op))
    //   return op;
    int counter = 1;
    for(map<string,string>::iterator it = exclusiveFail.begin(); it!=exclusiveFail.end();++it)
    {
        if(!op.compare(it->first))
            return (util::stringValueOf(counter)+"1");
        if(!op.compare(it->second))
            return (util::stringValueOf(counter)+"2");;
        
        counter++;
    }
    
    return "";
}

string getAltDependencePort(string op)
{
    int counter = 1;
    for(map<string,string>::iterator it = exclusiveAlt.begin(); it!=exclusiveAlt.end();++it)
    {
        if(!op.compare(it->first))
            return (util::stringValueOf(counter)+"1");
        if(!op.compare(it->second))
            return (util::stringValueOf(counter)+"2");;
        
        counter++;
    }
    
    return "";
}



/*
 * Replaces CHAR "specialChar" (unsupported by graphviz) to "htmlCode"
 *
 * Example:
 *            ">" to "&gt"
 *            "&" to "&amp"
 */
string changeChar2graphviz(string specialChar, string htmlCode ,string op){
    
    size_t pos = op.find(specialChar);
    while(pos != string::npos)
    {
        op.replace(pos,1,htmlCode);
        pos++;
        pos = op.find(specialChar,pos);
    }
    return op;
};


/*
 *Remove line comments
 */
string removeComment(string op)
{
    size_t pos = op.find("//");
    if(pos!= string::npos)
        op.resize(pos);
    return op;
}



/*
 * Changes from the operation name the characters unsupported by graphviz
 */
string cleanOperation(string op)
{
    op = removeComment(op);
    op = changeChar2graphviz("&","&amp;",op);
    op = changeChar2graphviz(">","&gt;",op);
    op = changeChar2graphviz("<","&lt;",op);
    return op;
}


//Retrive "SimpleAssertKLEE.c" from "[18] OS-lock_416680994-2-0&SimpleAssertKLEE.c@45"
string  getFilenameOp(string op)
{
    int filenameInitP = (int)op.find("&")+1;
    int filenameEndP = (int)op.find("@");
    string fileNameOp = op.substr(filenameInitP,filenameEndP-filenameInitP);
    return fileNameOp;

}

//expecting something like: [18] OS-lock_416680994-2-0&SimpleAssertKLEE.c@45 to get "45"
int  getLineOp(string op)
{
    int lineInitP = (int)op.find("@")+1;
    const char *cstr = op.substr(lineInitP,op.length()).c_str();
    return (int)atoi(cstr);
}

//expecting something like: [18] OS-lock_416680994-2-0&SimpleAssertKLEE.c@45 to get "45"
int getVarIDlock(string op)
{
    int initP = (int)op.find("_")+1;
    op = op.substr(initP,op.length());
    int endP = (int)op.find("-");
    int varIDlock = (int)atoi(op.substr(0,endP).c_str());
    return varIDlock;
}




//pthread_mutex_lock(&lock);
string getVarNameFromCodeLine(string codeLine)
{
    int init = (int)codeLine.find("(")+1;
    int end = (int)codeLine.find(")");
    string varName = codeLine.substr(init,end-init);
    return varName;

}


string getLockVarName(string filename, int line, int lockVarID)
{
    string codeLine = graphgen::getCodeLine(line, filename);
    string lockVarName = getVarNameFromCodeLine(codeLine);
    return lockVarName;
    
    
}



void storePair(int lockVarID,string lockVarName){

    //lockVarName.erase( std::remove( lockVarName.begin(), lockVarName.end(),'&'), lockVarName.end() );
    lockVariables.insert(pair<int,string>(lockVarID,lockVarName));
//    map<int,string> lockVariables; //map: lockID -> lockVariable
}


string getVarName(int varID){
    return lockVariables[varID];
}

//Store a lockVarName with its ID
void fillMaplockVariables(string op)
{
    //expecting something like: [18] OS-lock_416680994-2-0&SimpleAssertKLEE.c@45
    string filename = getFilenameOp(op); // SimpleAssertKLEE.c
    //cout << "\n\nFilename: "<< filename;
    int line = getLineOp(op);          //45
      //  cout << "\nline: "<< line;
    int lockVarID = getVarIDlock(op);  //416680994
        //cout << "\nvarID: "<< lockVarID;
    string lockVarName = getLockVarName(filename,line,lockVarID);
        //cout << "\nVarName: " << lockVarName;
    storePair(lockVarID,lockVarName);
    
    string aux = getVarName(lockVarID);
    cout << "\nMap " << aux;
}




/**
 *  For each alternate schedule, draws a graphviz graph of the thread segments that differ
 *  between the failing schedule and the alternate schedule. Moreover, it highlights the
 *  the segments that became atomic and the data-flow dependencies that changed.
 */
void graphgen::genAllGraphSchedules(vector<string> failSchedule, map<EventPair, vector<string> > altSchedules)
{
    //** define colors
    threadColors["0"] = "salmon";
    threadColors["1"] = "lightsteelblue2";
    threadColors["1.1"] = "cyan4";
    threadColors["1.2"] = "dodgerblue";
    threadColors["2"] = "darkolivegreen3";
    threadColors["3"] = "mediumpurple";
    threadColors["4"] = "darkgoldenrod1";
    threadColors["5"] = "khaki1";
    threadColors["6"] = "darkviolet";
    threadColors["7"] = "blue";
    threadColors["8"] = "firebrick1";
    threadColors["9"] = "chocolate4";
    threadColors["10"] = "red";
    
    //** compute data-dependencies for failSchedule
    //cout << "Failing schedule:\n";
    for(int oit = (int)failSchedule.size()-1; oit >= 0;oit--)
    {
        string opA = failSchedule[oit];
        
        //fill map lockVariables
        if(opA.find("OS-lock")!= string::npos){
            fillMaplockVariables(opA);
        }
        
        if(opA.find("OR-") == string::npos)
            continue;
        
        string varA = util::parseVar(opA);
        
        for(int iit = oit; iit >= 0; iit--)
        {
            string opB = failSchedule[iit];
            string varB = util::parseVar(opB);
            
            if(varA==varB && opB.find("OW")!=string::npos)
            {
                //cout << "   debug: varA " << varA << " == varB " << varB << "\n";
                //cout << "   debug: " << opA << " <-- " << opB << "\n";
                readDependFail[opA] = opB; //add dependence "A is data dependent on B"
                if(writeDependFail.count(opB)){
                    writeDependFail[opB].push_back(opA);
                }
                else{
                    vector<string> tmpRs;
                    tmpRs.push_back(opA);
                    writeDependFail[opB] = tmpRs;
                }
                
                dependIdsFail[opA] = oit; //add A's position in the schedule
                dependIdsFail[opB] = iit; //add B's position in the schedule
                numDepFull++;
                break;
            }
        }
    }
    
    for(map<EventPair, vector<string> >::iterator it = altSchedules.begin(); it!=altSchedules.end(); ++it)
    {
        //cout << "\n\nAlt schedule "<< pairToString(it->first, failSchedule);
        genGraphSchedule(failSchedule, it->first, it->second);
        altCounter++;
    }
}


void graphgen::genGraphSchedule(vector<string> failSchedule, EventPair invPair, vector<string> altSchedule)
{
    readDependAlt.clear();
    dependIdsAlt.clear();
    
    //** compute data-dependencies for AltSchedule
    for(int oit = altSchedule.size()-(float)1; oit >= 0;oit--)
    {
        string opA = altSchedule[oit];
        
        if(opA.find("OR-") == string::npos)
            continue;
        
        string varA = util::parseVar(opA);
        
        for(int iit = oit; iit >= 0; iit--)
        {
            string opB = altSchedule[iit];
            string varB = util::parseVar(opB);
            
            if(varA==varB && opB.find("OW")!=string::npos)
            {
                //cout << "   " << opA << " <-- " << opB << "\n";
                readDependAlt[opA] = opB; //add dependence "A is data dependent on B"
                if(writeDependAlt.count(opB)){
                    writeDependAlt[opB].push_back(opA);
                }
                else{
                    vector<string> tmpRs;
                    tmpRs.push_back(opA);
                    writeDependAlt[opB] = tmpRs;
                }
                dependIdsAlt[opA] = oit; //add A's position in the schedule
                dependIdsAlt[opB] = iit; //add B's position in the schedule
                break;
            }
        }
    }
    
    //** compute exclusive dependencies (i.e. dependencies that appear only in the failing
    //** schedule or in the alternate schedule)
    vector<int> exclusiveFailIds;
    vector<int> exclusiveAltIds;
    exclusiveFail.clear();
    exclusiveAlt.clear();
    
    for(map<string,string>::iterator dit = readDependFail.begin(); dit!=readDependFail.end();++dit)
    {
        string writeFail = dit->second;
        string writeAlt = readDependAlt[dit->first];
        //if(readDependAlt[dit->first]!=dit->second)
        if(writeFail!=writeAlt)
        {
            //FAILING SCHEDULE
            if(writeFail!=""){
                exclusiveFail[dit->first] = dit->second;
                exclusiveFailIds.push_back(dependIdsFail[dit->first]);
                exclusiveFailIds.push_back(dependIdsFail[dit->second]);
                cout << "Exclusive Fail:\t" << dit->first << " <-- " << dit->second << "\n";
                
                //add relevant threads
                relevantThreads.insert(util::parseThreadId(dit->first));
                relevantThreads.insert(util::parseThreadId(dit->second));
            }
            
            //ALT SCHEDULE
            if(writeAlt!=""){
                exclusiveAlt[dit->first] = writeAlt;
                exclusiveAltIds.push_back(dependIdsAlt[dit->first]);
                exclusiveAltIds.push_back(dependIdsAlt[writeAlt]);
                cout << "Exclusive Alt:\t" << dit->first << " <-- " << writeAlt << "\n";
                
                //add relevant threads
                relevantThreads.insert(util::parseThreadId(writeAlt));
            }
            numDepDifDebug++;
        }
    }
    
    //for each exclusive write in the alt schedule, add all data-dependencies from
    //reads in the same thread of the exclusive read (i.e. mark all reads that are affected by that particular write)
   /* for(map<string,string>::iterator ait = exclusiveAlt.begin(); ait!=exclusiveAlt.end();++ait)
    {
        string writeAlt = ait->second;
        string readAlt = ait->first;
        string tid = util::parseThreadId(readAlt);
        
        for(vector<string>::iterator rit = writeDependAlt[writeAlt].begin();
            rit != writeDependAlt[writeAlt].end();++rit)
        {
            string tmpR = *rit;
            if(tmpR.compare(readAlt) && tid==util::parseThreadId(tmpR))
            {
                exclusiveAlt[tmpR] = writeAlt;
                exclusiveAltIds.push_back(dependIdsAlt[tmpR]);
                cout << "Relevant Alt:\t" << tmpR << " <-- " << writeAlt << "\n";
            }
        }
        
        for(vector<string>::iterator rit = writeDependFail[writeAlt].begin();
            rit != writeDependFail[writeAlt].end();++rit)
        {
            string tmpR = *rit;
            if(!exclusiveFail.count(tmpR) && tid==util::parseThreadId(tmpR))
            {
                exclusiveFail[tmpR] = writeAlt;
                exclusiveFailIds.push_back(dependIdsFail[tmpR]);
                cout << "Relevant Fail:\t" << tmpR << " <-- " << writeAlt << "\n";
            }
        }
    }*/
    
    
    //** sort ids in ascending order
    sort(exclusiveFailIds.begin(),exclusiveFailIds.end());
    sort(exclusiveAltIds.begin(),exclusiveAltIds.end());
    
    
    //**    1) compute thread segments for both schedules
    //**    2) cut-off irrelevant, common prefix
    //**    3) cut-off threads that don't exhibit dependence changes in their segments
    //**    4) mark as "bold" the segments that became bigger in the alt schedule
    vector<ThreadSegment> segsFail;
    vector<ThreadSegment> segsAlt;
    
    //** ---- segments for failing schedule
    string op = failSchedule[0];
    string prevTid = util::parseThreadId(op); //indicates the last thread id observed
    
    int initSeg = 0;  //indicates the start of the current segment
    int dependIt = 0; //iterator for exclusiveFail/AltIds; allows to check if a given block encompasses operations with dependencies or not
    
    int oit;
    for(oit = 1; oit < failSchedule.size();oit++)
    {
        op = failSchedule[oit];
        string tid = util::parseThreadId(op);
        
        if(tid != prevTid)
        {
            ThreadSegment tseg;
            tseg.initPos = initSeg;
            tseg.endPos = oit-1; //the endPos is the last operation of the previous segment
            tseg.markAtomic = false;
            tseg.hasDependencies = false;
            tseg.tid = prevTid;
            
            //check if the current block comprises operations with dependencies
            for(int dit = dependIt; dit < exclusiveFailIds.size(); dit++)
            {
                dependIt = dit;
                
                if(tseg.endPos < exclusiveFailIds[dit])
                    break;
                
                if(tseg.initPos > exclusiveFailIds[dit])
                    continue;
                
                tseg.hasDependencies = true;
                tseg.dependencies.push_back(exclusiveFailIds[dit]);
            }
            
            //only add segments of relevant threads
            if(relevantThreads.find(tseg.tid)!=relevantThreads.end()){
                segsFail.push_back(tseg);
            }
            prevTid = tid;
            initSeg = oit;
        }
    }
    
    //** handle last case
    ThreadSegment tseg;
    tseg.initPos = initSeg;
    tseg.endPos = oit-1; //the endPos is the last operation of the previous segment
    tseg.markAtomic = false;
    tseg.hasDependencies = false;
    tseg.tid = prevTid;
    
    //check if the current block comprises operations with dependencies
    for(int dit = dependIt; dit < exclusiveFailIds.size(); dit++)
    {
        dependIt = dit;
        
        if(tseg.endPos < exclusiveFailIds[dit])
            break;
        
        if(tseg.initPos > exclusiveFailIds[dit])
            continue;
        
        tseg.hasDependencies = true;
        tseg.dependencies.push_back(exclusiveFailIds[dit]);
    }
    if(relevantThreads.find(tseg.tid)!=relevantThreads.end()){
        segsFail.push_back(tseg);
    }
    
    
    //** ---- segments for alt schedule
    op = altSchedule[0];
    prevTid = util::parseThreadId(op); //indicates the last thread id observed
    
    initSeg = 0;  //indicates the start of the current segment
    dependIt = 0; //iterator for exclusiveFail/AltIds; allows to check if a given block encompasses operations with dependencies or not
    
    for(oit = 1; oit < altSchedule.size();oit++)
    {
        op = altSchedule[oit];
        string tid = util::parseThreadId(op);
        
        if(tid != prevTid)
        {
            ThreadSegment tseg;
            tseg.initPos = initSeg;
            tseg.endPos = oit-1; //the endPos is the last operation of the previous segment
            tseg.markAtomic = false;
            tseg.hasDependencies = false;
            tseg.tid = prevTid;
            
            //check if the current block comprises operations with dependencies
            for(int dit = dependIt; dit < exclusiveAltIds.size(); dit++)
            {
                dependIt = dit;
                
                if(tseg.endPos < exclusiveAltIds[dit])
                    break;
                
                if(tseg.initPos > exclusiveAltIds[dit])
                    continue;
                
                tseg.hasDependencies = true;
                tseg.dependencies.push_back(exclusiveAltIds[dit]);
            }
            
            //only add segments of relevant threads
            if(relevantThreads.find(tseg.tid)!=relevantThreads.end()){
                segsAlt.push_back(tseg);
            }
            prevTid = tid;
            initSeg = oit;
        }
    }
    
    //** handle last case
    ThreadSegment lseg;
    lseg.initPos = initSeg;
    lseg.endPos = oit-1; //the endPos is the last operation of the previous segment
    lseg.markAtomic = false;
    lseg.hasDependencies = false;
    lseg.tid = prevTid;
    
    //check if the current block comprises operations with dependencies
    for(int dit = dependIt; dit < exclusiveAltIds.size(); dit++)
    {
        dependIt = dit;
        
        if(lseg.endPos < exclusiveAltIds[dit])
            break;
        
        if(lseg.initPos > exclusiveAltIds[dit])
            continue;
        
        lseg.hasDependencies = true;
        lseg.dependencies.push_back(exclusiveAltIds[dit]);
    }
    if(relevantThreads.find(lseg.tid)!=relevantThreads.end()){
        segsAlt.push_back(lseg);
    }
    
    //** cutoff common prefix
    vector<ThreadSegment>::iterator fit = segsFail.begin();
    
    //for each segment check if the the corresponding segment in alt schedule is of the same size of not
    while(fit != segsFail.end())
    {
        vector<ThreadSegment>::iterator ait = segsAlt.begin();
        while(ait != segsAlt.end() && fit != segsFail.end())
        {
            ThreadSegment fseg = *fit;
            ThreadSegment aseg = *ait;
            
            string fOp = failSchedule[fseg.initPos]; //first operation of the fail segment
            string aOp = altSchedule[aseg.initPos]; //first operation of the alt segment
            
            //if operations are different, then proceed
            if(fOp.compare(aOp)){
                ait++;
            }
            else
            {
                int fsize = fseg.endPos - fseg.initPos;
                int asize = aseg.endPos - aseg.initPos;
                
                if(!aseg.hasDependencies &&
                   !fseg.hasDependencies &&
                   asize == fsize)
                {
                    fit = segsFail.erase(fit);
                    ait = segsAlt.erase(ait);
                    
                    if(fit==segsFail.end())
                        break;
                    else
                        continue;
                }
                else if(asize > fsize && aseg.hasDependencies)
                {
                    ait->markAtomic = true;
                }
                //move forward in the failing segments and restart iterating over the alt segments
                fit++;
                ait = segsAlt.begin();
            }
        }
        if(fit==segsFail.end())
            break;
        else
            fit++;
    }
    
    
    //new - cutoff identical events within thread segments (this is not optimized, as it could have been done in the previous cycle..)
    fit = segsFail.begin();
    while(fit != segsFail.end())
    {
        vector<ThreadSegment>::iterator ait = segsAlt.begin();
        while(ait != segsAlt.end() && fit != segsFail.end())
        {
            //prune common prefix within the segments, until operations are different or one of them has a dependency
            string fOp = failSchedule[fit->initPos]; //first operation of the fail segment
            string aOp = altSchedule[ait->initPos]; //first operation of the alt segment
            
            if(fOp == aOp){
                bool isDependency = false;
                while(fOp == aOp && !isDependency)
                {
                    //check whether the head operations are involved in a dependency or not; stop pruning if so
                    for(vector<int>::iterator tmpit = fit->dependencies.begin(); tmpit != fit->dependencies.end(); ++tmpit){
                        if(fit->initPos == *tmpit){
                            isDependency = true;
                            break;
                        }
                    }
                    
                    for(vector<int>::iterator tmpit = ait->dependencies.begin(); tmpit != ait->dependencies.end() && !isDependency; ++tmpit){
                        if(ait->initPos == *tmpit){
                            isDependency = true;
                            break;
                        }
                    }
                    
                    if(!isDependency){
                        fit->initPos++;
                        ait->initPos++;
                        fOp = failSchedule[fit->initPos];
                        aOp = altSchedule[ait->initPos];
                    }
                }
                
                //erase potential empty segments
                if(fit->initPos > fit->endPos)
                    fit = segsFail.erase(fit);
                
                if(ait->initPos > ait->endPos)
                    ait = segsAlt.erase(ait);
                
                break;
            }
            else{
                ait++;
            }
        }
        fit++;
    }
    
    //draw graphviz file
    drawGraphviz(segsFail, segsAlt, failSchedule, altSchedule);
}



string makeInstrFriendly(string instruction){

    int filenameP =  (int)instruction.find("&");
    int lineP =      (int)instruction.find("@");
    int isOSlock = (int)instruction.find("OS-lock");
    int isOSunlock = (int)instruction.find("OS-unlock");
    string friendlyInstr = instruction;
    
    if(string::npos != filenameP && string::npos != lineP){
        
        string filename = getFilenameOp(friendlyInstr);
        int line = getLineOp(friendlyInstr);
        
        if (string::npos != isOSlock || string::npos != isOSunlock) {
            
            int varID = getVarIDlock(friendlyInstr);
            string lockVarName = getVarName(varID);
            string lockStr = " lock(";
            if (string::npos != isOSunlock) {
                lockStr = " unlock(";
            }
            friendlyInstr = filename + " L"+ to_string(line) + lockStr + lockVarName+");";
            return friendlyInstr;
        }
        string codeLine = graphgen::getCodeLine(line, filename);
        friendlyInstr = filename+" L"+codeLine;
    }
    return friendlyInstr;
}


string graphgen::getCodeLine(int line, string filename)
{
    numOps = 0;
    
    string opendir = "cd "+ sourceFilePath;
    //string opendir = "cd /Users/drk/Desktop/symbiosisProj/Tests/simpleAssert";
    string line2code = "nl -ba "+ filename+" | grep  \"  "+ util::stringValueOf(line) +  "\\t\"";
    string sysExeGetCodeLine = (opendir+"; "+line2code);
    char *command = (char *)sysExeGetCodeLine.c_str();
    
    lineCode_pid = util::popen2(command, &procW, &procR);
    if (!lineCode_pid)
    {
        perror("Problems with getCodeLine pipe");
        exit(1);
    }
    
    string ret;
    char c[LINEMAX];
    read(procR,c,1);
    while(c[0]!='\n')
    {
        ret = ret + c[0];
        read(procR,c,1);
        //getline(procRDK,c);
    }
    ret = ret.substr(ret.find_first_of("1234567890")); //remove de /x01 caracter
    cout  << "\ncodeLine: "<< ret << "\n";
    return ret;
    
}


/*
 * Draws the graphviz file for a given failing schedule and alternate schedule
 *
 */
void graphgen::drawGraphviz(vector<ThreadSegment> segsFail, vector<ThreadSegment> segsAlt, vector<string> failSchedule, vector<string> altSchedule)
{
    std::ofstream outFile;
    map<string,string> opToPort; //for a given operation, indicates its port label of form "tableId:port"
    
    string path = solutionFile.substr(0, solutionFile.find_last_of("/"));
    string appname = util::extractFileBasename(solutionFile);
    
    if(appname.find("solution")!=string::npos)
        appname.erase(appname.find("solution"),8); //try to parse app name for files
    if(appname.find("ALT")!=string::npos)
        appname.erase(appname.find("ALT"),3);
    if(appname.find(".txt")!=string::npos)
        appname.erase(appname.find(".txt"));
    path.append("/DSP/dsp_"+appname+"_Alt"+util::stringValueOf(altCounter)+".gv");
    
    outFile.open(path, ios::trunc);
    cout << "Saving graph to file: " << path << "\n";
    if(!outFile.is_open())
    {
        cerr << " -> Error opening file "<< path <<".\n";
        outFile.close();
        exit(0);
    }
    
    outFile << "digraph G {\n\tranksep=.25; size = \"7.5,10\";\n\tnode [shape=record]\n\n";
    
    //** draw all segments for the failing schedule
    for(int i = 0; i < segsFail.size(); i++)
    {
        ThreadSegment fseg = segsFail[i];
        outFile << "f" << i << " [fontname=\"Helvetica\", fontsize=\"11\", shape=none, margin=0,\n";
        outFile << "\tlabel=<<table border=\"0\" cellspacing=\"0\">\n";
        outFile << "\t\t<tr><td border=\"1\" bgcolor=\""<< threadColors[fseg.tid]<<"\"><font point-size=\"14\">T"<<fseg.tid<<"</font></td></tr>\n";
        
        
        /*
         *ORIGIN
         ThreadSegment fseg = segsFail[i];
         outFile << "f" << i << " [fontname=\"Helvetica\", fontsize=\"11\", shape=none, margin=0,\n";
         outFile << "\tlabel=<<table border=\"0\" cellspacing=\"0\">\n";
         outFile << "\t\t<tr><td border=\"1\" bgcolor=\""<< threadColors[fseg.tid]<<"\"><font point-size=\"14\">T"<<fseg.tid<<"</font></td></tr>\n";
         */
        
        for(int j = fseg.initPos; j <= fseg.endPos; j++)
        {
            string op = failSchedule[j];
            string port = getFailDependencePort(op);
            string friendlyStr = makeInstrFriendly(op);
            
            
            if(port.empty()){
                outFile << "\t\t<tr><td align=\"left\" border=\"1\">" << cleanOperation(friendlyStr) << "</td></tr>\n";
                //outFile << "\t\t<tr><td align=\"left\" border=\"1\">" << codeLine << "</td></tr>\n";
            }
            else{
                outFile << "\t\t<tr><td align=\"left\" border=\"1\" port=\""<<port<<"\" bgcolor=\"red\">" << cleanOperation(friendlyStr) << "</td></tr>\n";
                opToPort[("f"+op)] = "f"+util::stringValueOf(i)+":"+port+":e";
            }
            numEventsDifDebug++;
        }
        outFile << "\t</table>>\n]\n\n";
    }
    
    //** draw edges for the failing schedule
    for(int i = 0; i < segsFail.size()-1; i++)
    {
        outFile << "f"<<i<<" -> "<<"f"<<i+1<<";\n";
    }
    
    //** draw data-dependence edges for the failing schedule
    for(map<string,string>::iterator it = exclusiveFail.begin(); it!=exclusiveFail.end();++it)
    {
        outFile << opToPort[("f"+it->second)] << " -> " << opToPort[("f"+it->first)] << " [color=red, style=bold];\n";
    }
    
    outFile << "\n\n";
    
    //** draw all segments for the alternate schedule
    for(int i = 0; i < segsAlt.size(); i++)
    {
        ThreadSegment aseg = segsAlt[i];
        outFile << "a" << i << " [fontname=\"Helvetica\", fontsize=\"11\", shape=none, margin=0,\n";
        outFile << "\tlabel=<<table border=\"";
        
        if(aseg.markAtomic){
            outFile << "4\" cellspacing=\"0\">\n";
        }
        else{
            outFile << "0\" cellspacing=\"0\">\n";
        }
        
        outFile << "\t\t<tr><td border=\"1\" bgcolor=\""<< threadColors[aseg.tid]<<"\"><font point-size=\"14\">T"<<aseg.tid<<"</font></td></tr>\n";
        
        for(int j = aseg.initPos; j <= aseg.endPos; j++)
        {
            string op = altSchedule[j];
            
            //we don't want to print the failure operation in alt schedules
            if(op.find("FAILURE")!=string::npos){
                if(aseg.endPos-aseg.initPos == 0) //replace FAILURE for exit if block only contains one operation
                    op.replace(3, 7, "exit");
                else
                    continue;
            }
            
            string port = getAltDependencePort(op);
            string friendlyStr = makeInstrFriendly(op);
            
            if(port.empty()){
                outFile << "\t\t<tr><td align=\"left\" border=\"1\">" << cleanOperation(friendlyStr) << "</td></tr>\n";
            }
            else{
                outFile << "\t\t<tr><td align=\"left\" border=\"1\" port=\""<<port<<"\" bgcolor=\"green\">" << cleanOperation(friendlyStr) << "</td></tr>\n";
                opToPort[("a"+op)] = "a"+util::stringValueOf(i)+":"+port+":e";
            }
        }
        outFile << "\t</table>>\n]\n\n";
    }
    
    //** draw edges for the alternate schedule
    for(int i = 0; i < segsAlt.size()-1; i++)
    {
        outFile << "a"<<i<<" -> "<<"a"<<i+1<<";\n";
    }
    
    //** draw data-dependence edges for the alternate schedule
    for(map<string,string>::iterator it = exclusiveAlt.begin(); it!=exclusiveAlt.end();++it)
    {
        outFile << opToPort[("a"+it->second)] << " -> " << opToPort[("a"+it->first)] << " [color=darkgreen, style=bold];\n";
    }
    
    outFile << "}\n";
    outFile.close();
}
