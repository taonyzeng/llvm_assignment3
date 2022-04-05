// ECE CS 5544 Assignment 3: LOOPINVARIANTCODEMOTION.cpp
// Group: ZENG TAO
////////////////////// //////////////////////////////////////////////////////////

#include "LoopInvariantCodeMotion.hpp"

namespace llvm{

    void LICM::inspectBlock(Loop* loop, BasicBlock* b, std::vector<Instruction*>& invStmts) {

        // Skip this block if it is part of a subloop (thus, already processed)
        if (loopInfo->getLoopFor(b) != loop) {
            return;
        }

        // Iterate through all the intructions.
        for (BasicBlock::iterator II = b->begin(), IE = b->end(); II != IE; ++II) {
            Instruction* inst = &(*II);
            bool inv = isInvariant(loop, invStmts, inst);
            if (inv) {
                invStmts.push_back(inst);
            }
        }
    }


    bool LICM::isInvariant(Loop* L, std::vector<Instruction*> invStmts, Instruction* inst) {

        // Conditions given in the assignment
        if (!isSafeToSpeculativelyExecute(inst) ||
            inst->mayReadFromMemory() ||
            isa<LandingPadInst>(inst)) {
            return false;
        }

        // All operands must be constants or loop invStmts themselves
        for (User::op_iterator OI = inst->op_begin(), OE = inst->op_end(); OI != OE; ++OI) {
            Value *op = *OI;
            if (Instruction* op_inst = dyn_cast<Instruction>(op)) {
                if (L->contains(op_inst) && std::find(invStmts.begin(), invStmts.end(), op_inst) == invStmts.end()) {   // op_inst in loop, and it is not loop invariant
                    return false;
                }
            }
        }

        // If not returned false till now, then it is loop invariant
        return true;
    }

    void LICM::getAnalysisUsage(AnalysisUsage& AU) const {
        AU.setPreservesAll();
        AU.addRequired<LoopInfoWrapperPass>();
        AU.addRequired<DominatorPass>();
    }

    bool LICM::runOnLoop(Loop *L, LPPassManager &no_use) {

        bool modified = false;

        // Ignore loops without a pre-header
        BasicBlock* preheader = L->getLoopPreheader();
        if (!preheader) {
            return false;
        }

        // Else, get the loop info
        LoopInfo* loopInfo = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();

        DominatorPass &dominator = getAnalysis<DominatorPass>();                
        DomTree* tree = dominator.dom_analysis.dom_tree;

        std::vector<DomTreeNode*> worklist;
        worklist.push_back(tree->root);

        std::vector<Instruction*> invStmts; // Loop-invariant statements

        // Traverse in DFS order, so don't need to do multiple iterations. Use worklist as a stack
        while (!worklist.empty()) {
            DomTreeNode* n = worklist.back();
            BasicBlock* b = n->block;
            worklist.pop_back();

            /*if(n->parent){
                inspectBlock(L, b, invStmts);
            }*/
            // Skip this block if it is part of a subloop (thus, already processed)
            if (loopInfo->getLoopFor(b) != L) {
                continue;
            }

            // Iterate through all the intructions.
            for (BasicBlock::iterator II = b->begin(), IE = b->end(); II != IE; ++II) {
                Instruction* inst = &(*II);
                bool inv = isInvariant(L, invStmts, inst);
                if (inv) {
                    invStmts.push_back(inst);
                }
            }

            for (int i = 0; i < n->children.size(); ++i) {
                worklist.push_back(n->children[i]);
            }
        }

        // Check if instruction can be moved, and do code motion in the order in which invStmts were added (while maintaining dependencies)
        for (int j = 0; j < invStmts.size(); ++j) {
            Instruction* inst = invStmts[j];
            BasicBlock* b = inst->getParent();

            Instruction* end = &(preheader->back());
            inst->moveBefore(end);
            if (!modified) {
                modified = true;
            }
        }

        return modified;
    }

    char LICM::ID = 0;
    RegisterPass<LICM> X("loop-invariant-code-motion", "ECE/CS 5544 Loop invariant code motion");
}
