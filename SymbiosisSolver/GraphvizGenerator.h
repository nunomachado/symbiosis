//
//  GraphvizGenerator.h
//  symbiosisSolver
//
//  Created by Nuno Machado on 30/07/14.
//  Copyright (c) 2014 Nuno Machado. All rights reserved.
//

#ifndef __symbiosisSolver__GraphvizGenerator__
#define __symbiosisSolver__GraphvizGenerator__

#include <iostream>
#include <stdlib.h> 
#include <map>
#include <vector>
#include "Types.h"


namespace graphgen{
    
    std::string getCodeLine(int line, std::string finename);
    
    void genAllGraphSchedules(std::vector<std::string> failSchedule, std::map<EventPair, std::vector<std::string> > altSchedules);
    
    void genGraphSchedule(std::vector<std::string> failSchedule, EventPair invPair, std::vector<std::string> altSchedule);
    
    void drawGraphviz(std::vector<ThreadSegment> segsFail, std::vector<ThreadSegment> segsAlt, std::vector<std::string> failSchedule, std::vector<std::string> altSchedule);
}

#endif /* defined(__symbiosisSolver__GraphvizGenerator__) */
