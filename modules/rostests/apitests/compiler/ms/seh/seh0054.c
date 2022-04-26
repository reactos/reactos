// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for
// full license information.

#include <windows.h>
#include "seh.h"

char test[] = "seh0054.c";

LONG Echo(LONG Value) { return Value; }

int main() {
  LONG Counter;

  Counter = 0;

  try {
    try {
      if (Echo(Counter) == Counter) {
        Counter += 3;
        leave;

      } else {
        Counter += 100;
      }
    }
    finally {
      if (abnormal_termination() == FALSE) {
        Counter += 5;
      }
    }
    endtry

    if (Echo(Counter) == Counter) {
      Counter += 3;
      leave;

    } else {
      Counter += 100;
    }
  }
  finally {
    if (abnormal_termination() == FALSE) {
      Counter += 5;
    }
  }
  endtry

  if (Counter != 16) {
    printf("TEST 54 FAILED, Counter = %d\n", Counter);
    return -1;
  }

  return 0;
}
