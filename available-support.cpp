// ECE/CS 5544 S22 Assignment 2: available-support.cpp

////////////////////////////////////////////////////////////////////////////////

#include "available-support.h"

namespace llvm {
  // The Expression class is provided here to help
  // you work with the expressions we'll be concerned
  // about for the Available Expression analysis
  Expression::Expression (Instruction * I) {
    if (BinaryOperator * BO = dyn_cast<BinaryOperator>(I)) {
      this->v1 = BO->getOperand(0);
      this->v2 = BO->getOperand(1);
      this->op = BO->getOpcode();
    }
    else {
      errs() << "We're only considering BinaryOperators\n";
    }
  }

  // For two expressions to be equal, they must
  // have the same operation and operands.
  bool Expression::operator== (const Expression &e2) const {
    return this->v1 == e2.v1 && this->v2 == e2.v2 && this->op == e2.op;
  }

  // Less than is provided here in case you want
  // to use STL maps, which use less than for
  // equality checking by default
  bool Expression::operator< (const Expression &e2) const {
    if (this->v1 == e2.v1) {
      if (this->v2 == e2.v2) {
        if (this->op == e2.op) {
          return false;
        } else {
          return this->op < e2.op;
        }
      } else {
        return this->v2 < e2.v2;
      }
    } else {
      return this->v1 < e2.v1;
    }
  }

  // A pretty printer for Expression objects
  // Feel free to alter in any way you like
  std::string Expression::toString() const {
    std::string op = "?";
    switch (this->op) {
      case Instruction::Add:
      case Instruction::FAdd: op = "+"; break;
      case Instruction::Sub:
      case Instruction::FSub: op = "-"; break;
      case Instruction::Mul:
      case Instruction::FMul: op = "*"; break;
      case Instruction::UDiv:
      case Instruction::FDiv:
      case Instruction::SDiv: op = "/"; break;
      case Instruction::URem:
      case Instruction::FRem:
      case Instruction::SRem: op = "%"; break;
      case Instruction::Shl: op = "<<"; break;
      case Instruction::AShr:
      case Instruction::LShr: op = ">>"; break;
      case Instruction::And: op = "&"; break;
      case Instruction::Or: op = "|"; break;
      case Instruction::Xor: op = "xor"; break;
      default: break;
    }
    return getShortValueName(v1) + " " + op + " " + getShortValueName(v2);
  }

  const int VAR_NAME_START_IDX = 2;

  std::string valueToDefinitionVarStr(Value* v) {
    //Similar to valueToDefinitionStr, but we return just the defined var rather than the whole definition

    Value* def = getDefinitionVar(v);
    if (def == 0)
      return "";

    if (isa<Argument>(def) || isa<StoreInst>(def)) {
      return "%" + def->getName().str();
    }
    else {
      std::string str = valueToStr(def);
      int varNameEndIdx = str.find(' ',VAR_NAME_START_IDX);
      str = str.substr(VAR_NAME_START_IDX,varNameEndIdx-VAR_NAME_START_IDX);
      return str;
    }
  }

    /* Var definition util */
  Value* getDefinitionVar(Value* v) {
    // Definitions are assumed to be one of:
    // 1) Function arguments
    // 2) Store instructions (2nd argument is the variable being (re)defined)
    // 3) Instructions that start with "  %" (note the 2x spaces)
    //      Note that this is a pretty brittle and hacky way to catch what seems the most common definition type in LLVM.
    //      Unfortunately, we couldn't figure a better way to catch all definitions otherwise, as cases like
    //      "%0" and "%1" don't show up  when using "getName()" to identify definition instructions.
    //      There's got to be a better way, though...

    if (isa<Argument>(v)) {
      return v;
    }
    else if (isa<StoreInst>(v)) {
      return ((StoreInst*)v)->getPointerOperand();
    }
    else if (isa<Instruction>(v)){
      std::string str = valueToStr(v);
      if (str.length() > VAR_NAME_START_IDX && str.substr(0,VAR_NAME_START_IDX+1) == "  %")
        return v;
    }
    return 0;
  }

  std::string valueToDefinitionStr(Value* v) {
    //Verify it's a definition first
    Value* def = getDefinitionVar(v);
    if (def == 0)
      return "";

    std::string str = valueToStr(v);
    if (isa<Argument>(v)) {
      return str;
    }
    else {
        str = str.substr(VAR_NAME_START_IDX);
        return str;
    }

    return "";
  }


  //format the set to a string.
  std::string setToStr(std::vector<void*>& domain, BitVector& includedInSet) {
    std::stringstream ss;
    ss << "{";
    int cnt = 0;

    for (int i = 0; i < domain.size(); i++) {
      if (includedInSet[i]) {
        std::string&& defstr = valueToDefinitionStr((Value*)domain[i]);
        if(defstr.length() == 0){
          continue;
        }

        if (cnt > 0) ss << " | ";
        ss << defstr;

        cnt++;
      }
    }
    ss << "}";
    return ss.str();
  }


  // Silly code to print out a set of expressions in a nice
  // format
  void printSet(std::vector<Expression> * x) {
    bool first = true;
    outs() << "{";

    for (std::vector<Expression>::iterator it=x->begin(), iend=x->end(); it!=iend; ++it) {
      if (!first) {
        outs() << ", ";
      }
      else {
        first = false;
      }
      outs() << (it->toString());
    }
    outs() << "}\n";
  }

  // The following code may be useful for both of your passes:
  // If you recall, there is no "get the variable on the left
  // hand side" function in LLVM. Normally this is fine: we
  // just call getName(). This does not work, however, for
  // instructions that are publically 'unnamed', but internally
  // assigned a name like '%0' or '%1'. To get these names, the
  // following code does some really nasty stuff. It also grabs
  // raw integer values so our expressions can look a little
  // cleaner.
  //
  // Feel free to improve this code if you want nicer looking
  // results. For example, ConstantInts are the only pretty
  // printed Constants.
  //
  // Sadly, this code is a great example of not doing things
  // 'the LLVM way', especially since we're using std::string.
  // I encourage you to think of a way to make this code nicer
  // and let me know :)
  std::string getShortValueName(Value * v) {
    if (v->getName().str().length() > 0) {
      return "%" + v->getName().str();
    }
    else if (isa<Instruction>(v)) {
      std::string s = "";
      raw_string_ostream * strm = new raw_string_ostream(s);
      v->print(*strm);
      std::string inst = strm->str();
      size_t idx1 = inst.find("%");
      size_t idx2 = inst.find(" ",idx1);
      if (idx1 != std::string::npos && idx2 != std::string::npos) {
	      return inst.substr(idx1,idx2-idx1);
      }
      else {
	      return "\"" + inst + "\"";
      }
    }
    else if (ConstantInt * cint = dyn_cast<ConstantInt>(v)) {
      std::string s = "";
      raw_string_ostream * strm = new raw_string_ostream(s);
      cint->getValue().print(*strm,true);
      return strm->str();
    }
    else {
      std::string s = "";
      raw_string_ostream * strm = new raw_string_ostream(s);
      v->print(*strm);
      std::string inst = strm->str();
      return "\"" + inst + "\"";
    }
  }


      // Pretty printing utility functions
    std::string formatBitVector(BitVector& b)
    {
        unsigned int i;
        unsigned int b_size = b.size();

        std::stringstream ss;
        ss << "{";

        if(b_size == 0)
            ss << "-";
        else
        {
            for(i = 0; i < b.size() ; i++)
            {
                if(b[i] == true)
                    ss << "1";
                else
                    ss << "0";
            }
        }

        ss << "}";
        return ss.str();
    }

    std::string valueToStr(Value* v)
    {
        std::string res; 
        llvm::raw_string_ostream raw_st(res);
        v->print(raw_st);
        return res;
    }

    std::string formatSet(std::vector<void*>& domain, BitVector& on, int  mode) {
        std::stringstream ss;
        ss << "{";
        int cnt = 0;

        for (int i = 0; i < domain.size(); i++) {
            // If element i is on
            if (on[i]) {

                std::string ret("");

                switch(mode)
                {
                  case 0:
                      // Value*
                      ret = getShortValueName((Value*)domain[i]);
                      break;

                  case 1:
                      // Expression*
                      ret = ((Expression*)domain[i])->toString();
                      break;

                  default:
                      errs() << "Invalid mode :: " << mode << "\n";
                      break;
                }

                if(ret.length() == 0){
                  continue;
                }

                if (cnt > 0)
                  ss << " ,";

                ss << ret;

                cnt++;
            }
        }

        ss << " }";

        return ss.str();
    }

}
