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

void Compiler::finishProgram() {
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

// changes:
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
        // Insert the label for this instruction and jump to it
    if (currentInstructionIndex < instructionLabels.size()) {
        // Create a branch to the label
        builder.CreateBr(instructionLabels[currentInstructionIndex]);
        // Set insert point to the label
        builder.SetInsertPoint(instructionLabels[currentInstructionIndex]);
    }

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

    }

        // At the end of the method, increment the instruction index
    currentInstructionIndex++;
}

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


