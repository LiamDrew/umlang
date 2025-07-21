#pragma once
#include <cstdint>
#include <vector>

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
        llvm::Function* getcharFunc;

        std::vector<llvm::BasicBlock*> instructionBlocks;
        llvm::BasicBlock* dispatchBlock = nullptr;
        llvm::Value* nextInstructionPtr = nullptr;  // Points to next instruction to execute
        size_t currentInstructionIndex = 0;
        
        void createDispatchBlock();
        void jumpToDispatch();

        // llvm::Function* mapFunc;
        // llvm::Function* unmapFunc;

        // Execution engine
        std::unique_ptr<llvm::orc::LLJIT> jit;

 
        // Load Register
        void setRegisterValues(int reg, int value);

        // Conditional Move
        void conditionalMove(int regA, int regB, int regC);

        // Segmented Load
        // Segmented Store
        
        // Map segment
        // Unmap segment
        
        // Addition
        void compileAddition(int regA, int regB, int regC);
        // Multiplication
        void compileMul(int regA, int regB, int regC);
        // Division
        void compileDiv(int regA, int regB, int regC);
        
        // NAND
        void compileNand(int regA, int regB, int recC);
        
        // Print register
        void printRegister(int regC);
        // read into register
        void readIntoRegister(int regC);
        
        // Load program
        llvm::Error initializeJIT();


        std::vector<llvm::BasicBlock*> instructionLabels;

        llvm::BasicBlock* haltBlock = nullptr;

        void createHaltBlock();
            
    public:
        void createInstructionLabels(size_t numInstructions);

        void jumpToFirstInstruction();


        Compiler();
        // Compiler(size_t programSize);

        void compileLoadProgram(int regB, int regC);

        // Halt instruction compilation must be kept public for now
        void finishProgram();

        void compileInstruction(uint32_t word);

        void printIR();

        llvm::Error executeJIT();

};