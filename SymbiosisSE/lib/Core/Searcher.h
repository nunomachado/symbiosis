//===-- Searcher.h ----------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_SEARCHER_H
#define KLEE_SEARCHER_H

#include <vector>
#include <set>
#include <map>
#include <queue>

// Nuno: added function pass
#include "llvm/Pass.h"

// FIXME: Move out of header, use llvm streams.
#include <ostream>

namespace llvm {
  class BasicBlock;
    class Function;
    class Instruction;
    //Nuno: add bug redux
    class PostDominanceFrontier;
    class PostDominatorTree;
}

namespace klee {
    template<class T> class DiscretePDF;
    class ExecutionState;
    class Executor;
    //Nuno: add bug redux
    class KInstruction;
    struct KFunction;
    
    
  class Searcher {
  public:
    virtual ~Searcher();

    virtual ExecutionState &selectState() = 0;

    virtual void update(ExecutionState *current,
                        const std::set<ExecutionState*> &addedStates,
                        const std::set<ExecutionState*> &removedStates) = 0;

    virtual bool empty() = 0;

    // prints name of searcher as a klee_message()
    // TODO: could probably make prettier or more flexible
    virtual void printName(std::ostream &os) { 
      os << "<unnamed searcher>\n";
    }

    // pgbovine - to be called when a searcher gets activated and
    // deactivated, say, by a higher-level searcher; most searchers
    // don't need this functionality, so don't have to override.
    virtual void activate() {}
    virtual void deactivate() {}

    // utility functions

    void addState(ExecutionState *es, ExecutionState *current = 0) {
      std::set<ExecutionState*> tmp;
      tmp.insert(es);
      update(current, tmp, std::set<ExecutionState*>());
    }

    void removeState(ExecutionState *es, ExecutionState *current = 0) {
      std::set<ExecutionState*> tmp;
      tmp.insert(es);
      update(current, std::set<ExecutionState*>(), tmp);
    }
    
  //Nuno: added for bugredux
    enum CoreSearchType {
        DFS,
        RandomState,
        RandomPath,
        NURS_CovNew,
        NURS_MD2U,
        NURS_Depth,
        NURS_ICnt,
        NURS_CPICnt,
        NURS_QC
    };
};

  class DFSSearcher : public Searcher {
    std::vector<ExecutionState*> states;

  public:
    ExecutionState &selectState();
    void update(ExecutionState *current,
                const std::set<ExecutionState*> &addedStates,
                const std::set<ExecutionState*> &removedStates);
    bool empty() { return states.empty(); }
    void printName(std::ostream &os) {
      os << "DFSSearcher\n";
    }
  };
    
    
    //Nuno: added for bugredux {  ###########################
    
    class GeneralReplaySearcher : public Searcher {
    protected:
        Executor &executor;
        std::vector<ExecutionState*> states;
        
        std::vector<std::string> callSeq;
        int eventPtr;
        bool getToUserMain;
        
        std::string sourceFile;
        std::map<llvm::Function*,int> funcShortMap;
        std::vector<KInstruction*> targetInstList;
        std::map<llvm::Instruction*, int> curDistanceMap; //distances between functions
        
        std::map<llvm::Instruction*, int> curInsideFuncDisMap;
        std::vector<KFunction*> functions;
        
        //std::map<llvm::Function*, std::vector<llvm::Instruction*>* > cachedCallSite; //Nuno: commented this
        
        void CleanCachedCallSites();
        
        std::set<llvm::BasicBlock*> prunedBBSet;
        std::set<llvm::Function*> unvisitedFunc;
        
        int lastChoiceNumber;//remember last choice so that do not need to pick state
        ExecutionState *lastChoice;
        
        //symbiosis
        std::set<llvm::Function*> visitedFuncs; //** Nuno: set used to mark the functions containing the events of a given aviso trace
        std::map<llvm::BasicBlock*, int> bbCache; //** Nuno: map: BB -> distance to next event ; cache used to avoid re-computing event reachability for every instruction of a given BB
        
        
    public:
        bool getTarget;
        GeneralReplaySearcher(Executor &executor);
	    ~GeneralReplaySearcher();
        
	    void generateFuncShort();
	    void generateNewShortDistance();
	    void findNextTarget();
	    KInstruction* findInstFromSourceLine(std::string sourceline);
        
        
	    ExecutionState &selectState();
	    void update(ExecutionState *current,
	                const std::set<ExecutionState*> &addedStates,
	                const std::set<ExecutionState*> &removedStates);
	    bool empty() { return states.empty(); }
	    void printName(std::ostream &os) {
            os << "GeneralReplaySearcher\n";
	    }
    };
    
    //Nuno: replay call sequences in symbolic execution
    class AvisoReplaySearcher : public GeneralReplaySearcher {
        
    public:
        AvisoReplaySearcher(Executor &executor);
	    ~AvisoReplaySearcher();
        
        void generateNewShortDistance();
        void findNextTarget();
        void expandTraceFile(); //** Nuno: complete the aviso trace with events referring to the function call seq
        
        ExecutionState &selectState();
        
        bool empty() { return states.empty(); }
	    void printName(std::ostream &os) {
            os << "AvisoReplaySearcher\n";
	    }
        
    };
    
    
    //Nuno: replay crash stack in a shortest distance in ICFG.
    class ShortestPathSearcher : public Searcher {
        Executor &executor;
        std::vector<ExecutionState*> states;
        std::vector<std::string> callStack;
        std::set<llvm::BasicBlock*> prunedBBSet;
        unsigned callStackPtr;
        unsigned targetPtr;
        bool getTarget;
        
        int lastChoiceNumber;//remember last choice so that do not need to pick state
        ExecutionState *lastChoice;
        std::map<llvm::Function*,int> funcShortMap;
        std::vector<llvm::Instruction*> targetInstList;
        std::vector<llvm::Instruction*> stackcallList;
        std::map<llvm::Instruction*, int> curDistanceMap;
        
        std::map<llvm::Instruction*, int> curInsideFuncDisMap;
        std::vector<KFunction*> functions;
        
        std::set<llvm::Instruction*> visitedInstSet;
        
        
        //temp added for midway change target to accelerate search
        std::map<llvm::Instruction*, int> changeTargetDisMap;
        std::map<ExecutionState*, int> changeTargetReachFlag;
        llvm::Instruction* changeTargetInst;
        std::string changeTargetSource;
        
    public:
        ShortestPathSearcher(Executor &executor);
	    ~ShortestPathSearcher();
        
	    int getDistanceForBB(llvm::BasicBlock* bb, llvm::Instruction*);
	    void generateNewShortDistance();
	    void generateFuncMap();
	    void generateChangeDis();
	    void findNextTarget();
	    llvm::Instruction* findInstFromSourceLine(std::string sourceline);
        
        
        ExecutionState &selectState();
	    void update(ExecutionState *current,
	                const std::set<ExecutionState*> &addedStates,
	                const std::set<ExecutionState*> &removedStates);
	    bool empty() { return states.empty(); }
	    void printName(std::ostream &os) {
            os << "ShortestSearcher\n";
	    }
        
    };
    
    //Nuno: a class used to access dominator information
    class DomInterfacePass : public llvm::FunctionPass {
        static char ID;
        
	public:
        std::vector<llvm::Instruction*> outputList;
        std::vector<llvm::Instruction*>* inList;
        std::set<llvm::Function*>* unvisitedF;
        std::vector<llvm::BasicBlock*> outputPrunedList;
        
        DomInterfacePass(std::vector<llvm::Instruction*>* inputList, std::set<llvm::Function*>* unvisitedList);
	    virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const;
        
        virtual bool runOnFunction(llvm::Function &f);
    };
    
    //Nuno: added for bugredux } #########################

  class RandomSearcher : public Searcher {
    std::vector<ExecutionState*> states;

  public:
    ExecutionState &selectState();
    void update(ExecutionState *current,
                const std::set<ExecutionState*> &addedStates,
                const std::set<ExecutionState*> &removedStates);
    bool empty() { return states.empty(); }
    void printName(std::ostream &os) {
      os << "RandomSearcher\n";
    }
  };

  class WeightedRandomSearcher : public Searcher {
  public:
    enum WeightType {
      Depth,
      QueryCost,
      InstCount,
      CPInstCount,
      MinDistToUncovered,
      CoveringNew
    };

  private:
    Executor &executor;
    DiscretePDF<ExecutionState*> *states;
    WeightType type;
    bool updateWeights;
    
    double getWeight(ExecutionState*);

  public:
    WeightedRandomSearcher(Executor &executor, WeightType type);
    ~WeightedRandomSearcher();

    ExecutionState &selectState();
    void update(ExecutionState *current,
                const std::set<ExecutionState*> &addedStates,
                const std::set<ExecutionState*> &removedStates);
    bool empty();
    void printName(std::ostream &os) {
      os << "WeightedRandomSearcher::";
      switch(type) {
      case Depth              : os << "Depth\n"; return;
      case QueryCost          : os << "QueryCost\n"; return;
      case InstCount          : os << "InstCount\n"; return;
      case CPInstCount        : os << "CPInstCount\n"; return;
      case MinDistToUncovered : os << "MinDistToUncovered\n"; return;
      case CoveringNew        : os << "CoveringNew\n"; return;
      default                 : os << "<unknown type>\n"; return;
      }
    }
  };

  class RandomPathSearcher : public Searcher {
    Executor &executor;

  public:
    RandomPathSearcher(Executor &_executor);
    ~RandomPathSearcher();

    ExecutionState &selectState();
    void update(ExecutionState *current,
                const std::set<ExecutionState*> &addedStates,
                const std::set<ExecutionState*> &removedStates);
    bool empty();
    void printName(std::ostream &os) {
      os << "RandomPathSearcher\n";
    }
  };

  class MergingSearcher : public Searcher {
    Executor &executor;
    std::set<ExecutionState*> statesAtMerge;
    Searcher *baseSearcher;
    llvm::Function *mergeFunction;

  private:
    llvm::Instruction *getMergePoint(ExecutionState &es);

  public:
    MergingSearcher(Executor &executor, Searcher *baseSearcher);
    ~MergingSearcher();

    ExecutionState &selectState();
    void update(ExecutionState *current,
                const std::set<ExecutionState*> &addedStates,
                const std::set<ExecutionState*> &removedStates);
    bool empty() { return baseSearcher->empty() && statesAtMerge.empty(); }
    void printName(std::ostream &os) {
      os << "MergingSearcher\n";
    }
  };

  class BumpMergingSearcher : public Searcher {
    Executor &executor;
    std::map<llvm::Instruction*, ExecutionState*> statesAtMerge;
    Searcher *baseSearcher;
    llvm::Function *mergeFunction;

  private:
    llvm::Instruction *getMergePoint(ExecutionState &es);

  public:
    BumpMergingSearcher(Executor &executor, Searcher *baseSearcher);
    ~BumpMergingSearcher();

    ExecutionState &selectState();
    void update(ExecutionState *current,
                const std::set<ExecutionState*> &addedStates,
                const std::set<ExecutionState*> &removedStates);
    bool empty() { return baseSearcher->empty() && statesAtMerge.empty(); }
    void printName(std::ostream &os) {
      os << "BumpMergingSearcher\n";
    }
  };

  class BatchingSearcher : public Searcher {
    Searcher *baseSearcher;
    double timeBudget;
    unsigned instructionBudget;

    ExecutionState *lastState;
    double lastStartTime;
    unsigned lastStartInstructions;

  public:
    BatchingSearcher(Searcher *baseSearcher, 
                     double _timeBudget,
                     unsigned _instructionBudget);
    ~BatchingSearcher();

    ExecutionState &selectState();
    void update(ExecutionState *current,
                const std::set<ExecutionState*> &addedStates,
                const std::set<ExecutionState*> &removedStates);
    bool empty() { return baseSearcher->empty(); }
    void printName(std::ostream &os) {
      os << "<BatchingSearcher> timeBudget: " << timeBudget
         << ", instructionBudget: " << instructionBudget
         << ", baseSearcher:\n";
      baseSearcher->printName(os);
      os << "</BatchingSearcher>\n";
    }
  };

  class IterativeDeepeningTimeSearcher : public Searcher {
    Searcher *baseSearcher;
    double time, startTime;
    std::set<ExecutionState*> pausedStates;

  public:
    IterativeDeepeningTimeSearcher(Searcher *baseSearcher);
    ~IterativeDeepeningTimeSearcher();

    ExecutionState &selectState();
    void update(ExecutionState *current,
                const std::set<ExecutionState*> &addedStates,
                const std::set<ExecutionState*> &removedStates);
    bool empty() { return baseSearcher->empty() && pausedStates.empty(); }
    void printName(std::ostream &os) {
      os << "IterativeDeepeningTimeSearcher\n";
    }
  };

  class InterleavedSearcher : public Searcher {
    typedef std::vector<Searcher*> searchers_ty;

    searchers_ty searchers;
    unsigned index;

  public:
    explicit InterleavedSearcher(const searchers_ty &_searchers);
    ~InterleavedSearcher();

    ExecutionState &selectState();
    void update(ExecutionState *current,
                const std::set<ExecutionState*> &addedStates,
                const std::set<ExecutionState*> &removedStates);
    bool empty() { return searchers[0]->empty(); }
    void printName(std::ostream &os) {
      os << "<InterleavedSearcher> containing "
         << searchers.size() << " searchers:\n";
      for (searchers_ty::iterator it = searchers.begin(), ie = searchers.end();
           it != ie; ++it)
        (*it)->printName(os);
      os << "</InterleavedSearcher>\n";
    }
  };

}

#endif
