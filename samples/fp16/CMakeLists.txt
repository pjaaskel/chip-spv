
add_chip_test(fp16 fp16 PASSED haxpy-base.cpp)

# add_chip_test(fp16_math fp16_math PASSED half_math.cpp)
add_chip_binary(fp16_math half_math.cpp)
