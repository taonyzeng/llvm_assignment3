# llvm_assignment3
source code for compiler optimization assignment 3 


clang -emit-llvm -S dead.c -O0 -Xclang -disable-O0-optnone

opt -mem2reg dead.ll -S

opt -mem2reg -dce dead.ll -S 

opt -enable-new-pm=0 -load ./liveness.so --liveness ./test/liveness-test-m2r.bc -o out

lli -stats -force-interpreter testcode.bc

llvm-dis