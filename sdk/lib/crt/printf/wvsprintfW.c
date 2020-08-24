/*
 * COPYRIGHT:       GNU GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS crt library
 * PURPOSE:         Implementation of wvsprintfW
 * PROGRAMMER:      Timo Kreuzer
 */

#define _sxprintf wvsprintfW
#define USE_COUNT 0
#define USE_VARARGS 1
#define _UNICODE
#define USER32_WSPRINTF

#include "_sxprintf.c"
