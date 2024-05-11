/***
*mbsnbcpy_s_l.c - Copy one string to another, n bytes only (MBCS)
*
*       Copyright (c) Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       Copy one string to another, n bytes only (MBCS)
*
*******************************************************************************/
#ifndef _MBCS
    #error This file should only be compiled with _MBCS defined
#endif

#include <corecrt_internal_mbstring.h>
#include <corecrt_internal_securecrt.h>

#define _FUNC_NAME _mbsnbcpy_s_l
#define _COUNT _CountInBytes
#define _COUNT_IN_BYTES 1

#include "mbsncpy_s.inl"
