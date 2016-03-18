/*
 * COPYRIGHT:       GNU GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS crt library
 * FILE:            lib/sdk/crt/printf/sprintf_s.c
 * PURPOSE:         Implementation of sprintf_s
 * PROGRAMMER:      Timo Kreuzer
 */

#define _sxprintf sprintf_s
#define USE_COUNT 0
#define IS_SECAPI 1

#include "_sxprintf.c"
