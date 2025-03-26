// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for
// full license information.

#include <windows.h>
#include "seh.h"

char test[] = "SEH0017.c";

int main() {

  LONG Counter;

  Counter = 0;

  try {
    try {
      /* set counter = 1 */
      Counter += 1;
    }
    finally {
      /* set counter = 2 */
      Counter += 1;
#if defined(_MSC_VER) && !defined(__clang__)
      goto t12; /* can't jump into a try/finally */
#endif
    }
    endtry

  t12:
    ;
  }
  finally {
    /* set counter = 3 */
    Counter += 1;
  }
  endtry

  if (Counter != 3) {
    printf("TEST 17 FAILED. Counter = %d\n\r", Counter);
    return -1;
  }

  return 0;
}
