/*
 * COPYRIGHT:       GNU GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS crt library
 * FILE:            lib/sdk/crt/printf/wvsprintfW.c
 * PURPOSE:         Implementation of wvsprintfW
 * PROGRAMMER:      Timo Kreuzer
 */

#define _sxprintf wvsprintfW
#define USE_COUNT 0
#define USE_VARARGS 1
#define _UNICODE
#define USER32_WSPRINTF

#include "_sxprintf.c"
