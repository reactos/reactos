// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for
// full license information.

#include <windows.h>
#include "seh.h"

char test[] = "SEH0006.c";

int main() {
  LONG Counter;

  Counter = 0;

  try {
    try {
      Counter += 1;
      RaiseException(EXCEPTION_INT_OVERFLOW, 0, /* no flags */
                     0, NULL);
      // RtlRaiseException(&ExceptionRecord);
    }
    finally {
      if (abnormal_termination() != 0)
      /*
       * an exception is not an abnormal termination
       * therefore thi should get executed
       */
      {
        Counter += 1;
      }
    }
    endtry
  }
  except(Counter)
  /* counter should equal "EXECUTE HANDLER" */
  {
    if (Counter == 2)
    /*
     * counter should equal two and therefore
     * execute this code
     */
    {
      Counter += 1;
    }
  }
  endtry

  if (Counter != 3) {
    printf("TEST 6 FAILED. Counter = %d\n\r", Counter);
    return -1;
  }

  return 0;
}
