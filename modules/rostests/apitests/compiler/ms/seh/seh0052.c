// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for
// full license information.

#include <windows.h>
#include "seh.h"

char test[] = "seh0052.c";

LONG Echo(LONG Value) { return Value; }

int main() {
  LONG Counter;
  ULONG Index1;

  Counter = 0;

  try {
    for (Index1 = 0; Index1 < 10; Index1 += 1) {
      if (Echo(Index1) == Index1) {
        Counter += 3;
        leave;
      }
      Counter += 100;
    }
  }
  finally {
    if (AbnormalTermination() == 0) {
      Counter += 5;
    }
  }
  endtry

  if (Counter != 8) {
    printf("TEST 52 FAILED, Counter = %d\n", Counter);
    return -1;
  }

  return 0;
}
