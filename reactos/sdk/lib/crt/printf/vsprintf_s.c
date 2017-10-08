/*
 * COPYRIGHT:       GNU GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS crt library
 * FILE:            lib/sdk/crt/printf/vsprintf_s.c
 * PURPOSE:         Implementation of vsprintf_s
 * PROGRAMMER:      Timo Kreuzer
 */

#define _sxprintf vsprintf_s
#define USE_COUNT 0
#define USE_VARARGS 1
#define IS_SECAPI 1

#include "_sxprintf.c"
