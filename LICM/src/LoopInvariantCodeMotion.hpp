
#ifndef __LOOPINVARIANT_H
#define __LOOPINVARIANT_H


#include "../../LIBS/dataflow.h"
#include "../../Dominators/src/dominator.h"

#include "llvm/IR/IntrinsicInst.h"
#include "llvm/ADT/PostOrderIterator.h"
#include "llvm/ADT/SetOperations.h"
#include "llvm/Analysis/MemorySSA.h"
#include "llvm/Analysis/MemorySSAUpdater.h"
#include "llvm/IR/Dominators.h"
#include "llvm/Support/Debug.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Analysis/LoopIterator.h"

using namespace llvm;

#define DEBUG_TYPE  "licm"

namespace llvm_asst3{

    class LICM : public LoopPass {
        private:
            LoopInfo* LI;

            llvm::DominatorTree *DT;
            Loop *CurLoop;

            //A map of blocks in the loop to the block their instructions will be hoisted to.
            DenseMap<BasicBlock *, BasicBlock *> HoistDestinationMap;

            DenseMap<BranchInst *, BasicBlock *> HoistableBranches;

            int NumCreatedBlocks;
            int NumClonedBranches;

        public:
            static char ID;

            LICM() : LoopPass(ID) {}

            //check if the variables in the instruction is invariant.
            bool isInvariant(Loop* L, std::vector<Instruction*> invStmts, Instruction* inst);

            //move an instruction to its destined hosting basic block, which retrived by the getOrCreateHoistedBlock.
            bool hoistRegion(LoopInfo *LI, DominatorTree *DT,Loop *CurLoop);

            virtual void getAnalysisUsage(AnalysisUsage& AU) const;

            virtual bool runOnLoop(Loop *L, LPPassManager &no_use);

            // Check if this block is conditional based on a pending branch,
            //If not involved in a pending branch,  we simply hoist to preheader.
            //Or Else, Create new hoisted versions of blocks, which is not contained in the HoistDestinationMap.
            //and then link those new blocks with the branches. 
            //Finally, place the branch instruction at the end of the HoistTarget block.
            BasicBlock *getOrCreateHoistedBlock(BasicBlock *BB);

            //keep track of the branch instructions which are possibly valid for hoisting.
            void registerPossiblyHoistableBranch(BranchInst *BI);

    };

}

#endif