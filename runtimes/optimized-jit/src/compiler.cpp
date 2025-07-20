#include "compiler.h"
#include <iostream>
#include "llvm/ExecutionEngine/Orc/LLJIT.h"
#include "llvm/ExecutionEngine/Orc/ThreadSafeModule.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include <cassert>

// Compiler::Compiler(size_t programSize) : builder(context), programSize(programSize)
Compiler::Compiler() : builder(context)

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

    //response B changes:
    // After initializing registers, add:
    nextInstructionPtr = builder.CreateAlloca(
        llvm::Type::getInt32Ty(context),
        nullptr,
        "next_instruction_ptr"
    );
    
    // Initialize to 0 (start at first instruction)
    builder.CreateStore(
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0),
        nextInstructionPtr
    );

    // Initialize JIT
    if (auto err = initializeJIT()) {
        std::cerr << "Failed to init JIT" << std::endl;
    }

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

void Compiler::createInstructionLabels(size_t numInstructions) {
    // Create a label for each instruction
    for (size_t i = 0; i < numInstructions; i++) {
        std::string labelName = "instr_" + std::to_string(i);
        llvm::BasicBlock* label = llvm::BasicBlock::Create(
            context,
            labelName,
            currentFunction
        );
        instructionLabels.push_back(label);
    }
}

void Compiler::compileInstruction(uint32_t word)
{
    // // response B
    //     // For the first instruction, we need to branch from entry to instr_0
    // if (currentInstructionIndex == 0 && builder.GetInsertBlock()->getName() == "entry") {
    //     builder.CreateBr(instructionLabels[0]);
    // }
    
    // Set insert point to the current instruction's label
    builder.SetInsertPoint(instructionLabels[currentInstructionIndex]);
    

    uint32_t opcode = (word >> 28) & 0xF;

    uint32_t a = 0;

    uint32_t b = 0, c = 0;

    c = word & 0x7;
    b = (word >> 3) & 0x7;
    a = (word >> 6) & 0x7;

    // std::cerr << "Opcode is: " << opcode << std::endl;
    // Keeping track of if the instruction we are compiling adds a terminator
    // bool hasTerminator = false;

    bool addedTerminator = false;


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
            // // Finishing program
            // // The nature of LLVM creates issues when putting the terminator here
            // // finishProgram();
            // builder.CreateBr(haltBlock);
            // // transfersControl = true;
            // // break;
            // currentInstructionIndex++;
            // return;
            // // break;

                // Halt instruction
            if (!haltBlock) {
                createHaltBlock();
            }
            builder.CreateBr(haltBlock);
            addedTerminator = true;
            break;

            // currentInstructionIndex++;
            // return;  // Don't add another branch after this
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
            // std::cerr << "Trying to load program\n";
            // assert(false);
            compileLoadProgram(b, c);
            addedTerminator = true;  // jumpToDispatch adds the terminator

            // hasTerminator = true;
            // transfersControl = true;  // This transfers control to dispatch
            break;
            // currentInstructionIndex++;
            // return;

        }

    }

    // Only add branch to next instruction if:
    // 1. We didn't already add a terminator
    // 2. There is a next instruction
    if (!addedTerminator && currentInstructionIndex + 1 < instructionLabels.size()) {
        builder.CreateBr(instructionLabels[currentInstructionIndex + 1]);
    }
    
    currentInstructionIndex++;
}

// Original
void Compiler::compileLoadProgram(int regB, int regC) {
    // Load the target instruction index from register B
    llvm::Value* targetIndex = builder.CreateLoad(
        llvm::Type::getInt32Ty(context),
        registers[regC],
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

//response B:
void Compiler::createDispatchBlock() {
    dispatchBlock = llvm::BasicBlock::Create(
        context, 
        "dispatch", 
        currentFunction
    );
    
    // Save current insert point
    auto savedBlock = builder.GetInsertBlock();
    auto savedPoint = builder.GetInsertPoint();
    
    // Set insert point to dispatch block
    builder.SetInsertPoint(dispatchBlock);
    
    // Load the next instruction index
    llvm::Value* index = builder.CreateLoad(
        llvm::Type::getInt32Ty(context),
        nextInstructionPtr,
        "next_instr_index"
    );
    
    // Create switch instruction (jump table)
    // Default case will be the first instruction (or you could create an error block)
    llvm::SwitchInst* jumpTable = builder.CreateSwitch(
        index, 
        instructionLabels[0],  // default destination
        instructionLabels.size()  // number of cases
    );
    
    // Add cases for each instruction
    for (size_t i = 0; i < instructionLabels.size(); i++) {
        llvm::ConstantInt* caseValue = llvm::ConstantInt::get(
            llvm::Type::getInt32Ty(context), 
            i
        );
        jumpTable->addCase(caseValue, instructionLabels[i]);
    }
    
    // Restore insert point
    builder.SetInsertPoint(savedBlock, savedPoint);
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

void Compiler::jumpToFirstInstruction() {
    if (!instructionLabels.empty()) {
        builder.CreateBr(instructionLabels[0]);
    }
}

void Compiler::finishProgram() {
    //original
    // llvm::Value* returnValue = builder.CreateLoad(
    //     llvm::Type::getInt32Ty(context),
    //     registers[0],
    //     "return_val"
    // );
    // builder.CreateRet(returnValue);

    // response A
    //     // Branch to the halt block from the last instruction
    // if (!instructionLabels.empty() && currentInstructionIndex > 0) {
    //     builder.SetInsertPoint(instructionLabels[currentInstructionIndex-1]);
    //     if (!builder.GetInsertBlock()->getTerminator()) {
    //         builder.CreateBr(haltBlock);
    //     }
    // }

    // response B
    // If we're not already in the halt block, branch to it
    if (builder.GetInsertBlock() != haltBlock) {
        if (!haltBlock) {
            createHaltBlock();
        }
        
        // Only create a branch if the current block doesn't have a terminator
        if (!builder.GetInsertBlock()->getTerminator()) {
            builder.CreateBr(haltBlock);
        }
    }
}

void Compiler::createHaltBlock() {
    haltBlock = llvm::BasicBlock::Create(
        context,
        "halt",
        currentFunction
    );

    auto savedBlock = builder.GetInsertBlock();
    auto savedPoint = builder.GetInsertPoint();

    // set insert point to halt block
    builder.SetInsertPoint(haltBlock);

    // add return instruction
    llvm::Value* returnValue = builder.CreateLoad(
        llvm::Type::getInt32Ty(context),
        registers[0],
        "return_val"
    );
    builder.CreateRet(returnValue);

    builder.SetInsertPoint(savedBlock, savedPoint);
}

// original
// void Compiler::createDispatchBlock() {
//     dispatchBlock = llvm::BasicBlock::Create(
//         context, 
//         "dispatch", 
//         currentFunction
//     );
    
//     // We'll populate this later in finishProgram()
// }

// response A
    // Only branch to next instruction if this instruction doesn't transfer control
// and we're not at the last instruction
// if (!transfersControl && currentInstructionIndex + 1 < instructionLabels.size()) {
//     builder.CreateBr(instructionLabels[currentInstructionIndex + 1]);
// }

//     // Branch to next instruction only if we don't transfer control
// if (!transfersControl && currentInstructionIndex + 1 < instructionLabels.size()) {
//     builder.CreateBr(instructionLabels[currentInstructionIndex + 1]);
// }

// // response B:
// void Compiler::finishProgram() {
//     // Create dispatch block if it hasn't been created yet
//     if (!dispatchBlock && !instructionLabels.empty()) {
//         // not really a fan of this conditional logic
//         // assert(false);
//         createDispatchBlock();
//     }
    
//     // ... existing return logic ...
//     llvm::Value* returnValue = builder.CreateLoad(
//         llvm::Type::getInt32Ty(context),
//         registers[0],
//         "return_val"
//     );
//     builder.CreateRet(returnValue);
// }

