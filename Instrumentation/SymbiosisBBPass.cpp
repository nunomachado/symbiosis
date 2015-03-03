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
        Constant* MyPTCreateConst;
	Constant* MyAssertConst;
        const Type* pthreadTy;
	static bool bbAfterAssert; //indicates if we are in the successor of the assert_fail block
	static bool flagAssert; //flag that indicates if we've passed through a assert_fail block and, therefore, should instrument the subsequent one (which corresponds to the non-failing branch of the assertion)
        
        SymbiosisBBPass() : ModulePass(ID){}
        
        virtual bool runOnModule(Module &M)
        {
            LLVMContext &Context = M.getContext();
            
            if (M.getPointerSize() == llvm::Module::Pointer64)
            {
                pthreadTy = Type::getInt64Ty(M.getContext());
            }
            else
            {
                pthreadTy = Type::getInt32Ty(M.getContext());
            }
            
            //myBasicBlockEntry
            InstFuncConst = M.getOrInsertFunction("myBasicBlockEntry",
                                                  Type::getVoidTy(Context), //return type
                                                  Type::getInt32Ty(Context),//param0: basic block counter
                                                  NULL);
            Function *InstFunc = cast<Function>(InstFuncConst);
            InstFunc->setCallingConv(CallingConv::C);
            
            
            //myPThreadCreate
            MyPTCreateConst = M.getOrInsertFunction("myPThreadCreate",
                                                   Type::getVoidTy(Context), //return type
                                                   pthreadTy->getPointerTo(),//param0: thread pointer
                                                   NULL);
            Function *MyPTCreateFunc = cast<Function>(MyPTCreateConst);
            MyPTCreateFunc->setCallingConv(CallingConv::C);
            
            //myAssert
            MyAssertConst = M.getOrInsertFunction("myAssert",
                                                   Type::getVoidTy(Context), //return type
						   Type::getInt32Ty(Context),//param:int indicating failure (0) or success (1)
                                                   NULL);
            Function *MyAssertFunc = cast<Function>(MyAssertConst);
            MyAssertFunc->setCallingConv(CallingConv::C);

            for (Module::iterator F = M.begin(), E = M.end(); F != E; ++F) {
                if (F->isDeclaration()) continue;
                for (Function::iterator BB = F->begin(), E = F->end(); BB != E; ++BB) {
                    BasicBlock& B = *BB;
                    SymbiosisBBPass::runOnBasicBlock(B);

		     //set bbAfterAssert to true if we found an assert_fail call
		     if(flagAssert)
			bbAfterAssert = true;
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
            
            //loop over all of the instructions, searching for pthread_create or assertion calls 
            for (BasicBlock::iterator I = BB.begin(), E = BB.end(); I != E; ++I)
            {
                if (CallInst *CI = dyn_cast<CallInst>(I))
                {
                    Function *FUNC = CI->getCalledFunction();
                    //llvm::errs() << "function call: " << fun->getName() << "\n";
                    if (FUNC && FUNC->getName() == "pthread_create")
                    {
                        Value* threadpt = CI->getArgOperand(0);
                        //threadpt->dump();
                        //threadpt->getType()->dump();
                        std::vector<Value*> Args(1);
                        Args[0] = threadpt;
			BasicBlock::iterator tmpI = I;
			tmpI++;
                        Instruction* insertPoint = tmpI;
                        //llvm::errs() << "insertPoint: " << insertPoint << "\n";
                        CallInst::Create(MyPTCreateConst, Args.begin(), Args.end(), "", insertPoint);
			//I += 1;
                    }
 		   else if(FUNC && FUNC->getName() == "__assert_fail")
                    {
			//llvm::errs() << "assert fail!\n";
			std::vector<Value*> Args(1);
            		Args[0] = ConstantInt::get(IntegerType::get(Context, 32), 0);
			BasicBlock::iterator tmpI = I;
			//tmpI--;
                        Instruction* insertPoint = tmpI;
            		CallInst::Create(MyAssertConst, Args.begin(), Args.end(), "", insertPoint);
			flagAssert = true;
		    }	
		    else if(flagAssert && bbAfterAssert)
		    {
			//llvm::errs() << "assert success!\n";
			std::vector<Value*> Args(1);
            		Args[0] = ConstantInt::get(IntegerType::get(Context, 32), 1);
			BasicBlock::iterator tmpI = I;
			tmpI++;
                        Instruction* insertPoint = tmpI;
            		CallInst::Create(MyAssertConst, Args.begin(), Args.end(), "", insertPoint);
			flagAssert = false;
			bbAfterAssert = false;
		    }
                }
            }
            
            return true;
        }
    };
}

char SymbiosisBBPass::ID = 0;
int SymbiosisBBPass::bbcount = 0;
bool SymbiosisBBPass::flagAssert = false;
bool SymbiosisBBPass::bbAfterAssert = false;
static RegisterPass<SymbiosisBBPass> X("symbiosisBB", "Pass that instruments every basic block entry with a call to a Symbiosis monitor at runtime", false, false);
