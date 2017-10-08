/*
 * COPYRIGHT:       GNU GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS crt library
 * FILE:            lib/sdk/crt/printf/wsprintfA.c
 * PURPOSE:         Implementation of wsprintfA
 * PROGRAMMER:      Timo Kreuzer
 */

#define _sxprintf wsprintfA
#define USE_COUNT 0
#define USER32_WSPRINTF

#include "_sxprintf.c"
