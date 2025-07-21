// #include <iostream>
// #include <fstream>
// #include <vector>
// #include <cstdint>
// #include <filesystem>
// #include "program_loader.h"
// #include "compiler.h"

// int main(int argc, char *argv[]) {
//     if (argc != 2) {
//         std::cerr << "Usage: " << argv[0] << " [program.um]\n";
//         return EXIT_FAILURE;
//     }

//     std::filesystem::path filePath(argv[1]);
//     if (not std::filesystem::exists(filePath)) {
//         std::cerr << "Error: File " << argv[1] << "does not exist\n";
//         return EXIT_FAILURE;
//     }

//     uint64_t fileSize = std::filesystem::file_size(filePath);
//     if (fileSize % 4 != 0) {
//         std::cerr << "A valid UM file must be a size divisible by 4 bytes\n";
//         return EXIT_FAILURE;
//     }

//     std::ifstream file(argv[1], std::ios::binary);
//     if (!file.is_open()) {
//         std::cerr << "Error: Could not open file " << argv[1] << std::endl;
//         return EXIT_FAILURE;
//     }
    
//     ProgramLoader loader;
//     loader.load_file(file);
    
//     // At this point, turn things over to the compiler
//     Compiler compiler;


//     for (size_t i = 0; i < loader.program.size(); i++) {
//         // print out the opcode for each instruction
//         uint32_t word = loader.program[i];

//         compiler.compileInstruction(word);
//     }

//     // compiler.setRegisterValues(1, 5);
//     // compiler.setRegisterValues(2, 3);

//     // compiler.compileAddition(0, 1, 2);

//     // Have the compiler print all the IR that was generated
//     compiler.printIR();

//     return 0;
// }


// /* I'm coming to realize that serious people build with C++. It just saves so
//  * much time and is so much more powerful than C. Much as I prefer C, C++ is
//  * the industry standard and the way forward.
//  * 
//  * These compiler guys who are building C++ compilers in C++ are the ultimate
//  * C++ wizards. It would serve me very well to follow this path I think. This
//  * path is calling my name, and I have basically nowhere else to go. I've
//  * already shows signs of being good at this and could have a bright future
//  * ahead of me if I keep things up/
//  * 
//  * It's funny that it takes forever for C++ code to build. I guess for a huge
//  * project like Linux, C makes more sense.
//  */

// /* For now, we are not going to use the memory system or anything like it
// * All we want to do is generate some sensible IR to represent UM
// * instructions.
// * 
// * As ever, this will be easy for the arithmetic instructions and difficult
// * for the memory access, I/O, and control flow instructions.
// * 
// */

#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <filesystem>
#include "program_loader.hpp"
#include "compiler.hpp"

int main(int argc, char *argv[]) {
    if (argc < 2 || argc > 3) {
        std::cerr << "Usage: " << argv[0] << " [program.um] [--jit|--print-ir]\n";
        std::cerr << "  --jit: Execute program using JIT compiler (default)\n";
        std::cerr << "  --print-ir: Print LLVM IR instead of executing\n";
        return EXIT_FAILURE;
    }
    
    std::filesystem::path filePath(argv[1]);
    if (not std::filesystem::exists(filePath)) {
        std::cerr << "Error: File " << argv[1] << " does not exist\n";
        return EXIT_FAILURE;
    }
    
    uint64_t fileSize = std::filesystem::file_size(filePath);
    if (fileSize % 4 != 0) {
        std::cerr << "A valid UM file must be a size divisible by 4 bytes\n";
        return EXIT_FAILURE;
    }
    
    std::ifstream file(argv[1], std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << argv[1] << std::endl;
        return EXIT_FAILURE;
    }
    
    // Parse command line arguments
    bool useJIT = true;
    if (argc == 3) {
        std::string arg(argv[2]);
        if (arg == "--print-ir") {
            useJIT = false;
        } else if (arg == "--jit") {
            useJIT = true;
        } else {
            std::cerr << "Unknown argument: " << arg << std::endl;
            return EXIT_FAILURE;
        }
    }
    
    ProgramLoader loader;
    loader.load_file(file);
    
    // At this point, turn things over to the compiler
    Compiler compiler;

    compiler.createInstructionLabels(loader.program.size());

    // Create initial branch to first instruction (add this line)
    compiler.jumpToFirstInstruction();

    for (size_t i = 0; i < loader.program.size(); i++) {
        uint32_t word = loader.program[i];
        compiler.compileInstruction(word);
    }

    // Adding this here to avoid putting a terminating block in the middle of a program
    compiler.finishProgram();
    
    if (useJIT) {
        // Execute using JIT
        if (auto err = compiler.executeJIT()) {
            std::cerr << "JIT execution failed: " << toString(std::move(err)) << std::endl;
            return EXIT_FAILURE;
        }
    } else {
        // Just print the IR
        std::cout << "Generated LLVM IR:\n";
        compiler.printIR();
    }
    
    return 0;
}