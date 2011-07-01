/*
 * COPYRIGHT:       GNU GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS crt library
 * FILE:            lib/sdk/crt/printf/_vsnwprintf.c
 * PURPOSE:         Implementation of _vsnwprintf
 * PROGRAMMER:      Timo Kreuzer
 */

#define _sxprintf _vsnwprintf
#define USE_COUNT 1
#define USE_VARARGS 1
#define _UNICODE

#include "_sxprintf.c"
