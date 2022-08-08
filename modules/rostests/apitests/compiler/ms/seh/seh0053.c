// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for
// full license information.

#include <windows.h>
#include "seh.h"

char test[] = "seh0053.c";

#define BLUE 0
#define RED 1

int main() {
  LONG Counter;
  ULONG Index2 = RED;

  Counter = 0;

  try {
    switch (Index2) {
    case BLUE:
      break;

    case RED:
      Counter += 3;
      leave;
    }

    Counter += 100;
  }
  finally {
    if (abnormal_termination() == FALSE) {
      Counter += 5;
    }
  }
  endtry

  if (Counter != 8) {
    printf("TEST 53 FAILED, Counter = %d\n", Counter);
    return -1;
  }

  return 0;
}
