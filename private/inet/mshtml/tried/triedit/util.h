// util.h : Utility inlines/functions
// Copyright (c)1999 Microsoft Corporation, All Rights Reserved
// added during 64 bit port of triedit.

// All throughout the triedit code, we assume that 
// the size of the document is not going to exceed
// 2GB. We assert the fact when we open the document
// in filter.cpp as well. If at later point, we do exceed 
// the document size of 2GB, we need to change this too.

// Safe conversion of pointer differences.

#ifndef __UTIL_H_
#define __UTIL_H_

inline WORD SAFE_PTR_DIFF_TO_WORD(SIZE_T lptrDiff)
{
    _ASSERTE(lptrDiff <= USHRT_MAX);
    return((WORD)lptrDiff);
};
inline WORD SAFE_INT_DIFF_TO_WORD(int iDiff)
{
    _ASSERTE(iDiff <= USHRT_MAX);
    return((WORD)iDiff);
};
inline int SAFE_PTR_DIFF_TO_INT(SIZE_T lptrDiff)
{
    _ASSERTE(lptrDiff <= UINT_MAX);
    return((int)lptrDiff);
};
inline DWORD SAFE_INT64_TO_DWORD(SIZE_T iValue)
{
    _ASSERTE(iValue <= UINT_MAX);
    return((DWORD)iValue);
};
#endif __UTIL_H_