
#ifndef __LOOPINVARIANT_H
#define __LOOPINVARIANT_H


#include "../../LIBS/dataflow.h"
#include "../../Dominators/src/dominator.h"

#include "llvm/IR/IntrinsicInst.h"
#include "llvm/ADT/PostOrderIterator.h"


namespace llvm{

    class LICM : public LoopPass {
        private:
            LoopInfo* loopInfo;

        public:
            static char ID;

            LICM() : LoopPass(ID) {}

            void inspectBlock(Loop* loop, BasicBlock* b, std::vector<Instruction*>& invStmts);

            bool isInvariant(Loop* L, std::vector<Instruction*> invStmts, Instruction* inst);

            virtual void getAnalysisUsage(AnalysisUsage& AU) const;

            virtual bool runOnLoop(Loop *L, LPPassManager &no_use);

    };

}

#endif