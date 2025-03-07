/***
*mbsset_s_l.c - Sets first n bytes of string to given character (MBCS)
*
*       Copyright (c) Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       Sets first n bytes of string to given character (MBCS)
*
*******************************************************************************/
#ifndef _MBCS
    #error This file should only be compiled with _MBCS defined
#endif

#include <corecrt_internal_mbstring.h>
#include <corecrt_internal_securecrt.h>

#define _FUNC_NAME _mbsnbset_s_l
#define _COUNT _CountInBytes
#define _COUNT_IN_BYTES 1

#include "mbsnset_s.inl"
