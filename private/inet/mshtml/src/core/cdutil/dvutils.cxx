//+---------------------------------------------------------------------
//
//  File:       dvutils.cxx
//
//  Contents:   Helper functions for implementing IDataObject and IViewObject
//
//----------------------------------------------------------------------

#include "headers.hxx"

//
//  Globals
//


// Names used to register the standard clipboard formats
//  Localization: Do not localize
static char * s_apstrClipName[] =
{
    "Embedded Object",
    "Embed Source",
    "Link Source",
    "Link Source Descriptor",
    "Object Descriptor",
    "MS Forms CLSID",
    "MS Forms Text",
};
// Array of common registered clip formats used in Forms^3
CLIPFORMAT g_acfCommon[ARRAY_SIZE(s_apstrClipName)];

HRESULT
CloneStgMedium(const STGMEDIUM *    pcstgmedSrc,
               STGMEDIUM *          pstgmedDest)
{
    HRESULT         hr;
    void *          pvDest;
    const void *    pcvSrc;
    DWORD           dwcbLen;
    HGLOBAL         hGlobalDest;

    memset(pstgmedDest, 0, sizeof(*pstgmedDest));

    // We only understand how to clone TYMED_HGLOBAL.
    if (pcstgmedSrc->tymed != TYMED_HGLOBAL)
    {
        hr = DV_E_TYMED;
        goto Cleanup;
    }

    hr = E_OUTOFMEMORY;
    pcvSrc = GlobalLock(pcstgmedSrc->hGlobal);
    if (!pcvSrc)
        goto Cleanup;


    dwcbLen = GlobalSize(pcstgmedSrc->hGlobal);
    hGlobalDest = GlobalAlloc((GMEM_MOVEABLE | GMEM_SHARE), dwcbLen);
    if (!hGlobalDest)
    {
        GlobalUnlock(pcstgmedSrc->hGlobal);
        goto Cleanup;
    }


    pvDest = GlobalLock(hGlobalDest);
    if (!pvDest)
    {
        GlobalFree(hGlobalDest);
        GlobalUnlock(pcstgmedSrc->hGlobal);
        goto Cleanup;
    }

    memcpy(pvDest, pcvSrc, dwcbLen);

    pstgmedDest->tymed = TYMED_HGLOBAL;
    pstgmedDest->hGlobal = hGlobalDest;
    pstgmedDest->pUnkForRelease = pcstgmedSrc->pUnkForRelease;
    if (pstgmedDest->pUnkForRelease)
        (pstgmedDest->pUnkForRelease)->AddRef();

    GlobalUnlock(hGlobalDest);
    GlobalUnlock(pcstgmedSrc->hGlobal);
    hr = S_OK;

Cleanup:
    RRETURN(hr);
}


BOOL
DVTARGETDEVICEMatchesRequest(const DVTARGETDEVICE * pcdvtdRequest,
                             const DVTARGETDEVICE * pcdvtdActual)
{
    BOOL bMatch;

    /*
    * A NULL requested PCDVTARGETDEVICE matches any NULL or non-NULL actual
    * PCDVTARGETDEVICE.
    *
    * Any non-NULL requested PCDVTARGETDEVICE matches any NULL actual
    * PCDVTARGETDEVICE.
    *
    * A non-NULL requested PCDVTARGETDEVICE only matches a non-NULL actual
    * PCDVTARGETDEVICE if the actual CDVTARGETDEVICE is an exact binary copy of
    * the requested CDVTARGETDEVICE.
    */

    if (pcdvtdRequest && pcdvtdActual)
        bMatch = (memcmp(pcdvtdRequest, pcdvtdActual,
                         min(pcdvtdRequest->tdSize, pcdvtdActual->tdSize))
                  == 0);
    else
        bMatch = TRUE;

    return(bMatch);
}


BOOL
TYMEDMatchesRequest(TYMED tymedRequest, TYMED tymedActual)
{
    // The actual TYMED matches the requested TYMED if they have any flags set
    // in common.
    return(tymedRequest & tymedActual);
}


BOOL
FORMATETCMatchesRequest(const FORMATETC *   pcfmtetcRequest,
                        const FORMATETC *   pcfmtetcActual)
{
    // Don't check lindex. Strictly speaking, it should be ignored only
    // for DVASPECT_ICON and DVASPECT_THUMBNAIL, but our clients (notably
    // Athena) usually set it to 0 instead of the correct default -1.
    return(pcfmtetcRequest->cfFormat == pcfmtetcActual->cfFormat &&
           DVTARGETDEVICEMatchesRequest(pcfmtetcRequest->ptd,
                                        pcfmtetcActual->ptd) &&
           pcfmtetcRequest->dwAspect == pcfmtetcActual->dwAspect &&
           TYMEDMatchesRequest((TYMED)(pcfmtetcRequest->tymed),
                               (TYMED)(pcfmtetcActual->tymed)));
}


//+---------------------------------------------------------------
//
//  Function:   RegisterClipFormats
//
//  Synopsis:   Initialize g_acfCommon, an array of common
//              registered clip formats used in Forms^3.
//              This array is indexed by the ICF_xxx enumeration.
//
//----------------------------------------------------------------

void
RegisterClipFormats()
{
    // Mac note: clipboard formats need not be registered because we are using
    //          Mac OLE instead of WLM OLE; Mac OLE uses a 4 character ID
    int i;

    for (i = 0; i < ARRAY_SIZE(g_acfCommon); i++)
    {
        g_acfCommon[i] = (CLIPFORMAT)
                RegisterClipboardFormatA(s_apstrClipName[i]);
    }
}

//+---------------------------------------------------------------
//
//  Function:   SetCommonClipFormats
//
//  Synopsis:   Set FORMATETC::cfFormat initialzed by CF_COMMON()
//              macro to the true registered clip format.
//
//  Arguments:  pfmtetc Array of to modify
//              cfmtetc Number of elements in the array
//
//----------------------------------------------------------------

void
SetCommonClipFormats(FORMATETC *pfmtetc, int cfmtetc)
{
    for (; cfmtetc > 0; pfmtetc++, cfmtetc--)
    {
        if (pfmtetc->cfFormat >= CF_PRIVATEFIRST &&
            pfmtetc->cfFormat <= CF_PRIVATELAST)
        {
            Assert(pfmtetc->cfFormat - CF_PRIVATEFIRST < ARRAY_SIZE(g_acfCommon));
            pfmtetc->cfFormat = g_acfCommon[pfmtetc->cfFormat - CF_PRIVATEFIRST];
        }
    }
}

//+---------------------------------------------------------------
//
//  Function:   FindCompatibleFormat
//
//  Synopsis:   Searches a table of FORMATETC structures and
//              returns the index of the first entry that is
//              compatible with a specified FORMATETC.
//
//  Arguments:  [FmtTable] -- the table of FORMATETCs
//              [iSize] -- the number of entries in the format table
//              [formatetc] -- the FORMATETC we are comparing for compatibility
//
//  Returns:    The index into the table of the compatible format, or
//              -1 if no compatible format was found.
//
//  Notes:      This function is typically used in conjunction with
//              IDataObject methods that need to check if a requested format
//              is available.
//
//----------------------------------------------------------------

int
FindCompatibleFormat(FORMATETC FmtTable[], int iSize, FORMATETC& formatetc)
{
    // look through the table for a compatible format
    for (int i = 0; i < iSize; i++)
    {
        if (FORMATETCMatchesRequest(&formatetc, &FmtTable[i]))
            return i;
    }
    return -1;
}


//+---------------------------------------------------------------
//
//  Function:   GetObjectDescriptor
//
//  Synopsis:   Extracts an OBJECTDESCRIPTOR from an IDataObject,
//              if available.
//
//  Arguments:  [pDataObj] -- data object from which to extract an object descriptor
//              [pDescOut] -- object descriptor structure to fill in
//
//  Returns:    Success iff the object descriptor could be extracted.
//              This does not copy out the dwFullUserTypeName or
//              dwSrcOfCopy strings.
//
//
//  Attention: most containers (like Excel) do not support GetDataHere
//             therefore we use GetData (frankman, Bug 5889)
//
//----------------------------------------------------------------

HRESULT
GetObjectDescriptor(LPDATAOBJECT pDataObj, LPOBJECTDESCRIPTOR pDescOut)
{
    HRESULT r;
    FORMATETC formatetc =
        { g_acfCommon[ICF_OBJECTDESCRIPTOR],
            NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };

    STGMEDIUM stgmedium;
    stgmedium.tymed = TYMED_HGLOBAL;
    stgmedium.hGlobal = 0;
    stgmedium.pUnkForRelease = NULL;

    if (OK(r = pDataObj->GetData(&formatetc, &stgmedium)))
    {
        if (pDescOut != NULL)
        {
            LPOBJECTDESCRIPTOR pObjDesc = (LPOBJECTDESCRIPTOR)GlobalLock(stgmedium.hGlobal);
            if (pObjDesc == NULL)
            {
                r = E_OUTOFMEMORY;
            }
            else
            {
                // note: in the future we may wish to copy out the strings
                // into two out parameters.  This would be used in
                // implementing the Paste Special dialog box.
                *pDescOut = *pObjDesc;
                pDescOut->dwFullUserTypeName = 0;
                pDescOut->dwSrcOfCopy = 0;
                GlobalUnlock(stgmedium.hGlobal);
            }
        }
    }
    ReleaseStgMedium(&stgmedium);
    RRETURN(r);
}

//+---------------------------------------------------------------
//
//  Function:   UpdateObjectDescriptor
//
//  Synopsis:   Updates the pointl and dwDrawAspects of an OBJECTDESCRIPTOR
//              on a data object
//
//  Arguments:  [pDataObj] -- the data object to update
//              [ptl] -- the pointl to update in the object descriptor
//              [dwAspect] -- the draw aspect to update in the object descriptor
//
//  Returns:    Success iff the object descriptor could be updated
//
//  Notes:      This method is for IDataObjects used in drag-drop.
//              The object being dragged supplies the object descriptor but only
//              the container knows where the point that the mouse button went
//              down relative to the corner of the object, and what aspect
//              of the object the container is displaying.
//              The container uses this method to fill in that missing information.
//              This performs a GetDataHere on the object to get a filled-in
//              object descriptor.  It then updates the pointl and dwDrawAspect
//              fields and uses SetData to update the object.
//
//----------------------------------------------------------------

HRESULT
UpdateObjectDescriptor(LPDATAOBJECT pDataObj, POINTL& ptl, DWORD dwAspect)
{
    HRESULT r;
    FORMATETC formatetc =
        { g_acfCommon[ICF_OBJECTDESCRIPTOR],
            NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };

    STGMEDIUM stgmedium;
    stgmedium.tymed = TYMED_HGLOBAL;
    stgmedium.hGlobal = 0;
    stgmedium.pUnkForRelease = NULL;
    if (OK(r = pDataObj->GetData(&formatetc, &stgmedium)))
    {
        LPOBJECTDESCRIPTOR pObjDesc = (LPOBJECTDESCRIPTOR)GlobalLock(stgmedium.hGlobal);
        if (pObjDesc == NULL)
        {
            r = E_OUTOFMEMORY;
        }
        else
        {
            pObjDesc->pointl = ptl;
            pObjDesc->dwDrawAspect = dwAspect;
            r = pDataObj->SetData(&formatetc, &stgmedium, FALSE);
            GlobalUnlock(stgmedium.hGlobal);
        }
    }

    ReleaseStgMedium(&stgmedium);
    RRETURN(r);
}



//+-------------------------------------------------------------------------
//
//  Member:     FormSetClipboard(IDataObject *pdo)
//
//  Synopsis:   helper function to set the clipboard contents
//
//--------------------------------------------------------------------------
HRESULT
FormSetClipboard(IDataObject *pdo)
{
    HRESULT hr;
    hr = OleSetClipboard(pdo);
    
    if (!hr && !GetPrimaryObjectCount())
    {
        hr = THR(OleFlushClipboard());
    }
    else
    {
        TLS(pDataClip) = pdo;
    }

    RRETURN(hr);
}
