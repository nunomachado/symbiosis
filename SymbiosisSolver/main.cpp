//
//  main.cpp
//  symbiosisSolver
//
//  Created by Nuno Machado on 30/12/13.
//  Copyright (c) 2013 Nuno Machado. All rights reserved.
//
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <iostream>
#include <fstream>
#include <map>
#include <set>
#include <vector>
#include <string.h>
#include <stack>
#include <dirent.h>
#include <algorithm>
#include "Operations.h"
#include "ConstraintModelGenerator.h"
#include "Util.h"
#include "Types.h"
#include "Parameters.h"
#include "GraphvizGenerator.h"

using namespace std;


map<string, vector<RWOperation> > readset;              //map var id -> vector with variable's read operations
map<string, vector<RWOperation> > writeset;             //map var id -> vector with variable's write operations
map<string, vector<LockPairOperation> > lockpairset;    //map object id -> vector with object's lock pair operations
map<string, SyncOperation> startset;                    //map thread id -> thread's start operation
map<string, SyncOperation> exitset;                     //map thread id -> thread's exit operation
map<string, vector<SyncOperation> > forkset;            //map thread id -> vector with thread's fork operations
map<string, vector<SyncOperation> > joinset;            //map thread id -> vector with thread's join operations
map<string, vector<SyncOperation> > waitset;            //map object id -> vector with object's wait operations
map<string, vector<SyncOperation> > signalset;          //map object id -> vector with object's signal operations
vector<SyncOperation> syncset;
vector<PathOperation> pathset;
map<string, map<string, stack<LockPairOperation> > > lockpairStack;   //map object id -> (map thread id -> stack with incomplete locking pairs)
int numIncLockPairs = 0;    //number of incomplete locking pairs, taking into account all objects
map<string, vector<string> > symTracesByThread;         //map thread id -> vector with the filenames of the symbolic traces
vector<string> solution;                                //vector that stores a given schedule (i.e. solution) found by the solver (used in --fix-mode)

AvisoTrace atrace;          //map: thread Id -> vector<avisoEvent>
AvisoEventVector fulltrace; //sorted vector containing all avisoEvents


/**
 *  Parse the input arguments.
 */
void parse_args(int argc, char *const* argv)
{
    int c;
    
    if(argc < 5)
    {
        cerr << "Not enough arguments.\nUsage:\n#FAILING SCHEDULE FINDING MODE\n--trace-folder=/path/to/symbolic/traces/folder \n--with-solver=/path/to/solver/executable \n--model=/path/to/output/constraint/model/file \n--solution=/path/to/output/solution/file \n--debug (print optional debug info)\n\n#BUG FIXING MODE\n--fix-mode (this flag must be set on in order to run in bug fixing mode) \n--model=/path/to/input/constraint/model/file\n--solution=/path/to/input/solution/file \n--with-solver=/path/to/solver/executable \n--debug (print optional debug info)\n";
        exit(1);
    }
    
    while(1)
    {
        static struct option long_options[] =
        {
            //{"fail-thread", required_argument, 0, 'r'},
            {"trace-folder", required_argument, 0, 'c'},
            {"aviso-trace", required_argument, 0, 'a'},
            {"with-solver", required_argument, 0, 's'},
            {"model", required_argument, 0, 'm'},
            {"solution", required_argument, 0, 'l'},
            {"debug", no_argument, 0, 'd'},
            {"fix-mode", no_argument, 0, 'f'},
           
            {0, 0, 0, 0}
        };
        /* getopt_long stores the option index here. */
        int option_index = 0;
        
        c = getopt_long(argc, argv, "", long_options, &option_index);
        
        
        /* Detect the end of the options. */
        if (c == -1)
            break;
        
        switch (c)
        {
            case 'd':
                debug = true;
                break;
                
            case 'a':
                avisoFilePath = optarg;
                break;
                
            case 'c':
                symbFolderPath = optarg;
                break;
                
            case 's':
                solverPath = optarg;
                break;
                
            case 'm':
                formulaFile = optarg;
                break;
                
            case 'l':
                solutionFile = optarg;
                break;
                
            case 'f':
                bugFixMode = true;
                break;
                
            /*case 'r':
                assertThread = optarg;
                break;
            */
            case '?':
                /* getopt_long already printed an error message. */
                break;
                
            default:
                abort ();
        }
    }
    
    /* Print any remaining command line arguments (not options). */
    if (optind < argc)
    {
        printf ("non-option ARGV-elements: ");
        while (optind < argc)
            printf ("%s ", argv[optind++]);
        putchar ('\n');
        exit(1);
    }
    
    if(bugFixMode && (formulaFile.empty() || solutionFile.empty() || solverPath.empty()))
    {
        cerr << "Not enough arguments for bugFixMode.\nUsage: --model=/path/to/input/constraint/model\n--solution=/path/to/input/solution\n--with-solver=/path/to/solver/executable\n";
        exit(1);
    }
    else if(!bugFixMode && (formulaFile.empty() || solutionFile.empty() || solverPath.empty() || symbFolderPath.empty()))
    {
        cerr << "Not enough arguments.\nUsage:\n--trace-folder=/path/to/symbolic/traces/folder\n--aviso-trace=/path/to/aviso/trace\n--with-solver=/path/to/solver/executable\n--model=/path/to/output/constraint/model\n--solution=/path/to/output/solution\n";
        exit(1);
    }
    
    //** pretty print
    if(bugFixMode)
        cout << "# MODE: FIND BUG'S ROOT CAUSE\n";
    else
        cout << "# MODE: FIND BUG-TRIGGERING SCHEDULE\n";
    
     if(!avisoFilePath.empty()) cout << "# AVISO TRACE: " << avisoFilePath << "\n";
     if(!symbFolderPath.empty()) cout << "# SYMBOLIC TRACES: " << symbFolderPath << "\n";
     cout << "# SOLVER: " << solverPath << "\n";
     cout << "# CONSTRAINT MODEL: " << formulaFile << "\n";
     cout << "# SOLUTION: " << solutionFile << "\n";
    
    cout << "\n";
}

/**
 * Parse the symbolic information contained in a trace and
 * populate the respective data strucutures
 */
void parse_constraints(string symbFilePath)
{
    bool rwconst = false;
    bool pathconst = false;
    int line = 0;
    string filename;
    string syncType;
    string threadId;
    string obj;
    string var;
    int varId;
    
    map<string, int> varIdCounters; //map var name -> int counter to uniquely identify the i-th similar operation
    map<string, int> reentrantLocks; //map lock obj -> counter of reentrant acquisitions (used to differentiate reentrant acquisitions of the same lock)
    
    ifstream fin;
    symbFilePath = symbFolderPath+"/"+symbFilePath;
    fin.open(symbFilePath);
    if (!fin.good())
    {
        util::print_state(fin);
        cerr << " -> Error opening file "<< symbFilePath <<".\n";
        fin.close();
        exit(0);
    }
    
    std::cout << ">> Parsing " << util::extractFileBasename(symbFilePath) << "\n";
    
    // read each line of the file
    while (!fin.eof())
    {
        // read an entire line into memory
        char buf[MAX_LINE_SIZE];
		fin.getline(buf, MAX_LINE_SIZE); 
        char* token;
        string event = buf;
        
        switch (buf[0]) {
            case '<':
                token = strtok (buf,"<>");
                if(!strcmp(token,"readwrite"))  //is readwrite constraints
                {
                    rwconst = true;
                    if(debug) cout << "parsing readwrite constraints...\n";
                }
                else if(!strcmp(token,"path"))
                {
                    pathconst = true;
                    if(debug) cout << "parsing path constraints...\n";
                }
                else if(!strcmp(token,"pathjpf"))
                {
                    pathconst = true;
                    jpfMode = true;
                    if(debug) cout << "parsing path constraints...\n";
                }
                else if(!strcmp(token,"assertThread_ok"))
                {
                    string tmpfname = util::extractFileBasename(symbFilePath);
                    tmpfname = tmpfname.substr(1,tmpfname.find_first_of("_\n")-1); //extract thread id from file name
                    failedExec = false;
                    assertThread = tmpfname;
                    cout << "# ASSERT THREAD: " << assertThread << " (ok)\n";
                }
                else if(!strcmp(token,"assertThread_fail"))
                {
                    string tmpfname = util::extractFileBasename(symbFilePath);
                    tmpfname = tmpfname.substr(1,tmpfname.find_first_of("_\n")-1); //extract thread id from file name
                    failedExec = true;
                    assertThread = tmpfname;
                    cout << "# ASSERT THREAD: " << assertThread << " (fail)\n";
                }
                break;
                
            case '$':   //indicates that is a write
            {
                string tmp = buf; //tmp is only used to ease the check of the last character
                if(tmp.back() == '$')
                {
                    token = strtok (buf,"$"); //token is the written value
                    if(token == NULL){
                        //token = "0";
                        token[0] = '0';
                        token[1] = '\0';
                    }
                    RWOperation* op = new RWOperation(threadId, var, 0, line, filename, token, true);
                    
                    //update variable id
                    string varname = op->getOrderConstraintName();
                    if(!varIdCounters.count(varname))
                    {
                        varIdCounters[varname] = 0;
                    }
                    else
                    {
                        varIdCounters[varname] = varIdCounters[varname] + 1;
                    }
                    op->setVariableId(varIdCounters[varname]);
                    
                    writeset[var].push_back(*op);
                    operationsByThread[threadId].push_back(op);
                    
                }
                else
                {
                    cout << tmp << "\n";
                    tmp.erase(0,1); //erase first '$'
                    string value = "";
                    while(tmp.back() != '$')
                    {
                        value.append(tmp);
                        fin.getline(buf, MAX_LINE_SIZE);
                        tmp = buf;
                    }
                    tmp.erase(tmp.find('$'),1); //erase last '$'
                    value.append(tmp); //add last expression part
                    
                    RWOperation* op = new RWOperation(threadId, var, 0, line, filename, value, true);
                    
                    //update variable id
                    string varname = op->getOrderConstraintName();
                    if(!varIdCounters.count(varname))
                    {
                        varIdCounters[varname] = 0;
                    }
                    else
                    {
                        varIdCounters[varname] = varIdCounters[varname] + 1;
                    }
                    op->setVariableId(varIdCounters[varname]);
                    
                    writeset[var].push_back(*op);
                    operationsByThread[threadId].push_back(op);
                }
                break;
            }
                
            case 'T':  //indicates that is a path constraint
            {
                string tmp = buf;
             
                //make sure that this is a path condition and not the name of the file
                if(tmp.find("@")==string::npos){
                    threadId = tmp.substr(1,tmp.find(":")-1); //get the thread id
                    
                    string expr;
                    tmp.erase(0,tmp.find(":")+1);
                    
                    while(expr.back() != ')' || !util::isClosedExpression(expr))
                    {
                        expr.append(tmp);
                        if(util::isClosedExpression(expr))
                            break;
                        fin.getline(buf, MAX_LINE_SIZE);
                        tmp = buf;
                    }
                    //expr.append(tmp); //handles the last case in which the last char is ')'
                    
                    //remove unnecessary spaces from expression
                    size_t space = expr.find("  ");
                    while(space!= std::string::npos)
                    {
                        size_t endspace = expr.find_first_not_of(" ",space);
                        expr.replace(space, endspace-space, " ");
                        space = expr.find("  ");
                    }
                    
                    PathOperation* po = new PathOperation(threadId, "", 0, 0, filename, expr);
                    pathset.push_back(*po);
                    break;
                }
            }
                
            default:  //constraint has form line:constraint
            {
                if(!strcmp(buf,""))
                    break;
                token = strtok (buf,"@");
                filename = token;
                token = strtok (NULL,"-:");
                line = atoi(token);
                token = strtok (NULL,"-:"); //token = type (S,R, or W)
                
                if(!strcmp(token,"S"))  //handle sync constraints
                {
                    token = strtok (NULL,"-_");
                    syncType = token;
                    if(!strcmp(token,"lock"))
                    {
                        token = strtok (NULL,"-_");
                        obj = token;
                        
                        token = strtok (NULL,"-\n");
                        threadId = token;
                        
                        //check whether it is a reentrant lock
                        //if so, change the name of the object to OBJrN, where N is the number of reentrant acquisitions
                        /*if(reentrantLocks.count(obj) && reentrantLocks[obj] > 0){
                            int rcounter = reentrantLocks[obj];
                            string newobj = obj + "r" + util::stringValueOf(rcounter);
                            rcounter++;
                            reentrantLocks[obj] = rcounter;
                            obj = newobj;
                        }
                        else{
                            reentrantLocks[obj] = 1;
                        }*/
                        
                        //we don't add a constraint for a reentrant lock
                        if(reentrantLocks.count(obj) == 0 || reentrantLocks[obj] == 0){
                            reentrantLocks[obj] = 1;
                        }
                        else if(reentrantLocks[obj] > 0){
                            int rcounter = reentrantLocks[obj];
                            rcounter++;
                            reentrantLocks[obj] = rcounter;
                            continue;
                        }
                        
                        //add the lock operation to the thread memory order set in its correct order
                        SyncOperation* op = new SyncOperation(threadId,obj,0,line,filename,"lock");
                        
                        //update variable id
                        string varname = op->getOrderConstraintName();
                        if(!varIdCounters.count(varname)){
                            varIdCounters[varname] = 0;
                        }
                        else{
                            varIdCounters[varname] = varIdCounters[varname] + 1;
                        }
                        op->setVariableId(varIdCounters[varname]);
                        operationsByThread[threadId].push_back(op);
                        
                        //create new lock pair operation
                        LockPairOperation lo (threadId,obj,varIdCounters[varname],filename,line,-1,0);
                        
                        //the locking pair is not complete, so we add it to a temp stack
                        if(lockpairStack.count(obj)){
                            lockpairStack[obj][threadId].push(lo);
                        }
                        else{
                            stack<LockPairOperation> s;
                            s.push(lo);
                            lockpairStack[obj][threadId] = s;
                        }
                        numIncLockPairs++;
                    }
                    else if(!strcmp(token,"unlock"))
                    {
                        token = strtok (NULL,"-_");
                        obj = token;
                        
                        token = strtok (NULL,"-\n");
                        threadId = token;
                        
                        //check whether it is a reentrant unlock
                        /*if(reentrantLocks.count(obj) && reentrantLocks[obj] > 0){
                            int rcounter = reentrantLocks[obj];
                            rcounter--; //we have to decrement before renaming the obj to match the last rcounter
                            string newobj;
                            
                            continue;
                            
                            if(rcounter > 0){
                                newobj = obj + "r" + util::stringValueOf(rcounter);
                            }
                            else{
                                newobj = obj;
                            }
                            reentrantLocks[obj] = rcounter;
                            obj = newobj;
                        }*/
                        //we don't add a constraint for a reentrant lock
                        if(reentrantLocks.count(obj) && reentrantLocks[obj] >= 1){
                            int rcounter = reentrantLocks[obj];
                            rcounter--;
                            reentrantLocks[obj] = rcounter;
                            if(rcounter > 0)
                                continue;
                        }
                        
                        //the unlock completes the locking pair, thus we can add it to the lockpairset
                        LockPairOperation *lo = new LockPairOperation(lockpairStack[obj][threadId].top());
                        lockpairStack[obj][threadId].pop();
                        lo->setUnlockLine(line);
                        lo->setVariableName(obj);
                        numIncLockPairs--;
                        
                        //add the unlock operation to the thread memory order set in its correct order
                        SyncOperation* op = new SyncOperation(threadId,obj,0,line,filename,"unlock");
                        
                        //update variable id
                        string varname = lo->getUnlockOrderConstraintName();
                        if(!varIdCounters.count(varname))
                        {
                            varIdCounters[varname] = 0;
                        }
                        else
                        {
                            varIdCounters[varname] = varIdCounters[varname] + 1;
                        }
                        op->setVariableId(varIdCounters[varname]);
                        lo->setUnlockVarId(varIdCounters[varname]);
                        
                        lockpairset[obj].push_back(*lo);
                        operationsByThread[threadId].push_back(op);
                    }
                    else if(!strcmp(token,"fork"))
                    {
                        token = strtok (NULL,"-_");
                        obj = token;
                        
                        token = strtok (NULL,"-\n");
                        threadId = token;
                        
                        SyncOperation* op = new SyncOperation(threadId,obj,0,line,filename,syncType);
                        
                        if(forkset.count(threadId)){
                            forkset[threadId].push_back(*op);
                        }
                        else{
                            vector<SyncOperation> v;
                            v.push_back(*op);
                            forkset[threadId] = v;
                        }
                        operationsByThread[threadId].push_back(op);
                    }
                    else if(!strcmp(token,"join"))
                    {
                        token = strtok (NULL,"-_");
                        obj = token;
                        
                        token = strtok (NULL,"-\n");
                        if(token == NULL){
                            cout << ">> PARSING ERROR: Missing child thread id in event \""<< event <<"\"! Join event must have format \"S-join_childId-parentId\". Please change file " << util::extractFileBasename(symbFilePath) << " accordingly.\n";
                            exit(EXIT_FAILURE);
                        }
                        threadId = token;
                        
                        SyncOperation* op = new SyncOperation(threadId,obj,0,line,filename,syncType);
                        
                        if(joinset.count(threadId)){
                            joinset[threadId].push_back(*op);
                        }
                        else{
                            vector<SyncOperation> v;
                            v.push_back(*op);
                            joinset[threadId] = v;
                        }
                        operationsByThread[threadId].push_back(op);
                    }
                    else if(!strcmp(token,"wait") || !strcmp(token,"timedwait"))
                    {
                        token = strtok (NULL,"-_");
                        obj = token;
                        
                        token = strtok (NULL,"-\n");
                        threadId = token;
                        
                        SyncOperation* op = new SyncOperation(threadId,obj,0,line,filename,syncType);
                        
                        //update variable id
                        string varname = op->getOrderConstraintName();
                        if(!varIdCounters.count(varname))
                        {
                            varIdCounters[varname] = 0;
                        }
                        else
                        {
                            varIdCounters[varname] = varIdCounters[varname] + 1;
                        }
                        op->setVariableId(varIdCounters[varname]);
                        
                        if(waitset.count(obj)){
                            waitset[obj].push_back(*op);
                        }
                        else{
                            vector<SyncOperation> v;
                            v.push_back(*op);
                            waitset[obj] = v;
                        }
                        
                        //as the wait operations release and acquire locks internally, we also have to account for that behavior
                        vector<Operation*> tmpvec = operationsByThread[threadId];
                        for(vector<Operation*>::reverse_iterator in = tmpvec.rbegin(); in!=tmpvec.rend(); ++in)
                        {
                            SyncOperation* lop = dynamic_cast<SyncOperation*>(*in);
                            if(lop!=0 && lop->getType() == "lock")
                            {
                                //1 - create an unlock operation and the corresponding locking pair
                                string lockobj = lop->getVariableName();
                                LockPairOperation *lo = new LockPairOperation(lockpairStack[lockobj][threadId].top());
                                lockpairStack[lockobj][threadId].pop();
                                lo->setUnlockLine(line);
                                lo->setVariableName(lockobj);
                                lo->setVariableId(0); //we need to set var id to 0 in order to match the correct key for lock operations (see 'lock' case above)
                                lo->setVariableId(varIdCounters[lo->getLockOrderConstraintName()]); //now, set the var id to the correct value
                                
                                //create an extra unlock operation
                                SyncOperation* unlop = new SyncOperation(threadId,lockobj,0,line,filename,"unlock");
                                
                                //update unlock var id
                                string uvarname = lo->getUnlockOrderConstraintName();
                                if(!varIdCounters.count(uvarname)){
                                    varIdCounters[uvarname] = 0;
                                }
                                else{
                                    varIdCounters[uvarname] = varIdCounters[uvarname] + 1;
                                }
                                unlop->setVariableId(varIdCounters[uvarname]);
                                lo->setUnlockVarId(varIdCounters[uvarname]);
                                
                                //add the extra unlock to the thread memory order set
                                lockpairset[lockobj].push_back(*lo);
                                operationsByThread[threadId].push_back(unlop);
                                
                                //2 - insert timedwait in the operations vector
                                operationsByThread[threadId].push_back(op);
                                
                                //3 - create a new lock operation
                                SyncOperation* newlop = new SyncOperation(threadId,lockobj,0,line,filename,"lock");
                                
                                //update lock variable id
                                string newlvarname = newlop->getOrderConstraintName();
                                if(!varIdCounters.count(newlvarname)){
                                    varIdCounters[newlvarname] = 0;
                                }
                                else{
                                    varIdCounters[newlvarname] = varIdCounters[newlvarname] + 1;
                                }
                                newlop->setVariableId(varIdCounters[newlvarname]);
                                
                                //add the lock operation to the thread memory order set in its correct order
                                operationsByThread[threadId].push_back(newlop);
                                
                                //create a new locking pair
                                LockPairOperation newlpair (threadId,lockobj,varIdCounters[newlvarname],filename,line,-1,0);
                                
                                //the locking pair is not complete, so we add it to a temp stack
                                lockpairStack[lockobj][threadId].push(newlpair);
                                
                                break;
                            }
                        }
                    }
                    else if(!strcmp(token,"signal") || !strcmp(token,"signalall"))
                    {
                        token = strtok (NULL,"-_");
                        obj = token;
                        
                        token = strtok (NULL,"-\n");
                        threadId = token;
                        
                        SyncOperation* op = new SyncOperation(threadId,obj,0,line,filename,syncType);
                        
                        //update variable id
                        string varname = op->getOrderConstraintName();
                        if(!varIdCounters.count(varname)){
                            varIdCounters[varname] = 0;
                        }
                        else{
                            varIdCounters[varname] = varIdCounters[varname] + 1;
                        }
                        op->setVariableId(varIdCounters[varname]);
                        
                        if(signalset.count(obj)){
                            signalset[obj].push_back(*op);
                        }
                        else{
                            vector<SyncOperation> v;
                            v.push_back(*op);
                            signalset[obj] = v;
                        }
                        operationsByThread[threadId].push_back(op);
                    }
                    else if(!strcmp(token,"start"))
                    {
                        token = strtok (NULL,"-\n");
                        threadId = token;
                        
                        SyncOperation* op = new SyncOperation(threadId,"",0,line,filename,syncType);
                        startset[threadId] = *op;
                        operationsByThread[threadId].push_back(op);
                    }
                    else if(!strcmp(token,"exit"))
                    {
                        token = strtok (NULL,"-\n");
                        threadId = token;
                        
                        SyncOperation* op = new SyncOperation(threadId,"",0,line,filename,syncType);
                        exitset[threadId] = *op;
                        operationsByThread[threadId].push_back(op);
                    }
                    else //syncType is unknown
                    {
                        token = strtok (NULL,"-\n");
                        threadId = token;
                        
                        SyncOperation* op = new SyncOperation(threadId,"",0,line,filename,syncType);
                        syncset.push_back(*op);
                        operationsByThread[threadId].push_back(op);
                    }
                    
                } else if(!strcmp(token,"R"))
                {
                    token = strtok (NULL,"-");
                    var = token;
                    token = strtok (NULL,"-");
                    while(token[0] == '>'){
                        var.append("-");
                        var.append(token);
                        token = strtok (NULL,"-");
                    }
                    threadId = token;
                    token = strtok (NULL,"-\n");
                    varId = atoi(token);
                    
                    RWOperation* op = new RWOperation(threadId, var, varId, line, filename,"", false);
                    readset[var].push_back(*op);
                    operationsByThread[threadId].push_back(op);
                    
                } else if(!strcmp(token,"W"))
                {
                    token = strtok (NULL,"-");
                    var = token;
                    token = strtok (NULL,"-");
                    while(token[0] == '>'){
                        var.append("-");
                        var.append(token);
                        token = strtok (NULL,"-");
                    }
                    threadId = token;
                }
                break;
            }
        }
    } //end while
    fin.close();
    
    //resolve written values that are references to other writes
    for(map<string, vector<RWOperation> >:: iterator oit = writeset.begin(); oit != writeset.end(); ++oit)
    {
        for(vector<RWOperation>:: iterator iit = oit->second.begin(); iit != oit->second.end(); ++iit)
        {
            RWOperation wOp = *iit;
            string wRef = wOp.getValue(); //check whether the written value is a reference to another write
            if(wRef.substr(0,2) == "W-")
            {
                //find that referenced write and replace the value with the referenced one
                for(vector<RWOperation>:: iterator rit = oit->second.begin(); rit != oit->second.end(); ++rit)
                {
                    RWOperation wOp2 = *rit;
                    if(wRef == wOp2.getConstraintName()){
                        
                    }
                }
            }
        }
    }
    
    //add non-closed locking pairs to lockpairset
    while(numIncLockPairs > 0)
    {
        for(vector<Operation*>::reverse_iterator rit = operationsByThread[threadId].rbegin();
            rit != operationsByThread[threadId].rend() && numIncLockPairs > 0; ++rit)
        {
            SyncOperation* tmplop = dynamic_cast<SyncOperation*>(*rit);
            if(tmplop!=0 && tmplop->getType() == "lock"
               && lockpairStack[tmplop->getVariableName()][threadId].size() > 0)
            {
                string tmpobj = tmplop->getVariableName();
                LockPairOperation* lo = new LockPairOperation(lockpairStack[tmpobj][threadId].top());
                int prevLine = operationsByThread[threadId].back()->getLine(); //** line of the last event in thread's trace
                lo->setUnlockLine(prevLine+1);
                lo->setFakeUnlock(true);
                lo->setVariableId(tmplop->getVariableId());
                
                
                //** create a fake unlock (placed after the last event in the schedule) and complete the locking pair
                SyncOperation* op = new SyncOperation(threadId,lo->getVariableName(),0,prevLine+1,filename,"unlockFake");
                
                //update var id
                string uvarname = lo->getUnlockOrderConstraintName();
                if(!varIdCounters.count(uvarname)){
                    varIdCounters[uvarname] = 0;
                }
                else{
                    varIdCounters[uvarname] = varIdCounters[uvarname] + 1;
                }

                op->setVariableId(varIdCounters[uvarname]);
                lo->setUnlockVarId(varIdCounters[uvarname]);
                
                operationsByThread[threadId].push_back(op);
                lockpairset[lo->getVariableName()].push_back(*lo);
                lockpairStack[tmpobj][threadId].pop();
                numIncLockPairs--;
            }
        }
    }
    lockpairStack.clear(); //** doing this we guarantee that we only add lock constraints only once
    
    //add the exit events...
	int prevLine = operationsByThread[threadId].back()->getLine();
    SyncOperation* op;
    if(assertThread == threadId){
       // if(failedExec)
           op = new SyncOperation(threadId, "", 0, prevLine + 1, filename, "AssertFAIL");
        //else
          //  op = new SyncOperation(threadId, "", 0, prevLine + 1, filename, "AssertOK");
    }
	else
        op = new SyncOperation(threadId, "", 0, prevLine + 1, filename, "exit");
	operationsByThread[threadId].push_back(op);
	exitset[threadId] = *op;
  
}

/**
 *  Parse the events contained in the Aviso trace.
 */
void parse_avisoTrace()
{
    ifstream fin;
    fin.open(avisoFilePath);
    
    if (!fin.good())
    {
        util::print_state(fin);
        cout << " -> Error opening file "<< avisoFilePath <<".\n";
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
            aetmp.filename = util::extractFileBasename(token);
            
            token = strtok (NULL," :"); //token == line of code
            aetmp.loc = atoi(token);
            
            //cout << "TID: " << aetmp.tid << " Filename: " << aetmp.filename << " Loc: "<< aetmp.loc << "\n";
            
            atrace[aetmp.tid].push_back(aetmp);
            fulltrace.push_back(aetmp);
        }
    }
    fin.close();
    
    
    if(debug)
    {
        cout<< "\n### AVISO TRACE\n";
        
        for (unsigned i = 0; i < fulltrace.size(); i++) {
            cout << "[" << fulltrace[i].tid << "] " << util::extractFileBasename(fulltrace[i].filename) << "@" << fulltrace[i].loc << "\n";
        }
        cout << "\n";
    }
}

/**
 *  Generate and solve the contraint model
 *  for a given set of symbolic traces
 */
bool verifyConstraintModel(ConstModelGen *cmgen)
{
    bool success = false;
    
    cout << "\n### GENERATING CONSTRAINT MODEL\n";
    cmgen->openOutputFile(); //** opens a new file to store the model
    
    cout << "[Solver] Adding memory-order constraints...\n";
    cmgen->addMemoryOrderConstraints(operationsByThread);
    
    cout << "[Solver] Adding read-write constraints...\n";
    cmgen->addReadWriteConstraints(readset,writeset,operationsByThread);
    
    cout << "[Solver] Adding path constraints...\n";
    cmgen->addPathConstraints(pathset);
    
    cout << "[Solver] Adding locking-order constraints...\n";
    cmgen->addLockingConstraints(lockpairset);
    
    cout << "[Solver] Adding fork/start constraints...\n";
    cmgen->addForkStartConstraints(forkset, startset);
    
    cout << "[Solver] Adding join/exit constraints...\n";
    cmgen->addJoinExitConstraints(joinset, exitset);
    
    cout << "[Solver] Adding wait/signal constraints...\n";
    cmgen->addWaitSignalConstraints(waitset, signalset);
    
   /* cout << "[Solver] Adding Aviso constraints...\n";
    cmgen->addAvisoConstraints(operationsByThread, fulltrace);
     //*/
    cout << "\n### SOLVING CONSTRAINT MODEL: Z3\n";
    success = cmgen->solve();
    
    //** clean data structures
    cmgen->resetSolver();
    readset.clear();
    writeset.clear();
    lockpairset.clear();
    startset.clear();
    exitset.clear();
    forkset.clear();
    joinset.clear();
    waitset.clear();
    signalset.clear();
    syncset.clear();
    pathset.clear();
    lockpairStack.clear();
    operationsByThread.clear();
    
    return success;
}


/**
 *  Function used to update the counters used to generate all combinations
 *  of symbolic trace files.
 */
bool updateCounters(vector<string> keys, vector<int> *traceCounterByThread)
{
    for(int i = (int)keys.size()-1; i>=0; i--)
    {
        string tid = keys[i];
        if((*traceCounterByThread)[i] < symTracesByThread[tid].size()-1)
        {
            (*traceCounterByThread)[i]++;
            return true;
        }
        else
        {
            (*traceCounterByThread)[i] = 0;
        }
    }
    cout << ">> No more combinations left!\n";
    return false;
}

/**
 *  Comparator to sort filenames in ascending order of their length
 *
 */
bool filenameComparator(string a, string b)
{
    return (a.length() < b.length());
}

/**
 *  Identify the files containing symbolic traces pick
 *  a set of traces to generate the constraint model
 */
void generateConstraintModel()
{
    string symbolicFile = "";
    bool foundBug = false;  //boolean var indicating whether the solver found a model that triggers the bug or not
    vector<string> keys;    //vector of strings indicating the names of the threads (used for optimizing the iterations over the other data structures)
    int attempts = 0;       //number of attempts to obtain the buggy interleaving tested so far
    
    
    //** instatiate a constrain model generator object
    ConstModelGen* cmgen = new ConstModelGen();
    cmgen->createZ3Solver();
    
    //** find symbolic trace files and populate map symTracesByThread
    DIR* dirFile = opendir(symbFolderPath.c_str());
    if ( dirFile )
    {
        struct dirent* hFile;
        while (( hFile = readdir( dirFile )) != NULL )
        {
            if ( !strcmp( hFile->d_name, "."  )) continue;
            if ( !strcmp( hFile->d_name, ".." )) continue;
            
            // in linux hidden files all start with '.'
            if (hFile->d_name[0] == '.' ) continue;
            
            if ( strstr( hFile->d_name, "T" ))
            {
                char filename[250];
                strcpy(filename, hFile->d_name);
               // std::cerr << "found a symbolic trace file: " << filename << "\n";
                
                //** extract the thread id to serve as key in the map
                string tid = filename;
                tid = tid.substr(tid.find("T")+1,tid.find("_")-1);
                
                if(symTracesByThread.count(tid))
                {
                    string shortname = util::extractFileBasename(filename);
                    symTracesByThread[tid].push_back(shortname);
                }
                else
                {
                    vector<string> vec;
                    vec.push_back(util::extractFileBasename(filename));
                    symTracesByThread[tid] = vec;
                }
            }
        }
        closedir( dirFile );
    }
    
    //** test all combinations of traces to find a feasible buggy interleaving
    vector<int> traceCounterByThread; //vector of thread counters (each array position corresponds to a given thread): used to iteratively pick a different thread symb trace to generate the constraint model
    
    //** initialize counters and sort files in ascending order of their name (i.e. path length)
    for(map<string, vector<string> >:: iterator it = symTracesByThread.begin(); it != symTracesByThread.end() ; ++it)
    {
        traceCounterByThread.push_back(0);
        keys.push_back(it->first);
        
        //** sort files in ascending order of their name (i.e. path length)
        std::sort(symTracesByThread[it->first].begin(), symTracesByThread[it->first].end(), filenameComparator);
    }
    
    do
    {
        std::cout << "\n---- ATTEMPT " << attempts << "\n";
        
        //** pick one symbolic trace per thread
        for(int i = 0; i < keys.size(); i++)
        {
            string tid = keys[i];
            parse_constraints(symTracesByThread[tid][traceCounterByThread[i]]);
        }
        
        //debug: print constraints
        if(debug)
        {
            cout<< "\n-- READ SET\n";
            for(map< string, vector<RWOperation> >::iterator out = readset.begin(); out != readset.end(); ++out)
            {
                vector<RWOperation> tmpvec = out->second;
                for(vector<RWOperation>::iterator in = tmpvec.begin() ; in != tmpvec.end(); ++in)
                {
                    in->print();
                }
            }
            
            cout<< "\n-- WRITE SET\n";
            for(map< string, vector<RWOperation> >::iterator out = writeset.begin(); out != writeset.end(); ++out)
            {
                vector<RWOperation> tmpvec = out->second;
                for(vector<RWOperation>::iterator in = tmpvec.begin() ; in != tmpvec.end(); ++in)
                    in->print();
            }
            
            cout<< "\n-- LOCKPAIR SET\n";
            for(map<string, vector<LockPairOperation> >::iterator out = lockpairset.begin(); out != lockpairset.end(); ++out)
            {
                vector<LockPairOperation> tmpvec = out->second;
                for(vector<LockPairOperation>::iterator in = tmpvec.begin() ; in!=tmpvec.end(); ++in)
                    in->print();
            }
            
            cout<< "\n-- WAIT SET\n";
            for(map<string, vector<SyncOperation> >::iterator out = waitset.begin(); out != waitset.end(); ++out)
            {
                vector<SyncOperation> tmpvec = out->second;
                for(vector<SyncOperation>::iterator in = tmpvec.begin() ; in!=tmpvec.end(); ++in)
                    in->print();
            }
            
            cout<< "\n-- SIGNAL SET\n";
            for(map<string, vector<SyncOperation> >::iterator out = signalset.begin(); out != signalset.end(); ++out)
            {
                vector<SyncOperation> tmpvec = out->second;
                for(vector<SyncOperation>::iterator in = tmpvec.begin() ; in!=tmpvec.end(); ++in)
                    in->print();
            }
            
            
            cout<< "\n-- FORK SET\n";
            for (map<string, vector<SyncOperation> >::iterator it=forkset.begin(); it!=forkset.end(); ++it)
            {
                vector<SyncOperation> tmpvec = it->second;
                for(vector<SyncOperation>::iterator in = tmpvec.begin(); in!=tmpvec.end(); ++in)
                    in->print();
            }
            
            cout<< "\n-- JOIN SET\n";
            for (map<string, vector<SyncOperation> >::iterator it=joinset.begin(); it!=joinset.end(); ++it)
            {
                vector<SyncOperation> tmpvec = it->second;
                for(vector<SyncOperation>::iterator in = tmpvec.begin(); in!=tmpvec.end(); ++in)
                    in->print();
            }
            
            cout<< "\n-- START SET\n";
            for (map<string, SyncOperation>::iterator it=startset.begin(); it!=startset.end(); ++it)
            {
                it->second.print();
            }
            
            cout<< "\n-- EXIT SET\n";
            for (map<string, SyncOperation>::iterator it=exitset.begin(); it!=exitset.end(); ++it)
            {
                it->second.print();
            }
            
            if(!syncset.empty())
            {
                cout<< "\n-- OTHER SYNC SET\n";
                for(vector<SyncOperation>::iterator it = syncset.begin() ; it!=syncset.end(); ++it)
                    it->print();
            }
            
            cout<< "\n-- PATH SET\n";
            for(vector<PathOperation>::iterator it = pathset.begin() ; it!=pathset.end(); ++it)
                it->print();
            
            cout<< "\n### OPERATIONS BY THREAD\n";
            for (map<string, vector<Operation*> >::iterator it=operationsByThread.begin(); it!=operationsByThread.end(); ++it)
            {
                cout << "-- Thread " << it->first <<"\n";
                vector<Operation*> tmpvec = it->second;
                for(vector<Operation*>::iterator in = tmpvec.begin(); in!=tmpvec.end(); ++in)
                {
                    (*in)->print();
                }
            }
        }
        
        //** generate the constraint model and try to solve it
        foundBug = verifyConstraintModel(cmgen);
        attempts++;
        
    }while(!foundBug && updateCounters(keys, &traceCounterByThread));
    
    cmgen->closeSolver();
}


/* Generate pairs of events to be inverted, between a given set of operations
 * and the operations in the unsat core.
 * A pair is comprised of two segments, where each segment is itself a pair
 * indicating the init and the end positions in the schedule 
 *
 *  mapOpToId -> maps events to its id in the array containing the failing schedule
 *  opsToInvert -> vector of events to be inverted in the new schedule
 */
vector<EventPair> generateEventPairs(map<string, int> mapOpToId, vector<string> opsToInvert)
{
    vector<EventPair> eventPairs;
    for(vector<string>::iterator it = opsToInvert.begin(); it!=opsToInvert.end();++it)
    {
        string op1 = *it;
        string tid1 = util::parseThreadId(op1);
        string var1 = util::parseVar(op1);
        Segment seg1 = std::make_pair(mapOpToId[op1],mapOpToId[op1]);
        
        /* if the operation is not wrapped by a lock, the segment will be a pair
         * with the position of the operation in the schedule array.
         * Otherwise, it is a pair with the positions of the lock/unlock operations in the array.*/
        int i;
        for(i = mapOpToId[op1]; i > 0; i--)
        {
            string op2 = solution[i];
            string tid2 = util::parseThreadId(op2);
            
            if(op2.find("-lock")!=std::string::npos && tid2 == tid1)
            {
                //the operation is wrapped by a lock
                seg1.first = i;
                break;
            }
            else if (op2.find("-unlock")!=std::string::npos)
            {
                //the operation is not wrapped by a lock
                i = 0;
            }
        }
        
        if(i > 0) //if wrapped by a lock, find the corresponding unlock
        {
            for(i = mapOpToId[op1]; i < solution.size(); i++)
            {
                string op2 = solution[i];
                if(op2.find("-unlock")!=std::string::npos)
                {
                    seg1.second = i;
                    break;
                }
            }
        }
        
        /* generate pairs for the operation in the bug condition and
         the operations in the unsatCore (from other threads)*/
        cout << "\n>> Event Pairs for '"<< op1 <<"':\n";
        for(int i = 0; i<unsatCore.size(); i++)
        {
            int pos = unsatCore[i];
            string op2 = solution[pos];
            string tid2 = util::parseThreadId(op2);
            
            //** disregard events of the same thread, as well as exits/joins
            if(tid1 == tid2
               || op2.find("-exit-")!=string::npos
               || op2.find("-FAILURE-")!=string::npos
               || op2.find("-AssertOK-")!=string::npos
               || op2.find("-AssertFAIL-")!=string::npos
               || op2.find("-join_")!=string::npos)
                continue;
            
            //** disregard read operations and events (which are not locks) on different variables
            string var2 = util::parseVar(op2);
            if(op2.find("R-")!=string::npos
               || (op2.find("lock")==string::npos && var1 != var2))
                continue;
            
            Segment seg2 = std::make_pair(pos, pos);
            EventPair p;
            
            //** check whether the operation to be inverted occurs before or after the segment containing the bug condition operation
            if(seg1.first < seg2.first){
                
                //** if the op occurs concurrently with the critical section, than it is not wrapped by locks and thus can be directly inverted with the bug condition operation
                if(seg1.second > seg2.first){
                    Segment segR = std::make_pair(mapOpToId[op1],mapOpToId[op1]);
                    p = std::make_pair(seg2, segR);
                }
                else
                    p = std::make_pair(seg1, seg2);
            }
            else
                p = std::make_pair(seg2, seg1);
            eventPairs.insert(eventPairs.begin(), p);
            
            cout << pairToString(p, solution);
        }
    }
    return eventPairs;
}

/* Generate a new schedule by inverting the event pairs in the original failing schedule.
 */
vector<string> generateNewSchedule(EventPair invPair)
{
    vector<string> bugCore; //vector used to store the events that compose the 'bug core'; if the event pair produces a sat schedule, then we save the bugCore in the map altSchedules
    
    //** generate the new schedule by copying the original schedule, apart from the events to be reordered
    int i = 0;
    vector<string> newSchedule;
    
    //** for a pair (A,B) to be inverted, add events from init to A
    for(i = 0; i < solution.size(); i++)
    {
        if(i == invPair.first.first){
            break;
        }
        newSchedule.push_back(solution[i]);
    }
    
    //** from A to B add all events that not belong to A's thread
    int j;
    vector<string> aThreadOps; //vector containing the operations after A that belong to A's thread
    //parse A's thread id
    string opA = solution[i];
    size_t init, end;
    string tidA = util::parseThreadId(opA);
    
    //parse B's thread id
    string opB = solution[invPair.second.first];
    string tidB = util::parseThreadId(opB);
    
    for(j = i; j < solution.size(); j++)
    {
        //stop if we hit the first event of segment B
        if(j == invPair.second.first)
        {
            break;
        }
        
        //parse op thread id
        string opC = solution[j];
        string tidC = util::parseThreadId(opC);
        
        //** we don't want to reorder the events of the other threads (Nuno: we're not doing this at the moment)
        if(tidC!=tidB)//if(tidA == tidB)
            aThreadOps.push_back(opC);
        else
            newSchedule.push_back(opC);
    }
    
    //** add all events of segment B before A
    //** (there might be some events belonging to other threads in seg B,
    //** so add them to aThreadOps)
    // (NOTE: at the end of this loop, 'i' will point to the operation right after segment B's last operation)
    for(i = invPair.second.first; i <= invPair.second.second; i++)
    {
        //parse op thread id
        string opC = solution[i];
        end = opC.find_last_of("-");
        if(opC.find("exit")!=string::npos)
        {
            init = opC.find_last_of("-")+1;
            end = opC.find_last_of("@");
        }
        else
            init = opC.find_last_of("-",end-1)+1;
        string tidC = opC.substr(init,end-init);
        
        if(tidC==tidB)
        {
            newSchedule.push_back(opC);
            bugCore.push_back(opC);
        }
        else
        {
            aThreadOps.push_back(opC);
        }
    }
    
    //** add events of segment of A, and all the other of A's thread that
    //** occurred between A and B
    for(j = 0; j < aThreadOps.size(); j++)
    {
        newSchedule.push_back(aThreadOps[j]);
        bugCore.push_back(aThreadOps[j]);
    }
    
    //** finally, add the remaining events from B to the end
    for(j = i; j < solution.size(); j++)
    {
        newSchedule.push_back(solution[j]);
    }
    
    return newSchedule;
}


/**
 *  Algorithm to find the root cause of a given concurrency bug.
 *
 *  Outline:
 *  1) solve constraint model with the bug-inducing schedule and
 *  invert the bug condition to find the constraints that
 *  make the model to be unsat.
 *
 *  2) generate event pairs for the operations in the buggy schedule
 *  that appear in the unsat core and are part of the bug condition
 *
 *  3) for each pair, invert the event order in the buggy schedule
 *  and solve the constraint model with the no-bug constraint again. If
 *  the new model is sat, then it means that we found the root cause.
 *  Otherwise, simply attempt with another pair.
 */
void findBugRootCause()
{
    map<string, int> mapOpToId; //map: operation name -> id in 'solution' array
    map<EventPair, vector<string> > altSchedules; //set used to store the event pairs that yield a sat non-failing alternative schedule
    bool success; //indicates whether we have found a bug-avoiding schedule
    int numAttempts = 0; //counts the number of attempts to find a sat alternate schedule
    
    ConstModelGen* cmgen = new ConstModelGen();
    cmgen->createZ3Solver();
    
    ifstream inSol(solutionFile);
    string lineSol;
    
    //** store in 'solution' the failing schedule  
    while (getline(inSol, lineSol))
    {
        //** parse first element of the constraint (e.g. for (< O1 O2) -> parse O1)
        size_t init = lineSol.find_first_of("<")+2;
        size_t end = lineSol.find_first_of(" ",init);
        string constraint = lineSol.substr(init,end-init);
        solution.push_back(constraint);
    }
    inSol.close();
    
    cmgen->solveWithSolution(solution,true); //** solve the model with the bug condition inverted in order to get the unsat core
    
    //** sort because the values in unsatCore are often in descending order (which the opposite of the memory-order of the program)
    std::sort(unsatCore.begin(),unsatCore.end());
    
    //** check if the unsat core begins within a region wrapped by a lock; if so, fetch all the operations until the locking operation
    for(int i = 0; i<unsatCore.size();i++)
    {
        string op = solution[unsatCore[i]];
        if(op.find("-lock")!=std::string::npos)
            break;
        else if (op.find("-unlock")!=std::string::npos) //we are missing a lock, search backwards in the solution schedule for it
        {
            for(int j = unsatCore[0]-1; j>0; j--)
            {
                string op2 = solution[j];
                unsatCore.insert(unsatCore.begin(), j);
                if(op2.find("-lock")!=std::string::npos)
                    break;
            }
            break;
        }
    }
    
    cout << "\n>> Operations in unsat core ("<< unsatCore.size() <<"):\n";
    for(int i = 0; i<unsatCore.size();i++)
    {
        string op = solution[unsatCore[i]];
        cout << op << "\n";
        mapOpToId[op] = unsatCore[i];
    }
    
    cout << "\n>> Operations in bug condition ("<< bugCondOps.size() <<"):\n";
    for(int i = 0; i < bugCondOps.size(); i++)
    {
        string bop = bugCondOps[i];
        cout << bop <<"\n";
        
        //find the position of each op in the bug condition in the solution array
        for(int j = 0; j<solution.size();j++)
        {
            string op = solution[j];
            if(op.find(bop)!=string::npos)
            {
                int pos = j;
                mapOpToId[bop] = pos;
                break;
            }
        }
    }
    
    //generate event pairs with the operations from the bug condition
    vector<EventPair> eventPairs = generateEventPairs(mapOpToId, bugCondOps);
    
    /* Generate a new schedule by inverting the event pairs and try to solve the original constraint model.
     * If it is sat with the bug condition inverted, than we found the root cause of the bug;
     * otherwise, attempt again with a new pair
     */
    for(vector<EventPair>::iterator it = eventPairs.begin(); it!=eventPairs.end();++it)
    {
        EventPair invPair; //the pair to be inverted
        invPair = *it;
        vector<string> newSchedule = generateNewSchedule(invPair);
        
        cout << "\n------------------------\n";
        cout << "["<< ++numAttempts <<"] Attempt by inverting pair:\n" << pairToString(invPair, solution) << "\n";
        
        bugCondOps.clear(); //clear bugCondOps to avoid getting repeated operations
        
        success = cmgen->solveWithSolution(newSchedule,true);
        if(success)
        {
            cout << "\n>> FOUND BUG AVOIDING SCHEDULE:\n" << bugCauseToString(invPair, solution);
            //altSchedules[invPair] = bugCore;
            altSchedules[invPair] = newSchedule;
            break;
        }
    }
    
    //if we haven't found any alternate schedule by manipulating the events in the bug condition
    //let's broad the search scope to consider all reads on variables that appear in the bug condition
    if(!success)
    {
        cout << "\n\n>> No alternate schedule found! Increase search scope to consider other read operations on the variables contained in the bug condition.\n";
        
        //find the new operations ops to be inverted
        vector<string> opsToInvert;
        for(int i = 0; i < bugCondOps.size(); i++)
        {
            string bop = bugCondOps[i];
            string bvar = util::parseVar(bop);
            cout << "> For "<< bvar <<":\n";
            for(int j = 0; j < mapOpToId[bop]; j++)
            {
                string sop = solution[j];
                string svar = util::parseVar(sop);
                
                //store operation if its a read on the same var that of the Op in the bug condition
                if(svar == bvar && sop.find("R-")!=string::npos)
                {
                    cout << sop << "\n";
                    opsToInvert.push_back(sop);
                    mapOpToId[sop] = j;
                }
            }
        }
        
        //generate event pairs with the new set of operations
        eventPairs.clear();
        eventPairs = generateEventPairs(mapOpToId, opsToInvert); //NOTE: the unsat at this point might be different from the first one.. this might cut-off some events (?)
        
        //generate the respective new schedule and attempt to solve the model again
        for(vector<EventPair>::iterator it = eventPairs.begin(); it!=eventPairs.end();++it)
        {
            EventPair invPair; //the pair to be inverted
            invPair = *it;
            vector<string> newSchedule = generateNewSchedule(invPair);
            
            cout << "\n------------------------\n";
            cout << "["<< ++numAttempts <<"] Attempt by inverting pair:\n" << pairToString(invPair, solution) << "\n";
            
            bugCondOps.clear(); //clear bugCondOps to avoid getting repeated operations
            
            success = cmgen->solveWithSolution(newSchedule,true);
            if(success)
            {
                cout << "\n>> FOUND BUG AVOIDING SCHEDULE:\n" << bugCauseToString(invPair, solution);
                //altSchedules[invPair] = bugCore;
                altSchedules[invPair] = newSchedule;
                break;
            }
        }
    }
    
    //print data-dependencies and stats only when Symbiosis has found an alternate schedule
    if(success)
    {
        //** compute data dependencies
        cout << "=======================================\n";
        cout << "DATA DEPENDENCIES: \n\n";
        
        graphgen::genAllGraphSchedules(solution,altSchedules);
        
        cout << "=======================================\n";
        cout << "STATISTICS: \n";
        cout << "\n#Events in the full failing schedule: " << solution.size();
        cout << "\n#Events in the unsat core: " << unsatCore.size();
        cout << "\n#Events in the diff-debug schedule: " << numEventsDifDebug;
        cout << "\n#Data-dependencies in the full failing schedule: " << numDepFull;
        cout << "\n#Data-dependencies in the diff-debug schedule: " << numDepDifDebug << "\n";
    }
    
}

int main(int argc, char *const* argv)
{
    parse_args(argc, argv);
    if(bugFixMode)
    {
        findBugRootCause();
    }
    else
    {
        //parse_avisoTrace();
        generateConstraintModel();
        
        
        for(vector<Operation> ::iterator it = failScheduleOrd.begin(); it != failScheduleOrd.end(); ++it) {
            //it->print();
            RWOperation* tmprw = dynamic_cast<RWOperation*>(&(*it));
            if(tmprw!=0)
            {
                cout << "IS RW!\n";
            }
            //cout << it->getOrderConstraintName() << ", File: "<< it->getFilename() << "\n";
        }

    }
    
    return 0;
}

