/*
 * COPYRIGHT:       GNU GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS crt library
 * FILE:            lib/sdk/crt/printf/_vsprintf_p.c
 * PURPOSE:         Implementation of _vsprintf_p
 * PROGRAMMER:      Samuel Serapión
 */

#define _sxprintf _vsprintf_p
#define USE_COUNT 1
#define USE_VARARGS 1

#include "_sxprintf.c"
