/*
*******************************************************************************
*
*   Copyright (C) 2001 - 2005, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  newcol.cpp
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2001jul24
*   created by: Vladimir Weinstein
*/

/******************************************************************************
 * This is the module that uses new collation
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "unicode/ucol.h"

// Very simple example code - sticks a sortkey in the buffer
// Not much error checking
int32_t getSortKey_current(const char *locale, const UChar *string, int32_t sLen, uint8_t *buffer, int32_t bLen) {
  UErrorCode status = U_ZERO_ERROR;
  UCollator *coll = ucol_open(locale, &status);
  if(U_FAILURE(status)) {
    return -1;
  }
  int32_t result = ucol_getSortKey(coll, string, sLen, buffer, bLen);
  ucol_close(coll);
  return result;  
}

// This one can be used for passing to qsort function
// Not thread safe or anything
static UCollator *compareCollator = NULL;

int compare_current(const void *string1, const void *string2) {
  if(compareCollator != NULL) {
    UCollationResult res = ucol_strcoll(compareCollator, (UChar *) string1, -1, (UChar *) string2, -1);
    if(res == UCOL_LESS) {
      return -1;
    } else if(res == UCOL_GREATER) {
      return 1;
    } else {
      return 0;
    }
  } else {
    return 0;
  }
}

void initCollator_current(const char *locale) {
  UErrorCode status = U_ZERO_ERROR;
  compareCollator = ucol_open(locale, &status);
}

void closeCollator_current(void) {
  ucol_close(compareCollator);
  compareCollator = NULL;
}


extern "C" void test_current(UChar data[][5], uint32_t size, uint32_t maxlen, uint8_t keys[][32]) {
  uint32_t i = 0;
  int32_t keySize = 0;
  UVersionInfo uvi;

  u_getVersion(uvi);
  fprintf(stderr, "Entered current, version: [%d.%d.%d.%d]\nMoving to sortkeys\n", uvi[0], uvi[1], uvi[2], uvi[3]);

  for(i = 0; i<size; i++) {
    keySize = getSortKey_current("ja", data[i], -1, keys[i], 32);
    fprintf(stderr, "For i=%d, size of sortkey is %d\n", i, keySize);
  }

  fprintf(stderr, "Done sortkeys, doing qsort test\n");
  
  initCollator_current("ja");
  qsort(data, size, maxlen*sizeof(UChar), compare_current);
  closeCollator_current();

  fprintf(stderr, "Done current!\n");
}


