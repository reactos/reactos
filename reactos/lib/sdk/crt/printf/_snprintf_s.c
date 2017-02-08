/*
 * COPYRIGHT:       GNU GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS crt library
 * FILE:            lib/sdk/crt/printf/_snprintf_s.c
 * PURPOSE:         Implementation of _snprintf_s
 * PROGRAMMER:      Timo Kreuzer
 */

#define _sxprintf _snprintf_s
#define USE_COUNT 1
#define IS_SECAPI 1

#include "_sxprintf.c"
