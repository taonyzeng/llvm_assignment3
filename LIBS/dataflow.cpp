// ECE/CS 5544 S22 Assignment 2: dataflow.cpp
// Group:

////////////////////////////////////////////////////////////////////////////////

#include "dataflow.h"

using namespace std;

namespace llvm {

    void DataFlow::printResult(DataFlowResult output){
        for(auto entry : output.result)
        {
            outs() << "BB " << entry.first->getName() << "\n";

            outs() << formatBitVector(entry.second.in) << "\n";
            outs() << formatBitVector(entry.second.out) << "\n";
        }
    }

    /* Applying Meet Operator */
    BitVector DataFlow::applyMeetOp(vector<BitVector>& inputs) {
        BitVector result;

        if (!inputs.empty()) {
            for (int i = 0; i < inputs.size(); i++) {
                if (i == 0) {
                    result = inputs[i];
                }
                else {
                    if (meetup_op == MeetOp::UNION) {
                        result |= inputs[i];
                    }
                    else if (meetup_op == MeetOp::INTERSECTION) {
                        result &= inputs[i];
                    }
                    else{
                        outs() << "Unknown Meetop \n";
                        exit(EXIT_FAILURE);
                    }
                }
            }
        }

        return result;
    }

    /* Apply analysis on Function F */
    DataFlowResult DataFlow::run(Function &F, std::vector<void*> domain,
                                 BitVector boundaryCond, BitVector initCond) {
        std::unordered_map<BasicBlock*, BlockResult> result;
        bool modified = false;

        // Map domain values to index in bitvector
        std::unordered_map<void*, int> domainToIndex;
        for (int i = 0; i < domain.size(); i++)
            domainToIndex[(void*)domain[i]] = i;

        // Find Boundary Blocks
        vector<BasicBlock*> boundaryBlocks;
        switch (direction) {

            case Direction::FORWARD:{
                // Entry = first block
                boundaryBlocks.push_back(&F.front());
                break;
            }
            case Direction::BACKWARD:{
                // TODO: Improve this. What if the function has exit(0)? Or use CFGNode from LLVM?
                // Exit = blocks with a return statement (could be multiple)
                for(Function::iterator I = F.begin(), E = F.end(); I != E; ++I)
                    if (isa<ReturnInst>(I->getTerminator()))
                        boundaryBlocks.push_back(&*I);
                break;
            }
            default:
                errs() << "Invalid direction \n";
                break;
        }

        //DBG(outs() << "Boundary Blocks :" << boundaryBlocks.size() << "\n");
        //DBG(outs() << "Initialize Boundary Blocks \n");

        // Initialize Boundary Blocks
        BlockResult boundaryRes = BlockResult();
        // Forward analysis => Initialize IN, Backward analysis => OUT
        BitVector* value = (direction == Direction::FORWARD) ? &boundaryRes.in : &boundaryRes.out;

        *value = boundaryCond;
        boundaryRes.transferOutput.element = boundaryCond;
        for (vector<BasicBlock*>::iterator I = boundaryBlocks.begin(), E = boundaryBlocks.end(); I != E; ++I) {
            result[*I] = boundaryRes;	// TODO: If we run into errors, this might be a cause (pointer problems!)
        }

        //DBG(outs() << "Initialize Interior Blocks \n");

        // Initialize Interior Blocks
        BlockResult intRes = BlockResult();
        // Forward analysis => Initialize IN, Backward analysis => OUT
        BitVector* intVal = (direction == Direction::FORWARD) ? &intRes.out : &intRes.in;

        *intVal = initCond;
        intRes.transferOutput.element = initCond;
        for (Function::iterator BB = F.begin(), BE = F.end(); BB != BE; ++BB) {
            BasicBlock* block = &*BB;
            if (std::find(boundaryBlocks.begin(),boundaryBlocks.end(), block) == boundaryBlocks.end()) {
                // If it is not one of the boundary blocks
                result[block] = intRes;
            }
        }

        // Generate "neighbor" list: For forward analysis, these are predecessors, for backward analysis these are successors
        // So we won't have to switch on direction every time
        std::unordered_map<BasicBlock*, vector<BasicBlock*>> blockNeighbors;

        for (Function::iterator BB = F.begin(), BE = F.end(); BB != BE; ++BB) {
            vector<BasicBlock*> neighborList;
            BasicBlock* block = &*BB;
            
            switch (direction) {
                case Direction::FORWARD:
                    for (pred_iterator neighbor = pred_begin(block), E = pred_end(block); neighbor != E; ++neighbor)
                        neighborList.push_back(*neighbor);

                    break;

                case Direction::BACKWARD:
                    for (succ_iterator neighbor = succ_begin(block), E = succ_end(block); neighbor != E; ++neighbor)
                        neighborList.push_back(*neighbor);
                    break;

                default:
                    errs() << "Invalid direction \n";
                    break;
            }

            blockNeighbors[block] = neighborList;
        }

        // Prepare an order in which we will traverse BasicBlocks. This is to prevent having to write analysis code twice (for each direction)
        vector<BasicBlock*> traversalOrder;

        if(direction == Direction::FORWARD){
            ReversePostOrderTraversal<Function*> TR(&F);
            for (ReversePostOrderTraversal<Function*>::rpo_iterator I = TR.begin(), E = TR.end(); I != E; ++I) {
                traversalOrder.push_back(*I);
            }
        }
        else if(direction == Direction::BACKWARD){
            for (po_iterator<BasicBlock*> I = po_begin(&F.getEntryBlock()), E = po_end(&F.getEntryBlock()); I != E; ++I) {
                traversalOrder.push_back(*I);
            }
        }
        else{
            errs() << "Invalid direction \n";
        }

        // Keep processing blocks until convergence
        bool converged = false;
        while (!converged) {
            converged = true;

            for (vector<BasicBlock*>::iterator BB = traversalOrder.begin(), BE = traversalOrder.end(); BB != BE; ++BB) {

                // Get the current result for the block
                BlockResult& blockRes = result[*BB];

                // Store it to later check if it changed
                BlockResult oldBlockRes = blockRes;
                BitVector oldVal = (direction == Direction::FORWARD) ? blockRes.out : blockRes.in;

                // Collect Neighbor Results for Meet
                vector<BitVector> meetInputs;

                for (vector<BasicBlock*>::iterator NI = blockNeighbors[*BB].begin(), NE = blockNeighbors[*BB].end(); NI != NE; ++NI) {
                    BlockResult& neighborRes = result[*NI];
                    BitVector neighVal = neighborRes.transferOutput.element;

                    // Union the value if we find a match with neighbor-specific value
                    std::unordered_map<BasicBlock*, BitVector>::iterator match = neighborRes.transferOutput.neighborVals.find(*BB);
                    if (match != neighborRes.transferOutput.neighborVals.end()) {
                        neighVal |= match->second;
                    }

                    meetInputs.push_back(neighVal);
                }

                // Apply Meet Operator, Store in (IN/OUT) depending on direction
                BitVector* blockInput = (direction == Direction::FORWARD) ? &blockRes.in : &blockRes.out;
                if (!meetInputs.empty())
                    *blockInput = applyMeetOp(meetInputs);

                //Apply transfer function to input set in order to get output set for this iteration
                blockRes.transferOutput = transferFn(*blockInput, domain, domainToIndex, *BB);
                // TODO: Memoize GEN, KILL (and other stuff) to avoid recomputations
                BitVector* blockOutput = (direction == Direction::FORWARD) ? &blockRes.out : &blockRes.in;
                *blockOutput = blockRes.transferOutput.element;

                // If something changed, set converged to false
                if (converged) {
                    // Either the IN/OUT set itself changed, or a neighbor-specific value changed
                    // (actually, for phi nodes, the size will also change, so can just check that)
                    if (*blockOutput != oldVal ||
                        blockRes.transferOutput.neighborVals.size() != oldBlockRes.transferOutput.neighborVals.size())
                        converged = false;
                }
            }
        }

        // Setup output
        DataFlowResult output;
        output.domainToIndex = domainToIndex;
        output.result = result;
        output.modified = modified;

        return output;
    }


}
