// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for
// full license information.

#include <windows.h>
#include "seh.h"

char test[] = "seh0057.c";

int main() {
  LONG Counter;

  Counter = 0;

  try {
    Counter += 1;
  }
  finally {
    if (abnormal_termination() == FALSE) {
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
  }
  endtry

  if (Counter != 9) {
    printf("TEST 57 FAILED, Counter = %d\n", Counter);
    return -1;
  }

  return 0;
}
