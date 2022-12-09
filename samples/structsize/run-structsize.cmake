execute_process(
  COMMAND ./structsize
  RESULT_VARIABLE errorcode
  OUTPUT_VARIABLE stdout
  ERROR_VARIABLE stderr)

if(errorcode)
  message(FATAL_ERROR
    "FAIL: HIP program returned an error code '${errorcode}'.")
endif()

file(WRITE "structsize-stdout.txt" "${stdout}")

execute_process(
  COMMAND ${CMAKE_COMMAND} -E compare_files "structsize-stdout.txt" "structsize.xstdout"
  RESULT_VARIABLE errorcode)

if(errorcode)
  message(FATAL_ERROR "FAIL: Standard output does not match 'structsize.xstdout'")
endif()

message(STATUS "PASSED")
