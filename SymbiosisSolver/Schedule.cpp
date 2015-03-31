//
//  Schedule.cpp
//  SymbiosisSolverXcode
//
//  Created by Daniel Ribeiro Quinta on 20/03/15.
//  Copyright (c) 2015 Daniel Ribeiro Quinta. All rights reserved.
//

#include "Schedule.h"
#include "Parameters.h"
#include "ConstraintModelGenerator.h"
#include "Util.h"

using namespace std;


//Print Schedule using OrderConstraintName
void scheduleLIB::printSch(Schedule sch){
    int i = 0 ;
    cout << "Schedule size: " << sch.size()<< endl;
    cout << "Schedule contextSwitches: " << getContextSwitchNum(sch)<< endl;
   
    
    
    for(Schedule ::iterator it = sch.begin(); it != sch.end(); ++it) {
        int strID = util::intValueOf((*it)->getThreadId());
        string tabs = util::threadTabsPP(strID);
        
        cout << tabs <<"["<< i <<"] ";//<<  (*it)->getThreadId();
        cout << (*it)->getOrderConstraintName() << endl;
        i++;
    }
    cout << endl << endl;
}


//return a non empty vector<string (operations)>
vector<string> cleanEmpty(vector<string> *globalOrderTmp)
{
    vector<string> globalOrder;
    for(int i = 0; i < globalOrderTmp->size(); i++){
        string op = (*globalOrderTmp)[i];
        if(!op.empty()){
            globalOrder.push_back(op);
        }
    }
    return globalOrder;
}



//fill Schedule
void scheduleLIB::loadSchedule(vector<string> *globalOrderTmp)
{
    map<string,vector<Operation*> > t2op = operationsByThread;
    //clean empty positions in globalOrder
    vector<string> globalOrder = cleanEmpty(globalOrderTmp);
    Schedule scheduleTmp; //TODO isto deixou de fazer sentido pq temos estruturas diferentes para fail and altschedule
    scheduleTmp.clear();
    scheduleTmp.reserve(globalOrder.size());
    
    if(!bugFixMode)
    {
        for(int i = 0; i < globalOrder.size(); i++)
        {
            string op = globalOrder[i];
            
            int id = util::getTid(op);
            string tid = util::stringValueOf(id);
            
            //fill failScheduleOrd
            util::fillScheduleOrd(tid, &t2op, &scheduleTmp);
        }
        failScheduleOrd.clear();
        failScheduleOrd.reserve(globalOrder.size());
        failScheduleOrd = scheduleTmp;
    }
    else
    {
        altScheduleOrd = globalOrder;
    }
}


// transform a given schedule do a string's vector
std::vector<std::string> scheduleLIB::schedule2string(Schedule schedule)
{
    vector<string> listOp;
    for(Schedule::iterator it= schedule.begin() ; it != schedule.end(); it ++)
    {
        listOp.push_back((*it)->getOrderConstraintName());
    }
    return listOp;
}


//saveSch2file
void scheduleLIB::saveScheduleFile(string filename, vector<string> listOp){
    // open the output file to store the solution
    string solConst;
    std::ofstream solFile;
    
    solFile.open(filename, ios::trunc);
    if(!solFile.is_open())
    {
        cerr << " -> Error opening file "<< formulaFile <<".\n";
        solFile.close();
        exit(0);
    }
    cout << "Saving solution to file: " << filename << endl;
    
    int labelsol = 0; //label counter (we can't use i, because some positions in globalOrder array may be empty)
    for(int i = 0; i < listOp.size()-1; i++)
    {
        string op = listOp[i];
        
        //add solution constraint to
        solConst = "(assert (! (< "+listOp[i]+" "+listOp[i+1]+" ):named solution"+util::stringValueOf(labelsol)+"))\n";
        solFile << solConst;  //write to solution file
        labelsol++;
        
    }
    solFile.close();
}


//return the number of context switches
int scheduleLIB::getContextSwitchNum(Schedule sch){
    int count = 0;
    string oldTid = sch[0]->getThreadId();
    for(Schedule::iterator it = sch.begin(); it != sch.end()-1; it++)
    {
        string nextTid = (*(it+1))->getThreadId();
        if (nextTid != oldTid)
        {
            count++;
        }
        oldTid = nextTid;
    }
    return count;
}


//return the operation/action thread ID
string scheduleLIB::getTidOperation(Operation op)
{
    return op.getThreadId();
}


// return TID size with the TID start position
int scheduleLIB::getTEIsize(Schedule schedule, int initPosition)
{
    string tid = getTidOperation(*schedule[initPosition]); //(schedule,initPosition);
    //Schedule::iterator it = schedule.begin()+initPosition;
    int size = 1;
    
    //size incrementation until the thread ID of the next action is different
    for(Schedule::iterator it = schedule.begin()+initPosition+1; it !=schedule.end(); it++){
        if (tid == getTidOperation(**it))
        {
            size++;
        }
        else break; //it = schedule.end();
    }
    return size;
    
}

//insert TEI in a schedule
Schedule scheduleLIB::insertTEI(Schedule schedule, int newPosition, Schedule tei)
{
    Schedule::iterator newPositionIt = schedule.begin()+newPosition;
    schedule.insert(newPositionIt,tei.begin(),tei.end());
    return schedule;
}

//removeTEI from a schedule
Schedule scheduleLIB::removeTEI(Schedule schedule, int initPosition)
{
    int size = getTEIsize(schedule,initPosition);
    Schedule::iterator it = schedule.begin()+initPosition;
    schedule.erase(it,it+size);
    return schedule;
}


Schedule scheduleLIB::getTEI(Schedule schedule, int startPostion){
    int size = getTEIsize(schedule, startPostion);
    
    //Schedule tei = schedule; if erase deletes the main struct
    //erase TAIL
    Schedule::iterator it = schedule.begin();
    schedule.erase(it+startPostion+size, schedule.end());
    //erase HEAD
    it = schedule.begin();
    schedule.erase(it,it+startPostion);
    
    return schedule;
}

//change TEI block to another location (TEI - thread execution interval)
Schedule scheduleLIB::moveTEISch(Schedule list,int newPositon, int oldPosition)
{
    Schedule tei = getTEI(list,oldPosition);
    return insertTEI(removeTEI(list,oldPosition),newPositon+1, tei);
}


//cheeck if action in a given position is the last one in its TEI
bool scheduleLIB::isLastActionTEI(Schedule sch, int pos)
{
    string Tid = getTidOperation(*sch[pos]);
    //cout << (*sch[pos]).getConstraintName() << endl;
    if(pos < sch.size()-1)
    {
        string nextTid = getTidOperation(*sch[pos+1]);
        return (Tid != nextTid);
    }
    else
        return false;
}


//return next action positon within the same thread: < 0 false | >= 0 next action position
int scheduleLIB::hasNextTEI(Schedule sch, int pos)
{
    int nextTEIPosition = -1;
    string Tid = getTidOperation(*sch[pos]);
    string nextTid = "";
    for (Schedule::iterator it= sch.begin()+pos+1; it != sch.end(); it ++)
    {
        nextTid = getTidOperation(**it);
        if(Tid == nextTid)
            return (int) distance(sch.begin(),it);
    }
    return nextTEIPosition;
    
}

//create a string vector of actions, e.i. used in solver.
vector<string> scheduleLIB::getSolutionStr(Schedule schedule){
    vector<string> actionsList;
    int i=0 ;
    int size = 0 ;
    cout << size << "\n" ;
    for(Schedule::iterator it = schedule.begin(); it != schedule.end(); it++)
    {
        //cout << i << " Solution: " << (*it)->getOrderConstraintName() << endl;
        actionsList.push_back((*it)->getOrderConstraintName());
        i++;
        
    }
    return actionsList;
}


Schedule scheduleLIB::moveUpTEI(Schedule schedule,ConstModelGen *cmgen, bool isReverse)
{
    int i=0;
    int prox = -1; // < 0 false ; >= 0 represents the action posion
    
    Schedule currentSch = schedule;
    Schedule oldSch = currentSch;
    Schedule reverseSch;
    bool valid;
    
    for(Schedule::iterator it = schedule.begin(); it != schedule.end();it++)
    {
        if(isLastActionTEI(currentSch,i))
        {
            prox = hasNextTEI(currentSch,i);
            if(prox != -1)
            {
                currentSch = moveTEISch(currentSch,i,prox);
                if(isReverse)
                {
                    reverseSch = currentSch;
                    reverse(reverseSch.begin(),reverseSch.end());
                    valid = cmgen->solveWithSolution(getSolutionStr(reverseSch), false);
                }
                else
                    valid = cmgen->solveWithSolution(getSolutionStr(currentSch), false);
                
                if (valid)
                    oldSch = currentSch; // save the new solution in oldSch
                else
                    currentSch = oldSch; // return to the last valid solution
            }
        }
        i++;
    }
    return oldSch;
}


Schedule scheduleLIB::moveDownTEI(Schedule schedule, ConstModelGen *cmgen)
{
    reverse(schedule.begin(),schedule.end());
    
    Schedule sch = moveUpTEI(schedule, cmgen, true);
    
    reverse(sch.begin(),sch.end());
    
    return sch;
}


Schedule scheduleLIB::scheduleSimplify(Schedule schedule, ConstModelGen *cmgen)
{
    Schedule currentSch = schedule;
    Schedule oldSch = schedule;
    bool continueS = true ;
    int count = 0;
    while(continueS)
    {
        //removeLastTEI
        //NOT IMPLEMENTED
        
        //move-Up-TEI
        bool notReverse = false;
        currentSch = scheduleLIB::moveUpTEI(currentSch, cmgen, notReverse);
        
        //move-Down-TEI
        currentSch = scheduleLIB::moveDownTEI(currentSch, cmgen);
        
        if(getContextSwitchNum(currentSch) < getContextSwitchNum(oldSch))
        {
            oldSch = currentSch;    //simplification continues
            count++;
            cout << count << " simplifications" << endl;
        }
        else
            continueS = false; //no effect simplification, exit cycle.
        
    }
    return oldSch;
}
