// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for
// full license information.

#include <windows.h>
#include "seh.h"

char test[] = "SEH0009.c";

void AccessViolation(PLONG BlackHole, PLONG BadAddress) {
  *BlackHole += *BadAddress;
  return;
}

int main() {
  PLONG BadAddress;
  PLONG BlackHole;
  LONG Counter;

  BadAddress = (PLONG)((PVOID)0);
  BlackHole = &Counter;

  Counter = 0;

  try {
    Counter += 1;
    AccessViolation(BlackHole, BadAddress);
  }
  except((GetExceptionCode() == STATUS_ACCESS_VIOLATION) ? 1 : 0)
  /*
   * should be ACCESS VIOLATOIN 0xC0000005L) causing
   * execution of handler
   */
  {
    Counter += 1;
  }
  endtry

  if (Counter != 2) {
    printf("TEST 9 FAILED. Counter = %d\n\r", Counter);
    return -1;
  }

  return 0;
}
