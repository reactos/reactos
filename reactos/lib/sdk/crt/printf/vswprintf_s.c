/*
 * COPYRIGHT:       GNU GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS crt library
 * FILE:            lib/sdk/crt/printf/vswprintf_s.c
 * PURPOSE:         Implementation of vswprintf_s
 * PROGRAMMER:      Timo Kreuzer
 */

#define _sxprintf vswprintf_s
#define USE_COUNT 0
#define USE_VARARGS 1
#define _UNICODE
#define IS_SECAPI 1

#include "_sxprintf.c"
