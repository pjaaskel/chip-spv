# Test symbol access
add_chip_binary(
	hipTestDeviceLink
	hipDeviceLink.cpp
	hipDeviceLinkRead.cpp
	hipDeviceLinkWrite.cpp)

target_compile_options(hipTestDeviceLink PRIVATE -fPIE -fgpu-rdc)
target_link_libraries(hipTestDeviceLink CHIP deviceInternal)
target_link_options(hipTestDeviceLink PRIVATE -fPIE -fgpu-rdc --hip-link)

if(DEFINED LLVM_VERSION AND "${LLVM_VERSION}" VERSION_GREATER_EQUAL 14)
  target_link_options(hipTestDeviceLink PRIVATE --offload=spirv64)
endif()

add_test(NAME hipTestDeviceLink
	 	COMMAND "${CMAKE_CURRENT_BINARY_DIR}/hipTestDeviceLink")

set_tests_properties(hipTestDeviceLink PROPERTIES
	PASS_REGULAR_EXPRESSION PASSED)
