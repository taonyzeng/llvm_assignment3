all: dominator.so sub-make

LIB_DIR = ../LIBS
SRC_DIR = src
CXX = clang
CXXFLAGS = $(shell llvm-config --cxxflags) -fcolor-diagnostics -g -O0 -fPIC
OPT = opt
TEST = tests

available-support.o: $(LIB_DIR)/available-support.cpp $(LIB_DIR)available-support.h

dataflow.o: $(LIB_DIR)/dataflow.cpp $(LIB_DIR)/dataflow.h
dominance.o: $(SRC_DIR)/dominance.cpp $(SRC_DIR)/dominance.h
dominator.o: $(SRC_DIR)/dominator.cpp


%.so: $(LIB_DIR)/dataflow.o $(LIB_DIR)/available-support.o $(SRC_DIR)/dominance.o $(SRC_DIR)/dominator.o
	$(CXX) -dylib -shared $^ -o $@

sub-make:
	make -C tests

# CLEAN
clean:
	rm -f *.o *~ *.so out* $(TEST)/*.bc                 
