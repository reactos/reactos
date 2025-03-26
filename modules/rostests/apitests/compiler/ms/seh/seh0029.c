// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for
// full license information.

#include <windows.h>
#include "seh.h"

#define faill()
#define startest()
#define finish()

char test[] = "SEH0029.c";

void AccessViolation(PLONG BlackHole, PLONG BadAddress) {
  *BlackHole += *BadAddress;
  return;
}

int main() {
  PCHAR BadByte;
  PLONG BlackHole;
  LONG Counter;
  DWORD ExceptionCode;

  BadByte = (PCHAR)((PVOID)0);
  BadByte += 1;
  BlackHole = &Counter;

  Counter = 0;

  try {
    /* set counter = 1 */
    Counter += 1;
    /*
     * create a DATA MISALIGNMENT ERROR by passing
     * a Byte into a LONG. Passing (PLONG)1.
     */
    AccessViolation(BlackHole, (PLONG)BadByte);
  }
  except((ExceptionCode = GetExceptionCode()),
         ((ExceptionCode == STATUS_DATATYPE_MISALIGNMENT)
              ? 1
              : ((ExceptionCode == STATUS_ACCESS_VIOLATION) ? 1 : 0))) {
    Counter += 1;
  }
  endtry

  if (Counter != 2) {
    printf("TEST 29 FAILED. Counter = %d\n\r", Counter);
    return -1;
  } else {
    /*
    ISSUE-REVIEW:a-sibyvi-2003/10/20
            This test was expecting STATUS_DATATYPE_MISALIGNMENT
            which is no longer true for UTC and Phoenix. So changing the test
            to expect Acces Violation instead.
    ISSUE-REVIEW:v-simwal-2011-01-25 Either MISALIGNMENT or ACCESS VIOLATION
    counts as a pass.
        */
    if ((ExceptionCode != STATUS_ACCESS_VIOLATION) &&
        (ExceptionCode != STATUS_DATATYPE_MISALIGNMENT)) {
      printf(
          "TEST 29 FAILED. Expected ACCESS_VIOLATION, got exception = %d\n\r",
          ExceptionCode);
      return -1;
    }
  }
  return 0;
}
