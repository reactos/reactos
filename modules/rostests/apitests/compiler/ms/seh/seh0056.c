// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for
// full license information.

#include <windows.h>
#include "seh.h"

char test[] = "SEH0056.c";

int main() {
  LONG Counter;

  Counter = 0;

  try {
    Counter += 1;
    RaiseException(EXCEPTION_INT_OVERFLOW, 0, /* no flags */
                   0, NULL);
  }
  except(Counter) {
    try {
      Counter += 3;
    }
    finally {
      if (abnormal_termination() == FALSE) {
        Counter += 5;
      }
    }
    endtry
  }
  endtry

  if (Counter != 9) {
    printf("TEST 56 FAILED.  Counter = %d\n\r", Counter);
    return -1;
  }

  return 0;
}
