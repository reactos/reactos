/*
 * COPYRIGHT:       GNU GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS crt library
 * PURPOSE:         Implementation of _snwprintf_s
 * PROGRAMMER:      Timo Kreuzer
 */

#define _sxprintf _snwprintf_s
#define USE_COUNT 1
#define _UNICODE
#define IS_SECAPI 1

#include "_sxprintf.c"
