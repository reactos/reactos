/*
 * COPYRIGHT:       GNU GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS crt library
 * FILE:            lib/sdk/crt/printf/_vsnwprintf_s.c
 * PURPOSE:         Implementation of _vsnwprintf_s
 * PROGRAMMER:      Timo Kreuzer
 */

#define _sxprintf _vsnwprintf_s
#define USE_COUNT 1
#define USE_VARARGS 1
#define _UNICODE
#define IS_SECAPI 1

#include "_sxprintf.c"
