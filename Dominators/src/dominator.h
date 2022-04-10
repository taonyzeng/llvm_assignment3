#ifndef __DOMINATOR_H
#define __DOMINATOR_H

#include "llvm/ADT/DenseMap.h"
#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/LegacyPassManagers.h"

#include "dominance.h"

namespace llvm_asst3 {

	class DominatorPass : public LoopPass {
		public:
			static char ID;

			llvm_asst3::DominanceAnalysis dom_analysis;

			DominatorPass() : LoopPass(ID) {};

			virtual void getAnalysisUsage(AnalysisUsage& AU) const;

			virtual bool runOnLoop(Loop *L, LPPassManager &no_use);
	};
}


#endif