#CS5544 Assignment 3: README
#Group: ZENG TAO

/////////////////////////////////
# llvm_assignment3

1. Dominators

To compile the dominators
# go to the Dominators folder
$cd Dominators

# run make to compile,it will compile the dominators and the test C files under tests folder.
$make 

# To run the dominator pass
$opt -enable-new-pm=0 -load ../dominator.so --dominators ./tests/m2r_nested_main.bc -o out


2. DCE
To compile the dead code elimination pass
# go to the DCE folder
$cd DCE

# run make to compile the DCE pass and the test files under the tests folder. 
$make



opt -mem2reg dead.ll -S

opt -mem2reg -dce dead.ll -S 

opt -enable-new-pm=0 -load ./liveness.so --liveness ./test/liveness-test-m2r.bc -o out

lli -stats -force-interpreter testcode.bc

llvm-dis

opt -enable-new-pm=0 --loop-simplify -load ../LoopInvariantCodeMotion.so --loop-invariant-code-motion ./m2r_licm_test_2.bc -o out_licm_test2