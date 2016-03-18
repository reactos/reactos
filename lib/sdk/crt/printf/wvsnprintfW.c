/*
 * COPYRIGHT:       GNU GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS crt library
 * FILE:            lib/sdk/crt/printf/wvsnprintfW.c
 * PURPOSE:         Implementation of wvsnprintfW
 * PROGRAMMER:      Timo Kreuzer
 */

#define _sxprintf wvsnprintfW
#define USE_COUNT 1
#define USE_VARARGS 1
#define _UNICODE
#define USER32_WSPRINTF

#include "_sxprintf.c"
