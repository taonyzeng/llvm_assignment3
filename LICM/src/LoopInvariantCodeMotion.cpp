// 15-745 S15 Assignment 3: DeadCodeElimination.cpp
// Group: jarulraj, nkshah
////////////////////// //////////////////////////////////////////////////////////


#include "../../dataflow.h"
#include "../../Dominators/src/dominator.h"

#include "llvm/IR/IntrinsicInst.h"
#include "llvm/ADT/PostOrderIterator.h"

using namespace llvm;

namespace {

    class LICM : public LoopPass {

        public:
            static char ID;

            LICM() : LoopPass(ID) {

            }

            bool isInvariant(Loop* L, std::vector<Instruction*> invStmts, Instruction* inst) {

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

            virtual void getAnalysisUsage(AnalysisUsage& AU) const {
                AU.setPreservesAll();
                AU.addRequired<LoopInfoWrapperPass>();
                AU.addRequired<DominatorPass>();
            }

            bool runOnLoop(Loop *L, LPPassManager &no_use) {

                bool modified = false;


                // Ignore loops without a pre-header
                BasicBlock* preheader = L->getLoopPreheader();
                if (!preheader) {
                    return false;
                }

                // Else, get the loop info
                LoopInfo &LI = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();


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

                    // Skip this block if it is part of a subloop (thus, already processed)
                    if (LI.getLoopFor(b) != L) {
                        return false;
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

                // Conditions for hoisting out of the loop
                // In SSA, everything is assigned only once, and must be done before all its uses. So only need to check if all loop exits are dominated.

                /* No more checking of dominating all loop exits
                // Find all loop exits
                std::vector<BasicBlock*> exitblocks;
                std::vector<BasicBlock*> blocks = L->getBlocks();
                for(int i=0; i<blocks.size(); ++i) {
                BasicBlock* BB = blocks[i];
                for (succ_iterator SI = succ_begin(BB), SE = succ_end(BB); SI != SE; ++SI) {
                if (!L->contains(*SI)) {
                exitblocks.push_back(BB);
                break;
                }
                }
                }
                */

                // Check if instruction can be moved, and do code motion in the order in which invStmts were added (while maintaining dependencies)
                for (int j = 0; j < invStmts.size(); ++j) {
                    Instruction* inst = invStmts[j];
                    BasicBlock* b = inst->getParent();

                    bool all_dominate = true;
                    /* No more checking of dominating all loop exits
                    // Check if it dominates all loop exits
                    for(int i=0; i<exitblocks.size(); ++i) {
                    if (!dominates(b,exitblocks[i],dominanceResult)) {
                    all_dominate = false;
                    break;
                    }
                    }
                    */

                    if (all_dominate) {
                        Instruction* end = &(preheader->back());
                        inst->moveBefore(end);
                        if (!modified) {
                            modified = true;
                        }
                    }
                }

                return modified;
            }

    };

    char LICM::ID = 0;
    RegisterPass<LICM> X("loop-invariant-code-motion", "ECE/CS 5544 Loop invariant code motion");
}
