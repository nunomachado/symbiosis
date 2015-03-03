//
//  FindCallGraph.h
//  
//
//  Created by Nuno Machado on 03/06/14.
//
//

#ifndef ____FindCallGraph__
#define ____FindCallGraph__

#include <iostream>
#include <llvm/Analysis/CallGraph.h>
#include <set>

class FindCallGraph: public llvm::ModulePass {
    
public:
    
    static char ID;
    std::map< llvm::Function *, std::set<llvm::Function *> > callersOf;
    FindCallGraph();
    virtual const char *getPassName() const;
    
    virtual bool doInitialization (llvm::Module &M);
    virtual bool runOnModule (llvm::Module &M);
    virtual bool doFinalization (llvm::Module &M);
    virtual void getAnalysisUsage (llvm::AnalysisUsage &AU) const;
    
private:
    std::map< llvm::Function *, std::set<llvm::BasicBlock *> > boundariesInFunc;
    std::map< llvm::Function *, unsigned long > allocaCostOf;
    void findAllCallers( llvm::Module &M );
};

#endif /* defined(____FindCallGraph__) */



