/***
*mbsnbcat_s_l.c - concatenate string2 onto string1, max length n bytes
*
*       Copyright (c) Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       defines mbsnbcat_s_l() - concatenate maximum of n bytes
*
*******************************************************************************/
#ifndef _MBCS
    #error This file should only be compiled with _MBCS defined
#endif

#include <corecrt_internal_mbstring.h>
#include <corecrt_internal_securecrt.h>

#define _FUNC_NAME _mbsnbcat_s_l
#define _COUNT _CountInBytes
#define _COUNT_IN_BYTES 1

#include "mbsncat_s.inl"
