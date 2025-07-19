#include "compiler.h"
#include <iostream>
#include "llvm/ExecutionEngine/Orc/LLJIT.h"
#include "llvm/ExecutionEngine/Orc/ThreadSafeModule.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include <cassert>

Compiler::Compiler() : builder(context)
// Compiler::Compiler(size_t programSize) : builder(context), programSize(programSize)

{
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();

    module = std::make_unique<llvm::Module>("um_program", context);
    module->setTargetTriple("arm64-apple-macosx15.0.0");

    llvm::FunctionType* funcType = llvm::FunctionType::get(
        llvm::Type::getInt32Ty(context),
        false
    );

    currentFunction = llvm::Function::Create(
        funcType,
        llvm::Function::ExternalLinkage,
        "main",
        module.get()
    );

    llvm::BasicBlock* entryBlock = llvm::BasicBlock::Create(
        context,
        "entry",
        currentFunction
    );
    builder.SetInsertPoint(entryBlock);

    llvm::FunctionType* putcharType = llvm::FunctionType::get(
        llvm::Type::getInt32Ty(context), // returns int
        {llvm::Type::getInt32Ty(context)}, // takes int parameter
        false // not variadic... what does that mean
    );

    putcharFunc = llvm::Function::Create(
        putcharType,
        llvm::Function::ExternalLinkage,
        "putchar",
        module.get()
    );

    llvm::FunctionType* getcharType = llvm::FunctionType::get(
        llvm::Type::getInt32Ty(context),
        {}, // takes no parameters
        false
    );

    getcharFunc = llvm::Function::Create(
        getcharType,
        llvm::Function::ExternalLinkage,
        "getchar",
        module.get()
    );



    // initialize the registers as allocas (stack variables)
    for (int i = 0; i < 8; i++) {
        registers[i] = builder.CreateAlloca(
            llvm::Type:: getInt32Ty(context),
            nullptr,
            "reg" + std::to_string(i)
        );

        // Initialize all registers to 0
        setRegisterValues(i, 0);
    }

    // Initialize JIT
    if (auto err = initializeJIT()) {
        std::cerr << "Failed to init JIT" << std::endl;
    }

    // IDK about this mods.. this should be simpler

    // // Create a variable to track the next instruction to execute
    // nextInstructionPtr = builder.CreateAlloca(
    //     llvm::Type::getInt32Ty(context),
    //     nullptr,
    //     "next_instruction"
    // );
    
    // // Initialize to instruction 0
    // builder.CreateStore(
    //     llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0),
    //     nextInstructionPtr
    // );
}

llvm::Error Compiler::initializeJIT()
{
    auto jitOrErr = llvm::orc::LLJITBuilder().create();
    if (not jitOrErr) {
        return jitOrErr.takeError();
    }

    jit = std::move(*jitOrErr);

    // C standard library stuff I don't understand at all
    auto &ES = jit->getExecutionSession();
    auto &JD = jit->getMainJITDylib(); // what?

    auto putcharAddr = llvm::orc::ExecutorAddr::fromPtr(reinterpret_cast<void*>(&putchar));
    auto getcharAddr = llvm::orc::ExecutorAddr::fromPtr(reinterpret_cast<void*>(&getchar));

    llvm::orc::SymbolMap symbols;
    symbols[jit->mangleAndIntern("putchar")] = llvm::orc::ExecutorSymbolDef(putcharAddr, llvm::JITSymbolFlags::Exported);
    symbols[jit->mangleAndIntern("getchar")] = llvm::orc::ExecutorSymbolDef(getcharAddr, llvm::JITSymbolFlags::Exported);

    if (auto err = JD.define(llvm::orc::absoluteSymbols(symbols))) {
        return err;
    }

    return llvm::Error::success();
}

void Compiler::printRegister(int regC)
{
    llvm::Value* charValue = builder.CreateLoad(
        llvm::Type::getInt32Ty(context),
        registers[regC],
        "putchar_value"
    );

    // call putchar with register value
    builder.CreateCall(putcharFunc, {charValue}, "putchar_call");
}

void Compiler::readIntoRegister(int regC)
{
    llvm::Value* inputChar = builder.CreateCall(getcharFunc, {}, "getchar_call");

    builder.CreateStore(inputChar, registers[regC]);
}

// void Compiler::finishProgram() {
//     // First, fix up all the fall-through branches
//     for (size_t i = 0; i < instructionBlocks.size() - 1; i++) {
//         llvm::BasicBlock* block = instructionBlocks[i];
        
//         // Check if this block already has a terminator
//         if (!block->getTerminator()) {
//             // Add fall-through to next instruction
//             builder.SetInsertPoint(block);
//             builder.CreateBr(instructionBlocks[i + 1]);
//         }
//     }
    
//     // Handle the last instruction block if it doesn't have a terminator
//     if (!instructionBlocks.empty()) {
//         llvm::BasicBlock* lastBlock = instructionBlocks.back();
//         if (!lastBlock->getTerminator()) {
//             builder.SetInsertPoint(lastBlock);
//             llvm::Value* returnValue = builder.CreateLoad(
//                 llvm::Type::getInt32Ty(context),
//                 registers[0],
//                 "return_val"
//             );
//             builder.CreateRet(returnValue);
//         }
//     }
    
//     // Now set up the dispatch block
//     if (dispatchBlock) {
//         builder.SetInsertPoint(dispatchBlock);
        
//         llvm::Value* nextInstr = builder.CreateLoad(
//             llvm::Type::getInt32Ty(context),
//             nextInstructionPtr,
//             "next_instr"
//         );
        
//         // Create switch statement
//         llvm::BasicBlock* defaultBlock = llvm::BasicBlock::Create(
//             context, 
//             "invalid_jump", 
//             currentFunction
//         );
        
//         llvm::SwitchInst* switchInst = builder.CreateSwitch(
//             nextInstr, 
//             defaultBlock, 
//             instructionBlocks.size()
//         );
        
//         // Add case for each instruction
//         for (size_t i = 0; i < instructionBlocks.size(); i++) {
//             llvm::ConstantInt* caseValue = llvm::ConstantInt::get(
//                 llvm::Type::getInt32Ty(context), 
//                 i
//             );
//             switchInst->addCase(caseValue, instructionBlocks[i]);
//         }
        
//         // Handle invalid jump (just return 0)
//         builder.SetInsertPoint(defaultBlock);
//         builder.CreateRet(llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0));
//     }
    
//     // If no dispatch block was created, just return from the entry block
//     if (instructionBlocks.empty()) {
//         builder.CreateRet(llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0));
//     }
// }

// void Compiler::finishProgram() {
//     llvm::Value* returnValue = builder.CreateLoad(
//         llvm::Type::getInt32Ty(context),
//         registers[0],
//         "return_val"
//     );
//     builder.CreateRet(returnValue);
// }

void Compiler::finishProgram() {
    // Set up the halt block
    // builder.SetInsertPoint(haltBlock);
    
    llvm::Value* returnValue = builder.CreateLoad(
        llvm::Type::getInt32Ty(context),
        registers[0],
        "return_val"
    );
    builder.CreateRet(returnValue);
}

void Compiler::printIR() {
    module->print(llvm::outs(), nullptr);
}

// set initial register values
void Compiler::setRegisterValues(int reg, int value) {
    llvm::Value* constant = llvm::ConstantInt::get(
        llvm::Type::getInt32Ty(context),
        value
    );
    builder.CreateStore(constant, registers[reg]);
}

// UM addition
void Compiler::compileAddition(int regA, int regB, int regC)
{
    llvm::Value* valueB = builder.CreateLoad(
        llvm::Type::getInt32Ty(context),
        registers[regB],
        "load_regB"
    );

    llvm::Value* valueC = builder.CreateLoad(
        llvm::Type::getInt32Ty(context),
        registers[regC],
        "load_regC"
    );

    llvm::Value* result = builder.CreateAdd(valueB, valueC, "add_result");

    builder.CreateStore(result, registers[regA]);
}

void Compiler::compileMul(int regA, int regB, int regC) {
    llvm::Value* valueB = builder.CreateLoad(
        llvm::Type::getInt32Ty(context),
        registers[regB],
        "load_regB"
    );

    llvm::Value* valueC = builder.CreateLoad(
        llvm::Type::getInt32Ty(context),
        registers[regC],
        "load_regC"
    );

    llvm::Value* result = builder.CreateMul(valueB, valueC, "mul_result");

    builder.CreateStore(result, registers[regA]);
}

void Compiler::compileDiv(int regA, int regB, int regC) {
    llvm::Value *valueB = builder.CreateLoad(
        llvm::Type::getInt32Ty(context),
        registers[regB],
        "load_regB"
    );

    llvm::Value* valueC = builder.CreateLoad(
        llvm::Type::getInt32Ty(context),
        registers[regC],
        "load_regC"
    );

    llvm::Value* result = builder.CreateUDiv(valueB, valueC, "div_result");

    builder.CreateStore(result, registers[regA]);
}

void Compiler::conditionalMove(int regA, int regB, int regC) {

    llvm::Value *valueC = builder.CreateLoad(
        llvm::Type::getInt32Ty(context),
        registers[regC],
        "load_regC"
    );

    llvm::Value *valueB = builder.CreateLoad(
        llvm::Type::getInt32Ty(context),
        registers[regB],
        "load_regB"
    );

    llvm::Value *currentA = builder.CreateLoad(
        llvm::Type::getInt32Ty(context),
        registers[regA],
        "load_regC"
    );

    // making condition
    llvm::Value *zero = llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0);
    llvm::Value *condition = builder.CreateICmpNE(valueC, zero, "cmp_regC_zero");

    // select
    llvm::Value *result = builder.CreateSelect(condition, valueB, currentA, 
        "conditional_move");

    builder.CreateStore(result, registers[regA]);
}

void Compiler::compileNand(int regA, int regB, int regC) {
    llvm::Value *valueB = builder.CreateLoad(
        llvm::Type::getInt32Ty(context),
        registers[regB],
        "load_regB"
    );

    llvm::Value *valueC = builder.CreateLoad(
        llvm::Type::getInt32Ty(context),
        registers[regC],
        "load_regC"
    );

    llvm::Value *andResult = builder.CreateAnd(valueB, valueC, "and_result");

    llvm::Value *nandResult = builder.CreateNot(andResult, "nand_result");

    builder.CreateStore(nandResult, registers[regA]);
}

void Compiler::compileInstruction(uint32_t word)
{
    uint32_t opcode = (word >> 28) & 0xF;

    uint32_t a = 0;

    uint32_t b = 0, c = 0;

    c = word & 0x7;
    b = (word >> 3) & 0x7;
    a = (word >> 6) & 0x7;

    // std::cerr << "Opcode is: " << opcode << std::endl;

    switch (opcode) {
        case 13: {
            // Load register
            a = (word >> 25) & 0x7;
            uint32_t val = word & 0x1FFFFFF;
            setRegisterValues(a, val);
            break;
        }

        case 0: {
            conditionalMove(a, b, c);
            break;
        }

        case 3: {
            // Adding two numbers
            compileAddition(a, b, c);
            break;
        }

        case 4: {
            compileMul(a, b, c);
            break;
        }

        case 5: {
            compileDiv(a, b, c);
            break;
        }

        case 6: {
            compileNand(a, b, c);
            break;
        }

        case 7: {
            // Finishing program
            // The nature of LLVM creates issues when putting the terminator here
            // finishProgram();
            // builder.CreateBr(haltBlock);
            break;
        }

        case 10: {
            // add some IR to print out a particular register.
            printRegister(c);
            break;
        }

        case 11: {
            readIntoRegister(c);
            break;
        }

        case 12: {
            // load program
            std::cerr << "Trying to load program\n";
            // assert(false);
            compileLoadProgram(b, c);
            break;
        }

        // // Create fall-through to next instruction (if not last)
        // if (instructionIndex + 1 < instructionBlocks.size()) {
        //     builder.CreateBr(instructionBlocks[instructionIndex + 1]);
        // } else {
        //     // Last instruction - jump to exit
        //     builder.CreateBr(exitBlock);
        // }

        // currentInstructionIndex++;
        // if (currentInstructionIndex < programSize) {
        //     // Only create fall-through if this wasn't a terminal instruction
        //     if (opcode != 7 && opcode != 12) {
        //         builder.CreateBr(instructionBlocks[currentInstructionIndex]);
        //     }
        //     builder.SetInsertPoint(instructionBlocks[currentInstructionIndex]);
        // }


    }
}

// void Compiler::compileInstruction(uint32_t word) {
//     // Create a basic block for this instruction
//     std::string blockName = "inst_" + std::to_string(currentInstructionIndex);
//     llvm::BasicBlock* currentBlock = llvm::BasicBlock::Create(
//         context, 
//         blockName, 
//         currentFunction
//     );
//     instructionBlocks.push_back(currentBlock);
    
//     // If this is the first instruction, branch to it from entry
//     if (currentInstructionIndex == 0) {
//         builder.CreateBr(currentBlock);
//     }
    
//     // Set insertion point to this block
//     builder.SetInsertPoint(currentBlock);
    
//     // Compile the actual instruction
//     uint32_t opcode = (word >> 28) & 0xF;
    
//     uint32_t a = 0, b = 0, c = 0;
//     c = word & 0x7;
//     b = (word >> 3) & 0x7;
//     a = (word >> 6) & 0x7;

//     switch (opcode) {
//         case 13: {
//             // Load register
//             a = (word >> 25) & 0x7;
//             uint32_t val = word & 0x1FFFFFF;
//             setRegisterValues(a, val);
//             break;
//         }
        
//         case 0: {
//             conditionalMove(a, b, c);
//             break;
//         }
        
//         case 3: {
//             compileAddition(a, b, c);
//             break;
//         }
        
//         case 4: {
//             compileMul(a, b, c);
//             break;
//         }
        
//         case 5: {
//             compileDiv(a, b, c);
//             break;
//         }
        
//         case 6: {
//             compileNand(a, b, c);
//             break;
//         }
        
//         case 7: {
//             // Halt - don't add fall-through
//             currentInstructionIndex++;
//             return;
//         }
        
//         case 10: {
//             printRegister(c);
//             break;
//         }
        
//         case 11: {
//             readIntoRegister(c);
//             break;
//         }
        
//         case 12: {
//             // Load program - jump to dispatch
//             compileLoadProgram(b, c);
//             currentInstructionIndex++;
//             return;  // Don't add fall-through
//         }
        
//         default:
//             break;
//     }
    
//     // For most instructions, just fall through to next instruction
//     // We'll fix this up later if needed
//     currentInstructionIndex++;
// }

void Compiler::compileLoadProgram(int regB, int regC) {
    // Load the target instruction index from register B
    llvm::Value* targetIndex = builder.CreateLoad(
        llvm::Type::getInt32Ty(context),
        registers[regB],
        "load_target_index"
    );
    
    // Store it as the next instruction to execute
    builder.CreateStore(targetIndex, nextInstructionPtr);
    
    // Jump to dispatch
    jumpToDispatch();
}

void Compiler::jumpToDispatch() {
    if (!dispatchBlock) {
        createDispatchBlock();
    }
    builder.CreateBr(dispatchBlock);
}

void Compiler::createDispatchBlock() {
    dispatchBlock = llvm::BasicBlock::Create(
        context, 
        "dispatch", 
        currentFunction
    );
    
    // We'll populate this later in finishProgram()
}

llvm::Error Compiler::executeJIT()
{
    // Verify the module before JIT compilation
    std::string errorStr;
    llvm::raw_string_ostream errorStream(errorStr);
    if (llvm::verifyModule(*module, &errorStream)) {
        return llvm::make_error<llvm::StringError>(
            "Module verification failed: " + errorStr,
            llvm::inconvertibleErrorCode()
        );
    }
    
    // Create a new context for the ThreadSafeModule
    auto newContext = std::make_unique<llvm::LLVMContext>();
    
    // Clone the module to the new context
    auto clonedModule = llvm::CloneModule(*module);
    
    // Create a ThreadSafeModule and add it to the JIT
    auto tsm = llvm::orc::ThreadSafeModule(std::move(clonedModule), std::move(newContext));
    
    if (auto err = jit->addIRModule(std::move(tsm))) {
        return err;
    }
    
    // Look up the main function
    auto mainSymbol = jit->lookup("main");
    if (!mainSymbol) {
        return mainSymbol.takeError();
    }
    
    // Get the address and cast it to a function pointer
    auto mainAddr = mainSymbol->getValue();
    auto mainFunc = reinterpret_cast<int(*)()>(mainAddr);
    
    // Execute the function
    // std::cout << "Executing JIT compiled program..." << std::endl;
    int result = mainFunc();
    // std::cout << "\nProgram finished with exit code: " << result << std::endl;
    
    return llvm::Error::success();
}

// llvm::Error Compiler::executeJIT()
// {
//     //verify module
//     std::string errorStr;
//     llvm::raw_string_ostream errorStream(errorStr);
//     if (llvm::verifyModule(*module, &errorStream)) {
//         return llvm::make_error<llvm::StringError>("Module verification failed: " + errorStr, llvm::inconvertibleErrorCode());
//     }

//     auto tsm = llvm::orc::ThreadSafeModule(std::move(module), std::make_unique<llvm::LLVMContext>(std::move(context)));

//     if (auto err = jit->addIRModule(std::move(tsm))) {
//         return err;
//     }

//     // look up main function
//     auto mainSymbol = jit->lookup("main");
//     if (not mainSymbol) {
//         return mainSymbol.takeError();
//     }

//     auto mainAddr = mainSymbol->getValue();
//     auto mainFunc = reinterpret_cast<int(*)()>(mainAddr);

//     std::cout << "Executed JIT compiled" << std::endl;
//     int result = mainFunc();
//     std::cout << "\nProgram finished with exit code: " << result << std::endl;

//     return llvm::Error::success();
// }
