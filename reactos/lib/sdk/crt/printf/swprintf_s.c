/*
 * COPYRIGHT:       GNU GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS crt library
 * FILE:            lib/sdk/crt/printf/swprintf_s.c
 * PURPOSE:         Implementation of swprintf_s
 * PROGRAMMER:      Timo Kreuzer
 */

#define _sxprintf swprintf_s
#define USE_COUNT 0
#define USE_VARARGS 0
#define _UNICODE
#define IS_SECAPI 1

#include "_sxprintf.c"
