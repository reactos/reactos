/***
*mbccpy_s.c - Copy one character  to another (MBCS)
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Copy one MBCS character to another (1 or 2 bytes)
*
*******************************************************************************/

#include <corecrt_internal_mbstring.h>

#pragma warning(suppress:__WARNING_POSTCONDITION_NULLTERMINATION_VIOLATION) // 26036
_REDIRECT_TO_L_VERSION_4(errno_t, _mbccpy_s, unsigned char *, size_t , int *, const unsigned char *)
