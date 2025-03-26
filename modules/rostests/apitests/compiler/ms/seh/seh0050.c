// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for
// full license information.

#include <windows.h>
#include "seh.h"

char test[] = "SEH0050.c";

int main() {
  ULONG Index1;
  ULONG Index2 = 1;
  LONG Counter;

  Counter = 0;
  Index1 = 1;

  switch (Index2) {
  case 0:
    Counter += 100;
    break;
  case 1:
    try {
      try {
        if ((Index1 & 0x1) == 1) {
          Counter += 1;
        }
      }
      finally { Counter += 2; } endtry
      Counter += 4;
    }
    finally {
      Counter += 5;
#if defined(_MSC_VER) && !defined(__clang__)
      break;
#endif
    }
    endtry
#ifndef _MSC_VER
    break;
#endif
    Counter += 6;
  }

  if (Counter != 12) {
    printf("TEST 50 FAILED. Counter = %d\n\r", Counter);
    return -1;
  }

  return 0;
}
