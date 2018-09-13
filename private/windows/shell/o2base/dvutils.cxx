//+---------------------------------------------------------------------
//
//  File:       dvutils.cxx
//
//  Contents:   Helper functions for implementing IDataObject and IViewObject
//
//----------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

//
//  Globals
//

UINT OleClipFormat[OCF_LAST+1];         // array of OLE standard clipboard formats

// these are the names of the standard OLE clipboard formats that need to
// be registered.
LPWSTR OleClipNames[OCF_LAST+1] =
{
    L"ObjectLink",
    L"OwnerLink",
    L"Native",
    L"FileName",
    L"NetworkName",
    L"DataObject",
    L"Embedded Object",
    L"Embed Source",
    L"Link Source",
    L"Link Source Descriptor",
    L"Object Descriptor",
    L"OleDraw"
};


//+---------------------------------------------------------------
//
//  Function:   RegisterOleClipFormats
//
//  Synopsis:   Initializes the OleClipFormat table of standard
//              OLE clipboard formats.
//
//  Notes:      The OleClipFormat table is a table of registered,
//              standard, OLE-related clipboard formats.  The table
//              is indexed by the OLECLIPFORMAT enumeration.
//              Before this table can be used it must be initialized
//              via this function.
//              This function is usually called in the WinMain or
//              LibMain of the module using the OleClipFormat table.
//
//----------------------------------------------------------------

void
RegisterOleClipFormats(void)
{
    for (int i = 0; i<= OCF_LAST; i++)
        OleClipFormat[i] = RegisterClipboardFormat(OleClipNames[i]);
}

//+---------------------------------------------------------------
//
//  Function:   IsCompatibleDevice, public
//
//  Synopsis:   Compares two DEVICETARGET structures and returns
//              TRUE if they are compatible.
//
//  Arguments:  [ptdLeft] -- A pointer to a device target
//              [ptdRight] -- Another pointer to a device target
//
//  Notes:      The target devices are compatible if they are both
//              NULL or if they are identical.  Otherwise they are
//              incompatible.
//
//----------------------------------------------------------------

BOOL
IsCompatibleDevice(DVTARGETDEVICE FAR* ptdLeft, DVTARGETDEVICE FAR* ptdRight)
{
    // same address of td; must be same (handles NULL case)
    if (ptdLeft == ptdRight)
        return TRUE;

    // if ones NULL, and the others not then they are incompatible
    if ((ptdRight == NULL) || (ptdLeft == NULL))
        return FALSE;

    // different sizes, not equal
    if (ptdLeft->tdSize != ptdRight->tdSize)
        return FALSE;

    return _fmemcmp(ptdLeft, ptdRight, (size_t)ptdLeft->tdSize) == 0;
}

//+---------------------------------------------------------------
//
//  Function:   IsCompatibleFormat, public
//
//  Synopsis:   Compares two FORMATETC structures and returns
//              TRUE if they are compatible.
//
//  Arguments:  [f1] -- A FORMATETC structure
//              [f2] -- Another FORMATETC structure
//
//  Notes:      This function ignores the lindex member of the
//              FORMATETCs.
//
//----------------------------------------------------------------

BOOL
IsCompatibleFormat(FORMATETC& f1, FORMATETC& f2)
{
    return f1.cfFormat == f2.cfFormat
            && IsCompatibleDevice(f1.ptd, f2.ptd)
            && (f1.dwAspect & f2.dwAspect) != 0L
            && (f1.tymed & f2.tymed) != 0;
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
        if (IsCompatibleFormat(formatetc, FmtTable[i]))
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
//----------------------------------------------------------------

HRESULT
GetObjectDescriptor(LPDATAOBJECT pDataObj, LPOBJECTDESCRIPTOR pDescOut)
{
    HRESULT r;
    HGLOBAL hglobal = GlobalAlloc(GMEM_SHARE, sizeof(OBJECTDESCRIPTOR)+256);
    LPOBJECTDESCRIPTOR pObjDesc = (LPOBJECTDESCRIPTOR)GlobalLock(hglobal);
    if (pObjDesc == NULL)
    {
        DOUT(L"o2base/dvutils/GetObjectDescriptor failed\r\n");
        r = E_OUTOFMEMORY;
    }
    else
    {
        FORMATETC formatetc =
            { (CLIPFORMAT)OleClipFormat[OCF_OBJECTDESCRIPTOR],
                NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };

        STGMEDIUM stgmedium;
        stgmedium.tymed = TYMED_HGLOBAL;
        stgmedium.hGlobal = hglobal;
        stgmedium.pUnkForRelease = NULL;

        if (OK(r = pDataObj->GetDataHere(&formatetc, &stgmedium)))
        {
            if (pDescOut != NULL)
            {
                // note: in the future we may wish to copy out the strings
                // into two out parameters.  This would be used in
                // implementing the Paste Special dialog box.
                *pDescOut = *pObjDesc;
                pDescOut->dwFullUserTypeName = 0;
                pDescOut->dwSrcOfCopy = 0;
            }
        }
        GlobalUnlock(hglobal);
        GlobalFree(hglobal);
    }
    return r;
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
    HGLOBAL hglobal = GlobalAlloc(GMEM_SHARE, sizeof(OBJECTDESCRIPTOR)+256);
    LPOBJECTDESCRIPTOR pObjDesc = (LPOBJECTDESCRIPTOR)GlobalLock(hglobal);
    if (pObjDesc == NULL)
    {
        DOUT(L"o2base/dvutils/UpdateObjectDescriptor failed\r\n");
        r = E_OUTOFMEMORY;
    }
    else
    {
        FORMATETC formatetc =
            { (CLIPFORMAT)OleClipFormat[OCF_OBJECTDESCRIPTOR],
                NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };

        STGMEDIUM stgmedium;
        stgmedium.tymed = TYMED_HGLOBAL;
        stgmedium.hGlobal = hglobal;
        stgmedium.pUnkForRelease = NULL;

        if (OK(r = pDataObj->GetDataHere(&formatetc, &stgmedium)))
        {
            pObjDesc->pointl = ptl;
            pObjDesc->dwDrawAspect = dwAspect;
            r = pDataObj->SetData(&formatetc, &stgmedium, FALSE);
        }
        GlobalUnlock(hglobal);
        GlobalFree(hglobal);
    }
    return r;
}

//+---------------------------------------------------------------
//
//  Function:   DrawMetafile
//
//  Synopsis:   Creates a metafile from an IViewObject using the Draw method
//
//  Arguments:  [pVwObj] -- the view object
//              [rc] -- rectangle on the view object to draw
//              [dwAspect] -- aspect of the view object to draw, typically
//                           content or icon
//              [pHMF] -- place where handle to the metafile is drawn
//
//  Returns:    Success iff the metafile was drawn successfully.
//
//----------------------------------------------------------------

HRESULT
DrawMetafile(LPVIEWOBJECT pVwObj,
        RECT& rc,
        DWORD dwAspect,
        HMETAFILE FAR* pHMF)
{
    HRESULT r;
    HMETAFILE hmf;
    HDC hdc;

    if ((hdc = CreateMetaFile(NULL)) == NULL)
    {
        DOUT(L"o2base/dvutils/DrawMetafile failed to create metafile\r\n");
        r = E_OUTOFMEMORY;
    }
    else
    {
        SetMapMode(hdc, MM_ANISOTROPIC);
        SetWindowOrgEx(hdc, 0, 0, NULL);
        SetWindowExtEx(hdc, rc.right, rc.bottom, NULL);

        r = OleDraw(pVwObj, dwAspect, hdc, &rc);

        hmf = CloseMetaFile(hdc);

        if (!OK(r))
        {
            DeleteMetaFile(hmf);
            hmf = NULL;
        }

        if (hmf == NULL)
        {
            DOUT(L"o2base/dvutils/DrawMetafile failed\r\n");
            r = E_OUTOFMEMORY;
        }
    }
    *pHMF = hmf;
    return r;
}


