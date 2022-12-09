set(SOURCE structsize.cc)
set_source_files_properties(${SOURCE} PROPERTIES LANGUAGE CXX)
add_executable(structsize ${SOURCE})
target_link_libraries(structsize PUBLIC CHIP deviceInternal)
set_target_properties(structsize PROPERTIES CXX_STANDARD_REQUIRED ON)
target_include_directories(structsize PUBLIC
  ${CMAKE_SOURCE_DIR}/HIP/include ${CMAKE_SOURCE_DIR}/include)
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/run-structsize.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/run-structsize.cmake
  COPYONLY)
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/structsize.xstdout
  ${CMAKE_CURRENT_BINARY_DIR}/structsize.xstdout
  COPYONLY)
add_test(
  NAME structsize
  COMMAND "${CMAKE_COMMAND}" -P ${CMAKE_CURRENT_BINARY_DIR}/run-structsize.cmake)

