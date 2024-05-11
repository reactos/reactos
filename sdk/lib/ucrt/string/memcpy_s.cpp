//
// memcpy_s.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Provides external definitions of the inline functions memcpy_s and memmove_s
// for use by objects compiled with older versions of the CRT headers.
//
#define _CRT_MEMCPY_S_INLINE extern __inline
#include <corecrt_memcpy_s.h>
