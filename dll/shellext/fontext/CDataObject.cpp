/*
 * PROJECT:     ReactOS Font Shell Extension
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     CFontMenu implementation
 * COPYRIGHT:   Copyright 2019,2020 Mark Jansen (mark.jansen@reactos.org)
 */

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(fontext);


#if 0
static inline void DumpDataObjectFormats(IDataObject* pObject)
{
    CComPtr<IEnumFORMATETC> pEnumFmt;
    HRESULT hr = pObject->EnumFormatEtc(DATADIR_GET, &pEnumFmt);

    if (FAILED_UNEXPECTEDLY(hr))
        return;

    FORMATETC fmt;
    while (S_OK == pEnumFmt->Next(1, &fmt, NULL))
    {
        char szBuf[512];
        GetClipboardFormatNameA(fmt.cfFormat, szBuf, sizeof(szBuf));
        ERR("Format: %s\n", szBuf);
        ERR("Tymed: %u\n", fmt.tymed);
        if (fmt.tymed & TYMED_HGLOBAL)
        {
            ERR("TYMED_HGLOBAL supported\n");
        }
    }
}
#endif


HRESULT _CDataObject_CreateInstance(PCIDLIST_ABSOLUTE folder, UINT cidl, PCUITEMID_CHILD_ARRAY apidl,
                                    REFIID riid, LPVOID* ppvOut)
{
    HRESULT hr = CIDLData_CreateFromIDArray(folder, cidl, apidl, (IDataObject**)ppvOut);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    // Now that we have an IDataObject with the shell itemid list (CFSTR_SHELLIDLIST, aka HIDA) format
    // we will augment this IDataObject with the CF_HDROP format. (Full filepaths)
    // This enabled the objects for the 'copy' and drag to copy actions

    CComHeapPtr<BYTE> data;

    // First we allocate room for the DROPFILES structure
    data.AllocateBytes(sizeof(DROPFILES));
    UINT offset = sizeof(DROPFILES);

    // Then we walk all files
    for (UINT n = 0; n < cidl; ++n)
    {
        const FontPidlEntry* fontEntry = _FontFromIL(apidl[n]);
        if (fontEntry)
        {
            CStringW File = g_FontCache->Filename(fontEntry, true);
            if (!File.IsEmpty())
            {
                // Now append the path (+ nullterminator) to the buffer
                UINT len = offset + (File.GetLength() + 1) * sizeof(WCHAR);
                data.ReallocateBytes(len);
                if (!data)
                {
                    ERR("Unable to allocate memory for the CF_HDROP\n");
                    return hr;
                }
                BYTE* dataPtr = data;
                StringCbCopyW((STRSAFE_LPWSTR)(dataPtr + offset), len - offset, File);
                offset = len;
            }
            else
            {
                ERR("No file found for %S\n", fontEntry->Name);
            }
        }
    }

    // Append the final nullterminator (double null terminated list)
    data.ReallocateBytes(offset + sizeof(UNICODE_NULL));
    LPWSTR str = (LPWSTR)((BYTE*)data + offset);
    *str = UNICODE_NULL;
    offset += sizeof(UNICODE_NULL);

    // Fill in the required fields
    DROPFILES* pDrop = (DROPFILES*)(BYTE*)data;
    pDrop->fWide = 1;
    pDrop->pFiles = sizeof(DROPFILES);
    // Zero out the rest
    pDrop->pt.x = pDrop->pt.y = 0;
    pDrop-> fNC = NULL;

    // Prepare the format descriptors
    STGMEDIUM medium = {0};
    medium.tymed = TYMED_HGLOBAL;

    // Copy the data to an HGLOBAL
    medium.hGlobal = GlobalAlloc(GHND, offset);
    if (medium.hGlobal)
    {
        LPVOID blob = GlobalLock(medium.hGlobal);
        if (blob)
        {
            CopyMemory(blob, (BYTE*)data, offset);
            GlobalUnlock(medium.hGlobal);

            CComPtr<IDataObject> spDataObject(*(IDataObject**)ppvOut);
            if (spDataObject)
            {
                FORMATETC etc = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
                hr = spDataObject->SetData(&etc, &medium, TRUE);
            }
        }
        else
        {
            ERR("Unable to lock the hGlobal?!\n");
        }
    }
    else
    {
        ERR("Unable to allocate %u bytes for the hGlobal\n", offset);
    }

    return hr;
}

