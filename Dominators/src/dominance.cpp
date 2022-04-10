/////////////////////////////////////////
// CS 5544 Assignment 3: Dominator
// Group: ZENG TAO
/////////////////////////////////////////

#include "dominance.h"

using namespace std;

namespace llvm_asst3 {

    // Compute Dominance Relations
    DataFlowResult DominanceAnalysis::computeDom(Loop* L) {

        // Domain = basic blocks. Since our data flow runs on functions, get a block in the loop, 
        // and then get its parent function.
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

    //
    // For a given node, find the immediate dominator in the tree using a reverse
    // breadth first search. If the current node is not the initial node, and the
    // current node is in the initial node's strict dominators list, return the
    // current node.
    //
    BasicBlock* DominanceAnalysis::getIdom(DataFlowResult& domResult, BasicBlock* node) {
      std::queue<BasicBlock*> worklist;
      std::unordered_set<BasicBlock*> visited;
      // Set initial conditions.
      BitVector path = domResult.result[node].out;
      worklist.push(node);
      while (!worklist.empty()) {
        BasicBlock* current = worklist.front();
        worklist.pop();
        // Skip if visited.
        if (visited.count(current)) {
          continue;
        }
        // Did we find the idom?
        int curIndex = domResult.domainToIndex[current];
        if (node != current && path[curIndex]) {
            return current;
        }
        // Mark visited and add all predecessors to the worklist.
        visited.insert(current);
        for (pred_iterator I = pred_begin(current), IE = pred_end(current);
            I != IE; ++I) {
          worklist.push(*I);
        }
      }
      // Return null if there is no idom node (possible for 1st block).
      return NULL;
    }



    std::unordered_map<BasicBlock*, BasicBlock*> DominanceAnalysis::computeIdom(DataFlowResult dominanceResult) {
        std::unordered_map<BasicBlock*, BasicBlock*> idom_map;
        std::vector<BasicBlock*> domain(dominanceResult.domainToIndex.size());  // Reconstruct index -> BasicBlock* mapping from domainToIndex
        for (auto kv : dominanceResult.domainToIndex) {
            domain[kv.second] = (BasicBlock*)kv.first;
        }

        for (int indB = 0; indB < domain.size(); ++indB) {
            BasicBlock* idom = getIdom(dominanceResult, domain[indB]);
            idom_map[domain[indB]] = idom;
        }

        return idom_map;
    }

    void DominanceAnalysis::printIdom(std::unordered_map<BasicBlock*, BasicBlock*> idom, Loop* L) {
        std::vector<BasicBlock*> BBs = L->getBlocks();
        errs() << "\nLoop<depth = " << L->getLoopDepth() << ">\n";
        //errs() << "\n\nNew Loop\n";
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
