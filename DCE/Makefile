
all: DeadCodeElimination.so sub-make

LIB_DIR = ../LIBS
SRC_DIR = src
CXX = clang
CXXFLAGS = $(shell llvm-config --cxxflags) -fcolor-diagnostics -g -O0 -fPIC
OPT = opt
TEST = tests

available-support.o: $(LIB_DIR)/available-support.cpp $(LIB_DIR)available-support.h

dataflow.o: $(LIB_DIR)/dataflow.cpp $(LIB_DIR)/dataflow.h

DeadCodeElimination.o: $(SRC_DIR)/DeadCodeElimination.cpp


%.so: $(LIB_DIR)/dataflow.o $(LIB_DIR)/available-support.o $(SRC_DIR)/DeadCodeElimination.o
	$(CXX) -dylib -shared $^ -o $@

sub-make:
	make -C tests

# CLEAN
clean:
	rm -f *.o *~ *.so out $(TEST)/*.bc out* $(SRC_DIR)/*.o                
