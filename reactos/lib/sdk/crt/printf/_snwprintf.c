/*
 * COPYRIGHT:       GNU GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS crt library
 * FILE:            lib/sdk/crt/printf/_snwprintf.c
 * PURPOSE:         Implementation of _snwprintf
 * PROGRAMMER:      Timo Kreuzer
 */

#define _sxprintf _snwprintf
#define USE_COUNT 1
#define _UNICODE

#include "_sxprintf.c"
