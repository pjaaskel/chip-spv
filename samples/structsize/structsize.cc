/*
 * Copyright (c) 2022 Pekka Jääskeläinen / Intel Finland Oy
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <cstdlib>
#include <stdio.h>
#include <iostream>

#include "hip/hip_runtime.h"

struct Foo {
  int A;
  long B;
};

__global__ void struct_arg(struct Foo F, long *out) {
  out[0] = F.A + F.B;
  out[1] = sizeof(struct Foo);
  out[2] = F.A;
  out[3] = F.B;
}

int main(int argc, char *argv[]) {
  void *RetValVoid;
  hipHostMalloc(&RetValVoid, sizeof(long) * 4);
  auto out = reinterpret_cast<long *>(RetValVoid);

  Foo F;
  F.A = 10;
  F.B = 100;

  struct_arg<<<dim3(1), dim3(1), 0, 0>>> (F, out);
  hipStreamSynchronize(0);

  std::cout << "Result: " << out[0] << std::endl;
  std::cout << "Device side sizeof: " << out[1] << " (host: "
	    << sizeof(Foo) << ") " << std::endl;
  std::cout << "Components: " << out[2] << " " << out[3] << std::endl;


  return 0;
}
