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

#else
    /* Map str*W functions to wcs* function */

    #define isspaceW iswspace
    #define strchrW  wcschr
    #define strcmpiW _wcsicmp
    #define strcpyW  wcscpy
    #define strlenW  wcslen
    #define strncmpW wcsncmp
    #define strtolW  wcstol
    #define strtoulW wcstoul

#endif

#endif
