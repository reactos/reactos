// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for
// full license information.

#include <windows.h>
#include "seh.h"

char test[] = "SEH0028.c";

void addtwo(long First, long Second, long *Place) {
  RaiseException(EXCEPTION_INT_OVERFLOW, 0, /*no flags*/ 0, 0);
  /* not executed due to exception being raised */
  *Place = First + Second;
  return;
}

int main() {
  LONG Counter;

  Counter = 0;

  try {
    /* set counter = 1 */
    Counter += 1;
    addtwo(0x7fff0000, 0x10000, &Counter);
  }
  except((GetExceptionCode() == (STATUS_INTEGER_OVERFLOW)) ? 1 : 0)
  /* 1==EXECUTE HANDLER after unwinding */
  {
    /* set counter = 2 */
    Counter += 1;
  }
  endtry

  if (Counter != 2) {
    printf("TEST 28 FAILED. Counter = %d\n\r", Counter);
    return -1;
  }

  return 0;
}
