//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:       atexit.cxx
//
//  Contents:   Provides our own implementation of the atexit function
//
//----------------------------------------------------------------------------

#include "headers.hxx"

typedef void (__cdecl *_PVFV)(void);

extern int      _cpvfv = 0;
extern _PVFV *  _apvfv = NULL;

EXTERN_C HANDLE g_hProcessHeap;

int __cdecl atexit(_PVFV pfunc)
{
    int rc = 1;

    if ((_cpvfv % 8) == 0)
    {
        if (_apvfv)
        {
            _apvfv = (_PVFV *) HeapReAlloc(
                    g_hProcessHeap,
                    0,
                    _apvfv,
                    (_cpvfv + 8) * sizeof(_PVFV));
            if (_apvfv == NULL)
                goto Error;
        }
        else
        {
            _apvfv = (_PVFV *) HeapAlloc(
                    g_hProcessHeap,
                    0,
                    (_cpvfv + 8) * sizeof(_PVFV));
            if (_apvfv == NULL)
                goto Error;
        }
    }
    _apvfv[_cpvfv++] = pfunc;

Cleanup:
    return rc;

Error:
    rc = 0;
    goto Cleanup;
}

