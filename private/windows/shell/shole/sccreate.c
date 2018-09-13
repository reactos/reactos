#include "shole.h"
#include "ids.h"
#include <winnlsp.h>
#include <stdio.h>

#define ARRAYSIZE(a)    (sizeof(a)/sizeof(a[0]))

#ifdef SAVE_OBJECTDESCRIPTOR
extern "C" const WCHAR c_wszDescriptor[] = WSTR_SCRAPITEM L"ODS";


HRESULT Scrap_SaveODToStream(IStorage *pstgDoc, OBJECTDESCRIPTOR * pods)
{
    //
    // According to Anthony Kitowicz, we must clear this flag.
    //
    pods->dwStatus &= ~OLEMISC_CANLINKBYOLE1;

    IStream *pstm;
    HRESULT hres = pstgDoc->CreateStream(c_wszDescriptor, STGM_CREATE|STGM_WRITE|STGM_SHARE_EXCLUSIVE, 0, 0, &pstm);
    if (SUCCEEDED(hres))
    {
        ULONG cbWritten;
        hres = pstm->Write(pods, pods->cbSize, &cbWritten);
        DebugMsg(DM_TRACE, TEXT("sc TR - Scrap_SOD descriptor written (%x, %d, %d)"),
                 hres, pods->cbSize, cbWritten);
        pstm->Release();

        if (FAILED(hres) || cbWritten<pods->cbSize) {
            pstgDoc->DestroyElement(c_wszDescriptor);
        }
    }
    else
    {
        DebugMsg(DM_TRACE, TEXT("sc TR - Scrap_SOD pstg->CreateStream failed (%x)"), hres);
    }

    return hres;
}

HRESULT Scrap_SaveObjectDescriptor(IStorage *pstgDoc, IDataObject *pdtobj, IPersistStorage *pps, BOOL fLink)
{
    STGMEDIUM medium;
    FORMATETC fmte = {fLink ? CF_LINKSRCDESCRIPTOR : CF_OBJECTDESCRIPTOR, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    HRESULT hres = pdtobj->GetData(&fmte, &medium);
    if (hres == S_OK)
    {
        DebugMsg(DM_TRACE, TEXT("sc TR - Scrap_SOD found CF_OBJECTDESCRIPTOR (%x)"), medium.hGlobal);
        LPOBJECTDESCRIPTOR pods = (LPOBJECTDESCRIPTOR)GlobalLock(medium.hGlobal);
        if (pods)
        {
            hres = Scrap_SaveODToStream(pstgDoc, pods);
            GlobalUnlock(medium.hGlobal);
        }
        ReleaseStgMedium(&medium);
    }
//
// Attempt to create object descriptor
//
#if 0
    else
    {
        hres = E_FAIL;
        DebugMsg(DM_TRACE, TEXT("sc TR - Scrap_SOD CAN'T find CF_OBJECTDESCRIPTOR (%x)"), hres);
        LPOLEOBJECT pole;
        if (SUCCEEDED(pps->QueryInterface(IID_IOleObject, (LPVOID*)&pole)))
        {
            IMalloc *pmem;
            if (SUCCEEDED(CoGetMalloc(MEMCTX_TASK, &pmem)))
            {
                LPWSTR pwsz;
                if (SUCCEEDED(pole->GetUserType(USERCLASSTYPE_FULL, &pwsz)))
                {
                    UINT cbStr = pmem->GetSize(pwsz) + 1;
                    UINT cb = sizeof(OBJECTDESCRIPTOR) + cbStr;
                    LPOBJECTDESCRIPTOR pods=(LPOBJECTDESCRIPTOR)LocalAlloc(LPTR, cb);
                    if (pods)
                    {
                        pods->cbSize = cb;
                        pole->GetUserClassID(&pods->clsid);
                        pole->GetMiscStatus(DVASPECT_CONTENT, &pods->dwStatus);
                        pole->GetExtent(DVASPECT_CONTENT, &pods->sizel);
                        pods->dwFullUserTypeName = sizeof(OBJECTDESCRIPTOR);
                        MoveMemory(pods+1, pwsz, cbStr);
                        hres = Scrap_SaveODToStream(pstgDoc, pods);
                    }
                    pmem->Free(pwsz);
                }
                pmem->Release();
            }
            pole->Release();
        }
    }
#endif

    return hres;
}
#else
#define Scrap_SaveObjectDescriptor(pstgDoc, pdtobj, fLink) (0)
#endif // SAVE_OBJECTDESCRIPTOR

#ifdef FIX_ROUNDTRIP
extern "C" const TCHAR c_szCLSID[] = TEXT("CLSID");

//
//  This function opens the HKEY for the specified CLSID or its sub-key.
//
// Parameters:
//  rclsid    -- Specifies the CLSID
//  pszSubKey -- Specifies the subkey name, may be NULL
//
// Returns:
//  non-NULL, if succeeded; the caller must RegCloseKey it.
//  NULL, if failed.
//
HKEY _OpenCLSIDKey(REFCLSID rclsid, LPCTSTR pszSubKey)
{
#ifdef UNICODE
    WCHAR szCLSID[256];
    if (StringFromGUID2(rclsid, szCLSID, ARRAYSIZE(szCLSID)))
    {
#else
    WCHAR wszCLSID[256];
    if (StringFromGUID2(rclsid, wszCLSID, ARRAYSIZE(wszCLSID)))
    {
        TCHAR szCLSID[80];
        WideCharToMultiByte(CP_ACP, 0, wszCLSID, -1, szCLSID, ARRAYSIZE(szCLSID), NULL, NULL);
#endif

        TCHAR szKey[256];
        if (pszSubKey) {
            wsprintf(szKey, TEXT("%s\\%s\\%s"), c_szCLSID, szCLSID, pszSubKey);
        } else {
            wsprintf(szKey, TEXT("%s\\%s"), c_szCLSID, szCLSID);
        }
        DebugMsg(DM_TRACE, TEXT("sc TR - _OpelCLSIDKey RegOpenKey(%s)"), szKey);

        HKEY hkey;
        if (RegOpenKey(HKEY_CLASSES_ROOT, szKey, &hkey)==ERROR_SUCCESS)
        {
            return hkey;
        }
    }
    return NULL;
}

extern "C" const WCHAR c_wszFormatNames[] = WSTR_SCRAPITEM L"FMT";
#define CCH_FORMATNAMES (ARRAYSIZE(c_wszFormatNames)-1)

//
//  This function generates the stream name (UNICODE) for the spcified
// clipboard format.
//
// Parameters:
//  pszFormat -- Specifies the clipboard format ("#0"-"#15" for predefined ones)
//  wszStreamName -- Specifies the UNICODE buffer.
//  cchmax -- Specifies the size of buffer.
//
void _GetCacheStreamName(LPCTSTR pszFormat, LPWSTR wszStreamName, UINT cchMax)
{
    MoveMemory(wszStreamName, c_wszFormatNames, sizeof(c_wszFormatNames));
#ifdef UNICODE
    lstrcpyn(wszStreamName+CCH_FORMATNAMES,pszFormat,cchMax-CCH_FORMATNAMES);
#ifdef DEBUG
    DebugMsg(DM_TRACE, TEXT("sc TR _GetCacheStreamName returning %s"), wszStreamName);
#endif
#else
    MultiByteToWideChar(CP_ACP, 0, pszFormat, -1,
                        wszStreamName+CCH_FORMATNAMES,
                        cchMax-CCH_FORMATNAMES);
#ifdef DEBUG
    TCHAR szT[256];
    WideCharToMultiByte(CP_ACP, 0, wszStreamName, -1, szT, ARRAYSIZE(szT), NULL, NULL);
    DebugMsg(DM_TRACE, TEXT("sc TR _GetCacheStreamName returning %s"), szT);
#endif
#endif
}

HRESULT Scrap_CacheOnePictureFormat(LPCTSTR pszFormat, FORMATETC * pfmte, STGMEDIUM * pmedium, REFCLSID rclsid, LPSTORAGE pstgDoc, LPDATAOBJECT pdtobj)
{
    LPPERSISTSTORAGE ppstg;
    HRESULT hres = OleCreateDefaultHandler(rclsid, NULL, IID_IPersistStorage, (LPVOID *)&ppstg);
    DebugMsg(DM_TRACE, TEXT("sc TR Scrap_CacheOPF OleCreteDefHandler returned %x"), hres);
    if (SUCCEEDED(hres))
    {
        //
        // Generate the stream name based on the clipboard format name.
        //
        WCHAR wszStorageName[256];
        _GetCacheStreamName(pszFormat, wszStorageName, ARRAYSIZE(wszStorageName));

        LPSTORAGE pstgPicture;
        hres = pstgDoc->CreateStorage(wszStorageName, STGM_DIRECT | STGM_READWRITE | STGM_CREATE | STGM_SHARE_EXCLUSIVE,
                    0, 0, &pstgPicture);
        if (SUCCEEDED(hres))
        {
            ppstg->InitNew(pstgPicture);

            LPOLECACHE pcache;
            hres = ppstg->QueryInterface(IID_IOleCache, (LPVOID*)&pcache);
            DebugMsg(DM_TRACE, TEXT("sc TR Scrap_CacheOPF QI returned %x"), hres);
            if (SUCCEEDED(hres))
            {
                hres = pcache->Cache(pfmte, ADVF_PRIMEFIRST, NULL);
                DebugMsg(DM_TRACE, TEXT("sc TR pcache->Cache returned %x"), hres);
                hres = pcache->SetData(pfmte, pmedium, FALSE);
                DebugMsg(DM_TRACE, TEXT("sc TR pcache->SetData returned %x"), hres);
                pcache->Release();

                if (SUCCEEDED(hres))
                {
                    hres = OleSave(ppstg, pstgPicture, TRUE);
                    DebugMsg(DM_TRACE, TEXT("sc TR Scrap_CacheOnePictureFormat OleSave returned (%x)"), hres);
                    ppstg->HandsOffStorage();

                    if (SUCCEEDED(hres))
                    {
                        hres = pstgPicture->Commit(STGC_OVERWRITE);
                        DebugMsg(DM_TRACE, TEXT("sc TR Scrap_CacheOnePictureFormat Commit() returned (%x)"), hres);
                    }
                }
            }

            pstgPicture->Release();

            if (FAILED(hres))
            {
                pstgDoc->DestroyElement(wszStorageName);
            }
        }

        ppstg->Release();
    }

    return hres;
}

//
//  This function stores the specified format of clipboard data.
//
// Parameters:
//  pszFormat -- Specifies the clipboard format ("#0"-"#15" for predefined ones)
//  pstgDoc -- Specifies the top level IStorage.
//  pdtobj -- Sepcified the data object we should get the data from.
//
HRESULT Scrap_CacheOneFormat(LPCTSTR pszFormat, LPSTORAGE pstgDoc, LPDATAOBJECT pdtobj)
{
    UINT cf = RegisterClipboardFormat(pszFormat);
    STGMEDIUM medium;
    FORMATETC fmte = {(CLIPFORMAT)cf, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};

    const CLSID * pclsid = NULL;
    switch(cf)
    {
    case CF_METAFILEPICT:
        pclsid = &CLSID_Picture_Metafile;
        fmte.tymed = TYMED_MFPICT;
        break;

    case CF_ENHMETAFILE:
        pclsid = &CLSID_Picture_EnhMetafile;
        fmte.tymed = TYMED_ENHMF;
        break;

    case CF_PALETTE:
    case CF_BITMAP:
        pclsid = &CLSID_Picture_Dib;
        fmte.tymed = TYMED_GDI;
        break;
    }

    //
    // Get the specified format of data (TYMED_GLOBAL only)
    //
    HRESULT hres = pdtobj->GetData(&fmte, &medium);
    if (hres == S_OK)
    {
        if (medium.tymed != TYMED_HGLOBAL)
        {
            hres = Scrap_CacheOnePictureFormat(pszFormat, &fmte, &medium, *pclsid, pstgDoc, pdtobj);
        }
        else
        {
            //
            // Global lock the data.
            //
            UINT cbData = (UINT)GlobalSize(medium.hGlobal);
            const BYTE * pbData = (const BYTE*)GlobalLock(medium.hGlobal);
            if (pbData)
            {
                //
                // Generate the stream name based on the clipboard format name.
                //
                WCHAR wszStreamName[256];
                _GetCacheStreamName(pszFormat, wszStreamName, ARRAYSIZE(wszStreamName));

                //
                // Create the stream.
                //
                LPSTREAM pstm;
                hres = pstgDoc->CreateStream(wszStreamName, STGM_CREATE|STGM_WRITE|STGM_SHARE_EXCLUSIVE, 0, 0, &pstm);
                if (SUCCEEDED(hres))
                {
                    //
                    // Save the size of data.
                    //
                    ULONG cbWritten;
                    hres = pstm->Write(&cbData, sizeof(cbData), &cbWritten);
                    if (SUCCEEDED(hres) && cbWritten==sizeof(cbData))
                    {
                        //
                        // Save the data itself.
                        //
                        hres = pstm->Write(pbData, cbData, &cbWritten);
                        DebugMsg(DM_TRACE, TEXT("sc TR - Scrap_Save %s written (%x, %d, %d)"),
                             pszFormat, hres, cbData, cbWritten);
                    }
                    pstm->Release();

                    //
                    // If anything goes wrong, destroy the stream.
                    //
                    if (FAILED(hres) || cbWritten<cbData)
                    {
                        pstgDoc->DestroyElement(wszStreamName);
                        hres = E_FAIL;
                    }
                }
                GlobalUnlock(medium.hGlobal);
            }
            else
            {
                hres = E_FAIL;
            }
        }
        ReleaseStgMedium(&medium);
    }
    else
    {
        DebugMsg(DM_TRACE, TEXT("sc TR - Scrap_CacheOneFormat IDO::GetData(cf=%x,tm=%x) failed (%x)"),
                 fmte.cfFormat, fmte.tymed, hres);
    }

    return hres;
}

//
//  This function caches the specified format of data if the data object
// support that format.
//
// Parameters:
//  szFormat -- Specifies the format to be cahced
//  pstgDoc  -- Specifies the top level IStorage
//  pdtobj   -- Specifies the data object from where we get data
//  pstm     -- Specifies the stream we should write cached format name.
//
// Returns:
//  TRUE if the data object support it.
//
BOOL Scrap_MayCacheOneFormat(LPCTSTR szFormat, LPSTORAGE pstgDoc, LPDATAOBJECT pdtobj, LPSTREAM pstm)
{
    //
    // Try to cache the format.
    //
    HRESULT hres = Scrap_CacheOneFormat(szFormat, pstgDoc, pdtobj);
    if (SUCCEEDED(hres))
    {
        //
        //  Store the name of format only if we actually
        // succeeded to cache the data.
        //
#ifdef UNICODE
        CHAR szAnsiFormat[128];
        WideCharToMultiByte(CP_ACP, 0,
                            szFormat, -1,
                            szAnsiFormat, ARRAYSIZE(szAnsiFormat),
                            NULL, NULL );
        USHORT cb = (USHORT)lstrlenA(szAnsiFormat);
#else
        USHORT cb = (USHORT)lstrlen(szFormat);
#endif
        pstm->Write(&cb, sizeof(cb), NULL);
        DebugMsg(DM_TRACE, TEXT("sc TR - Scrap_MayCacheOneFormat writing %s, %d"), szFormat, cb);
#ifdef UNICODE
        pstm->Write(szAnsiFormat, cb, NULL);
#else
        pstm->Write(szFormat, cb, NULL);
#endif

        return TRUE;
    }

    return FALSE;
}

//
// Returns:
//  TRUE, if the specified format is already cached (from Global list).
//
BOOL Scrap_IsAlreadyCached(UINT acf[], UINT ccf, LPCTSTR szFormat)
{
    if (ccf)
    {
        UINT cf = RegisterClipboardFormat(szFormat);
        for (UINT icf=0; icf<ccf; icf++) {
            if (acf[icf]==cf) {
                return TRUE;
            }
        }
    }
    return FALSE;
}

extern "C" const TCHAR c_szCacheFMT[] = TEXT("DataFormats\\PriorityCacheFormats");
#define REGSTR_PATH_SCRAP TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\ShellScrap")
extern "C" const TCHAR c_szGlobalCachedFormats[] = REGSTR_PATH_SCRAP TEXT("\\PriorityCacheFormats");

//
//  This function enumerate the list of to-be-cached clipboard data and
// calls Scrap_CacheOneFormat for each of them.
//
// Parameters:
//  pstgDoc -- Specifies the top level IStorage.
//  pdtobj  -- Specifies the data object we'll get the data from.
//  pps     -- Specifies the "embedded object" (to get CLSID from)
//
void Scrap_CacheClipboardData(LPSTORAGE pstgDoc, LPDATAOBJECT pdtobj, LPPERSIST pps)
{
    //
    //  Create the stream where we'll store the list of actually
    // cached formats, which might be just a subset of to-be-cached
    // format specified in the registry.
    //
    LPSTREAM pstm;
    HRESULT hres = pstgDoc->CreateStream(c_wszFormatNames, STGM_CREATE|STGM_WRITE|STGM_SHARE_EXCLUSIVE, 0, 0, &pstm);

    DebugMsg(DM_TRACE, TEXT("sc TR S_CCD CreateStream returned %x"), hres);

    if (SUCCEEDED(hres))
    {
        USHORT cb;
        HKEY hkey;
        TCHAR szFormatName[128];
        DWORD cchValueName;
        DWORD dwType;
        UINT  acf[CCF_CACHE_GLOBAL];
        UINT  ccf = 0;

        //
        // First, try the formats in the global list.
        //
        if (RegOpenKey(HKEY_LOCAL_MACHINE, c_szGlobalCachedFormats, &hkey)==ERROR_SUCCESS)
        {
            //
            // For each global to-be-cached format...
            //
            for(int iValue=0; iValue<CCF_CACHE_GLOBAL ;iValue++)
            {
                //
                //  Get the value name of the iValue'th value. The value
                // name specifies the clipboard format.
                // ("#0"-"#15" for predefined formats).
                //
                cchValueName = ARRAYSIZE(szFormatName);
                if (RegEnumValue(hkey, iValue, szFormatName, &cchValueName, NULL,
                                 &dwType, NULL, NULL)==ERROR_SUCCESS)
                {
                    DebugMsg(DM_TRACE, TEXT("sc TR - Scrap_CacheClipboardData found with %s, %x (Global)"), szFormatName, dwType);
                    if (Scrap_MayCacheOneFormat(szFormatName, pstgDoc, pdtobj, pstm))
                    {
                        acf[ccf++] = RegisterClipboardFormat(szFormatName);
                    }
                }
                else
                {
                    break;
                }
            }

            RegCloseKey(hkey);
        }

        //
        // Then, try the CLSID specific formats.
        //
        // Get the CLSID of the "embedded object" (the body of scrap)
        //
        CLSID clsid;
        hres = pps->GetClassID(&clsid);
        if (SUCCEEDED(hres))
        {
            //
            // Open the key for the list of to-be-cached formats.
            //
            hkey = _OpenCLSIDKey(clsid, c_szCacheFMT);
            if (hkey)
            {
                //
                // For each class specific to-be-cached format...
                //
                for(int iValue=0; iValue<CCF_CACHE_CLSID ;iValue++)
                {
                    //
                    //  Get the value name of the iValue'th value. The value
                    // name specifies the clipboard format.
                    // ("#0"-"#15" for predefined formats).
                    //
                    cchValueName = ARRAYSIZE(szFormatName);

                    if (RegEnumValue(hkey, iValue, szFormatName, &cchValueName, NULL,
                                     &dwType, NULL, NULL)==ERROR_SUCCESS)
                    {
                        if (!Scrap_IsAlreadyCached(acf, ccf, szFormatName))
                        {
                            DebugMsg(DM_TRACE, TEXT("sc TR - Scrap_CacheClipboardData found with %s, %x (CLSID specific)"), szFormatName, dwType);
                            Scrap_MayCacheOneFormat(szFormatName, pstgDoc, pdtobj, pstm);
                        }
                        else
                        {
                            DebugMsg(DM_TRACE, TEXT("sc TR - Scrap_CacheClipboardData skipping %s (already cached)"), szFormatName);
                        }
                    }
                    else
                    {
                        break;
                    }
                }

                //
                // HACK: NT 3.5's RegEdit does not support named values...
                //
                for(iValue=0; iValue<CCF_CACHE_CLSID ;iValue++)
                {
                    TCHAR szKeyName[128];
                    if (RegEnumKey(hkey, iValue, szKeyName, ARRAYSIZE(szKeyName))==ERROR_SUCCESS)
                    {
                        LONG cbValue = ARRAYSIZE(szFormatName);
                        if ((RegQueryValue(hkey, szKeyName, szFormatName, &cbValue)==ERROR_SUCCESS) && cbValue)
                        {
                            if (!Scrap_IsAlreadyCached(acf, ccf, szFormatName))
                            {
                                DebugMsg(DM_TRACE, TEXT("sc TR - Scrap_CacheClipboardData found with %s, %x (CLSID specific)"), szFormatName, dwType);
                                Scrap_MayCacheOneFormat(szFormatName, pstgDoc, pdtobj, pstm);
                            }
                            else
                            {
                                DebugMsg(DM_TRACE, TEXT("sc TR - Scrap_CacheClipboardData skipping %s (already cached)"), szFormatName);
                            }
                        }
                    }
                    else
                    {
                        break;
                    }
                }

                RegCloseKey(hkey);
            }
        }

        //
        // Put the terminator.
        //
        cb = 0;
        pstm->Write(&cb, sizeof(cb), NULL);
        pstm->Release();

    }

}
#endif // FIX_ROUNDTRIP

// out:
//      pszName - short name for object type ("Worksheet", "Word Document", etc)
//
// returns:
//

HRESULT Scrap_Save(IStorage *pstg, IStorage *pstgDoc, IDataObject *pdtobj, BOOL fLink, LPTSTR pszName)
{
    IPersistStorage *pps;
    HRESULT hres;

    if (fLink)
    {
        FORMATETC fmte = {CF_METAFILEPICT, NULL, DVASPECT_ICON, -1, TYMED_MFPICT};
        hres = OleCreateLinkFromData(pdtobj, IID_IPersistStorage, OLERENDER_FORMAT,
                                     &fmte, NULL, pstg, (LPVOID*)&pps);
        DebugMsg(DM_TRACE, TEXT("sc Scrap_Save OleCreateLinkFromData(FMT) returned (%x)"), hres);
    }
    else
    {
        hres = OleCreateFromData(pdtobj, IID_IPersistStorage, OLERENDER_DRAW,
                                 NULL, NULL, pstg, (LPVOID*)&pps);
        DebugMsg(DM_TRACE, TEXT("sc Scrap_Save OleCreateFromData(FMT) returned (%x)"), hres);
    }

    if (SUCCEEDED(hres))
    {
        hres = OleSave(pps, pstg, TRUE);        // fSameStorage=TRUE

        DebugMsg(DM_TRACE, TEXT("sc Scrap_Save OleSave returned (%x)"), hres);

        if (SUCCEEDED(hres) && pszName)
        {
            LPOLEOBJECT pole;
            if (SUCCEEDED(pps->QueryInterface(IID_IOleObject, (LPVOID*)&pole)))
            {
                IMalloc *pmem;
                if (SUCCEEDED(CoGetMalloc(MEMCTX_TASK, &pmem)))
                {
                    LPWSTR pwsz;
                    if (SUCCEEDED(pole->GetUserType(USERCLASSTYPE_SHORT, &pwsz)))
                    {
#ifdef UNICODE
                        lstrcpyn(pszName, pwsz, 64);    // What is 64?
#else
                        WideCharToMultiByte(CP_ACP, 0, pwsz, -1, pszName, 64, NULL, NULL);
#endif

                        DebugMsg(DM_TRACE, TEXT("sc Scrap_Save short name (%s)"), pszName);

                        // Assert(lstrlen(pszName) < 15); // USERCLASSTYPE_SHORT docs say so
                        pmem->Free(pwsz);
                    }
                    pmem->Release();
                }
                pole->Release();
            }
        }

        // This is no-op if SAVE_OBJECTDESCRIPTOR is not defined.
        Scrap_SaveObjectDescriptor(pstgDoc, pdtobj, pps, fLink);

#ifdef FIX_ROUNDTRIP
        if (!fLink)
        {
            Scrap_CacheClipboardData(pstgDoc, pdtobj, pps);
        }
#endif // FIX_ROUNDTRIP

        hres = pps->HandsOffStorage();

        pps->Release();
    }

    return hres;
}

// get some text from the data object
//
// out:
//      pszOut  filled in with text
//

HRESULT Scrap_GetText(IDataObject *pdtobj, LPTSTR pszOut, UINT cchMax)
{
    UINT cbMac = (cchMax-1)*sizeof(pszOut[0]);
    memset(pszOut, 0, cchMax * sizeof(pszOut[0]));

    STGMEDIUM medium;
    HRESULT hres;
#ifdef UNICODE
    UINT CodePage = CP_ACP;
    BOOL bUnicode = TRUE;

    FORMATETC fmte = { CF_UNICODETEXT, NULL, DVASPECT_CONTENT, -1, TYMED_ISTREAM | TYMED_HGLOBAL };
    hres = pdtobj->QueryGetData( &fmte );
    if (hres != S_OK)           // S_FALSE means no.
    {
        fmte.cfFormat = CF_TEXT;
        bUnicode = FALSE;
    }
    hres = pdtobj->GetData(&fmte, &medium);

    if (bUnicode == FALSE && hres == S_OK)
    {
        //
        //  We have ANSI text but no UNICODE Text.  Look for RTF in order to
        //  see if we can find a language id so that we can use the correct
        //  code page for the Ansi to Unicode translation.
        //
        FORMATETC fetcrtf = { 0, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL | TYMED_ISTREAM };
        fetcrtf.cfFormat = (CLIPFORMAT)RegisterClipboardFormat(TEXT("Rich Text Format"));
        STGMEDIUM mediumrtf;

        if (SUCCEEDED(pdtobj->GetData(&fetcrtf, &mediumrtf)))
        {
            CHAR szBuf[MAX_PATH * 8];
            LPSTR pszRTF = NULL;

            if (mediumrtf.tymed == TYMED_ISTREAM)
            {
                if (SUCCEEDED(mediumrtf.pstm->Read((LPTSTR)szBuf, ARRAYSIZE(szBuf), NULL)))
                {
                    pszRTF = szBuf;
                }
            }
            else
            {
                pszRTF = (LPSTR)GlobalLock(mediumrtf.hGlobal);
            }

            if (pszRTF)
            {
                LPSTR pTmp;

                //
                //  Find the language id used for this text.
                //
                //  Ugly way to search, but can't use c-runtimes in the
                //  shell.
                //
                CHAR szLang[5];
                UINT LangID = 0;

                pTmp = pszRTF;
                while (*pTmp)
                {
                    if ((*pTmp == '\\') &&
                        *(pTmp + 1)    && (*(pTmp + 1) == 'l') &&
                        *(pTmp + 2)    && (*(pTmp + 2) == 'a') &&
                        *(pTmp + 3)    && (*(pTmp + 3) == 'n') &&
                        *(pTmp + 4)    && (*(pTmp + 4) == 'g'))
                    {
                        //
                        //  Get number following the \lang identifier.
                        //
                        int ctr;

                        pTmp += 5;
                        for (ctr = 0;
                             (ctr < 4) && (*(pTmp + ctr)) &&
                             ((*(pTmp + ctr)) >= '0') && ((*(pTmp + ctr)) <= '9');
                             ctr++)
                        {
                            szLang[ctr] = *(pTmp + ctr);
                        }
                        szLang[ctr] = 0;

                        for (pTmp = szLang; *pTmp; pTmp++)
                        {
                            LangID *= 10;
                            LangID += (*pTmp - '0');
                        }

                        break;
                    }
                    pTmp++;
                }
                if (LangID)
                {
                    if (!GetLocaleInfo( LangID,
                                        LOCALE_IDEFAULTANSICODEPAGE |
                                          LOCALE_RETURN_NUMBER,
                                        (LPTSTR)&CodePage,
                                        sizeof(UINT) / sizeof(TCHAR) ))
                    {
                        CodePage = CP_ACP;
                    }
                }

                if (mediumrtf.tymed == TYMED_HGLOBAL)
                {
                    GlobalUnlock(mediumrtf.hGlobal);
                }
            }
        }
    }

#else
    FORMATETC fmte = { CF_TEXT, NULL, DVASPECT_CONTENT, -1, TYMED_ISTREAM|TYMED_HGLOBAL };
    hres = pdtobj->GetData(&fmte, &medium);
#endif

    if (SUCCEEDED(hres))
    {
        DebugMsg(DM_TRACE, TEXT("sh TR - Scrap_GetText found CF_TEXT/CF_UNICODETEXT in %d"), medium.tymed);
        if (medium.tymed == TYMED_ISTREAM)
        {
            hres = medium.pstm->Read(pszOut, cbMac, NULL);
        }
        else if (medium.tymed == TYMED_HGLOBAL)
        {
            DebugMsg(DM_TRACE, TEXT("sh TR - Scrap_GetText found CF_TEXT/CF_UNICODETEXT in global"));
            LPCTSTR pszSrc = (LPCTSTR)GlobalLock(medium.hGlobal);
            if (pszSrc)
            {
#ifdef UNICODE
                if ( fmte.cfFormat == CF_TEXT )
                {
                    MultiByteToWideChar( CodePage, 0,
                                         (LPSTR)pszSrc, cchMax,
                                         pszOut, cchMax );
                }
                else
#endif
                {
                    MoveMemory(pszOut, pszSrc, cbMac);
                }
                GlobalUnlock(medium.hGlobal);
            }
        }
        ReleaseStgMedium(&medium);
    }

    return hres;
}


// Avoid linking lots of CRuntime stuff.
#undef isdigit
#undef isalpha
#define isdigit(ch) (ch>=TEXT('0') && ch<=TEXT('9'))
#define isalpha(ch) ((ch&0xdf)>=TEXT('A') && (ch&0xdf)<=TEXT('Z'))

#define CCH_MAXLEADINGSPACE     256
#define CCH_COPY                16

//
// create a fancy name for a scrap/doc shortcut given the data object to get some
// text from
//
// out:
//      pszNewName      - assumed to be 64 chars at least
//

BOOL Scrap_GetFancyName(IDataObject *pdtobj, UINT idTemplate, LPCTSTR pszPath, LPCTSTR pszTypeName, LPTSTR pszNewName)
{
    TCHAR szText[CCH_MAXLEADINGSPACE + CCH_COPY + 1];
    HRESULT hres = Scrap_GetText(pdtobj, szText, ARRAYSIZE(szText));

    if (SUCCEEDED(hres))
    {
#ifdef UNICODE
        DebugMsg(DM_TRACE, TEXT("sc TR - Scrap_GetFancyName CF_UNICODETEXT has (%s)"), szText);
#else
        DebugMsg(DM_TRACE, TEXT("sc TR - Scrap_GetFancyName CF_TEXT has (%s)"), szText);
#endif
        LPTSTR pszStart;
        //
        // skip leading space/non-printing characters
        //
        for (pszStart = szText; *pszStart <= TEXT(' '); pszStart++)
        {
            if (*pszStart == TEXT('\0'))
                return FALSE;   // empty string

            if (pszStart - szText >= CCH_MAXLEADINGSPACE)
                return FALSE;   // too many leading space
        }
        DebugMsg(DM_TRACE, TEXT("sc TR - Scrap_GetFancyName pszStart (%s)"), pszStart);

        //
        // Chacter conversion
        //  (1) non-printing characters -> ' '
        //  (2) invalid characters -> '_'
        //
        for (LPTSTR pszT = pszStart; *pszT && ((pszT-pszStart) < CCH_COPY); pszT = CharNext(pszT))
        {
            TCHAR ch = *pszT;
            if (ch <= TEXT(' '))
            {
                *pszT = TEXT(' ');
            }
            else if (ch < 127 && !isdigit(ch) && !isalpha(ch))
            {
                switch(ch)
                {
                case TEXT('$'):
                case TEXT('%'):
                case TEXT('\''):
                case TEXT('-'):
                case TEXT('_'):
                case TEXT('@'):
                case TEXT('~'):
                case TEXT('`'):
                case TEXT('!'):
                case TEXT('('):
                case TEXT(')'):
                case TEXT('{'):
                case TEXT('}'):
                case TEXT('^'):
                case TEXT('#'):
                case TEXT('&'):
                    break;

                default:
                    *pszT = TEXT('_');
                    break;
                }
            }
        }
        *pszT = 0;

        TCHAR szTemplate[MAX_PATH];
        TCHAR szName[MAX_PATH];

        LoadString(HINST_THISDLL, idTemplate, szTemplate, ARRAYSIZE(szTemplate));
        wsprintf(szName, szTemplate, pszTypeName, pszStart);

        PathCombine(szName, pszPath, szName);

        if (!PathFileExists(szName))
        {
            DebugMsg(DM_TRACE, TEXT("sc TR - Scrap_GetFancyName (%s)"), szName);
            lstrcpy(pszNewName, szName);
            return TRUE;
        }
    }
    return FALSE;
}

// *** WARNING ***
//
// Scrap_CreateFromDataObject is a TCHAR export from SHSCRAP.DLL that is used by SHELL32.DLL. If you
// change its calling convention, you must modify shell32's wrapper as well as well.
//
// *** WARNING ***
HRESULT WINAPI Scrap_CreateFromDataObject(LPCTSTR pszPath, IDataObject *pdtobj, BOOL fLink, LPTSTR pszNewFile)
{
    HRESULT hres;
    TCHAR szTemplateS[32];
    TCHAR szTemplateL[128];
    TCHAR szTypeName[64];
#ifndef UNICODE
    WCHAR wszNewFile[MAX_PATH];
#endif
    IStorage *pstg;
    UINT idErr = 0;

    DebugMsg(DM_TRACE, TEXT("sc TR - Scrap_CreateFromDataObject called at %s"), pszPath);

    LoadString(HINST_THISDLL, fLink ? IDS_BOOKMARK_S : IDS_SCRAP_S, szTemplateS, ARRAYSIZE(szTemplateS));
    LoadString(HINST_THISDLL, fLink ? IDS_BOOKMARK_L : IDS_SCRAP_L, szTemplateL, ARRAYSIZE(szTemplateL));

    PathYetAnotherMakeUniqueName(pszNewFile, pszPath, szTemplateS, szTemplateL);

    DebugMsg(DM_TRACE, TEXT("sc TR - Scrap_CreateFromDataObject creating %s"), pszNewFile);

#ifdef UNICODE
    hres = StgCreateDocfile(pszNewFile,
                    STGM_DIRECT | STGM_READWRITE | STGM_CREATE | STGM_SHARE_EXCLUSIVE,
                    0, &pstg);
#else
    MultiByteToWideChar(CP_ACP, 0, pszNewFile, -1, wszNewFile, ARRAYSIZE(wszNewFile));

    hres = StgCreateDocfile(wszNewFile,
                    STGM_DIRECT | STGM_READWRITE | STGM_CREATE | STGM_SHARE_EXCLUSIVE,
                    0, &pstg);
#endif

    if (SUCCEEDED(hres))
    {
        IStorage *pstgContents;

        hres = pstg->CreateStorage(c_wszContents, STGM_DIRECT | STGM_READWRITE | STGM_CREATE | STGM_SHARE_EXCLUSIVE,
                    0, 0, &pstgContents);

        if (SUCCEEDED(hres))
        {
            hres = Scrap_Save(pstgContents, pstg, pdtobj, fLink, szTypeName);
            if (SUCCEEDED(hres))
            {
                hres = pstgContents->Commit(STGC_OVERWRITE);
                if (FAILED(hres))
                    idErr = IDS_ERR_COMMIT;
            }
            else
            {
                idErr = IDS_ERR_SCRAPSAVE;
            }
            pstgContents->Release();
        }
        else
        {
            idErr = IDS_ERR_CREATESTORAGE;
        }

        //
        // We need to delete the file, if failed to save/commit.
        //
        if (SUCCEEDED(hres))
        {
            hres = pstg->Commit(STGC_OVERWRITE);
            if (FAILED(hres))
                idErr = IDS_ERR_COMMIT;
        }

        pstg->Release();

        if (FAILED(hres))
            DeleteFile(pszNewFile);
    }
    else
    {
        idErr = IDS_ERR_CREATEDOCFILE;
    }

    if (SUCCEEDED(hres))
    {
        if (IsLFNDrive(pszPath))
        {
            TCHAR szFancyName[MAX_PATH];

            if (Scrap_GetFancyName(pdtobj, fLink ? IDS_TEMPLINK : IDS_TEMPSCRAP, pszPath, szTypeName, szFancyName))
            {
                if (MoveFile(pszNewFile, szFancyName))
                    lstrcpy(pszNewFile, szFancyName);
            }
        }
    }
    else
    {
        DisplayError((HWND)NULL, hres, idErr, pszNewFile);
    }

    return hres;
}

#ifdef DEBUG
static const LPFNSCRAPCREATEFROMDATAOBJECT s_pfn = Scrap_CreateFromDataObject;
#endif
