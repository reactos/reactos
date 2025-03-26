// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for
// full license information.

#include <windows.h>
#include "seh.h"

char test[] = "SEH0051.c";

LONG Echo(LONG Value) { return Value; }

int main() {
  LONG Counter;

  Counter = 0;

  try {
    if (Echo(Counter) == Counter) {
      Counter += 3;
      leave;
    } else {
      Counter += 100;
    }
  }
  finally {
    if (AbnormalTermination() == 0) {
      Counter += 5;
    }
  }
  endtry

  if (Counter != 8) {
    printf("test 51 failed.  Counter = %d\n", Counter);
    return -1;
  }

  return 0;
}
