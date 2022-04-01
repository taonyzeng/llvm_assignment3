all: LoopInvariantCodeMotion.so sub-make

LIB_DIR = ../
SRC_DIR = src
DOM_PASS_DIR=../Dominators/src

CXX = clang
CXXFLAGS = $(shell llvm-config --cxxflags) -fcolor-diagnostics -g -O0 -fPIC
OPT = opt
TEST = tests

available-support.o: $(LIB_DIR)/available-support.cpp $(LIB_DIR)available-support.h
dataflow.o: $(LIB_DIR)/dataflow.cpp $(LIB_DIR)/dataflow.h
dominance.o: $(DOM_PASS_DIR)/dominance.cpp $(DOM_PASS_DIR)/dominance.h
dominator.o: $(DOM_PASS_DIR)/dominator.cpp $(DOM_PASS_DIR)/dominator.h
LoopInvariantCodeMotion.o: $(SRC_DIR)/LoopInvariantCodeMotion.cpp


LoopInvariantCodeMotion.so: $(LIB_DIR)/dataflow.o $(LIB_DIR)/available-support.o $(DOM_PASS_DIR)/dominance.o $(DOM_PASS_DIR)/dominator.o $(SRC_DIR)/LoopInvariantCodeMotion.o
	$(CXX) -dylib -shared $^ -o $@

sub-make:
	make -C tests

# CLEAN
clean:
	rm -f *.o *~ *.so *.ll $(TEST)/*.bc out*             
