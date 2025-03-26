// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for
// full license information.

#include <windows.h>
#include "seh.h"

char test[] = "SEH0007.c";

int main() {
  PLONG BadAddress;
  PLONG BlackHole;
  LONG Counter;

  // startest();

  BadAddress = (PLONG)((PVOID)0);
  BlackHole = &Counter;

  Counter = 0;
  try {
    try {
      Counter += 1;
      *BlackHole += *BadAddress;
    }
    finally {
      if (abnormal_termination() != 0)
      /*
       * should execute handler as not abnormal
       * termination
       */
      {
        Counter += 1;
      }
    }
    endtry
  }
  except(Counter)
  /* counter is positive == EXECUTE_HANDLER */
  {
    if (Counter == 2)
    /* counter should equal 2 and execute handler */
    {
      Counter += 1;
    }
  }
  endtry

  if (Counter != 3) {
    printf("TEST 7 FAILED. Counter = %d\n\r", Counter);
    return -1;
  }

  return 0;
}
