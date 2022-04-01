/////////////////////////////////////////
// 15-745 S15 Assignment 3:
// Group: nkshah, jarulraj
/////////////////////////////////////////

#ifndef __DOMINANCE_H
#define __DOMINANCE_H

#include <set>
#include <queue>
#include <vector>
#include <unordered_map>

#include "llvm/ADT/DenseMap.h"
#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Analysis/LoopInfo.h"

#include "../../dataflow.h"

using namespace std;

namespace llvm {

	// Dominance Tree Node	
	class DomTreeNode {
		public:
			std::vector<DomTreeNode*> children;
			DomTreeNode* parent;
			BasicBlock* block;
			DomTreeNode(BasicBlock* b, DomTreeNode* p) { block = b; parent = p; }
	};

	// Dominance Tree
	class DomTree {
		public:
			std::vector<DomTreeNode*> nodes;
			DomTreeNode* root;
			DomTree() {}
	};


	// Dominance Analysis
	class DominanceAnalysis : public DataFlow {

		public:
			DomTree* dom_tree;

			DominanceAnalysis() : DataFlow(Direction::FORWARD, MeetOp::INTERSECTION) {	}

			// Compute Dominance Relations
			DataFlowResult computeDom(Loop* L);

			// Function for checking if block X dominates block Y
			bool dominates(BasicBlock* X, BasicBlock* Y, DataFlowResult dom);

			// Compute Immediate Dominance Relationship
			std::unordered_map<BasicBlock*, BasicBlock*> computeIdom(DataFlowResult dominanceResult);

			// Print Immediate Dominance Relationship
			void printIdom(std::unordered_map<BasicBlock*, BasicBlock*> idom, Loop* L);

			// Compute Dominance Tree
			void computeDominanceTree(std::unordered_map<BasicBlock*, BasicBlock*> idom, Loop* L);


		protected:

			// Transfer function is simple: GEN = {Self}, KILL = emptyset
			TransferOutput transferFn(BitVector input, std::vector<void*> domain, std::unordered_map<void*, int> domainToIndex, BasicBlock* block) {
			  TransferOutput result;
			  result.element = input;	// Get the IN set
			  result.element.set(domainToIndex[block]);	// Add self
			  return result;
			}
	};


}

#endif
