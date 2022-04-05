/////////////////////////////////////////
// CS 5544 Assignment 3: Dominator
// Group: ZENG TAO
/////////////////////////////////////////

#include <set>
#include <queue>
#include <vector>

#include "llvm/ADT/DenseMap.h"
#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Analysis/LoopInfo.h"

#include "dominance.h"

using namespace std;

namespace llvm {

    // Compute Dominance Relations
    DataFlowResult DominanceAnalysis::computeDom(Loop* L) {

        // Domain = basic blocks. Since our data flow runs on functions, get a block in the loop, and then get its parent function.
        // Need to convert BBs to void* for our generic implementation
        Function* F = L->getBlocks().front()->getParent();
        Function::BasicBlockListType &FuncBBs = F->getBasicBlockList();
        std::vector<void*> domain;
        for (Function::BasicBlockListType::iterator BI = FuncBBs.begin(), BE = FuncBBs.end(); BI != BE; ++BI) {
            domain.push_back((void*)(&(*BI)));
        }

        int numElems = domain.size();

        // IN of first block is emptyset
        BitVector boundaryCond(numElems, false);

        // Initial condition = Universal set (because meet is intersection)
        BitVector initCond(numElems, true);

        // Run dominance analysis
        return this->run(*F, domain, boundaryCond, initCond);
    }

    // Function for checking if block X dominates block Y
    bool DominanceAnalysis::dominates(BasicBlock* X, BasicBlock* Y, DataFlowResult dom) {
        return dom.result[Y].in[dom.domainToIndex[X]];
    }

    std::unordered_map<BasicBlock*, BasicBlock*> DominanceAnalysis::computeIdom(DataFlowResult dominanceResult) {
        std::unordered_map<BasicBlock*, BasicBlock*> idom;
        std::vector<BasicBlock*> domain(dominanceResult.domainToIndex.size());  // Reconstruct index -> BasicBlock* mapping from domainToIndex
        for (std::unordered_map<void*,int>::iterator it = dominanceResult.domainToIndex.begin(); it != dominanceResult.domainToIndex.end(); ++it ) {
            domain[it->second] = (BasicBlock*)it->first;
        }

        // A IDOM B = A DOM B && (~exists C s.t. A DOM C && C DOM B)
        for (int indB = 0; indB < domain.size(); ++indB) {
            BasicBlock* B = domain[indB];

            for (int indA=0; indA < domain.size(); ++indA) {
                if (indA == indB)
                    continue;
                BasicBlock* A = domain[indA];
                if (!dominates(A,B,dominanceResult))
                    continue;
                bool isIdom = true;

                // Now, check if there exists a C != A such that A dom C & C dom B
                for (int indC=0; indC < domain.size(); ++indC) {
                    if (indC == indA || indC == indB)
                        continue;

                    BasicBlock* C = domain[indC];
                    if (dominates(A,C,dominanceResult) && dominates(C,B,dominanceResult)) {
                        isIdom = false;
                        break;
                    }
                }

                if (isIdom) {
                    if(idom.find(B) != idom.end()) {
                        errs() << "IDOM ALREADY PRESENT!! " << B->getName() << " idom " << idom.find(B)->second->getName() << "\n";
                        errs() << "NOW ADDING!! " << B->getName() << " idom " << A->getName() << "\n";
                        assert(0);
                    }
                    idom[B] = A;
                    // break;
                }
            }
        }

        return idom;
    }

    void DominanceAnalysis::printIdom(std::unordered_map<BasicBlock*, BasicBlock*> idom, Loop* L) {
        std::vector<BasicBlock*> BBs = L->getBlocks();
        errs() << "\n\nNew Loop\n";
        for (int i=0; i < BBs.size(); ++i) {
            errs() << BBs[i]->getName() << " idom " << idom[BBs[i]]->getName() << "\n";
        }
        return;
    }
    
    void DominanceAnalysis::computeDominanceTree(std::unordered_map<BasicBlock*, BasicBlock*> idom, Loop* L) {
    	DomTree* tree = new DomTree();
 		std::vector<BasicBlock*> blocks = L->getBlocks();
 		std::unordered_map<BasicBlock*,DomTreeNode*> lookup;
 		for(std::vector<BasicBlock*>::iterator BI = blocks.begin(), BE = blocks.end(); BI != BE; ++BI) {
 			BasicBlock* b = *BI;
 			BasicBlock* p;
 			if (BI==blocks.begin())	{ // Header
 				p = NULL;
 			}
 			else {
 				p = idom[b];
 			}
 			
 			// Create a node for this if it doesn't exist already.
 			DomTreeNode* n;
 			if (lookup.find(b) != lookup.end()) {
 				n = lookup[b];
 			}
 			else {
 				n = new DomTreeNode(b,NULL);
	 			lookup[b] = n;
 			}

 			// Same for the parent
 			DomTreeNode* pn;
 			if (lookup.find(p) != lookup.end()) {
 				pn = lookup[p];
 			}
 			else {
 				pn = new DomTreeNode(p,NULL);
	 			lookup[p] = pn;
 			}

			n->parent = pn;				// Add parent to this node
			pn->children.push_back(n);	// Add this node to the children of the parent
 			tree->nodes.push_back(n);
 			
 		}
 		
 		tree->root = lookup[L->getBlocks().front()];
 		this->dom_tree = tree;
    }

}
