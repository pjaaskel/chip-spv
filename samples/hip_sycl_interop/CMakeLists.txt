add_subdirectory(onemkl_gemm_wrapper)

add_chip_binary(hip_sycl_interop hip_sycl_interop.cpp)
add_dependencies(build_tests_standalone hip_sycl_interop)
target_link_options(hip_sycl_interop PRIVATE -fsycl -L${CMAKE_BINARY_DIR} -Wl,-rpath=${CMAKE_BINARY_DIR}:${ICPX_CORE_LIBDIR}:${ICPX_SYCL_LIBDIR})
target_link_libraries(hip_sycl_interop onemkl_gemm_wrapper)
target_include_directories(hip_sycl_interop PUBLIC ${CHIP_SRC_DIR}/HIP/include ${CHIP_SRC_DIR}/include)

add_test(NAME "hip_sycl_interop"
    COMMAND "${CMAKE_CURRENT_BINARY_DIR}/hip_sycl_interop"
)

set_tests_properties("hip_sycl_interop" PROPERTIES
    PASS_REGULAR_EXPRESSION "${TEST_PASS}"
    TIMEOUT 60
)