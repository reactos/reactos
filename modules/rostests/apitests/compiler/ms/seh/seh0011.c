// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for
// full license information.

#include <windows.h>
#include "seh.h"

char test[] = "SEH0011.c";

void AccessViolation(PLONG BlackHole, PLONG BadAddress) {
  *BlackHole += *BadAddress;
  return;
}

void tfAccessViolation(PLONG BlackHole, PLONG BadAddress, PLONG Counter) {
  try {
    AccessViolation(BlackHole, BadAddress);
  }
  finally {
    if (abnormal_termination() != 0)
    /*
     * not abnormal termination
     * counter should equal 99
     */
    {
      *Counter = 99;
    } else {
      *Counter = 100;
    }
  } endtry
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
    tfAccessViolation(BlackHole, BadAddress, &Counter);
  }
  except((GetExceptionCode() == STATUS_ACCESS_VIOLATION) ? 1 : 0)
  /*
   * acception raised was 0xC00000005L (ACCESS VIOLATION)
   * execute handler
   */
  {
    Counter -= 1;
  }
  endtry

  if (Counter != 98) {
    printf("TEST 11 FAILED. Counter = %d\n\r", Counter);
    return -1;
  }

  return 0;
}
