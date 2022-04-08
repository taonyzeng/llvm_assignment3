#CS5544 Assignment 3: README
#Group: ZENG TAO

/////////////////////////////////
# llvm_assignment3 Dominators

To compile the dominators
# go to the Dominators folder
$cd Dominators

# run make to compile,it will compile the dominators and the test C files under tests folder.
$make 

# To run the dominator pass
$opt -enable-new-pm=0 -load ./dominator.so --dominators ./tests/m2r_nested_main.bc -o out