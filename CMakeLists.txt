#=============================================================================
#   CMake build system files
#
#   Copyright (c) 2021-22 CHIP-SPV developers
#
#   Permission is hereby granted, free of charge, to any person obtaining a copy
#   of this software and associated documentation files (the "Software"), to deal
#   in the Software without restriction, including without limitation the rights
#   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
#   copies of the Software, and to permit persons to whom the Software is
#   furnished to do so, subject to the following conditions:
#
#   The above copyright notice and this permission notice shall be included in
#   all copies or substantial portions of the Software.
#
#   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
#   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
#   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
#   THE SOFTWARE.
#
#=============================================================================

# temporary
set(CMAKE_WARN_DEPRECATED OFF)
add_compile_options(-Wno-format-extra-args)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-duplicate-decl-specifier \
-Wno-tautological-constant-compare  -Wno-c++20-extensions -Wno-unused-result \
-Wno-delete-abstract-non-virtual-dtor -Wno-deprecated-declarations -Wunused-command-line-argument -gdwarf-4")
# end temporary

# Currently we require symlinks to clang and clang++ to be found in PATH.
set(CMAKE_C_COMPILER clang CACHE STRING "Clang C compiler" FORCE)
set(CMAKE_CXX_COMPILER clang++ CACHE STRING "Clang C++ compiler" FORCE)

cmake_minimum_required(VERSION 3.16 FATAL_ERROR)
project(CHIP-SPV
  VERSION 1.0
  DESCRIPTION "HIP implementation for runtimes that support SPIR-V"
  LANGUAGES C CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
include(chip_spv_cmake_macros)

###############################################################################
# CHIP-SPV CMAKE DEPENDENCIES

include(FindLLVM)

if(NOT DEFINED OpenCL_LIBRARY)
  message(STATUS "OpenCL_LIBRARY was not set. Searching for libOpenCL.so in LD_LIBRARY_PATH")
  find_library(OpenCL_LIBRARY NAMES OpenCL PATHS ENV LD_LIBRARY_PATH ./ NO_CACHE)
  get_filename_component(OpenCL_DIR ${OpenCL_LIBRARY} DIRECTORY CACHE)
  message(STATUS "Setting OpenCL_DIR to ${OpenCL_LIBRARY}")
endif()

if(NOT DEFINED LevelZero_LIBRARY)
  message(STATUS "LevelZero_LIBRARY was not set. Searching for ze_loader.so in LD_LIBRARY_PATH")
  find_library(LevelZero_LIBRARY NAMES ze_loader PATHS ENV LD_LIBRARY_PATH ./ NO_CACHE)
  get_filename_component(LevelZero_DIR ${LevelZero_LIBRARY} DIRECTORY CACHE)
  message(STATUS "Setting LevelZero_DIR to ${LevelZero_DIR}")
endif()

message(STATUS "OpenCL_LIBRARY: ${OpenCL_LIBRARY}")
message(STATUS "LevelZero_LIBRARY: ${LevelZero_LIBRARY}")

if(NOT OpenCL_LIBRARY AND NOT LevelZero_LIBRARY)
  message(FATAL_ERROR "At least one of OpenCL,Level0 libraries must be available")
endif()

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)
set(PTHREAD_LIBRARY Threads::Threads)

# CHIP-SPV CMAKE DEPENDENCIES
###############################################################################

###############################################################################
# CHIP-SPV CMAKE VARIABLBLES/OPTIONS
set(CHIP_SRC
  src/spirv.cc
  src/spirv_hiprtc.cc
  src/CHIPDriver.cc
  src/CHIPBackend.cc
  src/CHIPBindings.cc
  src/logging.cc
)

if(OpenCL_LIBRARY)
  list(APPEND CHIP_SRC
    src/backend/OpenCL/CHIPBackendOpenCL.cc
    src/backend/OpenCL/SVMemoryRegion.cc)
endif()

if(LevelZero_LIBRARY)
  list(APPEND CHIP_SRC
    src/backend/Level0/CHIPBackendLevel0.cc)
endif()

set(CHIP_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE INTERNAL "CHIP-SPV source directory")

set(CHIP_SPV_LINK_FLAGS "")
set(CHIP_SPV_COMPILE_FLAGS "")
list(APPEND CHIP_SPV_DEFINITIONS "")

# used by hipcc/hipconfig
set(HIP_PATH "" CACHE PATH "Path to the CHIP-SPV installation")
set(HIP_COMPILER "clang" CACHE STRING "CHIP-SPV compiler")
set(HIP_RUNTIME "spirv" CACHE STRING "CHIP-SPV runtime")
set(HIP_PLATFORM "spirv" CACHE STRING "CHIP-SPV platform")
set(HIP_ARCH "spirv" CACHE STRING "CHIP-SPV architecture")

set(CMAKE_CXX_STANDARD 17 CACHE STRING "The C++ standard to use.")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# INSTALL DIRECTORY VARIABLES
set(BIN_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/bin)
set(LIB_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/lib)
set(INCLUDE_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/include)
set(SHARE_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/share)
set(CONFIG_PACKAGE_INSTALL_DIR ${LIB_INSTALL_DIR}/cmake/hip)
set(CONFIG_LANG_PACKAGE_INSTALL_DIR ${LIB_INSTALL_DIR}/cmake/hip-lang)

# TODO: Add CHIP_ prefix
option(ERROR_IF_NOT_IMPLEMENTED "Some HIP functionality is not yet implemented in CHIP-SPV. If such a function is called, throw hipErrorNotSupported" ON)
option(LAZY_JIT "JIT kernels at the time of the first kernel invocation instead of at the start of the program execution" OFF)
option(CACHE_KERNELS "Save the compiled kernel to disk to speed up program startup" OFF)
option(VERBOSE "Verbose compilation" OFF)
option(BUILD_SHARED_LIBS "Build CHIP-SPV as a shared library" ON)
option(BUILD_DOCS "Build doxygen documentation" OFF)
option(ENFORCE_QUEUE_SYNCHRONIZATION "Enforce the correct HIP stream semantics of synchronizing queues with the default queue" ON)
option(PER_THREAD_DEFAULT_STREAM "Each thread has a thread local default stream. Default: OFF - use legacy stream" OFF)
option(LEVEL_ZERO_IMMEDIATE_QUEUES "Enable/disable the use of immediate command queues" OFF)
option(MALLOC_SHARED_WORKAROUND "Use zeMallocHost instead of zeMallocShared as it causes segfaults on some systems" ON)
option(LLVM_USE_INTERGRATED_SPIRV "Use LLVM's intergrated SPIR-V backend for emitting device binary instead of SPIR-V translator. Requires LLVM 15" OFF)
option(SET_RPATH "Add CMAKE_INSTALL_PREFIX/lib to the RPATH for CHIP-SPV executables" ON)
option(ENABLE_FAILING_TESTS "Enable tests which are known to fail or be unreliable" OFF)
option(ENABLE_UNCOMPILABLE_TESTS "Enable tests which are known to not compile" OFF)

set(CMAKE_BUILD_TYPE "DEBUG" CACHE STRING "The build type to use DEBUG, RELEASE, RELWITHDEBINFO, MINSIZEREL")
set(CHIP_SPV_DEFAULT_INSTALL_DIR "${CMAKE_SOURCE_DIR}/install" CACHE PATH "The installation directory for CHIP-SPV")
set(HIP_ENABLE_SPIRV "-D__HIP_PLATFORM_SPIRV__=" CACHE INTERNAL "Compiler difinition which enables SPIR-V defines")
# FOR EXTERNAL USE
set(HIP_OFFLOAD_COMPILE_OPTIONS "" CACHE STRING "space separated list of compiler flags needed for generating offload code")
set(HIP_OFFLOAD_LINK_OPTIONS "" CACHE STRING "space separated list of compiler flags needed for linking binaries with GPU offload")
# CHIP-SPV CMAKE VARIABLBLES/OPTIONS
###############################################################################

###############################################################################
# CHIP-SPV CMAKE OPTIONS PARSING

if(MALLOC_SHARED_WORKAROUND)
  list(APPEND CHIP_SPV_DEFINITIONS MALLOC_SHARED_WORKAROUND)
endif()

set(DISABLE_OPAQUE_PTRS_OPT "")
if (NOT CLANG_VERSION_LESS_15)
  # LLVM 15 switches to opaque pointer mode by default which
  # llvm-spirv currently does not support. Switch the mode off.
  #
  # NOTE: https://reviews.llvm.org/D130766 will disable the mode off
  #       for SPIR-V targets but the patch has not yet landed on the
  #       LLVM 15 branch.
  #
  # NOTE: Bitcode library must be compiled with the same opaque
  #       pointer setting.
  set(DISABLE_OPAQUE_PTRS_OPT -Xclang -no-opaque-pointers)
endif()

if(LLVM_USE_INTERGRATED_SPIRV)
  if(CLANG_VERSION_LESS_14)
    message(FATAL_ERROR "-DLLVM_USE_INTERGRATED_SPIRV=On is not supported for LLVM 13 and less")
  else()
    set(SPIRV_EMITTER_OPTS "-fintegrated-objemitter")
  endif()
else()
  set(SPIRV_EMITTER_OPTS "")
endif()

if(ENFORCE_QUEUE_SYNCHRONIZATION)
  list(APPEND CHIP_SPV_DEFINITIONS ENFORCE_QUEUE_SYNC)
endif()

if(PER_THREAD_DEFAULT_STREAM)
  list(APPEND CHIP_SPV_DEFINITIONS HIP_API_PER_THREAD_DEFAULT_STREAM)
endif()

if(LEVEL_ZERO_IMMEDIATE_QUEUES)
  list(APPEND CHIP_SPV_DEFINITIONS L0_IMM_QUEUES)
endif()

if(VERBOSE)
  set(CMAKE_VERBOSE_MAKEFILE ON)
  add_compile_options("-v")
endif()

string(TOUPPER "${CMAKE_BUILD_TYPE}" uppercase_CMAKE_BUILD_TYPE)
if(uppercase_CMAKE_BUILD_TYPE STREQUAL "DEBUG")
  add_compile_options("-Wall")
  list(APPEND CHIP_SPV_DEFINITIONS SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE)
else()
  list(APPEND CHIP_SPV_DEFINITIONS SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_WARN)
endif()

if(ERROR_IF_NOT_IMPLEMENTED)
  list(APPEND CHIP_SPV_DEFINITIONS CHIP_ERROR_ON_UNIMPL)
endif()

if(BUILD_SHARED_LIBS)
  message(STATUS "Buiding CHIP-SPV as a shared library")
  add_library(CHIP SHARED ${CHIP_SRC})
  set(CHIP_LIB_NAME "libCHIP.so")
else()
  message(STATUS "Buiding CHIP-SPV as a static library")
  add_library(CHIP STATIC ${CHIP_SRC})
  set(CHIP_LIB_NAME "libCHIP.a")
endif()

set(CHIP_INTERFACE_LIBS ${PTHREAD_LIBRARY})
if(OpenCL_LIBRARY)
  list(APPEND CHIP_SPV_DEFINITIONS HAVE_OPENCL)
  list(PREPEND CHIP_INTERFACE_LIBS ${OpenCL_LIBRARY})
endif()

if(LevelZero_LIBRARY)
  list(APPEND CHIP_SPV_DEFINITIONS HAVE_LEVEL0)
  list(PREPEND CHIP_INTERFACE_LIBS ${LevelZero_LIBRARY})
endif()

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX "${CHIP_SPV_DEFAULT_INSTALL_DIR}" CACHE PATH "Install path prefix" FORCE)
endif()

set(HIP_PATH ${CMAKE_INSTALL_PREFIX})
message(STATUS "CHIP-SPV will be installed to: ${CMAKE_INSTALL_PREFIX}")
# CHIP-SPV CMAKE OPTIONS PARSING
###############################################################################

###############################################################################
# CHIP-SPV BINARIES & TESTS

add_subdirectory(llvm_passes)
add_subdirectory(bitcode)

enable_testing()
add_subdirectory(HIP/tests/catch catch)
add_subdirectory(tests/cuda)
add_subdirectory(./samples samples)
# Make CHIP depend on devicelib_bc and LLVMHipPasses for
# convenience. The CHIP module itself does not depend on these but
# HIP program compilation does.
add_dependencies(CHIP devicelib_bc LLVMHipPasses)
add_dependencies(samples CHIP)

add_subdirectory(bin)

# custom target to avoid tests that are known to fail
# each entry in the regexp list must be without whitespace and end with "|\"
#
# Note that this list only contains tests external to CHIP-SPV,
# such as those frome HIP's testsuite; the internal tests
# should be disabled based on value ENABLE_FAILING_TESTS option
set(FLAKY_TESTS
"Unit_hipMemcpyAsync_hipMultiMemcpyMultiThread - |\
Unit_hipMemcpyAsync_hipMultiMemcpyMultiThreadMultiStream - |\
Unit_hipStreamCreateWithPriority_ValidateWithEvents$"
)
add_custom_target(flaky_tests COMMAND ${CMAKE_CTEST_COMMAND} --timeout 120 "--output-on-failure" --repeat until-fail:1000 -j 10 -R ${FLAKY_TESTS} USES_TERMINAL VERBATIM)

add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} --timeout 120 "--output-on-failure" -E
"Unit_hipDeviceGetCacheConfig_FuncTst$|\
Unit_hipEventIpc$|\
Unit_hipGetDeviceFlags_FuncTst$|\
Unit_hipGraphAddDependencies_Functional$|\
Unit_hipGraphAddEmptyNode_Functional$|\
Unit_hipGraphAddMemcpyNode_Functional$|\
Unit_hipGraph_BasicFunctional$|\
Unit_hipGraph_SimpleGraphWithKernel$|\
Unit_hipHostMalloc_Coherent$|\
Unit_hiprtc_saxpy$|\
Unit_hipHostMalloc_NonCoherent$|\
Unit_hipMemcpyAsync_hipMultiMemcpyMultiThread - |\
Unit_hipMemcpyAsync_hipMultiMemcpyMultiThreadMultiStream - |\
Unit_hipMemPrefetchAsync_NonPageSz$|\
Unit_hipMemsetAsync_SetMemoryWithOffset$|\
Unit_hipMemset_SetMemoryWithOffset$|\
Unit_hipOccupancyMaxActiveBlocksPerMultiprocessor_Negative$|\
Unit_hipOccupancyMaxActiveBlocksPerMultiprocessor_rangeValidation$|\
Unit_hipOccupancyMaxPotentialBlockSize_rangeValidation$|\
Unit_hipStreamCreateWithPriority_ValidateWithEvents$|\
Unit_AnyAll_CompileTest$|\
Unit_ballot$|\
Unit_deviceFunctions_CompileTest___fmaf_ieee|\
Unit_deviceFunctions_CompileTest___funnelshift|\
Unit_funnelshift$|\
Unit_syncthreads_and$|\
Unit_syncthreads_count$|\
Unit_syncthreads_or$"
USES_TERMINAL VERBATIM)

# CHIP-SPV BINARIES & TESTS
###############################################################################

###############################################################################
# CHIP-SPV COMPILATION SETUP
# For building on top of HIP-COMMON
target_compile_options(CHIP PRIVATE ${HIP_ENABLE_SPIRV} ${CHIP_SPV_COMPILE_FLAGS})
target_compile_definitions(CHIP PRIVATE ${CHIP_SPV_DEFINITIONS})

target_link_libraries(CHIP INTERFACE ${CHIP_INTERFACE_LIBS})

# Previously these were set to SYSTEM which resulted in CMake picking up
# OpenCL headers from the system where the version might differ resulting in errors.
target_include_directories(CHIP
  PUBLIC
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/cuspv>"
  "$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/HIP/include>"
  "$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>"
  "$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include/cuspv>"
  PRIVATE
  "${CMAKE_SOURCE_DIR}/src"
  "${CMAKE_SOURCE_DIR}/include"
)
# CHIP-SPV COMPILATION SETUP
###############################################################################

###############################################################################
# HIP OFFLOAD FLAGS
set(HIP_OFFLOAD_COMPILE_ONLY_OPTIONS_ -x hip --target=x86_64-linux-gnu
  ${SPIRV_EMITTER_OPTS} ${DISABLE_OPAQUE_PTRS_OPT})

# hipDeviceLink sample requires --offload=spirv64 flag at link time. Why? 
if(NOT CLANG_VERSION_LESS_14)
  set(HIP_OFFLOAD_COMPILE_ONLY_OPTIONS_ ${HIP_OFFLOAD_COMPILE_ONLY_OPTIONS_}
      --offload=spirv64
      # By default RocmInstallationDetector::AddHIPIncludeArgs in the
      # HIP-Clang inserts include wrapper headers which cause compile
      # errors when compiling HIP sources for SPIR-V target.  This flag
      # excludes the wrappers.
      "-nohipwrapperinc")
endif()

if(CLANG_VERSION_LESS_14 )
  set(HIP_LINK_DEVICELIB_BUILD --hip-device-lib-path=${CMAKE_BINARY_DIR}/share --hip-device-lib=devicelib.bc --hip-llvm-pass-path=${CMAKE_BINARY_DIR}/llvm_passes)
  set(HIP_LINK_DEVICELIB_INSTALL --hip-device-lib-path=${SHARE_INSTALL_DIR} --hip-device-lib=devicelib.bc --hip-llvm-pass-path=${LIB_INSTALL_DIR}/llvm)
else()
  set(HIP_LINK_DEVICELIB_BUILD --hip-path=${CMAKE_BINARY_DIR})
  set(HIP_LINK_DEVICELIB_INSTALL --hip-path=${CMAKE_INSTALL_PREFIX})
endif()

# For use by hipcc
set(HIP_OFFLOAD_COMPILE_OPTIONS_INSTALL_
  ${HIP_ENABLE_SPIRV} 
  ${HIP_OFFLOAD_COMPILE_ONLY_OPTIONS_} 
  ${HIP_LINK_DEVICELIB_INSTALL})

set(HIP_OFFLOAD_COMPILE_OPTIONS_BUILD_
  ${HIP_ENABLE_SPIRV}
  ${HIP_OFFLOAD_COMPILE_ONLY_OPTIONS_}
  ${HIP_LINK_DEVICELIB_BUILD})

# HIP applications need to link against libCHIP.so; add it to rpath
list(APPEND HIP_OFFLOAD_LINK_OPTIONS_INSTALL_
  "-L${LIB_INSTALL_DIR}" "-lCHIP" "-L${OpenCL_DIR}" "-L${LevelZero_DIR}" "-lOpenCL" "-lze_loader")
list(APPEND HIP_OFFLOAD_LINK_OPTIONS_BUILD_
  "-L${CMAKE_BINARY_DIR}" "-lCHIP" "-L${OpenCL_DIR}" "-L${LevelZero_DIR}" "-lOpenCL" "-lze_loader")
if(SET_RPATH)
  list(APPEND HIP_OFFLOAD_LINK_OPTIONS_INSTALL_ "-Wl,-rpath,${LIB_INSTALL_DIR}")
  list(APPEND HIP_OFFLOAD_LINK_OPTIONS_BUILD_ "-Wl,-rpath,${CMAKE_BINARY_DIR}")
  set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
endif()

# Workaround istead of trying to generate the CMake generator expression
string(REPLACE ";" " " HIP_OFFLOAD_COMPILE_OPTIONS_INSTALL "${HIP_OFFLOAD_COMPILE_OPTIONS_INSTALL_}")
string(REPLACE ";" " " HIP_OFFLOAD_COMPILE_OPTIONS_BUILD "${HIP_OFFLOAD_COMPILE_OPTIONS_BUILD_}")
string(REPLACE ";" " " HIP_OFFLOAD_LINK_OPTIONS_INSTALL "${HIP_OFFLOAD_LINK_OPTIONS_INSTALL_}")
string(REPLACE ";" " " HIP_OFFLOAD_LINK_OPTIONS_BUILD "${HIP_OFFLOAD_LINK_OPTIONS_BUILD_}")
message(STATUS "Generated HIP_OFFLOAD_COMPILE_OPTIONS: ${HIP_OFFLOAD_COMPILE_OPTIONS_INSTALL}")
message(STATUS "Generated HIP_OFFLOAD_LINK_OPTIONS: ${HIP_OFFLOAD_LINK_OPTIONS_INSTALL}")

# Must separate these targets into deviceInteral to be used for building samples,etc.
# and device for external export. Have to do this because generator expressions don't seem to work here.
# TODO: Is there a better way to do this?
add_library(deviceInternal INTERFACE)
target_compile_options(deviceInternal INTERFACE 
  ${HIP_ENABLE_SPIRV} 
  ${HIP_OFFLOAD_COMPILE_ONLY_OPTIONS_}
  ${HIP_LINK_DEVICELIB_BUILD})
target_link_options(deviceInternal INTERFACE 
  ${HIP_LINK_DEVICELIB_BUILD})
target_link_libraries(deviceInternal INTERFACE CHIP)

add_library(device INTERFACE)
target_compile_options(device INTERFACE 
  ${HIP_ENABLE_SPIRV} 
  ${HIP_OFFLOAD_COMPILE_ONLY_OPTIONS_}
  ${HIP_LINK_DEVICELIB_INSTALL})
target_link_options(device INTERFACE 
  ${HIP_LINK_DEVICELIB_INSTALL})
target_link_libraries(device INTERFACE CHIP)

# same as device on CHIP-SPV but provides compatibility with AMD
add_library(host INTERFACE)
target_link_libraries(host INTERFACE device)
# HIP OFFLOAD FLAGS
###############################################################################

###############################################################################
# CHIP-SPV INSTALLATION AND PACKAGING
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

# Setup .hipVersion 
# Install .hipVersion for hipcc. The file is used to detect a HIP
# runtime and clang extracts HIP version from it.
set(_versionInfo "# Auto-generated by cmake\n")
set(HIP_VERSION_MAJOR 5)
set(HIP_VERSION_MINOR 1)
set(HIP_VERSION_PATCH 0)
add_to_config(_versionInfo HIP_VERSION_MAJOR "${HIP_VERSION_MAJOR}")
add_to_config(_versionInfo HIP_VERSION_MINOR "${HIP_VERSION_MINOR}")
add_to_config(_versionInfo HIP_VERSION_PATCH "${HIP_VERSION_PATCH}")

# Generate and install .hipVersion file.
file(WRITE ${PROJECT_BINARY_DIR}/bin/.hipVersion ${_versionInfo})
install(FILES ${PROJECT_BINARY_DIR}/bin/.hipVersion DESTINATION bin)

# Setup .hipInfo. One for install and another for build directory.
string(TIMESTAMP _timestamp UTC)
set(_hipInfo_install "# Auto-generated by cmake on ${_timestamp} UTC\n")
message(STATUS "HIP_PATH: " ${HIP_PATH})
message(STATUS "HIP Runtime: " ${HIP_RUNTIME})
message(STATUS "HIP Compiler: " ${HIP_COMPILER})
add_to_config(_hipInfo_install HIP_PATH "${HIP_PATH}")
add_to_config(_hipInfo_install HIP_RUNTIME "${HIP_RUNTIME}")
add_to_config(_hipInfo_install HIP_COMPILER "${HIP_COMPILER}")
add_to_config(_hipInfo_install HIP_ARCH "${HIP_ARCH}")
add_to_config(_hipInfo_install
  HIP_OFFLOAD_COMPILE_OPTIONS "${HIP_OFFLOAD_COMPILE_OPTIONS_INSTALL}")
add_to_config(_hipInfo_install
  HIP_OFFLOAD_LINK_OPTIONS "${HIP_OFFLOAD_LINK_OPTIONS_INSTALL}")

# Build version
set(_hipInfo_build "# Auto-generated by cmake on ${_timestamp} UTC\n")
add_to_config(_hipInfo_build HIP_PATH "${CMAKE_BINARY_DIR}")
add_to_config(_hipInfo_build HIP_RUNTIME "${HIP_RUNTIME}")
add_to_config(_hipInfo_build HIP_COMPILER "${HIP_COMPILER}")
add_to_config(_hipInfo_build HIP_ARCH "${HIP_ARCH}")
add_to_config(_hipInfo_build
  HIP_OFFLOAD_COMPILE_OPTIONS "${HIP_OFFLOAD_COMPILE_OPTIONS_BUILD}")
add_to_config(_hipInfo_build
  HIP_OFFLOAD_LINK_OPTIONS "${HIP_OFFLOAD_LINK_OPTIONS_BUILD}")

INSTALL(TARGETS CHIP host device
  EXPORT hip-targets
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
INSTALL(EXPORT hip-targets DESTINATION ${CONFIG_PACKAGE_INSTALL_DIR} NAMESPACE hip::)

# hip-config.cmake generation
include(CMakePackageConfigHelpers)

configure_package_config_file(
  ${CMAKE_SOURCE_DIR}/cmake/hip-config-in.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/hip-config.cmake
  INSTALL_DESTINATION ${CONFIG_PACKAGE_INSTALL_DIR}
  PATH_VARS LIB_INSTALL_DIR INCLUDE_INSTALL_DIR BIN_INSTALL_DIR SHARE_INSTALL_DIR
)

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/hip-config-version.cmake
  VERSION "${HIP_VERSION_MAJOR}.${HIP_VERSION_MINOR}.${HIP_VERSION_GITDATE}"
  COMPATIBILITY SameMajorVersion
)
install(
  FILES
  ${CMAKE_CURRENT_BINARY_DIR}/hip-config.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/hip-config-version.cmake
  DESTINATION
  ${CONFIG_PACKAGE_INSTALL_DIR}
)
install ( EXPORT hip-targets  FILE hip-targets.cmake NAMESPACE hip:: DESTINATION lib/cmake/hip  COMPONENT binary)

# Generate .hipInfo
file(WRITE "${PROJECT_BINARY_DIR}/share/.hipInfo_install" ${_hipInfo_install})
file(WRITE "${PROJECT_BINARY_DIR}/share/.hipInfo" ${_hipInfo_build})
install(FILES ${PROJECT_BINARY_DIR}/share/.hipInfo_install
  DESTINATION ${SHARE_INSTALL_DIR} RENAME .hipInfo)

# Copy hipconfig, hipvars, etc to bin
file(COPY "${CMAKE_SOURCE_DIR}/HIP/bin" DESTINATION "${PROJECT_BINARY_DIR}")
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/HIP/bin DESTINATION . USE_SOURCE_PERMISSIONS)

set(CHIP_CMAKE_INSTALL_LIBDIR ${CMAKE_INSTALL_LIBDIR})
set(CHIP_CMAKE_INSTALL_INCLUDEDIR ${CMAKE_INSTALL_INCLUDEDIR})

install(TARGETS CHIP
  EXPORT CHIPTargets
  LIBRARY DESTINATION ${CHIP_CMAKE_INSTALL_LIBDIR}
  INCLUDES DESTINATION ${CHIP_CMAKE_INSTALL_INCLUDEDIR}
)

install(FILES ${PROJECT_BINARY_DIR}/${CHIP_LIB_NAME} DESTINATION lib)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/HIP/include DESTINATION . USE_SOURCE_PERMISSIONS)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include DESTINATION . USE_SOURCE_PERMISSIONS)

install(EXPORT CHIPTargets
  FILE CHIPTargets.cmake
  NAMESPACE CHIP::
  DESTINATION cmake/CHIP)

# CHIP-SPV INSTALLATION AND PACKAGING
###############################################################################

if(BUILD_DOCS)
  find_package(Doxygen REQUIRED)
  set(DOXYGEN_GENERATE_HTML YES)
  set(DOXYGEN_GENERATE_MAN NO)
  set(DOXYGEN_OUTPUT_DIRECTORY doxygen)
  set(DOXYGEN_USE_MDFILE_AS_MAINPAGE README.md)
  doxygen_add_docs(gendocs README.md src llvm_passes
                  COMMENT "generate documentation for source")
endif()
