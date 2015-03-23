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
    
    
    
    

    int getContextSwitchNum(Schedule sch);     //return the number of context switches
    void printSch (Schedule sch);     //Print Schedule using OrderConstraintName
    std::string getTidOperation(Operation op);     //return the operation/action thread ID
    int getTEIsize(Schedule schedule, int initPosition);     // receive TID start position and return TID size with the
    bool isLastActionTEI(Schedule sch, int pos);    //cheeck if action in a given position is the last one in its TEI
    int hasNextTEI(Schedule sch, int pos);     //return next action positon within the same thread: < 0 false | >= 0 next action position
    Schedule insertTEI(Schedule schedule, int newPosition, Schedule tei); //insert TEI in a schedule
    Schedule removeTEI(Schedule schedule, int initPosition); //removeTEI from a schedule
    Schedule getTEI(Schedule schedule, int startPostion);
    Schedule moveTEISch(Schedule list,int newPositon, int oldPosition);    //change TEI block to another location (TEI - thread execution interval)
    Schedule moveUpTEI(Schedule schedule,ConstModelGen *cmgen, bool isReverse);
    Schedule moveDownTEI(Schedule schedule,ConstModelGen *cmgen);
    Schedule scheduleSimplify(Schedule schedule,ConstModelGen *cmgen);
    std::vector<std::string> getSolutionStr(Schedule _schedule);  //create a string vector of actions, e.i. used in solver.
}

#endif /* defined(__SymbiosisSolverXcode__Schedule__) */
