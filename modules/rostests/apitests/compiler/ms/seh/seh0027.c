// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for
// full license information.

#include <windows.h>
#include "seh.h"

char test[] = "SEH0027.c";

void rtlRaiseStatus(DWORD Status) {
  RaiseException(Status, 0, /*no flags*/ 0, 0);
  return;
}

ULONG except3(PEXCEPTION_POINTERS ExceptionPointers, PLONG Counter) {
  PEXCEPTION_RECORD ExceptionRecord;

  ExceptionRecord = ExceptionPointers->ExceptionRecord;
  if ((ExceptionRecord->ExceptionCode == (STATUS_INTEGER_OVERFLOW)) &&
      ((ExceptionRecord->ExceptionFlags & 0x10) == 0)) {
    /* set counter = 23 */
    *Counter += 17;
    rtlRaiseStatus(EXCEPTION_EXECUTE_HANDLER);
  } else if ((ExceptionRecord->ExceptionCode == EXCEPTION_EXECUTE_HANDLER) &&
             ((ExceptionRecord->ExceptionFlags & 0x10) != 0)) {
    /* set counter = 42 */
    *Counter += 19;
    /* return COTINUE SEARCH */
    return 0;
  }
  /* never gets here due to exception being rasied */
  *Counter += 23;
  return 1;
}

void except1(PLONG Counter) {
  try {
    /* set counter = 6 */
    *Counter += 5;
    RaiseException(EXCEPTION_INT_OVERFLOW, 0, /*no flags*/ 0, 0);
  }
  except(except3(GetExceptionInformation(), Counter)) { *Counter += 7; }
  endtry
  /* set counter = 59 */
  *Counter += 9;
  return;
}

ULONG except2(PEXCEPTION_POINTERS ExceptionPointers, PLONG Counter) {
  PEXCEPTION_RECORD ExceptionRecord;

  ExceptionRecord = ExceptionPointers->ExceptionRecord;
  if ((ExceptionRecord->ExceptionCode == EXCEPTION_EXECUTE_HANDLER) &&
      ((ExceptionRecord->ExceptionFlags & 0x10) == 0)) {
    /* set counter = 53 */
    *Counter += 11;
    return 1;
  } else {
    *Counter += 13;
    return 0;
  }
}

int main() {
  LONG Counter;

  Counter = 0;

  try {
    try {
      /* set counter = 1 */
      Counter += 1;
      except1(&Counter);
    }
    except(except2(GetExceptionInformation(), &Counter)) {
      /* set counter = 55 */
      Counter += 2;
    }
    endtry
  }
  except(1) { Counter += 3; }
  endtry

  if (Counter != 55) {
    printf("TEST 27 FAILED. Counter = %d\n\r", Counter);
    return -1;
  }

  return 0;
}
