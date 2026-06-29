/***
*mbsncat_s_l.c - concatenate string2 onto string1, max length n
*
*       Copyright (c) Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       defines mbsncat_s_l() - concatenate maximum of n characters
*
*******************************************************************************/
#ifndef _MBCS
    #error This file should only be compiled with _MBCS defined
#endif

#include <corecrt_internal_mbstring.h>
#include <corecrt_internal_securecrt.h>

#define _FUNC_NAME _mbsncat_s_l
#define _COUNT _CountInChars
#define _COUNT_IN_BYTES 0

#include "mbsncat_s.inl"
