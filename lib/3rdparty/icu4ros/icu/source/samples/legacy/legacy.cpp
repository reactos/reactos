/*
*******************************************************************************
*
*   Copyright (C) 2001 - 2005, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  main.cpp
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2001jul24
*   created by: Vladimir Weinstein
*/

/******************************************************************************
 * main program demonstrating using two versions of ICU in the same project
 ******************************************************************************/

#include <stdio.h>
#include "unicode/utypes.h"
#include "unicode/ustring.h"

extern "C" void test_current(UChar data[][5], uint32_t size, uint32_t maxLen, uint8_t keys[][32]);
extern "C" void test_legacy(UChar data[][5], uint32_t size, uint32_t maxlen, uint8_t keys[][32]);

void printZTUChar(const UChar *str) {
  while(*str != 0) {
    if(*str > 0x1F && *str < 0x80) {
      fprintf(stdout, "%c", (*str) & 0xFF);
    } else {
      fprintf(stdout, "\\u%04X", *str);
    }
    str++;
  }
}

void printArray(const char* const comment, const UChar UArray[][5], int32_t arraySize) {
  fprintf (stdout, "%s\n", comment);
  int32_t i = 0;
  for(i = 0; i<arraySize; i++) {
    fprintf(stdout, "%d ", i);
    printZTUChar(UArray[i]);
    fprintf(stdout, "\n");
  }
}

void printKeys(const char *comment, uint8_t keys[][32], int32_t keySize) {
  int32_t i = 0;
  uint8_t *currentKey = NULL;
  fprintf(stdout, "%s\n", comment);
  for(i = 0; i<keySize; i++) {
    currentKey = keys[i];
    while(*currentKey != 0) {
      if(*currentKey == 1) {
        fprintf(stdout, "01 ");
      } else {
        fprintf(stdout, "%02X", *currentKey);
      }
      currentKey++;
    }
    fprintf(stdout, " 00\n");
  }
}

    
//int main(int argc, const char * const argv[]) {
int main(int, const char * const *) {
  static const char* test[4] = {
    "\\u304D\\u3085\\u3046\\u0000",
    "\\u30AD\\u30E6\\u30A6\\u0000",
    "\\u304D\\u3086\\u3046\\u0000",
    "\\u30AD\\u30E5\\u30A6\\u0000"
  };

#if 0
  static const char* test2[4] = {
    "dbc\\u0000",
      "cbc\\u0000",
      "bbc\\u0000",
      "abc\\u0000"
  };
#endif

  static UChar uTest[4][5];

  static uint8_t keys[4][32];

  uint32_t i = 0;

  for(i = 0; i<4; i++) {
    u_unescape(test[i], uTest[i], 5);
  }
  printArray("Before current", uTest, 4);
  test_current(uTest, 4, 5, keys);
  printArray("After current", uTest, 4);
  printKeys("Current keys", keys, 4);

  for(i = 0; i<4; i++) {
    u_unescape(test[i], uTest[i], 5);
  }
  printArray("Before legacy", uTest, 4);
  test_legacy(uTest, 4, 5, keys);
  printArray("After legacy", uTest, 4);
  printKeys("Legacy keys", keys, 4);


  return 0;
}
