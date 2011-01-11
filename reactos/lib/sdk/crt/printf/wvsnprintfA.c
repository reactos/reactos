/*
 * COPYRIGHT:       GNU GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS crt library
 * FILE:            lib/sdk/crt/printf/wvsnprintfA.c
 * PURPOSE:         Implementation of wvsnprintfA
 * PROGRAMMER:      Timo Kreuzer
 */

#define _sxprintf wvsnprintfA
#define USE_COUNT 1
#define USE_VARARGS 1
#define USER32_WSPRINTF

#include "_sxprintf.c"
