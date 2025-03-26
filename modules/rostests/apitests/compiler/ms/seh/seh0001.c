// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for
// full license information.

#include <windows.h>
#include "seh.h"

char test[] = "SEH0001.c";

int main() {
  long Counter;

  Counter = 0;

  try {
    Counter += 1;
  }
  finally {
    if (abnormal_termination() == 0) {
      Counter += 1;
    }
  }
  endtry

  if (Counter != 2) {
    printf("TEST 1 FAILED.  Counter = %d\n\r", Counter);
    return -1;
  }
  return 0;
}
