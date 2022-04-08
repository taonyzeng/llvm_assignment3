#CS5544 Assignment 3: README
#Group: ZENG TAO

/////////////////////////////////
# llvm_assignment3 DCE

To compile the dead code elimination pass
# go to the DCE folder
$cd DCE

# run make to compile the DCE pass and the test files under the tests folder. 
$make

# To run the DCE pass
$opt -enable-new-pm=0 -load ./DeadCodeElimination.so --dead-code-elimination ./tests/m2r_dce_bench1.bc -o out_dce_bench1