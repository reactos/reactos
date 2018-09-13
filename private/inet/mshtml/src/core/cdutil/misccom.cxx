//+------------------------------------------------------------------------
//
//  File:       misccom.cxx
//
//  Contents:   Misc COM object helper functions.
//
//-------------------------------------------------------------------------

#include "headers.hxx"



//+------------------------------------------------------------------------
//
//  Function:   ClearInterfaceFn
//
//  Synopsis:   Sets an interface pointer to NULL, after first calling
//              Release if the pointer was not NULL initially
//
//  Arguments:  [ppUnk]     *ppUnk is cleared
//
//-------------------------------------------------------------------------

void
ClearInterfaceFn(IUnknown ** ppUnk)
{
    IUnknown * pUnk;

    pUnk = *ppUnk;
    *ppUnk = NULL;
    if (pUnk)
        pUnk->Release();
}



//+------------------------------------------------------------------------
//
//  Function:   ClearClassFn
//
//  Synopsis:   Nulls a pointer to a class, releasing the class via the
//              provided IUnknown implementation if the original pointer
//              is non-NULL.
//
//  Arguments:  [ppv]
//              [pUnk]
//
//-------------------------------------------------------------------------

void
ClearClassFn(void ** ppv, IUnknown * pUnk)
{
    *ppv = NULL;
    if (pUnk)
        pUnk->Release();
}



//+------------------------------------------------------------------------
//
//  Function:   ReplaceInterfaceFn
//
//  Synopsis:   Replaces an interface pointer with a new interface,
//              following proper ref counting rules:
//
//              = *ppUnk is set to pUnk
//              = if *ppUnk was not NULL initially, it is Release'd
//              = if pUnk is not NULL, it is AddRef'd
//
//              Effectively, this allows pointer assignment for ref-counted
//              pointers.
//
//  Arguments:  [ppUnk]
//              [pUnk]
//
//-------------------------------------------------------------------------

void
ReplaceInterfaceFn(IUnknown ** ppUnk, IUnknown * pUnk)
{
    IUnknown * pUnkOld = *ppUnk;

    *ppUnk = pUnk;

    //  Note that we do AddRef before Release; this avoids
    //    accidentally destroying an object if this function
    //    is passed two aliases to it

    if (pUnk)
        pUnk->AddRef();

    if (pUnkOld)
        pUnkOld->Release();
}



//+------------------------------------------------------------------------
//
//  Function:   ReleaseInterface
//
//  Synopsis:   Releases an interface pointer if it is non-NULL
//
//  Arguments:  [pUnk]
//
//-------------------------------------------------------------------------

void
ReleaseInterface(IUnknown * pUnk)
{
    if (pUnk)
        pUnk->Release();
}


//+---------------------------------------------------------------
//
// Function:    IsSameObject
//
// Synopsis:    Checks for COM identity
//
// Arguments:   pUnkLeft, pUnkRight
//
//+---------------------------------------------------------------

BOOL
IsSameObject(IUnknown *pUnkLeft, IUnknown *pUnkRight)
{
    IUnknown *pUnk1, *pUnk2;

    if (pUnkLeft == pUnkRight)
        return TRUE;

    if (pUnkLeft == NULL || pUnkRight == NULL)
        return FALSE;

    if (SUCCEEDED(pUnkLeft->QueryInterface(IID_IUnknown, (LPVOID *)&pUnk1)))
    {
        pUnk1->Release();
        if (pUnk1 == pUnkRight)
            return TRUE;
        if (SUCCEEDED(pUnkRight->QueryInterface(IID_IUnknown, (LPVOID *)&pUnk2)))
        {
            pUnk2->Release();
            return pUnk1 == pUnk2;
        }
    }
    return FALSE;
}

//+---------------------------------------------------------------
//
//  Function:   GetResource
//
//  Synopsis:   Loads any kind of resource.
//
//  Arguments:  [hinst] -- instance of the module with the resource
//              [lpstrId] -- the identifier of the resource
//              [lpstrType] -- the identifier of the resource type
//              [pcbSize] -- points to the out param: the number of bytes of resource data to load
//
//  Returns:    lpvBuf if the resource was successfully loaded, NULL otherwise
//
//  Notes:      This function combines Windows' FindResource, LoadResource,
//              LockResource.
//
//----------------------------------------------------------------

LPVOID
GetResource(HINSTANCE hinst,
            LPCTSTR lpstrId,
            LPCTSTR lpstrType,
            ULONG * pcbSize)
{
    LPVOID  lpv;
    HGLOBAL hgbl;
    HRSRC   hrsrc;

    hrsrc = FindResource(hinst, lpstrId, lpstrType);
    if (!hrsrc)
        return NULL;

    hgbl = LoadResource(hinst, hrsrc);
    if (!hgbl)
        return NULL;

    lpv = LockResource(hgbl);
    if ( pcbSize )
    {
        *pcbSize = lpv ? ::SizeofResource(hinst, hrsrc) : 0;
    }

#if !defined(_MAC) && !defined(UNIX)
#ifndef WIN16           //BUGBUGWIN16: on win16, this destroyes lpv
                        // need to fix this...
    //  Win95 is said to need this
    FreeResource(hgbl);
#endif    
#endif

    return lpv;
}


