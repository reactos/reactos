/*
 * COPYRIGHT:       GNU GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS crt library
 * PURPOSE:         Implementation of vsprintf
 * PROGRAMMER:      Timo Kreuzer
 */

#define _sxprintf vsprintf
#define USE_COUNT 0
#define USE_VARARGS 1

#include "_sxprintf.c"
