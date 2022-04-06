// CS 5544 Assignment 3: DeadCodeElimination
// Group: ZENG TAO
////////////////////// //////////////////////////////////////////////////////////

#ifndef __LOOPINVARIANT_H
#define __LOOPINVARIANT_H

#include "../../LIBS/dataflow.h"

#include "llvm/IR/IntrinsicInst.h"
#include "llvm/ADT/PostOrderIterator.h"

namespace llvm{

    // DCE Analysis class
    class DCEAnalysis : public DataFlow {
        public:

            DCEAnalysis() : DataFlow(Direction::INVALID_DIRECTION, MeetOp::INVALID_MEETOP) {}
            DCEAnalysis(Direction direction, MeetOp meet_op) : DataFlow(direction, meet_op) {}

        protected:
            TransferOutput transferFn(BitVector input,  
                                      std::vector<void*> domain, 
                                      std::unordered_map<void*, int> domainToIndex, 
                                      BasicBlock* block);
        private:
            BitVector initUseSet(BasicBlock* block, 
                                 BitVector& input, 
                                 std::unordered_map<void*, int>& domainToIndex);

            void processPhiNode(PHINode* phi_insn, 
                                TransferOutput& transferOutput, 
                                std::unordered_map<void*, int>& domainToIndex);

            void updateUseSet(BitVector& useSet, 
                                Instruction* insn, 
                                std::unordered_map<void*, int>& domainToIndex);
    };


    class DCE : public FunctionPass {
        public:
            static char ID;

            DCE();

            virtual void getAnalysisUsage(AnalysisUsage& AU) const;
            virtual bool runOnFunction(Function &F);
        private:
            // The pass
            DCEAnalysis pass;
            // Setup the pass
            std::vector<void*> domain;
            
            std::vector<Instruction*> computeDeleteSet(BasicBlock* BI, BitVector& SLV, DataFlowResult& output);

            void initDomain(Function &F);
    };
}

#endif
