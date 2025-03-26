// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for
// full license information.

#include <setjmp.h>
#include <windows.h>
#include "seh.h"

char test[] = "SEH0013.c";

void rtlRaiseException(DWORD Status) {
  RaiseException(Status, 0, /*no flags*/ 0, 0);
  return;
}

void AccessViolation(PLONG BlackHole, PLONG BadAddress) {
  *BlackHole += *BadAddress;
  return;
}

int main() {
  PLONG BadAddress;
  PCHAR BadByte;
  PLONG BlackHole;
  ULONG Index2 = 1;
  LONG Counter;

  BadAddress = (PLONG)((PVOID)0);
  BadByte = (PCHAR)((PVOID)0);
  BadByte += 1;
  BlackHole = &Counter;

  Counter = 0;

  try {
    AccessViolation(BlackHole, BadAddress);
  }
  except((GetExceptionCode() == STATUS_ACCESS_VIOLATION) ? 1 : 0)
  /*
   * exception handler should gete executed
   * setting Counter to 1
   */
  {
    Counter += 1;
    try {
      rtlRaiseException(EXCEPTION_CONTINUE_SEARCH);
    }
    except((GetExceptionCode() == EXCEPTION_CONTINUE_SEARCH) ? 1 : 0)
    /* exception handler should get executed */
    {
      if (Counter != 1) {
        printf("TEST 13 FAILED. Counter = %d\n\r", Counter);
        return -1;
      }
      /* set's counter to 2 */
      Counter += 1;
    }
    endtry
  }
  endtry

  if (Counter != 2) {
    printf("TEST 13 FAILED. Counter= %d\n\r", Counter);
    return -1;
  }

  return 0;
}
