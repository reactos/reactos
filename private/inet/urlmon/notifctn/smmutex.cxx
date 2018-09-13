//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       smmutex.cxx
//
//  Contents:   Class definition for shared memory mutex.
//
//  Classes:    CSmMutex
//
//  Functions:
//
//  History:    2-13-1997   JohannP (Johann Posch)   Created
//
//--------------------------------------------------------------------------
#include <notiftn.h>
#include "smmutex.hxx"

//+-------------------------------------------------------------------------
//
//  Member:     CSmMutex::Init
//
//  Synopsis:   Creates and/or gets access to inter-process mutex
//
//  Arguments:  [pszName] - name of mutex
//              [fGet] - whether to return with mutex owned.
//
//  Algorithm:
//
//  Notes:
//
//--------------------------------------------------------------------------
SCODE CSmMutex::Init(LPTSTR pszName, BOOL fGet)
{
    SCODE sc = S_OK;

    if (_hMutex == NULL)
    {

        // Holder for attributes to pass in on create.
        SECURITY_ATTRIBUTES secattr;

        secattr.nLength = sizeof(SECURITY_ATTRIBUTES);
        secattr.lpSecurityDescriptor = NULL;
        secattr.bInheritHandle = FALSE;

        // This class is designed based on the idea that any process
        // can be the creator of the mutex and therefore when
        // no processes are using the mutex it disappears.

        _hMutex = CreateMutex(&secattr, FALSE, pszName);

        if (_hMutex != NULL)
        {
            if (GetLastError() == ERROR_ALREADY_EXISTS)
            {
                // We know that after a handle is returned that if GetLastError
                // returns non-zero (actually ERROR_ALREADY_EXISTS), the current
                // process is not the one to create this object. So we set our
                // creation flag accordingly. The owner parameter is ignored by
                // CreateMutex if this isn't the first creator, so we want to
                // get the mutex as well so we can be sure that whoever created
                // it is done with it for the moment.

                _fCreated = FALSE;
            }
            else
            {
                _fCreated = TRUE;
            }
        }
        else
        {
            sc = HRESULT_FROM_WIN32(GetLastError());
        }
    }

    if (SUCCEEDED(sc) && (fGet))
    {
        Get();
    }

    // Note: we leave here with the mutex owned by this process iff the
    //       caller specified TRUE on the fGet parameter.
    return sc;
}
