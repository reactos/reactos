/*
 * COPYRIGHT:       GNU GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS crt library
 * FILE:            lib/sdk/crt/string/_wsplitpath_s.c
 * PURPOSE:         Implementation of _wsplitpath_s
 * PROGRAMMER:      Timo Kreuzer
 */

#define _tsplitpath_x _wsplitpath_s
#define _UNICODE
#define IS_SECAPI 1

#include "_tsplitpath_x.h"
