#include "compiler.h"
#include <iostream>
#include "llvm/ExecutionEngine/Orc/LLJIT.h"
#include "llvm/ExecutionEngine/Orc/ThreadSafeModule.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Transforms/Utils/Cloning.h"

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

    // llvm::FunctionType* getcharType = llvm::FunctionType::get(
    //     llvm::Type::getInt32Ty(context),
    //     {llvm::Type::getVoidTy(context)},
    //     false
    // );

    // getcharFunc = llvm::Function::Create(
    //     getcharType,
    //     llvm::Function::ExternalLinkage,
    //     "putchar",
    //     module.get()
    // );



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
    llvm::orc::SymbolMap symbols;
    symbols[jit->mangleAndIntern("putchar")] = llvm::orc::ExecutorSymbolDef(putcharAddr, llvm::JITSymbolFlags::Exported);

    if (auto err = JD.define(llvm::orc::absoluteSymbols(symbols))) {
        return err;
    }

    return llvm::Error::success();
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

    // std::cout << "Compiled: reg[" << regA << "] = reg[" << regB << "] + reg[" << regC << "]\n";
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

        case 7: {
            // Finishing program
            finishProgram();
            break;
        }

        case 3: {
            // Adding two numbers
            compileAddition(a, b, c);
            break;
        }

        case 10: {
            // add some IR to print out a particular register.
            printRegister(c);
            break;
        }
    }
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

