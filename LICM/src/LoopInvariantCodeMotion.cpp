// ECE CS 5544 Assignment 3: LOOPINVARIANTCODEMOTION.cpp
// Group: ZENG TAO
////////////////////// //////////////////////////////////////////////////////////

#include "LoopInvariantCodeMotion.hpp"

namespace llvm_asst3{

    /// When an instruction is found to only use loop invariant operands that
    /// is safe to hoist, this instruction is called to do the dirty work.
    ///
    static void hoist(Instruction &I, const DominatorTree *DT, const Loop *CurLoop, BasicBlock *Dest) {
           LLVM_DEBUG(dbgs() << "LICM hoisting to " << Dest->getNameOrAsOperand() << ": "
                             << I << "\n");
          
            if (isa<PHINode>(I))
                // Move the new node to the end of the phi list in the destination block.
                I.moveBefore(Dest->getFirstNonPHI());
            else
                // Move the new node to the destination block, before its terminator.
                I.moveBefore(Dest->getTerminator());
          
            I.updateLocationAfterHoist();
    }


    void LICM::registerPossiblyHoistableBranch(BranchInst *BI) {
         // We can only hoist conditional branches with loop invariant operands.
         if (!BI->isConditional() || !CurLoop->hasLoopInvariantOperands(BI))
           return;
      
         // The branch destinations need to be in the loop, and we don't gain
         // anything by duplicating conditional branches with duplicate successors,
         // as it's essentially the same as an unconditional branch.
         BasicBlock *TrueDest = BI->getSuccessor(0);
         BasicBlock *FalseDest = BI->getSuccessor(1);
         if (!CurLoop->contains(TrueDest) || !CurLoop->contains(FalseDest) ||
             TrueDest == FalseDest)
           return;
      
         // We can hoist BI if one branch destination is the successor of the other,
         // or both have common successor which we check by seeing if the
         // intersection of their successors is non-empty.
         // TODO: This could be expanded to allowing branches where both ends
         // eventually converge to a single block.
         SmallPtrSet<BasicBlock *, 4> TrueDestSucc, FalseDestSucc;
         TrueDestSucc.insert(succ_begin(TrueDest), succ_end(TrueDest));
         FalseDestSucc.insert(succ_begin(FalseDest), succ_end(FalseDest));
         BasicBlock *CommonSucc = nullptr;
         if (TrueDestSucc.count(FalseDest)) {
           CommonSucc = FalseDest;
         } else if (FalseDestSucc.count(TrueDest)) {
           CommonSucc = TrueDest;
         } else {
           set_intersect(TrueDestSucc, FalseDestSucc);
           // If there's one common successor use that.
           if (TrueDestSucc.size() == 1)
             CommonSucc = *TrueDestSucc.begin();
           // If there's more than one pick whichever appears first in the block list
           // (we can't use the value returned by TrueDestSucc.begin() as it's
           // unpredicatable which element gets returned).
           else if (!TrueDestSucc.empty()) {
             Function *F = TrueDest->getParent();
             auto IsSucc = [&](BasicBlock &BB) { return TrueDestSucc.count(&BB); };
             auto It = llvm::find_if(*F, IsSucc);
             assert(It != F->end() && "Could not find successor in function");
             CommonSucc = &*It;
           }
         }
         // The common successor has to be dominated by the branch, as otherwise
         // there will be some other path to the successor that will not be
         // controlled by this branch so any phi we hoist would be controlled by the
         // wrong condition. This also takes care of avoiding hoisting of loop back
         // edges.
         // TODO: In some cases this could be relaxed if the successor is dominated
         // by another block that's been hoisted and we can guarantee that the
         // control flow has been replicated exactly.
         if (CommonSucc && DT->dominates(BI, CommonSucc))
           HoistableBranches[BI] = CommonSucc;
    }

    BasicBlock* LICM::getOrCreateHoistedBlock(BasicBlock *BB) {
         // If BB has already been hoisted, return that
        if (HoistDestinationMap.count(BB))
            return HoistDestinationMap[BB];
      
        // Check if this block is conditional based on a pending branch
        auto HasBBAsSuccessor =
             [&](DenseMap<BranchInst *, BasicBlock *>::value_type &Pair) {
               return BB != Pair.second && (Pair.first->getSuccessor(0) == BB ||
                                            Pair.first->getSuccessor(1) == BB);
             };
         auto It = llvm::find_if(HoistableBranches, HasBBAsSuccessor);
      
         // If not involved in a pending branch, hoist to preheader
         BasicBlock *InitialPreheader = CurLoop->getLoopPreheader();
         if (It == HoistableBranches.end()) {
           LLVM_DEBUG(dbgs() << "LICM using "
                             << InitialPreheader->getNameOrAsOperand()
                             << " as hoist destination for "
                             << BB->getNameOrAsOperand() << "\n");
           HoistDestinationMap[BB] = InitialPreheader;
           return InitialPreheader;
         }
         BranchInst *BI = It->first;
         assert(std::find_if(++It, HoistableBranches.end(), HasBBAsSuccessor) ==
                    HoistableBranches.end() &&
                "BB is expected to be the target of at most one branch");
      
         LLVMContext &C = BB->getContext();
         BasicBlock *TrueDest = BI->getSuccessor(0);
         BasicBlock *FalseDest = BI->getSuccessor(1);
         BasicBlock *CommonSucc = HoistableBranches[BI];
         BasicBlock *HoistTarget = getOrCreateHoistedBlock(BI->getParent());
      
         // Create hoisted versions of blocks that currently don't have them
         auto CreateHoistedBlock = [&](BasicBlock *Orig) {
           if (HoistDestinationMap.count(Orig))
             return HoistDestinationMap[Orig];
           BasicBlock *New =
               BasicBlock::Create(C, Orig->getName() + ".licm", Orig->getParent());
           HoistDestinationMap[Orig] = New;
           DT->addNewBlock(New, HoistTarget);
           if (CurLoop->getParentLoop())
             CurLoop->getParentLoop()->addBasicBlockToLoop(New, *LI);
           ++NumCreatedBlocks;
           LLVM_DEBUG(dbgs() << "LICM created " << New->getName()
                             << " as hoist destination for " << Orig->getName()
                             << "\n");
           return New;
         };
         BasicBlock *HoistTrueDest = CreateHoistedBlock(TrueDest);
         BasicBlock *HoistFalseDest = CreateHoistedBlock(FalseDest);
         BasicBlock *HoistCommonSucc = CreateHoistedBlock(CommonSucc);
      
         // Link up these blocks with branches.
         if (!HoistCommonSucc->getTerminator()) {
           // The new common successor we've generated will branch to whatever that
           // hoist target branched to.
           BasicBlock *TargetSucc = HoistTarget->getSingleSuccessor();
           assert(TargetSucc && "Expected hoist target to have a single successor");
           HoistCommonSucc->moveBefore(TargetSucc);
           BranchInst::Create(TargetSucc, HoistCommonSucc);
         }
         if (!HoistTrueDest->getTerminator()) {
           HoistTrueDest->moveBefore(HoistCommonSucc);
           BranchInst::Create(HoistCommonSucc, HoistTrueDest);
         }
         if (!HoistFalseDest->getTerminator()) {
           HoistFalseDest->moveBefore(HoistCommonSucc);
           BranchInst::Create(HoistCommonSucc, HoistFalseDest);
         }
      
         // If BI is being cloned to what was originally the preheader then
         // HoistCommonSucc will now be the new preheader.
         if (HoistTarget == InitialPreheader) {
           // Phis in the loop header now need to use the new preheader.
           InitialPreheader->replaceSuccessorsPhiUsesWith(HoistCommonSucc);

           // The new preheader dominates the loop header.
           llvm::DomTreeNode *PreheaderNode = DT->getNode(HoistCommonSucc);
           llvm::DomTreeNode *HeaderNode = DT->getNode(CurLoop->getHeader());
           DT->changeImmediateDominator(HeaderNode, PreheaderNode);
           // The preheader hoist destination is now the new preheader, with the
           // exception of the hoist destination of this branch.
           for (auto &Pair : HoistDestinationMap)
             if (Pair.second == InitialPreheader && Pair.first != BI->getParent())
               Pair.second = HoistCommonSucc;
         }
      
         // Now finally clone BI.
         llvm::ReplaceInstWithInst(
             HoistTarget->getTerminator(),
             BranchInst::Create(HoistTrueDest, HoistFalseDest, BI->getCondition()));
         ++NumClonedBranches;
      
         assert(CurLoop->getLoopPreheader() &&
                "Hoisting blocks should not have destroyed preheader");
         return HoistDestinationMap[BB];
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
        AU.addRequired<DominatorTreeWrapperPass>();
    }


    /// Walk the specified region of the CFG (defined by all blocks dominated by
     /// the specified block, and that are in the current loop) in depth first
     /// order w.r.t the DominatorTree.  This allows us to visit definitions before
     /// uses, allowing us to hoist a loop body in one pass without iteration.
     ///
     bool LICM::hoistRegion(LoopInfo *LI, DominatorTree *DT,Loop *CurLoop) {
        outs() << "================================\n";
        outs() << "hoist started.....\n";
       // Keep track of instructions that have been hoisted, as they may need to be
       // re-hoisted if they end up not dominating all of their uses.
       std::vector<Instruction*> HoistedInstructions;
      
       // For PHI hoisting to work we need to hoist blocks before their successors.
       // We can do this by iterating through the blocks in the loop in reverse
       // post-order.
       LoopBlocksRPO Worklist(CurLoop);
       Worklist.perform(LI);
       bool Changed = false;
       for (BasicBlock *BB : Worklist) {
            // Only need to process the contents of this block if it is not part of a
            // subloop (which would already have been processed).
        if (LI->getLoopFor(BB) != CurLoop)
            continue;
      
        for (Instruction &I : llvm::make_early_inc_range(*BB)) {
           // Try hoisting the instruction out to the preheader.  We can only do
           // this if all of the operands of the instruction are loop invariant and
           // if it is safe to hoist the instruction. We also check block frequency
           // to make sure instruction only gets hoisted into colder blocks.
           // TODO: It may be safe to hoist if we are hoisting to a conditional block
           // and we have accurately duplicated the control flow from the loop header
           // to that block.
           if ( isInvariant(CurLoop, HoistedInstructions, &I)) {
                 hoist(I, DT, CurLoop, getOrCreateHoistedBlock(BB));
                 HoistedInstructions.push_back(&I);
                 Changed = true;
                 continue;
           }
      
           // Remember possibly hoistable branches so we can actually hoist them
           // later if needed.
           if (BranchInst *BI = dyn_cast<BranchInst>(&I))
                registerPossiblyHoistableBranch(BI);
         }
       }
      
       // If we hoisted instructions to a conditional block they may not dominate
       // their uses that weren't hoisted (such as phis where some operands are not
       // loop invariant). If so make them unconditional by moving them to their
       // immediate dominator. We iterate through the instructions in reverse order
       // which ensures that when we rehoist an instruction we rehoist its operands,
       // and also keep track of where in the block we are rehoisting to to make sure
       // that we rehoist instructions before the instructions that use them.
        Instruction *HoistPoint = nullptr;
        for (Instruction *I : reverse(HoistedInstructions)) {
           if (!llvm::all_of(I->uses(),
                             [&](Use &U) { return DT->dominates(I, U); })) {
             BasicBlock *Dominator =
                 DT->getNode(I->getParent())->getIDom()->getBlock();
             if (!HoistPoint || !DT->dominates(HoistPoint->getParent(), Dominator)) {
               if (HoistPoint)
                 assert(DT->dominates(Dominator, HoistPoint->getParent()) &&
                        "New hoist point expected to dominate old hoist point");
               HoistPoint = Dominator->getTerminator();
             }
             LLVM_DEBUG(dbgs() << "LICM rehoisting to "
                               << HoistPoint->getParent()->getNameOrAsOperand()
                               << ": " << *I << "\n");
             // moveInstructionBefore(*I, *HoistPoint, *SafetyInfo, MSSAU, SE);
             I->moveBefore(HoistPoint );
             HoistPoint = I;
             Changed = true;
           }
        }

        outs() << "hoist ended.....\n";
      
        return Changed;
     }


    bool LICM::runOnLoop(Loop *L, LPPassManager &no_use) {

        bool modified = false;

        // Ignore loops without a pre-header
        BasicBlock* preheader = L->getLoopPreheader();
        if (!preheader) {
            return false;
        }

        // Else, get the loop info
        LI = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
        DT = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();
        CurLoop = L;

        DominatorPass &dominator = getAnalysis<DominatorPass>();                
        DomTree* tree = dominator.dom_analysis.dom_tree;

        return hoistRegion(LI, DT, L);

        /*std::vector<DomTreeNode*> worklist;
        worklist.push_back(tree->root);

        std::vector<Instruction*> invStmts; // Loop-invariant statements

        // Traverse in DFS order, so don't need to do multiple iterations. Use worklist as a stack
        while (!worklist.empty()) {
            DomTreeNode* n = worklist.back();
            BasicBlock* b = n->block;
            worklist.pop_back();
            
            //if(n->parent){
            //    inspectBlock(L, b, invStmts);
            //}
            // Skip this block if it is part of a subloop (thus, already processed)
            if (LI->getLoopFor(b) != L) {
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

        return modified;*/
    }

    char LICM::ID = 0;
    RegisterPass<LICM> X("loop-invariant-code-motion", "ECE/CS 5544 Loop invariant code motion");
}
