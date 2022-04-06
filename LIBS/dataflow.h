// ECE/CS 5544 Assignment 2: dataflow.h
// Group: ZENG TAO
////////////////////////////////////////////////////////////////////////////////

#ifndef __CLASSICAL_DATAFLOW_H__
#define __CLASSICAL_DATAFLOW_H__


#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/PostOrderIterator.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/ValueMap.h"
#include "llvm/Pass.h"
#include "available-support.h"

#include <cstdio>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <string>
#include <iomanip>

using namespace std;

namespace llvm {

    //typedef std::vector<BasicBlock*> BasicBlockList;

    /*	For storing the output of a transfer function.
        We also store a list of BitVectors corresponding to predecessors/successors used to handle phi nodes) */
    struct TransferOutput {
        BitVector element;
        std::unordered_map<BasicBlock*, BitVector> neighborVals;
    };

    /* Stores the IN and OUT sets for a basic block. Also a variable to store the temporary output of the transfer function */
    struct BlockResult {
        BitVector in;
        BitVector out;

        TransferOutput transferOutput;
    };

    enum Direction {
        INVALID_DIRECTION,
        FORWARD,
        BACKWARD
    };

    enum MeetOp {
        INVALID_MEETOP,
        UNION,
        INTERSECTION
    };

    /* Result of pass on a function */
    struct DataFlowResult {
        /* Mapping from basic blocks to their results */
        std::unordered_map<BasicBlock*, BlockResult> result;

        /* Mapping from domain elements to indices in bitvectors
           (to figure out which bits are which values) */
        std::unordered_map<void*, int> domainToIndex;

        bool modified;
    };

    /* Basic Class for Data flow analysis. Specific analyses must extend this */
    class DataFlow {

        public:

            DataFlow(Direction direction, MeetOp meetup_op)
                : direction(direction), meetup_op(meetup_op)
                {
                }

                //store the generate set for each basic block.
                std::unordered_map<BasicBlock*, BitVector> genSet;

                //store the kill set for each basic block.
                std::unordered_map<BasicBlock*, BitVector> killSet;

                /* Applying Meet Operator */
                BitVector applyMeetOp(vector<BitVector>& inputs);

                /* Apply analysis on Function F */
                DataFlowResult run(Function &F, vector<void*> domain,
                                   BitVector boundaryCond, BitVector initCond);

                void printResult(DataFlowResult output);

        protected:
            /*      Transfer Function: To be implmented by the specific analysis.
                    Takes one set (IN/OUT), domain to index mapping, basic block, and outputs the other set (OUT/IN) */
            virtual TransferOutput transferFn(BitVector input,
                                              std::vector<void*> domain,
                                              std::unordered_map<void*, int> domainToIndex,
                                              BasicBlock* block) = 0;


        private:
            /* Pass-specific parameters */
            Direction direction;
            MeetOp meetup_op;
    };
}

#endif
