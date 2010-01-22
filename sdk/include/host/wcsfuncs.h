/*
  PROJECT:    ReactOS
  LICENSE:    GPL v2 or any later version
  FILE:       include/host/wcsfuncs.h
  PURPOSE:    Header for the "host_wcsfuncs" static library
  COPYRIGHT:  Copyright 2008 Colin Finck <mail@colinfinck.de>
*/

#ifndef _HOST_WCSFUNCS_H
#define _HOST_WCSFUNCS_H

#ifdef USE_HOST_WCSFUNCS
    /* Function prototypes */
    SIZE_T utf16_wcslen(PCWSTR str);
    PWSTR utf16_wcschr(PWSTR str, WCHAR c);
    INT utf16_wcsncmp(PCWSTR string1, PCWSTR string2, size_t count);
#else
    /* Define the utf16_ functions to the CRT functions */
    #define utf16_wcslen  wcslen
    #define utf16_wcschr  wcschr
    #define utf16_wcsncmp wcsncmp
#endif

#endif
