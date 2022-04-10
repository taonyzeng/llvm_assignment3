// CS5544 Assignment 3: DeadCodeElimination
// Group: ZENG TAO
////////////////////// //////////////////////////////////////////////////////////

#include "DeadCodeElimination.hpp"

namespace llvm_asst3{

    DCE::DCE() : FunctionPass(ID) {
        // Setup the pass
        Direction direction = Direction::BACKWARD;
        MeetOp meet_op = MeetOp::UNION;

        pass = DCEAnalysis(direction, meet_op);
    }

    void DCE::getAnalysisUsage(AnalysisUsage& AU) const {
        AU.setPreservesAll();
    }

    std::vector<Instruction*> DCE::computeDeleteSet(BasicBlock* block, BitVector& SLV, DataFlowResult& output){
        // Figure out instructions to delete
        std::vector<Instruction *> deleteSet;

        for (BasicBlock::reverse_iterator insn = block->rbegin(); insn != block->rend(); ++insn) {
            Instruction* I = &(*insn);

            // Check if insn is live
            if(I == block->getTerminator() ||  isa<DbgInfoIntrinsic>(I) ||
               isa<LandingPadInst>(I) ||  I->mayHaveSideEffects()){
                continue;
            }


            if(std::find(domain.begin(), domain.end(), I) != domain.end())
            {
                int valInd = output.domainToIndex[(void*)I];

                if(SLV[valInd] == false)
                {
                    DBG(outs() << "Going to delete instruction :: " << printValue(I) << "\n");
                    deleteSet.push_back(I);
                }
            }
        }

        return deleteSet;
    }

    void DCE::initDomain(Function &F){

        for(inst_iterator II = inst_begin(F), IE = inst_end(F); II!=IE; ++II) {
            Instruction& insn(*II);

            if(std::find(domain.begin(),domain.end(),(&(*II))) == domain.end())
                domain.push_back((void*)(&(*II)));
        }

        DBG(outs() << "------------------------------------------\n\n");
        DBG(outs() << "DOMAIN :: " << domain.size() << "\n");
        for(void* element : domain)
        {
            DBG(outs() << "Element : " << *((Value*) element) << "\n");
        }
        DBG(outs() << "------------------------------------------\n\n");
    }

    bool DCE::runOnFunction(Function &F) {
        // Print Information
        //std::string function_name = F.getName();
        DBG(outs() << "FUNCTION :: " << F.getName() << "\n");

        // Compute domain for function
        initDomain(F);

        // For SLVA, both are empty sets
        BitVector boundaryCond(domain.size(), false);
        BitVector initCond(domain.size(), false);
        bool modified = false;

        // Apply pass
        DataFlowResult output = pass.run(F, domain, boundaryCond, initCond);
        pass.printResult(output);

        // Prepare an order in which we will traverse BasicBlocks.
        for (po_iterator<BasicBlock*> BI = po_begin(&F.getEntryBlock()), BE = po_end(&F.getEntryBlock()); BI != BE; ++BI) {
            BasicBlock* block = *BI;

            // strong liveness at IN
            BitVector SLV = output.result[block].in;
            outs() << formatBitVector(SLV) << "\n";

            auto&& deleteSet = computeDeleteSet(block, SLV, output);
            //removed the variable not in the strong liveness variable set.
            for(auto I : deleteSet){
                if(!I->use_empty())
                    continue;
                DBG(outs() << "Deleting instruction :: " << printValue(I) << "\n");
                I->eraseFromParent();
                modified = true;
            }
        }

        // Potential modification
        return modified;
    }

    BitVector DCEAnalysis::initUseSet(BasicBlock* block, BitVector& input, std::unordered_map<void*, int>& domainToIndex){
        //First initialize
        BitVector useSet(input);

        // Iterate forward through the block to find live insns
        for (BasicBlock::iterator insn = block->begin(); insn != block->end(); ++insn) {
            Instruction *I = &(*insn);

            if(I == block->getTerminator() ||  isa<DbgInfoIntrinsic>(I) ||
               isa<LandingPadInst>(I) ||  I->mayHaveSideEffects())
            {
                std::unordered_map<void*, int>::iterator iter = domainToIndex.find((void*)I);

                if(iter != domainToIndex.end())
                {
                    int valInd = domainToIndex[(void*)I];

                    DBG(outs() << "Marking live instruction :: " << printValue(I) << "\n");

                    useSet.set(valInd);
                }
            }
        }

        return useSet;
    }

    void DCEAnalysis::updateUseSet(BitVector& useSet, Instruction* insn, std::unordered_map<void*, int>& domainToIndex){

        for (User::op_iterator opnd = insn->op_begin(), opE = insn->op_end(); opnd != opE; ++opnd) {
            Value* val = *opnd;

            if (isa<Instruction>(val)) {
                int valInd = domainToIndex[(void*)val];

                DBG(outs() << "Marking operand :: " << printValue(val) << "\n");

                useSet.set(valInd);
            }
        }
    }


    void DCEAnalysis::processPhiNode(PHINode* phi_insn, TransferOutput& transferOutput, std::unordered_map<void*, int>& domainToIndex){

        int domainSize = domainToIndex.size();
        for (int ind = 0; ind < phi_insn->getNumIncomingValues(); ind++) {

            Value* val = phi_insn->getIncomingValue(ind);

            if (isa<Instruction>(val)) {
                BasicBlock* valBlock = phi_insn->getIncomingBlock(ind);

                // neighborVals has no mapping for this block, then create one
                if (transferOutput.neighborVals.find(valBlock) == transferOutput.neighborVals.end())
                    transferOutput.neighborVals[valBlock] = BitVector(domainSize);

                int valInd = domainToIndex[(void*)val];

                DBG(outs() << "Marking phi operand :: " << printValue(val) << "\n");

                // Set the bit corresponding to "val"
                transferOutput.neighborVals[valBlock].set(valInd);
            }
        }
    }

    TransferOutput DCEAnalysis::transferFn(BitVector input,  std::vector<void*> domain, std::unordered_map<void*, int> domainToIndex, BasicBlock* block){
        TransferOutput transferOutput;

        // Calculating the set of locally exposed uses
        int domainSize = domainToIndex.size();
        BitVector defSet(domainSize);
        BitVector useSet = initUseSet(block, input, domainToIndex);

        // Iterate backward through the block, update SLV
        for (BasicBlock::reverse_iterator insn = block->rbegin(); insn != block->rend(); ++insn) {

            // Alter data flow only if insn itself is strongly live
            std::unordered_map<void*, int>::iterator iter = domainToIndex.find((void*)(&(*insn)));

            if(iter != domainToIndex.end())
            {
                int valInd = domainToIndex[(void*)(&(*insn))];
                if(useSet[valInd] == false)
                    continue;

                // Phi nodes: add operands to the list we store in transferOutput
                if (PHINode* phi_insn = dyn_cast<PHINode>(&*insn)) {
                    processPhiNode(phi_insn, transferOutput, domainToIndex);
                }
                //Non-phi nodes: Simply add operands to the use set
                else {
                    updateUseSet(useSet, &(*insn), domainToIndex);
                }

                // Definitions
                iter = domainToIndex.find((void*)&(*insn));
                if (iter != domainToIndex.end())
                    defSet.set((*iter).second);
            }
        }

        // Transfer function = useSet U (input - defSet)

        transferOutput.element = defSet;
        // Complement of defSet
        transferOutput.element.flip();
        // input - defSet = input INTERSECTION Complement of defSet
        transferOutput.element &= input;
        transferOutput.element |= useSet;

        DBG(outs() << "\n\n--------------------------------------------------\n\n");

        return transferOutput;
    }

    char DCE::ID = 0;
    RegisterPass<DCE> X("dead-code-elimination", "CS5544 Dead Code Elimination");
}
