//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1995.
//
//  File:       dutil.cxx
//
//  Contents:   Utility functions and macros for data objects
//
//  History:    14-Dec-95    BruceFo     Created
//
//----------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

#include "dutil.hxx"

LPIDA
DataObj_GetHIDA(
    LPDATAOBJECT pdtobj,
    STGMEDIUM* pmedium
    )
{
    FORMATETC fmte = {g_cfHIDA, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};

    if (NULL != pmedium)
    {
        pmedium->pUnkForRelease = NULL;
        pmedium->hGlobal = NULL;
    }

    if (NULL == pmedium)
    {
        if (SUCCEEDED(pdtobj->QueryGetData(&fmte)))
        {
            return (LPIDA)TRUE;
        }
        else
        {
            return (LPIDA)FALSE;
        }
    }
    else if (SUCCEEDED(pdtobj->GetData(&fmte, pmedium)))
    {
        return (LPIDA)GlobalLock(pmedium->hGlobal);
    }
    return NULL;
}


VOID
HIDA_ReleaseStgMedium(
    LPIDA pida,
    STGMEDIUM* pmedium
    )
{
    if ((NULL != pmedium->hGlobal) && (pmedium->tymed==TYMED_HGLOBAL))
    {
#if DBG == 1
        if (NULL != pida)
        {
            LPIDA pidaT = (LPIDA)GlobalLock(pmedium->hGlobal);
            appAssert(pidaT == pida);
            GlobalUnlock(pmedium->hGlobal);
        }
#endif
        GlobalUnlock(pmedium->hGlobal);
    }
    else
    {
        appAssert(FALSE);
    }

    ReleaseStgMedium(pmedium);
}

LPITEMIDLIST*
ILA_Clone(
    UINT cidl,
    LPCITEMIDLIST* apidl
    )
{
    LPITEMIDLIST* aNewPidl = new LPITEMIDLIST[cidl];
    if (NULL == aNewPidl)
    {
        return NULL;
    }

    for (UINT i = 0; i < cidl; i++)
    {
        aNewPidl[i] = ILClone(apidl[i]);
        if (NULL == aNewPidl[i])
        {
            // delete what we've allocated so far
            for (UINT j = 0; j < i; j++)
            {
                ILFree(aNewPidl[i]);
            }
            delete[] aNewPidl;
            return NULL;
        }
    }

    return aNewPidl;
}

VOID
ILA_Free(
    UINT cidl,
    LPITEMIDLIST* apidl
    )
{
    for (UINT i = 0; i < cidl; i++)
    {
        ILFree(apidl[i]);
    }
    delete[] apidl;
}
