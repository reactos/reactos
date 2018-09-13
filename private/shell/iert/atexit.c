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
#define _CRTBLD 1

#include "windows.h"

typedef void (__cdecl *_PVFV)(void);

extern int      _cpvfv = 0;
extern _PVFV *  _apvfv = NULL;

int __cdecl atexit(_PVFV pfunc)
{
    int rc = 1;

    if ((_cpvfv % 8) == 0)
    {
        if (_apvfv)
        {
            _PVFV * p = (_PVFV *) LocalReAlloc(_apvfv, (_cpvfv + 8) * sizeof(_PVFV), LMEM_MOVEABLE);
            if (_apvfv == NULL)
                goto Error;
            _apvfv = p;
        }
        else
        {
            _apvfv = (_PVFV *) LocalAlloc(LPTR, (_cpvfv + 8) * sizeof(_PVFV));
            if (_apvfv == NULL)
                goto Error;
        }
    }
    _apvfv[_cpvfv++] = pfunc;

Cleanup:
    return rc;

Error:
    rc = 0;
    _cpvfv = 0;
    goto Cleanup;
}

