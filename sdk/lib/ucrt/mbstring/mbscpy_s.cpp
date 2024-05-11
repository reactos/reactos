/***
*mbscpy_s.c - Copy one string to another (MBCS)
*
*       Copyright (c) Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       Copy one string to another (MBCS)
*
*******************************************************************************/
#ifndef _MBCS
    #error This file should only be compiled with _MBCS defined
#endif

#include <corecrt_internal_mbstring.h>

_REDIRECT_TO_L_VERSION_3(errno_t, _mbscpy_s, unsigned char *, size_t, const unsigned char *)
