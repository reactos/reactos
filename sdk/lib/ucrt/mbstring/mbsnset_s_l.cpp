/***
*mbsnset_s_l.c - Sets first n characters of string to given character (MBCS)
*
*       Copyright (c) Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       Sets first n characters of string to given character (MBCS)
*
*******************************************************************************/
#ifndef _MBCS
    #error This file should only be compiled with _MBCS defined
#endif

#include <corecrt_internal_mbstring.h>
#include <corecrt_internal_securecrt.h>

#define _FUNC_NAME _mbsnset_s_l
#define _COUNT _CountInChars
#define _COUNT_IN_BYTES 0

#include "mbsnset_s.inl"
