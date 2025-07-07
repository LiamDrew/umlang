This directory contains the UM tools and testing framework.

/tools contains the UM assembler and disassembler
/umbinary contains many UM assembled binaries for testing

The runtest.sh script can be used to test runtimes.
The maketest.sh script can be used to compile a UM assembly program into a UM
binary and add it to the binary 

It can be used in the following way:

./umasm/test.sh [executable_name] // runs all tests

./umasm/test.sh [executable_name] [test] // runs individual test