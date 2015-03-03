//
//  FindCallGraph.cpp
//  
//
//  Created by Nuno Machado on 03/06/14.
//
//

#include "FindCallGraph.h"
#include "llvm/Support/InstIterator.h"
//#include "llvm/IR/Module.h"
#include "llvm/Module.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/CFG.h"
#include <cstdio>
#include <iostream>

using namespace llvm;

FindCallGraph::FindCallGraph() : ModulePass(ID)
{
    
    llvm::outs() << "Starting the Call Graph Analysis\n";
    std::map< llvm::Function *, std::set<llvm::Function *> > callersOf =
    std::map< llvm::Function *, std::set<llvm::Function *> >();
}

bool FindCallGraph::doInitialization (Module &M)
{
    boundariesInFunc = std::map< llvm::Function *, std::set<llvm::BasicBlock *> >();
    return false;
}


void FindCallGraph::findAllCallers( Module &M){
    
    for(Module::iterator fi = M.begin(); fi != M.end(); ++fi)
    {
        for(Function::iterator bi = fi->begin(), bi_end = fi->end(); bi != bi_end; ++bi)
        {
            for(BasicBlock::iterator ii = bi->begin(), ii_end = bi->end(); ii != ii_end; ++ii)
            {
                Function *OtherF = NULL;
                
                CallInst *CI = dyn_cast<CallInst>(ii);
                if( CI != NULL ) {
                    
                    OtherF = CI->getCalledFunction();
                    
                }
                
                InvokeInst *II = dyn_cast<InvokeInst>(ii);
                if( II != NULL ) {
                    OtherF = II->getCalledFunction();
                }
                
                if( OtherF != NULL ){
                    if( callersOf.find(OtherF) == callersOf.end() ){
                        callersOf[OtherF] = std::set<Function *>();
                    }
                    
                    callersOf[OtherF].insert(fi);
                }
            }
        }
    }
}

bool FindCallGraph::runOnModule (Module &M)
{
    findAllCallers(M);
    return false;
}




void FindCallGraph::getAnalysisUsage (AnalysisUsage &AU) const
{
    /*Modifies the CFG!*/
    AU.addRequired<CallGraph>();
}

bool FindCallGraph::doFinalization (Module &M)
{
    return false;
}

const char *FindCallGraph::getPassName () const {
    return "Symbiosis Call Graph Analysis";
}

char FindCallGraph::ID = 0;

/*ModulePass *llvm::createFindCallGraph() {
    return new FindCallGraph();
}*/