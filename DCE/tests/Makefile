all: inputs

CXX = clang
TEST = .
OPT = opt

# TESTING
inputs : $(patsubst %.c,%.bc,$(wildcard $(TEST)/*.c)) 
 
%.tmp: %.c
	#$(CXX) -O0 -emit-llvm -Xclang -disable-O0-optnone $^ -o $@
	$(CXX) -emit-llvm -S $^ -O0 -Xclang -disable-O0-optnone -o $@

%.bc: %.tmp
	$(OPT) -mem2reg $^ -o m2r_$@

# CLEAN
clean:
	rm -f  *.ll *.o *~ *.so out*  $(TEST)/*.bc $(TEST)/*.ll