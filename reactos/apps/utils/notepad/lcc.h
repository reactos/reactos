/*
 * lcc.h
 *
 * Copyright 1999 by Marcel Baur <mbaur@g26.ethz.ch>
 * To be distributed under the Wine license
 *
 * This file is only required when compiling Notepad using LCC-WIN32
 */

#ifdef LCC

   #include <assert.h>
   #include "shellapi.h"

   #define HANDLE HANDLE
   #define INVALID_HANDLE_VALUE INVALID_HANDLE_VALUE

   #define WINE_RELEASE_INFO "Compiled using LCC"
   #define OIC_WINLOGO 0

   #ifndef LCC_HASASSERT
    /* prevent multiple inclusion of assert methods */

     int _assertfail(char *__msg, char *__cond, char *__file, int __line) {
  
       CHAR szMessage[255];

       strcat(szMessage, "Assert failure ");
       strcat(szMessage, __msg);
       strcat(szMessage, "\n");
       strcat(szMessage, "in line ");
       strcat(szMessage, "of file ");
       strcat(szMessage, __line);

       MessageBox(0, szMessage, "ERROR: ASSERT FAILURE", MB_ICONEXCLAMATION);
    }
  #endif

#endif
