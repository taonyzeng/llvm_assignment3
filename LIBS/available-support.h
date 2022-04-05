
////////////////////////////////////////////////////////////////////////////////

#ifndef __AVAILABLE_SUPPORT_H__
#define __AVAILABLE_SUPPORT_H__

#include <string>
#include <vector>

#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/BitVector.h"

#include <sstream>

// DEBUG mode
#undef DEBUG
//#define DEBUG 1

#ifdef DEBUG
#define DBG(a) a
#else
#define DBG(a)
#endif

namespace llvm {
    std::string getShortValueName(Value * v);

    class Expression {
    public:
      Value * v1;
      Value * v2;
      Instruction::BinaryOps op;
      Expression (Instruction * I);
      bool operator== (const Expression &e2) const;
      bool operator< (const Expression &e2) const;
      std::string toString() const;
    };

    void printSet(std::vector<Expression> * x);

    //Pretty printing utility functions
    std::string formatBitVector(BitVector& b);
    Value* getDefinitionVar(Value* v);

    /** Returns string representation of a set of domain elements with inclusion indicated by a bit vector
     Each element is output according to the given valFormatFunc function */
    std::string setToStr(std::vector<void*>& domain, BitVector& includedInSet);

    /** Returns string version of definition if the Value is in fact a definition, or an empty string otherwise.
     * eg: The defining instruction "%a = add nsw i32 %b, 1" will return exactly that: "%a = add nsw i32 %b, 1"*/
    std::string valueToDefinitionStr(Value* v);

    std::string valueToDefinitionVarStr(Value* v);

    std::string valueToStr(Value* v);

    std::string formatSet(std::vector<void*>& domain, BitVector& on, int mode);
}

#endif
