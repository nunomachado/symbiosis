//
//  Schedule.h
//  SymbiosisSolverXcode
//
//  Created by Daniel Ribeiro Quinta on 20/03/15.
//  Copyright (c) 2015 Daniel Ribeiro Quinta. All rights reserved.
//

#ifndef __SymbiosisSolverXcode__Schedule__
#define __SymbiosisSolverXcode__Schedule__

#include <stdio.h>
#include "Parameters.h"
#include "ConstraintModelGenerator.h"


namespace scheduleLIB{
    //protected:
    //Schedule _schedule;
    
    //ScheduleC(Schedule sch);
    void printSch (Schedule sch);     //Print Schedule using OrderConstraintName
    std::string getTidOperation(Operation op);     //return the operation/action thread ID
    int getTEIsize(Schedule schedule, int initPosition);     // return TID size with the TID start position
    
    //insert TEI in a schedule
    Schedule insertTEI(Schedule schedule, int newPosition, Schedule tei);
    
    //removeTEI from a schedule
    Schedule removeTEI(Schedule schedule, int initPosition);
    
    Schedule getTEI(Schedule schedule, int startPostion);
    
    //change TEI block to another location (TEI - thread execution interval)
    Schedule moveTEISch(Schedule list,int newPositon, int oldPosition);
    
    //cheeck if action in a given position is the last one in its TEI
    bool isLastActionTEI(Schedule sch, int pos);
    
    //return next action positon within the same thread: < 0 false | >= 0 next action position
    int hasNextTEI(Schedule sch, int pos);
    
    Schedule moveUpTEI(Schedule schedule,ConstModelGen *cmgen);
    Schedule moveDownTEI(Schedule schedule,ConstModelGen *cmgen);
    Schedule scheduleSimplify(Schedule schedule,ConstModelGen *cmgen);
    std::vector<std::string> getSolutionStr(Schedule _schedule);  //create a string vector of actions, e.i. used in solver.
}

#endif /* defined(__SymbiosisSolverXcode__Schedule__) */
