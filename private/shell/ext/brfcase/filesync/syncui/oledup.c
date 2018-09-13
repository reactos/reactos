//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 1993-1994
//
// File: oledup.c
//
//  This files contains duplicated code the OLE would provide.
//  We do this so we don't have to link to OLE for M6.
//  BUGBUG: we should remove this for M7 (why?  the shell does this too)
//
// History:
//  02-14-94 ScottH     Created (copied from shell)
//
//---------------------------------------------------------------------------

/////////////////////////////////////////////////////  INCLUDES

#include "brfprv.h"         // common headers


HRESULT MyReleaseStgMedium(LPSTGMEDIUM pmedium)
    {
    if (pmedium->pUnkForRelease)
        {
        pmedium->pUnkForRelease->lpVtbl->Release(pmedium->pUnkForRelease);
        }
    else
        {
        switch(pmedium->tymed)
            {
        case TYMED_HGLOBAL:
            GlobalFree(pmedium->hGlobal);
            break;

        default:
            // Not fullly implemented.
            MessageBeep(0);
            break;
            }
        }

    return NOERROR;
    }
