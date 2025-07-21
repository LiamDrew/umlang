cmake_minimum_required(VERSION 3.10)
project(UMCompiler)
set(CMAKE_CXX_STANDARD 17)

# Find LLVM
find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

# Add LLVM definitions and include directories
add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})
include_directories(src)
include_directories(../virt)

add_library(virt_obj OBJECT ../virt/virt.c)

target_include_directories(virt_obj PRIVATE ../virt)
set_property(TARGET virt_obj PROPERTY C_STANDARD 99)

set(SOURCES
    src/main.cpp
    src/compiler.cpp
    src/program_loader.cpp
)

# Add the executable
# add_executable(compiler ${SOURCES})
add_executable(compiler ${SOURCES} $<TARGET_OBJECTS:virt_obj>)

# Link against LLVM libraries - updated for JIT support
llvm_map_components_to_libnames(llvm_libs 
    core 
    orcjit 
    native
    support
    target
    mc
    executionengine
    runtimedyld
    object
)

target_link_libraries(compiler ${llvm_libs})

# Initialize native target for JIT
target_compile_definitions(compiler PRIVATE LLVM_NATIVE_TARGETMC_ENABLED)

# cmake_minimum_required(VERSION 3.10)
# project(UMCompiler)

# set(CMAKE_CXX_STANDARD 17)

# # Find LLVM
# find_package(LLVM REQUIRED CONFIG)

# message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
# message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

# # Add LLVM definitions and include directories
# add_definitions(${LLVM_DEFINITIONS})
# include_directories(${LLVM_INCLUDE_DIRS})

# include_directories(src)

# set(SOURCES
#     src/main.cpp
#     src/compiler.cpp
#     src/program_loader.cpp
# )

# # Add the executable
# add_executable(compiler ${SOURCES})

# # Link against LLVM libraries
# # llvm_map_components_to_libnames(llvm_libs support core irreader)
# # target_link_libraries(compiler ${llvm_libs})

# # Use this - more specific and avoids duplicates:
# # llvm_map_components_to_libnames(llvm_libs core support)
# llvm_map_components_to_libnames(llvm_libs core)
# target_link_libraries(compiler ${llvm_libs})
