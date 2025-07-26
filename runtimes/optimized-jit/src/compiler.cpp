#include "compiler.hpp"
#include <iostream>
#include "llvm/ExecutionEngine/Orc/LLJIT.h"
#include "llvm/ExecutionEngine/Orc/ThreadSafeModule.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include <cassert>

#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/StandardInstrumentations.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Utils.h"

#include "llvm/Transforms/Utils/Mem2Reg.h"
// #include "llvm/Transforms/Utils/SimplifyCFGOptions.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"



extern "C" {
    #include "virt.h"
}

// Compiler::Compiler() : builder(context)

// {
//     llvm::InitializeNativeTarget();
//     llvm::InitializeNativeTargetAsmPrinter();
//     llvm::InitializeNativeTargetAsmParser();

//     module = std::make_unique<llvm::Module>("um_program", context);
//     module->setTargetTriple("arm64-apple-macosx15.0.0");

//     // llvm::FunctionType* funcType = llvm::FunctionType::get(
//     //     llvm::Type::getInt32Ty(context),
//     //     {llvm::PointerType::getUnqual(context)},
//     //     false
//     // );

//     // Change this function type to take no parameters
//     llvm::FunctionType* funcType = llvm::FunctionType::get(
//         llvm::Type::getInt32Ty(context),
//         {}, // No parameters
//         false
//     );

//     currentFunction = llvm::Function::Create(
//         funcType,
//         llvm::Function::ExternalLinkage,
//         "main",
//         module.get()
//     );

//     // set up usable mem to be used
//     llvm::Function::arg_iterator args = currentFunction->arg_begin();
//     llvm::Value* usableMemParam = &*args;
//     usableMemParam->setName("usable_mem");

//     llvm::BasicBlock* entryBlock = llvm::BasicBlock::Create(
//         context,
//         "entry",
//         currentFunction
//     );
//     builder.SetInsertPoint(entryBlock);

//     // usableMemPtr = builder.CreateAlloca(
//     //     llvm::PointerType::getUnqual(context),
//     //     nullptr,
//     //     "usable_mem_ptr"
//     // );
//     // builder.CreateStore(usableMemParam, usableMemPtr);

//     // Remove the parameter handling since we don't need it
//     // Just create the usableMemPtr as a null pointer for now
//     usableMemPtr = builder.CreateAlloca(
//         llvm::PointerType::getUnqual(context),
//         nullptr,
//         "usable_mem_ptr"
//     );
    
//     // Initialize with null - you can modify this later if needed
//     llvm::Value* nullPtr = llvm::ConstantPointerNull::get(
//         llvm::PointerType::getUnqual(context)
//     );
//     builder.CreateStore(nullPtr, usableMemPtr);

//     llvm::FunctionType* putcharType = llvm::FunctionType::get(
//         llvm::Type::getInt32Ty(context), // returns int
//         {llvm::Type::getInt32Ty(context)}, // takes int parameter
//         false // not variadic... what does that mean
//     );

//     putcharFunc = llvm::Function::Create(
//         putcharType,
//         llvm::Function::ExternalLinkage,
//         "putchar",
//         module.get()
//     );

//     llvm::FunctionType* getcharType = llvm::FunctionType::get(
//         llvm::Type::getInt32Ty(context),
//         {}, // takes no parameters
//         false
//     );

//     getcharFunc = llvm::Function::Create(
//         getcharType,
//         llvm::Function::ExternalLinkage,
//         "getchar",
//         module.get()
//     );

//     llvm::FunctionType *vsCallocType = llvm::FunctionType::get(
//         llvm::Type::getInt32Ty(context),
//         {llvm::Type::getInt32Ty(context)},
//         false // not variadic
//     );

//     vsCallocFunc = llvm::Function::Create(
//         vsCallocType,
//         llvm::Function::ExternalLinkage,
//         "vs_calloc",
//         module.get()
//     );

//     llvm::FunctionType *vsFreeType = llvm::FunctionType::get(
//         llvm::Type::getVoidTy(context), //don't need void return type
//         {llvm::Type::getInt32Ty(context)},
//         false
//     );

//     vsFreeFunc = llvm::Function::Create(
//         vsFreeType,
//         llvm::Function::ExternalLinkage,
//         "vs_free",
//         module.get()
//     );



//     // initialize the registers as allocas (stack variables)
//     for (int i = 0; i < 8; i++) {
//         registers[i] = builder.CreateAlloca(
//             llvm::Type:: getInt32Ty(context),
//             nullptr,
//             "reg" + std::to_string(i)
//         );

//         // Initialize all registers to 0
//         setRegisterValues(i, 0);
//     }

//     //response B changes:
//     // After initializing registers, add:
//     nextInstructionPtr = builder.CreateAlloca(
//         llvm::Type::getInt32Ty(context),
//         nullptr,
//         "next_instruction_ptr"
//     );
    
//     // Initialize to 0 (start at first instruction)
//     builder.CreateStore(
//         llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0),
//         nextInstructionPtr
//     );

//     // Initialize JIT
//     if (auto err = initializeJIT()) {
//         std::cerr << "Failed to init JIT" << std::endl;
//     }

// }

Compiler::Compiler() : builder(context)
{
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();

    module = std::make_unique<llvm::Module>("um_program", context);
    module->setTargetTriple("arm64-apple-macosx15.0.0");

    // Function takes no parameters now
    llvm::FunctionType* funcType = llvm::FunctionType::get(
        llvm::Type::getInt32Ty(context),
        {}, // No parameters
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

    // Remove the problematic parameter handling
    usableMemPtr = builder.CreateAlloca(
        llvm::PointerType::getUnqual(context),
        nullptr,
        "usable_mem_ptr"
    );
    
    // Initialize with null
    llvm::Value* nullPtr = llvm::ConstantPointerNull::get(
        llvm::PointerType::getUnqual(context)
    );
    builder.CreateStore(nullPtr, usableMemPtr);

    // Set up external functions
    setupExternalFunctions(); // Make sure this method exists

    // Initialize registers
    for (int i = 0; i < 8; i++) {
        registers[i] = builder.CreateAlloca(
            llvm::Type::getInt32Ty(context),
            nullptr,
            "reg" + std::to_string(i)
        );
        setRegisterValues(i, 0);
    }

    // Initialize next instruction pointer
    nextInstructionPtr = builder.CreateAlloca(
        llvm::Type::getInt32Ty(context),
        nullptr,
        "next_instruction_ptr"
    );
    
    builder.CreateStore(
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0),
        nextInstructionPtr
    );

    // Initialize JIT
    if (auto err = initializeJIT()) {
        std::cerr << "Failed to init JIT" << std::endl;
    }
}

void Compiler::setupExternalFunctions()
{
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

    llvm::FunctionType *vsCallocType = llvm::FunctionType::get(
        llvm::Type::getInt32Ty(context),
        {llvm::Type::getInt32Ty(context)},
        false // not variadic
    );

    vsCallocFunc = llvm::Function::Create(
        vsCallocType,
        llvm::Function::ExternalLinkage,
        "vs_calloc",
        module.get()
    );

    llvm::FunctionType *vsFreeType = llvm::FunctionType::get(
        llvm::Type::getVoidTy(context), //don't need void return type
        {llvm::Type::getInt32Ty(context)},
        false
    );

    vsFreeFunc = llvm::Function::Create(
        vsFreeType,
        llvm::Function::ExternalLinkage,
        "vs_free",
        module.get()
    );
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
    auto vsCallocAddr = llvm::orc::ExecutorAddr::fromPtr(reinterpret_cast<void*>(&vs_calloc));
    auto vsFreeAddr = llvm::orc::ExecutorAddr::fromPtr(reinterpret_cast<void*>(&vs_free));

    llvm::orc::SymbolMap symbols;
    symbols[jit->mangleAndIntern("putchar")] = llvm::orc::ExecutorSymbolDef(putcharAddr, llvm::JITSymbolFlags::Exported);
    symbols[jit->mangleAndIntern("getchar")] = llvm::orc::ExecutorSymbolDef(getcharAddr, llvm::JITSymbolFlags::Exported);
    symbols[jit->mangleAndIntern("vs_calloc")] = llvm::orc::ExecutorSymbolDef(vsCallocAddr, llvm::JITSymbolFlags::Exported);
    symbols[jit->mangleAndIntern("vs_free")] = llvm::orc::ExecutorSymbolDef(vsFreeAddr, llvm::JITSymbolFlags::Exported);

    if (auto err = JD.define(llvm::orc::absoluteSymbols(symbols))) {
        return err;
    }

    return llvm::Error::success();
}


void Compiler::printIR() {
    module->print(llvm::outs(), nullptr);
}

void Compiler::runOptimizationPasses() {
    //     // Create the analysis managers
    // llvm::LoopAnalysisManager LAM;
    // llvm::FunctionAnalysisManager FAM;
    // llvm::CGSCCAnalysisManager CGAM;
    // llvm::ModuleAnalysisManager MAM;
    
    // // Create the pass builder
    // llvm::PassBuilder PB;
    
    // // Register all the basic analyses with the managers
    // PB.registerModuleAnalyses(MAM);
    // PB.registerCGSCCAnalyses(CGAM);
    // PB.registerFunctionAnalyses(FAM);
    // PB.registerLoopAnalyses(LAM);
    // PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);

    // // Create a lightweight function pass pipeline
    // llvm::FunctionPassManager FPM;
    
    // // Only add the most impactful passes for JIT code:
    // FPM.addPass(llvm::PromotePass());        // Convert allocas to registers (huge win for your register loads/stores)
    // FPM.addPass(llvm::InstCombinePass());    // Combine redundant instructions
    // FPM.addPass(llvm::SimplifyCFGPass());    // Simplify control flow
    
    // // Create module pass manager with just the function pass
    // llvm::ModulePassManager MPM;
    // MPM.addPass(llvm::createModuleToFunctionPassAdaptor(std::move(FPM)));
    
    // // Run the lightweight optimizations
    // MPM.run(*module, MAM);
// Simple approach: just run mem2reg on the function directly
    llvm::FunctionAnalysisManager FAM;
    llvm::PassBuilder PB;
    PB.registerFunctionAnalyses(FAM);
    
    // Create a minimal function pass manager
    llvm::FunctionPassManager FPM;
    FPM.addPass(llvm::PromotePass());  // This should be the correct name
    // FPM.addPass(llvm::InstCombinePass());
    
    // Run just on your main function
    FPM.run(*currentFunction, FAM);
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
    // Set insert point to the current instruction's label
    builder.SetInsertPoint(instructionLabels[currentInstructionIndex]);
    

    uint32_t opcode = (word >> 28) & 0xF;

    uint32_t a = 0;

    uint32_t b = 0, c = 0;

    c = word & 0x7;
    b = (word >> 3) & 0x7;
    a = (word >> 6) & 0x7;

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

        case 1: {
            // assert(false);
            compileLoad(a, b, c);
            break;

        }

        case 2: {
            // assert(false);
            compileStore(a, b, c);
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
            // Halt instruction
            if (!haltBlock) {
                createHaltBlock();
            }
            builder.CreateBr(haltBlock);
            addedTerminator = true;
            break;
        }

        case 8: {
            // Map segment
            compileMap(b, c);
            break;
        }

        case 9: {
            // Unmap segment
            compileUnmap(c);
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
            // std::cerr << "Trying to load program\n";
            // assert(false);
            compileLoadProgram(b, c);
            addedTerminator = true;  // jumpToDispatch adds the terminator
            break;
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

void Compiler::compileMap(int regB, int regC)
{
    llvm::Value* mapSize = builder.CreateLoad(
        llvm::Type::getInt32Ty(context),
        registers[regC],
        "load_map_size"
    );

    llvm::Value* mapResult = builder.CreateCall(vsCallocFunc, {mapSize}, "vs_calloc_call");

    builder.CreateStore(mapResult, registers[regB]);
}

void Compiler::compileUnmap(int regC)
{
    llvm::Value *freeAddr = builder.CreateLoad(
        llvm::Type::getInt32Ty(context),
        registers[regC],
        "unmap_addr"
    );

    // no name for void calls
    builder.CreateCall(vsFreeFunc, {freeAddr});
}

// void Compiler::compileLoad(int regA, int regB, int regC)
// {
//     // load segment id from register B
//     llvm::Value* segmentId = builder.CreateLoad(
//         llvm::Type::getInt32Ty(context),
//         registers[regB],
//         "load_segment_id"
//     );

//     // load the offset from register C
//     llvm::Value* offset = builder.CreateLoad(
//         llvm::Type::getInt32Ty(context),
//         registers[regC],
//         "load_offset"
//     );

//     // load usable memory base address
//     llvm::Value* usableMemBase = builder.CreateLoad(
//         llvm::PointerType::getUnqual(context),
//         usableMemPtr,
//         "usable_mem_base"
//     );

//     // calculate absolute address (check this)
//     llvm::Value *segmentPtr = builder.CreateGEP(
//         llvm::Type::getInt8Ty(context),
//         usableMemBase,
//         segmentId,
//         "segment_ptr"
//     );

//     // add the offset
//     llvm::Value* finalPtr = builder.CreateGEP(
//         llvm::Type::getInt8Ty(context),
//         segmentPtr,
//         offset,
//         "final_ptr"
//     );

//     // load value as uint32_t
//     llvm::Value* loadedValue = builder.CreateLoad(
//         llvm::Type::getInt32Ty(context),
//         finalPtr,
//         "loaded_value"
//     );

//     // store result in registerA
//     builder.CreateStore(loadedValue, registers[regA]);
// }


// Fixed pointer arithmetic for load operation
void Compiler::compileLoad(int regA, int regB, int regC)
{
    // load segment id from register B
    llvm::Value* segmentId = builder.CreateLoad(
        llvm::Type::getInt32Ty(context),
        registers[regB],
        "load_segment_id"
    );

    // load the offset from register C
    llvm::Value* offset = builder.CreateLoad(
        llvm::Type::getInt32Ty(context),
        registers[regC],
        "load_offset"
    );

    // load usable memory base address
    llvm::Value* usableMemBase = builder.CreateLoad(
        llvm::PointerType::getUnqual(context),
        usableMemPtr,
        "usable_mem_base"
    );

    // Cast to int32 pointer for proper arithmetic
    llvm::Value* int32BasePtr = builder.CreatePointerCast(
        usableMemBase,
        llvm::PointerType::get(llvm::Type::getInt32Ty(context), 0),
        "int32_base_ptr"
    );

    // Calculate segment address (segment_id * some_segment_size)
    // Assuming segments are stored as consecutive blocks
    llvm::Value* segmentPtr = builder.CreateGEP(
        llvm::Type::getInt32Ty(context),
        int32BasePtr,
        segmentId,
        "segment_ptr"
    );

    // Add the offset (now in terms of int32 elements)
    llvm::Value* finalPtr = builder.CreateGEP(
        llvm::Type::getInt32Ty(context),
        segmentPtr,
        offset,
        "final_ptr"
    );

    // Load the 32-bit value
    llvm::Value* loadedValue = builder.CreateLoad(
        llvm::Type::getInt32Ty(context),
        finalPtr,
        "loaded_value"
    );

    // Store result in register A
    builder.CreateStore(loadedValue, registers[regA]);
}


// void Compiler::compileStore(int regA, int regB, int regC)
// {
//     // load segmentID from register A
//     llvm::Value* segmentId = builder.CreateLoad(
//         llvm::Type::getInt32Ty(context),
//         registers[regA],
//         "store_segment_id"
//     );

//     // load offset from register B
//     llvm::Value* offset = builder.CreateLoad(
//         llvm::Type::getInt32Ty(context),
//         registers[regB],
//         "store_offset"
//     );

//     // load the value to store from register C
//     llvm::Value* valueToStore = builder.CreateLoad(
//         llvm::Type::getInt32Ty(context),
//         registers[regC],
//         "value_to_store"
//     );

//     llvm::Value* usableMemBase = builder.CreateLoad(
//         llvm::PointerType::getUnqual(context),
//         usableMemPtr,
//         "usable_mem_base"
//     );

//     llvm::Value* segmentPtr = builder.CreateGEP(
//         llvm::Type::getInt8Ty(context),
//         usableMemBase,
//         segmentId,
//         "segment_ptr"
//     );

//     llvm::Value* finalPtr = builder.CreateGEP(
//         llvm::Type::getInt8Ty(context),
//         segmentPtr,
//         offset,
//         "final_ptr"
//     );

//     builder.CreateStore(valueToStore, finalPtr);
// }


// Fixed pointer arithmetic for store operation
void Compiler::compileStore(int regA, int regB, int regC)
{
    // load segmentID from register A
    llvm::Value* segmentId = builder.CreateLoad(
        llvm::Type::getInt32Ty(context),
        registers[regA],
        "store_segment_id"
    );

    // load offset from register B
    llvm::Value* offset = builder.CreateLoad(
        llvm::Type::getInt32Ty(context),
        registers[regB],
        "store_offset"
    );

    // load the value to store from register C
    llvm::Value* valueToStore = builder.CreateLoad(
        llvm::Type::getInt32Ty(context),
        registers[regC],
        "value_to_store"
    );

    // load usable memory base address
    llvm::Value* usableMemBase = builder.CreateLoad(
        llvm::PointerType::getUnqual(context),
        usableMemPtr,
        "usable_mem_base"
    );

    // Cast to int32 pointer for proper arithmetic
    llvm::Value* int32BasePtr = builder.CreatePointerCast(
        usableMemBase,
        llvm::PointerType::get(llvm::Type::getInt32Ty(context), 0),
        "int32_base_ptr"
    );

    // Calculate segment address
    llvm::Value* segmentPtr = builder.CreateGEP(
        llvm::Type::getInt32Ty(context),
        int32BasePtr,
        segmentId,
        "segment_ptr"
    );

    // Add the offset
    llvm::Value* finalPtr = builder.CreateGEP(
        llvm::Type::getInt32Ty(context),
        segmentPtr,
        offset,
        "final_ptr"
    );

    // Store the value
    builder.CreateStore(valueToStore, finalPtr);
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
    // runOptimizationPasses();  // <-- dd this line here

    // Verify the module before JIT compilation
    std::string errorStr;
    llvm::raw_string_ostream errorStream(errorStr);
    if (llvm::verifyModule(*module, &errorStream)) {
        return llvm::make_error<llvm::StringError>(
            "Module verification failed: " + errorStr,
            llvm::inconvertibleErrorCode()
        );
    }
    
    // // Create a new context for the ThreadSafeModule
    // auto newContext = std::make_unique<llvm::LLVMContext>();
    
    // // Clone the module to the new context
    // auto clonedModule = llvm::CloneModule(*module);
    
    // // Create a ThreadSafeModule and add it to the JIT
    // auto tsm = llvm::orc::ThreadSafeModule(std::move(clonedModule), std::move(newContext));
    
    // if (auto err = jit->addIRModule(std::move(tsm))) {
    //     return err;
    // }
    
    // // Look up the main function
    // auto mainSymbol = jit->lookup("main");
    // if (!mainSymbol) {
    //     return mainSymbol.takeError();
    // }
    
    // // Get the address and cast it to a function pointer
    // auto mainAddr = mainSymbol->getValue();
    // auto mainFunc = reinterpret_cast<int(*)()>(mainAddr);
    
    // // Execute the function
    // // std::cout << "Executing JIT compiled program..." << std::endl;
    // int result = mainFunc();
    // // std::cout << "\nProgram finished with exit code: " << result << std::endl;
    
    // return llvm::Error::success();

        // Create ThreadSafeModule directly from your existing module and context
    // No cloning needed!

    // Create ThreadSafeModule directly without cloning
    // auto tsm = llvm::orc::ThreadSafeModule(std::move(module), nullptr);
    
    // if (auto err = jit->addIRModule(std::move(tsm))) {
    //     return err;
    // }
    
    // // Look up the main function
    // auto mainSymbol = jit->lookup("main");
    // if (!mainSymbol) {
    //     return mainSymbol.takeError();
    // }
    
    // // Get the address and cast it to a function pointer
    // auto mainAddr = mainSymbol->getValue();
    // auto mainFunc = reinterpret_cast<int(*)()>(mainAddr);
    
    // // Execute the function
    // int result = mainFunc();
    
    // return llvm::Error::success();

       // Create ThreadSafeModule - let it create its own context
    // This is the simplest approach that avoids ownership issues
   // The constructor wants a unique_ptr<LLVMContext>, not ThreadSafeContext
    auto tsm = llvm::orc::ThreadSafeModule(
        std::move(module), 
        std::make_unique<llvm::LLVMContext>()  // Create a new LLVMContext
    );
    
    if (auto err = jit->addIRModule(std::move(tsm))) {
        return err;
    }
    
    auto mainSymbol = jit->lookup("main");
    if (!mainSymbol) {
        return mainSymbol.takeError();
    }
    
    auto mainAddr = mainSymbol->getValue();
    auto mainFunc = reinterpret_cast<int(*)()>(mainAddr);
    
    int result = mainFunc();
    
    return llvm::Error::success();
}

void Compiler::jumpToFirstInstruction() {
    if (!instructionLabels.empty()) {
        builder.CreateBr(instructionLabels[0]);
    }
}

void Compiler::finishProgram() {
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
