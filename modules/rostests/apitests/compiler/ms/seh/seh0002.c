// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for
// full license information.

#include <windows.h>
#include "seh.h"

char test[] = "SEH0002.c";

int main() {

  LONG Counter;

  Counter = 0;

  try {
    Counter += 1;
  }
  except(Counter)
  /*
   * counter should be positive indicating "EXECUTE HANDLER"
   * but should never get here as no exception is raised
   */
  {
    Counter += 1;
  }
  endtry

  if (Counter != 1) {
    printf("TEST 2 FAILED.  Counter = %d\n\r", Counter);
    return -1;
  }

  return 0;
}
