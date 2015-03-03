//
//  SymbiosisBBPass.cpp
//  
//
//  Created by Nuno Machado on 16/10/14.
//
//

#include "llvm/Pass.h"
#include "llvm/Module.h"
#include "llvm/Function.h"
#include "llvm/BasicBlock.h"
#include "llvm/CallingConv.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Analysis/DebugInfo.h"
#include "llvm/Constants.h"
#include "llvm/User.h"
#include "llvm/Constants.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Instructions.h"
#include "llvm/InstrTypes.h"
#include "llvm/Type.h"
#include <stdint.h>

using namespace llvm;

namespace {
    struct SymbiosisBBPass : public ModulePass {
        static char ID;
        static int bbcount;
        Constant *InstFuncConst;
        
        SymbiosisBBPass() : ModulePass(ID){}
        
        virtual bool runOnModule(Module &M)
        {
            LLVMContext &Context = M.getContext();
            InstFuncConst = M.getOrInsertFunction("myBasicBlockEntry",
                                                  Type::getVoidTy(Context), //return type
                                                  Type::getInt32Ty(Context),//param0: basic block counter
                                                  NULL);
            Function *InstFunc = cast<Function>(InstFuncConst);
            InstFunc->setCallingConv(CallingConv::C);
            for (Module::iterator F = M.begin(), E = M.end(); F != E; ++F) {
                if (F->isDeclaration()) continue;
                for (Function::iterator BB = F->begin(), E = F->end(); BB != E; ++BB) {
                    BasicBlock& B = *BB;
                    SymbiosisBBPass::runOnBasicBlock(B);
                }
            }
            return true;
        }
        
        virtual bool runOnBasicBlock(BasicBlock& BB) {
            
            LLVMContext &Context = BB.getContext();
            std::vector<Value*> Args(1);
            Args[0] = ConstantInt::get(IntegerType::get(Context, 32), bbcount);
            CallInst::Create(InstFuncConst, Args.begin(), Args.end(), "", BB.getFirstNonPHI());
            bbcount++;
            
            return true;
        }
    };
}

char SymbiosisBBPass::ID = 0;
int SymbiosisBBPass::bbcount = 0;
static RegisterPass<SymbiosisBBPass> X("symbiosisBB", "Pass that instruments every basic block entry with a call to a Symbiosis monitor at runtime", false, false);
