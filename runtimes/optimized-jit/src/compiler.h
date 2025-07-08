#pragma once
#include <cstdint>
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/TargetParser/Host.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/JITSymbol.h"
#include "llvm/ExecutionEngine/Orc/LLJIT.h"
#include "llvm/ExecutionEngine/Orc/ThreadSafeModule.h"
#include "llvm/Support/Error.h"
#include "llvm/IR/Verifier.h"





class Compiler {
    private:
        llvm::LLVMContext context;
        std::unique_ptr<llvm::Module> module;
        llvm::IRBuilder<> builder;
        llvm::Function* currentFunction;
        llvm::Value* registers[8];
        llvm::Function* putcharFunc;
        // llvm::Function* getcharFunc;
        // llvm::Function* mapFunc;
        // llvm::Function* unmapFunc;

        // Execution engine
        std::unique_ptr<llvm::orc::LLJIT> jit;

        // set initial register values
        // UM addition
        // Load Register
        void setRegisterValues(int reg, int value);

        // Conditional Move

        // Segmented Load
        // Segmented Store
        // Addition
        // Multiplication
        // Division
        // NAND
        // Halt
        // Map segment
        // Unmap segment
        // Print register
        // read into register
        // Load program
        void compileAddition(int regA, int regB, int regC);

        void finishProgram();
        void printRegister(int regC);

        llvm::Error initializeJIT();
    
    public:
        Compiler();

        void compileInstruction(uint32_t word);

        void printIR();

        llvm::Error executeJIT();

};