#CS5544 Assignment 3: README
#Group: ZENG TAO

/////////////////////////////////
# llvm_assignment3 LICM

To compile the loop invariant code motion pass
# go to the LICM folder
$cd LICM

# run make to compile the LICM pass and the test files under the tests folder. 
$make

# To run the LICM pass
$opt -enable-new-pm=0 --loop-simplify -load ./LoopInvariantCodeMotion.so --loop-invariant-code-motion ./tests/m2r_licm_bench1.bc -o out_licm_bench1