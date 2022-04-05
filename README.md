# llvm_assignment3
source code for compiler optimization assignment 3 


clang -emit-llvm -S dead.c -O0 -Xclang -disable-O0-optnone

opt -mem2reg dead.ll -S

opt -mem2reg -dce dead.ll -S 

opt -enable-new-pm=0 -load ./liveness.so --liveness ./test/liveness-test-m2r.bc -o out

lli -stats -force-interpreter testcode.bc

llvm-dis


opt -enable-new-pm=0 --loop-simplify -load ../LoopInvariantCodeMotion.so --loop-invariant-code-motion ./m2r_licm_test_2.bc -o out_licm_test2