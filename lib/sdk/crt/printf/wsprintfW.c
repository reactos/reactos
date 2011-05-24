/*
 * COPYRIGHT:       GNU GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS crt library
 * FILE:            lib/sdk/crt/printf/wsprintfW.c
 * PURPOSE:         Implementation of wsprintfW
 * PROGRAMMER:      Timo Kreuzer
 */

#define _sxprintf wsprintfW
#define USE_COUNT 0
#define _UNICODE
#define USER32_WSPRINTF

#include "_sxprintf.c"
