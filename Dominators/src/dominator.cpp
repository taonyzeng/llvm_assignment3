

#include "dominator.h"

namespace llvm_asst3 {


	void DominatorPass::getAnalysisUsage(AnalysisUsage& AU) const {
		AU.setPreservesAll();
		AU.addRequired<LoopInfoWrapperPass>();
	}

	bool DominatorPass::runOnLoop(Loop *L, LPPassManager &no_use) {

		// Ignore loops without a pre-header
		BasicBlock* preheader = L->getLoopPreheader();
		if (!preheader) {
			return false;
		}

		// Else, get the loop info
		LoopInfo &LI = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();

		//run the dominator pass.
		DataFlowResult dominanceResult = dom_analysis.computeDom(L);
		
		//compute the immediate dominance for each of the basic block inside the loop. 
		std::unordered_map<BasicBlock*,BasicBlock*> idom = dom_analysis.computeIdom(dominanceResult);

		//print the immediate dominance information.
		dom_analysis.printIdom(idom,L);

		dom_analysis.computeDominanceTree(idom, L);
		return false;
	}

	//
	// Register Pass
	//
	char DominatorPass::ID = 0;
	RegisterPass<DominatorPass> Y("dominators", "ECE/CS 5544 dominator");
}