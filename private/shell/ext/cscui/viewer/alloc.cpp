//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       alloc.cpp
//
//--------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "alloc.h"


void * __cdecl operator new(
    size_t size
    )
{
    void *pv = LocalAlloc(LMEM_FIXED, size);
    if (NULL == pv)
        throw CException(ERROR_OUTOFMEMORY);

    return pv;
}

void __cdecl operator delete(
    void *ptr
    ) throw()
{
    if (NULL != ptr)
        LocalFree(ptr);
}

