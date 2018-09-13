#include "shole.h"
#include "ids.h"

#define INITGUID
#ifndef WINNT
#pragma data_seg(".text", "CODE")
#endif
#include <initguid.h>
#include "scguid.h"
#ifndef WINNT
#pragma data_seg()
#endif

// #define SAVE_OBJECTDESCRIPTOR
#define ARRAYSIZE(a)    (sizeof(a)/sizeof(a[0]))

extern "C" const WCHAR c_wszDescriptor[];

UINT _GetClipboardFormat(UINT id)
{
    static UINT s_acf[CFID_MAX] = { 0, 0, 0, 0, 0 };
    static const TCHAR * const c_aszFormat[CFID_MAX] = {
            TEXT("Embedded Object"),
            TEXT("Object Descriptor"),
            TEXT("Link Source Descriptor"),
            TEXT("Rich Text Format"),
            TEXT("Shell Scrap Object")
            };
    if (!s_acf[id])
    {
        s_acf[id] = RegisterClipboardFormat(c_aszFormat[id]);
    }
    return s_acf[id];
}

//===========================================================================
// CScrapData : Class definition
//===========================================================================

class CScrapData : public IDataObject, public IPersistFile
{
public:
    CScrapData();
    ~CScrapData();

    // IUnKnown
    virtual HRESULT __stdcall QueryInterface(REFIID,void **);
    virtual ULONG   __stdcall AddRef(void);
    virtual ULONG   __stdcall Release(void);

    // IDataObject
    virtual HRESULT __stdcall GetData(FORMATETC *pformatetcIn, STGMEDIUM *pmedium);
    virtual HRESULT __stdcall GetDataHere(FORMATETC *pformatetc, STGMEDIUM *pmedium);
    virtual HRESULT __stdcall QueryGetData(FORMATETC *pformatetc);
    virtual HRESULT __stdcall GetCanonicalFormatEtc(FORMATETC *pformatectIn, FORMATETC *pformatetcOut);
    virtual HRESULT __stdcall SetData(FORMATETC *pformatetc, STGMEDIUM *pmedium, BOOL fRelease);
    virtual HRESULT __stdcall EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC **ppenumFormatEtc);
    virtual HRESULT __stdcall DAdvise(FORMATETC *pformatetc, DWORD advf, IAdviseSink *pAdvSink, DWORD *pdwConnection);
    virtual HRESULT __stdcall DUnadvise(DWORD dwConnection);
    virtual HRESULT __stdcall EnumDAdvise(IEnumSTATDATA **ppenumAdvise);
    virtual HRESULT __stdcall IsDirty(void);

    // IPersistFile
    virtual HRESULT __stdcall GetClassID(CLSID *pClassID);
    virtual HRESULT __stdcall Load(LPCOLESTR pszFileName, DWORD dwMode);
    virtual HRESULT __stdcall Save(LPCOLESTR pszFileName, BOOL fRemember);
    virtual HRESULT __stdcall SaveCompleted(LPCOLESTR pszFileName);
    virtual HRESULT __stdcall GetCurFile(LPOLESTR *ppszFileName);

protected:
    HRESULT _OpenStorage(void);
    void    _CloseStorage(BOOL fResetFlags);
    INT     _GetFormatIndex(UINT cf);
    void    _FillCFArray(void);
#ifdef FIX_ROUNDTRIP
    HRESULT _RunObject(void);
#endif // FIX_ROUNDTRIP

#ifdef SAVE_OBJECTDESCRIPTOR
    HRESULT _GetObjectDescriptor(LPSTGMEDIUM pmedium, BOOL fGetHere);
#endif // SAVE_OBJECTDESCRIPTOR

    UINT         _cRef;
    BOOL         _fDoc:1;
    BOOL         _fItem:1;
    BOOL         _fObjDesc:1;
#ifdef FIX_ROUNDTRIP
    BOOL         _fRunObjectAlreadyCalled:1;
    LPDATAOBJECT _pdtobjItem;
#endif // FIX_ROUNDTRIP
    LPSTORAGE    _pstgDoc;
    LPSTORAGE    _pstgItem;
    LPSTREAM     _pstmObjDesc;
    TCHAR        _szPath[MAX_PATH];
    INT          _ccf;          // number of clipboard format.
    INT          _icfCacheMax;  // Max cache format index
    DWORD        _acf[64];      // 64 must be enough!
};

//===========================================================================
// CScrapData : Constructor
//===========================================================================
CScrapData::CScrapData(void) : _cRef(1), _pstgDoc(NULL), _pstgItem(NULL),
                               _fDoc(FALSE), _fItem(FALSE), _fObjDesc(FALSE),
                               _ccf(0),
#ifdef FIX_ROUNDTRIP
                               _pdtobjItem(NULL), _fRunObjectAlreadyCalled(FALSE),
#endif // FIX_ROUNDTRIP
                               _pstmObjDesc(NULL)
{
    _szPath[0] = TEXT('\0');
    g_cRefThisDll++;
}

CScrapData::~CScrapData()
{
#ifdef FIX_ROUNDTRIP
    if (_pdtobjItem) {
        _pdtobjItem->Release();
    }
#endif // FIX_ROUNDTRIP
    _CloseStorage(FALSE);
    g_cRefThisDll--;
}
//===========================================================================
// CScrapData : Member functions (private)
//===========================================================================
//
// private member CScrapData::_OpenStorage
//
HRESULT CScrapData::_OpenStorage(void)
{
    if (_pstgItem) {
        return S_OK;
    }

    HRESULT hres;
    WCHAR wszFile[MAX_PATH];

#ifdef UNICODE
    lstrcpyn(wszFile, _szPath, ARRAYSIZE(wszFile));
#ifdef DEBUG
    DebugMsg(DM_TRACE, TEXT("sc TR - CSD::GetStorage is called (%s)"), wszFile);
#endif
#else
    MultiByteToWideChar(CP_ACP, 0, _szPath, -1, wszFile, ARRAYSIZE(wszFile));

#ifdef DEBUG
    TCHAR szFile[MAX_PATH];
    WideCharToMultiByte(CP_ACP, 0, wszFile, -1, szFile, ARRAYSIZE(szFile), NULL, NULL);
    DebugMsg(DM_TRACE, TEXT("sc TR - CSD::GetStorage is called (%s)"), szFile);
#endif
#endif

    hres = StgOpenStorage(wszFile, NULL,
                          STGM_READ | STGM_SHARE_DENY_WRITE,
                          NULL, 0, &_pstgDoc);
    if (SUCCEEDED(hres))
    {
        _fDoc = TRUE;
        hres = _pstgDoc->OpenStorage(c_wszContents, NULL,
                            STGM_READ | STGM_SHARE_EXCLUSIVE,
                            NULL, 0, &_pstgItem);
        if (SUCCEEDED(hres))
        {
            HRESULT hresT;
            _fItem = TRUE;
#ifdef SAVE_OBJECTDESCRIPTOR
            hresT = _pstgDoc->OpenStream(c_wszDescriptor, 0,
                                STGM_READ | STGM_SHARE_EXCLUSIVE,
                                0, &_pstmObjDesc);
            _fObjDesc = SUCCEEDED(hresT);
#endif // SAVE_OBJECTDESCRIPTOR
        }
        else
        {
            DebugMsg(DM_TRACE, TEXT("sc TR - CSD::_OpenStorage _pstgDoc->OpenStorage failed (%x)"), hres);
            _pstgDoc->Release();
            _pstgDoc = NULL;
        }
    }
    else
    {
        DebugMsg(DM_TRACE, TEXT("sc TR - CSD::_OpenStorage StgOpenStorage failed (%x)"), hres);
    }

    DebugMsg(DM_TRACE, TEXT("sc TR - CSD::_OpenStorage _pstgDoc->OpenStorage returning (%x) %x"),
                hres, _pstmObjDesc);
    return hres;
}

void CScrapData::_CloseStorage(BOOL fResetFlags)
{
    if (_pstgItem) {
        _pstgItem->Release();
        _pstgItem = NULL;
    }
    if (_pstmObjDesc) {
        _pstmObjDesc->Release();
        _pstmObjDesc = NULL;
    }
    if (_pstgDoc) {
        _pstgDoc->Release();
        _pstgDoc = NULL;
    }

    if (fResetFlags) {
        _fItem = FALSE;
        _fObjDesc = FALSE;
        _fDoc = FALSE;
    }
}

INT CScrapData::_GetFormatIndex(UINT cf)
{
    for (INT i=0; i<_ccf; i++)
    {
        if (_acf[i] == cf)
        {
            return i;
        }
    }
    return -1;
}

#ifdef FIX_ROUNDTRIP
extern "C" const TCHAR c_szRenderFMT[] = TEXT("DataFormats\\DelayRenderFormats");
#endif // FIX_ROUNDTRIP

extern "C" const WCHAR c_wszFormatNames[];

//
//  This function filles the clipboard format array (_acf). Following
// clipboard format may be added.
//
// Step 1. CF_EMBEEDEDOBJECT
// Step 2. CF_OBJECTDESCRIPTOR
// Step 3. CF_SCRAPOBJECT
// Step 4. Cached clipboard formats (from a stream).
// Step 5. Delay Rendered clipbaord formats (from registry).
//
void CScrapData::_FillCFArray(void)
{
    _ccf=0;
    //
    // Step 1.
    //
    if (_fItem) {
        _acf[_ccf++] = CF_EMBEDDEDOBJECT;
    }

    //
    // Step 2.
    //
    if (_fObjDesc) {
        _acf[_ccf++] = CF_OBJECTDESCRIPTOR;
    }

    //
    // Step 3.
    //
    if (_fDoc)
    {
        _acf[_ccf++] = CF_SCRAPOBJECT;
    }

#ifdef FIX_ROUNDTRIP
    if (_pstgItem)
    {
        //
        // Step 3. Cached clipboard formats
        //
        //
        // Open the stream which contains the names of cached formats.
        //
        LPSTREAM pstm;
        HRESULT hres = _pstgDoc->OpenStream(c_wszFormatNames, NULL,
                                STGM_READ | STGM_SHARE_EXCLUSIVE,
                                NULL, &pstm);

        if (SUCCEEDED(hres))
        {
            //
            // For each cached format...
            //
            USHORT cb;
            DWORD cbRead;
            while(SUCCEEDED(pstm->Read(&cb, sizeof(cb), &cbRead)) && cbRead==sizeof(cb)
                  && cb && cb<128)
            {
                UINT cf = 0;

                //
                // Get the cached clipboard format name
                //
                CHAR szFormat[128];
                szFormat[cb] = '\0';
                hres = pstm->Read(szFormat, cb, &cbRead);
                if (SUCCEEDED(hres) && cbRead==cb && lstrlenA(szFormat)==cb)
                {
                    //
                    // Append it to the array.
                    //
#ifdef UNICODE
                    TCHAR wszFormat[128];
                    MultiByteToWideChar(CP_ACP, 0,
                                        szFormat, -1,
                                        wszFormat, ARRAYSIZE(wszFormat));
                    DebugMsg(DM_TRACE, TEXT("sc TR _FillCFA Found Cached Format %s"), wszFormat);
#else
                    DebugMsg(DM_TRACE, TEXT("sc TR _FillCFA Found Cached Format %s"), szFormat);
#endif
                    cf = RegisterClipboardFormatA(szFormat);

                    if (cf)
                    {
                        _acf[_ccf++] = cf;
                    }
                }
                else
                {
                    break;
                }
            }
            pstm->Release();
        }

        _icfCacheMax = _ccf;

        //
        // Step 4. Get the list of delay-rendered clipboard formats
        //
        LPPERSISTSTORAGE pps;
        hres = OleLoad(_pstgItem, IID_IPersistStorage, NULL, (LPVOID *)&pps);
        if (SUCCEEDED(hres))
        {
            //
            // Get the CLSID of embedding.
            //
            CLSID clsid;
            hres = pps->GetClassID(&clsid);
            if (SUCCEEDED(hres))
            {
                //
                // Open the key for delay-rendered format names.
                //
                extern HKEY _OpenCLSIDKey(REFCLSID rclsid, LPCTSTR pszSubKey);
                HKEY hkey = _OpenCLSIDKey(clsid, c_szRenderFMT);
                if (hkey)
                {
                    TCHAR szValueName[128];
                    //
                    // For each delay-rendered clipboard format...
                    //
                    for(int iValue=0; ;iValue++)
                    {
                        //
                        // Get the value name, which is the format name.
                        //
                        DWORD cchValueName = ARRAYSIZE(szValueName);
                        DWORD dwType;
                        if (RegEnumValue(hkey, iValue, szValueName, &cchValueName, NULL,
                                         &dwType, NULL, NULL)==ERROR_SUCCESS)
                        {
                            DebugMsg(DM_TRACE, TEXT("sc TR - CSD::_FillCFA RegEnumValue found %s, %x"), szValueName, dwType);
                            UINT cf = RegisterClipboardFormat(szValueName);

                            if (cf)
                            {
                                _acf[_ccf++] = cf;
                            }
                        }
                        else
                        {
                            break;
                        }
                    }

                    //
                    // HACK: NT 3.5's regedit does not support named value...
                    //
                    for(iValue=0; ;iValue++)
                    {
                        TCHAR szKeyName[128];
                        //
                        // Get the value name, which is the format name.
                        //
                        if (RegEnumKey(hkey, iValue, szKeyName, ARRAYSIZE(szKeyName))==ERROR_SUCCESS)
                        {
                            DebugMsg(DM_TRACE, TEXT("sc TR - CSD::_FillCFA RegEnumValue found %s"), szValueName);
                            LONG cbValue = ARRAYSIZE(szValueName);
                            if ((RegQueryValue(hkey, szKeyName, szValueName, &cbValue)==ERROR_SUCCESS) && cbValue)
                            {
                                UINT cf = RegisterClipboardFormat(szValueName);

                                if (cf)
                                {
                                    _acf[_ccf++] = cf;
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
            pps->Release();
        }
    }
#endif // FIX_ROUNDTRIP
}

#ifdef FIX_ROUNDTRIP
//
// private member CScrapData::_RunObject
//
HRESULT CScrapData::_RunObject(void)
{
    if (_pdtobjItem) {
        return S_OK;
    }

    if (_fRunObjectAlreadyCalled) {
        DebugMsg(DM_TRACE, TEXT("sc TR CSD::_RunObject returning E_FAIL"));
        return E_FAIL;
    }
    _fRunObjectAlreadyCalled = TRUE;

    HRESULT hres = _OpenStorage();

    DebugMsg(DM_TRACE, TEXT("sc TR CSD::_RunObject _OpenStorage returned %x"), hres);

    if (SUCCEEDED(hres) && _pstgItem)
    {
        LPOLEOBJECT pole;
        hres = OleLoad(_pstgItem, IID_IOleObject, NULL, (LPVOID *)&pole);
        DebugMsg(DM_TRACE, TEXT("sc TR CSD::_RunObject OleLoad returned %x"), hres);
        if (SUCCEEDED(hres))
        {
            DWORD dw=GetCurrentTime();
            hres = OleRun(pole);
            dw = GetCurrentTime()-dw;
            DebugMsg(DM_TRACE, TEXT("sc TR CSD::_RunObject OleRun returned %x (%d msec)"), hres, dw);
            if (SUCCEEDED(hres))
            {
                hres = pole->GetClipboardData(0, &_pdtobjItem);
                DebugMsg(DM_TRACE, TEXT("sc TR CSD::_RunObject GetClipboardData returned %x"), hres);
                if (FAILED(hres))
                {
                    hres = pole->QueryInterface(IID_IDataObject, (LPVOID*)&_pdtobjItem);
                    DebugMsg(DM_TRACE, TEXT("sc TR CSD::_RunObject QI(IID_IDataIbject) returned %x"), hres);
                }
            }
            pole->Release();
        }
    }

    return hres;
}
#endif // FIX_ROUNDTRIP

//===========================================================================
// CScrapData : Member functions (virtual IDataObject)
//===========================================================================
HRESULT CScrapData::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
    if (IsEqualIID(riid, IID_IDataObject) || IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObj = (LPDATAOBJECT)this;
        _cRef++;
        return S_OK;
    }
    else if (IsEqualIID(riid, IID_IPersistFile))
    {
        *ppvObj = (LPPERSISTFILE)this;
        _cRef++;
        return S_OK;
    }
    *ppvObj = NULL;

    return E_NOINTERFACE;
}

ULONG CScrapData::AddRef()
{
    _cRef++;
    return _cRef;
}

ULONG CScrapData::Release()
{
    _cRef--;
    if (_cRef > 0)
        return _cRef;

    DebugMsg(DM_TRACE, TEXT("sc TR - CSD::Release deleting this object"));
    delete this;
    return 0;
}


#ifdef SAVE_OBJECTDESCRIPTOR
HRESULT CScrapData::_GetObjectDescriptor(LPSTGMEDIUM pmedium, BOOL fGetHere)
{
    if (!_pstmObjDesc)
        return DATA_E_FORMATETC;

    LARGE_INTEGER dlib = { 0, 0 };
    HRESULT hres = _pstmObjDesc->Seek(dlib, STREAM_SEEK_SET, NULL);
    if (FAILED(hres))
        return hres;

    OBJECTDESCRIPTOR ods;
    ULONG cbRead;
    hres = _pstmObjDesc->Read(&ods.cbSize, sizeof(ods.cbSize), &cbRead);
    if (SUCCEEDED(hres) && cbRead == sizeof(ods.cbSize))
    {
        if (fGetHere)
        {
            if (GlobalSize(pmedium->hGlobal)<ods.cbSize) {
                hres = STG_E_MEDIUMFULL;
            }
        }
        else
        {
            pmedium->tymed = TYMED_HGLOBAL;
            pmedium->hGlobal = GlobalAlloc(GMEM_MOVEABLE, ods.cbSize);
            hres = pmedium->hGlobal ? S_OK : E_OUTOFMEMORY;
        }

        if (SUCCEEDED(hres))
        {
            LPOBJECTDESCRIPTOR pods = (LPOBJECTDESCRIPTOR)GlobalLock(pmedium->hGlobal);
            if (pods)
            {
                pods->cbSize = ods.cbSize;
                hres = _pstmObjDesc->Read(&pods->clsid, ods.cbSize-sizeof(ods.cbSize), NULL);
                GlobalUnlock(pmedium->hGlobal);
            }
            else
            {
                if (!fGetHere) {
                    GlobalFree(pmedium->hGlobal);
                    pmedium->hGlobal = NULL;
                }
                hres = E_OUTOFMEMORY;
            }
        }
    }

    DebugMsg(DM_TRACE, TEXT("sc TR - CSD::_GetObjectDescriptor returning (%x)"), hres);
    return hres;
}
#endif // SAVE_OBJECTDESCRIPTOR


HRESULT CScrapData::GetData(LPFORMATETC pformatetcIn, LPSTGMEDIUM pmedium)
{
#ifdef DEBUG
    if (pformatetcIn->cfFormat<CF_MAX) {
        DebugMsg(DM_TRACE, TEXT("sc TR - CSD::GetData called with %x,%x,%x"),
                             pformatetcIn->cfFormat, pformatetcIn->tymed, pmedium->tymed);
    } else {
        TCHAR szName[256];
        GetClipboardFormatName(pformatetcIn->cfFormat, szName, ARRAYSIZE(szName));
        DebugMsg(DM_TRACE, TEXT("sc TR - CSD::GetData called with %s,%x,%x"),
                             szName, pformatetcIn->tymed, pmedium->tymed);
    }
#endif

    HRESULT hres;

    pmedium->pUnkForRelease = NULL;
    pmedium->pstg = NULL;

    //
    // NOTES: We should avoid calling _OpenStorage if we don't support
    //  the format.
    //

    if (pformatetcIn->cfFormat == CF_EMBEDDEDOBJECT
        && pformatetcIn->tymed == TYMED_ISTORAGE && _fItem)
    {
        hres = _OpenStorage();
        if (SUCCEEDED(hres))
        {
            pmedium->tymed = TYMED_ISTORAGE;
            _pstgItem->AddRef();
            pmedium->pstg = _pstgItem;
        }
    }
    else if (pformatetcIn->cfFormat == CF_SCRAPOBJECT
        && pformatetcIn->tymed == TYMED_ISTORAGE && _fItem)
    {
        hres = _OpenStorage();
        if (SUCCEEDED(hres))
        {
            pmedium->tymed = TYMED_ISTORAGE;
            _pstgDoc->AddRef();
            pmedium->pstg = _pstgDoc;
        }
    }
#ifdef SAVE_OBJECTDESCRIPTOR
    else if (pformatetcIn->cfFormat == CF_OBJECTDESCRIPTOR
        && pformatetcIn->tymed == TYMED_HGLOBAL && _fObjDesc)
    {
        hres = _OpenStorage();
        if (SUCCEEDED(hres))
        {
            hres = _GetObjectDescriptor(pmedium, FALSE);
        }
    }
#endif // SAVE_OBJECTDESCRIPTOR
    else
    {
#ifdef FIX_ROUNDTRIP
        INT iFmt = _GetFormatIndex(pformatetcIn->cfFormat);

        if (iFmt != -1)
        {
            hres = _OpenStorage();
            if (FAILED(hres))
            {
                goto exit;
            }
        }

        if (iFmt>=_icfCacheMax)
        {
            //
            // Delayed Rendered format
            //
            if (SUCCEEDED(_RunObject()))
            {
                hres = _pdtobjItem->GetData(pformatetcIn, pmedium);
                DebugMsg(DM_TRACE, TEXT("sc TR - CSD::GetData called _pdtobjItem->GetData %x"), hres);
                return hres;
            }
        }
        else if (iFmt >= 0)
        {
            //
            // Cached Format
            //
            extern void _GetCacheStreamName(LPCTSTR pszFormat, LPWSTR wszStreamName, UINT cchMax);
            TCHAR szFormat[128];
            if (pformatetcIn->cfFormat<CF_MAX) {
                wsprintf(szFormat, TEXT("#%d"), pformatetcIn->cfFormat);
            } else {
                GetClipboardFormatName(pformatetcIn->cfFormat, szFormat, ARRAYSIZE(szFormat));
            }

            WCHAR wszStreamName[256];
            _GetCacheStreamName(szFormat, wszStreamName, ARRAYSIZE(wszStreamName));

            if (pformatetcIn->cfFormat==CF_METAFILEPICT
                || pformatetcIn->cfFormat==CF_ENHMETAFILE
                || pformatetcIn->cfFormat==CF_BITMAP
                || pformatetcIn->cfFormat==CF_PALETTE
                )
            {
                LPSTORAGE pstg;
                hres = _pstgDoc->OpenStorage(wszStreamName, NULL,
                                    STGM_READ | STGM_SHARE_EXCLUSIVE,
                                    NULL, 0, &pstg);
                DebugMsg(DM_TRACE, TEXT("sc TR - CSD::GetData OpenStorage returned (%x)"), hres);
                if (SUCCEEDED(hres))
                {
                    LPDATAOBJECT pdtobj;
#if 0
                    hres = OleLoad(pstg, IID_IDataObject, NULL, (LPVOID*)&pdtobj);
                    DebugMsg(DM_TRACE, TEXT("sc TR - CSD::GetData OleLoad returned (%x)"), hres);
#else
                    const CLSID* pclsid = NULL;
                    switch(pformatetcIn->cfFormat)
                    {
                    case CF_METAFILEPICT:
                        pclsid = &CLSID_Picture_Metafile;
                        break;
                
                    case CF_ENHMETAFILE:
                        pclsid = &CLSID_Picture_EnhMetafile;
                        break;
                
                    case CF_PALETTE:
                    case CF_BITMAP:
                        pclsid = &CLSID_Picture_Dib;
                        break;
                    }

                    LPPERSISTSTORAGE ppstg;
                    hres = OleCreateDefaultHandler(*pclsid, NULL, IID_IPersistStorage, (LPVOID *)&ppstg);
                    DebugMsg(DM_TRACE, TEXT("sc TR Scrap_CacheOPF OleCreteDefHandler returned %x"), hres);
                    if (SUCCEEDED(hres))
                    {
                        hres = ppstg->Load(pstg);
                        DebugMsg(DM_TRACE, TEXT("sc TR Scrap_CacheOPF ppstg->Load returned %x"), hres);
                        if (SUCCEEDED(hres))
                        {
                            hres = ppstg->QueryInterface(IID_IDataObject, (LPVOID*)&pdtobj);
                        }
                        ppstg->HandsOffStorage();
                        ppstg->Release();
                    }
#endif
                    if (SUCCEEDED(hres))
                    {
                        hres = pdtobj->GetData(pformatetcIn, pmedium);
                        DebugMsg(DM_TRACE, TEXT("sc TR - CSD::GetData pobj->GetData returned (%x)"), hres);
                        pdtobj->Release();
                    }
                    pstg->Release();
                    return hres;
                }
                // fall through
            }
            else // if (pformatetcIn->cfFormat==CF_...)
            {
                LPSTREAM pstm;
                hres = _pstgDoc->OpenStream(wszStreamName, NULL,
                                        STGM_READ | STGM_SHARE_EXCLUSIVE,
                                        0, &pstm);
                if (SUCCEEDED(hres))
                {
                    UINT cbData;
                    DWORD cbRead;
                    hres = pstm->Read(&cbData, sizeof(cbData), &cbRead);
                    if (SUCCEEDED(hres) && cbRead==sizeof(cbData))
                    {
                        LPBYTE pData = (LPBYTE)GlobalAlloc(GPTR, cbData);
                        if (pData)
                        {
                            hres = pstm->Read(pData, cbData, &cbRead);
                            if (SUCCEEDED(hres) && cbData==cbRead)
                            {
                                pmedium->tymed = TYMED_HGLOBAL;
                                pmedium->hGlobal = (HGLOBAL)pData;
                            }
                            else
                            {
                                hres = E_UNEXPECTED;
                                GlobalFree((HGLOBAL)pData);
                            }
                        }
                        else
                        {
                            hres = E_OUTOFMEMORY;
                        }
                    }
                    pstm->Release();

                    DebugMsg(DM_TRACE, TEXT("CSD::GetData(%s) returning %x"), szFormat, hres);
                    return hres;
                }
            }
        } // if (iFmt >= 0)
#endif // FIX_ROUNDTRIP
        hres = DATA_E_FORMATETC;
    }

exit:

#ifdef DEBUG
    TCHAR szFormat[256];
    GetClipboardFormatName(pformatetcIn->cfFormat,
                           szFormat, ARRAYSIZE(szFormat));

    DebugMsg(DM_TRACE, TEXT("sc TR - CSD::GetData called with %x,%x,%s and returning %x"),
                             pformatetcIn->cfFormat,
                             pformatetcIn->tymed,
                             szFormat, hres);
#endif

    return hres;
}

HRESULT CScrapData::GetDataHere(LPFORMATETC pformatetcIn, LPSTGMEDIUM pmedium )
{
    HRESULT hres;

#ifdef DEBUG
    if (pformatetcIn->cfFormat<CF_MAX) {
        DebugMsg(DM_TRACE, TEXT("sc TR - CSD::GetDataHere called with %x,%x,%x"),
                             pformatetcIn->cfFormat, pformatetcIn->tymed, pmedium->tymed);
    } else {
        TCHAR szName[256];
        GetClipboardFormatName(pformatetcIn->cfFormat, szName, ARRAYSIZE(szName));
        DebugMsg(DM_TRACE, TEXT("sc TR - CSD::GetDataHere called with %s,%x,%x"),
                             szName, pformatetcIn->tymed, pmedium->tymed);
    }
#endif

    hres = _OpenStorage();
    if (FAILED(hres)) {
        return hres;
    }

    if (pformatetcIn->cfFormat == CF_EMBEDDEDOBJECT
        && pformatetcIn->tymed == TYMED_ISTORAGE && pmedium->tymed == TYMED_ISTORAGE)
    {
        hres = _pstgItem->CopyTo(0, NULL, NULL, pmedium->pstg);
        DebugMsg(DM_TRACE, TEXT("sc TR - CSD::GetDataHere _pstgItem->CopyTo returned %x"), hres);
    }
    else if (pformatetcIn->cfFormat == CF_SCRAPOBJECT
        && pformatetcIn->tymed == TYMED_ISTORAGE && pmedium->tymed == TYMED_ISTORAGE)
    {
        hres = _pstgDoc->CopyTo(0, NULL, NULL, pmedium->pstg);
        DebugMsg(DM_TRACE, TEXT("sc TR - CSD::GetDataHere _pstgItem->CopyTo returned %x"), hres);
    }
#ifdef SAVE_OBJECTDESCRIPTOR
    else if ((pformatetcIn->cfFormat == CF_OBJECTDESCRIPTOR)
        && (pformatetcIn->tymed == TYMED_HGLOBAL) && _pstmObjDesc)
    {
        hres = _GetObjectDescriptor(pmedium, TRUE);
    }
#endif // SAVE_OBJECTDESCRIPTOR
    else
    {
#ifdef FIX_ROUNDTRIP
        if (_GetFormatIndex(pformatetcIn->cfFormat) >= 0 && SUCCEEDED(_RunObject()))
        {
            DebugMsg(DM_TRACE, TEXT("sc TR - CSD::GetDataHere calling _pdtobjItem->GetDataHere"));
            return _pdtobjItem->GetDataHere(pformatetcIn, pmedium);
        }
#endif // FIX_ROUNDTRIP
        hres = DATA_E_FORMATETC;
    }

    return hres;
}

HRESULT CScrapData::QueryGetData(LPFORMATETC pformatetcIn)
{
    HRESULT hres;
    if (_GetFormatIndex(pformatetcIn->cfFormat) >= 0) {
        hres = S_OK;
    } else {
        hres = DATA_E_FORMATETC;
    }

#ifdef DEBUG
    TCHAR szFormat[256] = TEXT("");
    GetClipboardFormatName(pformatetcIn->cfFormat, szFormat, ARRAYSIZE(szFormat));
    DebugMsg(DM_TRACE, TEXT("sc TR - CSD::QueryGetData(%x,%s,%x) returning %x"),
                    pformatetcIn->cfFormat, szFormat, pformatetcIn->tymed, hres);
#endif

    return hres;
}

HRESULT CScrapData::GetCanonicalFormatEtc(LPFORMATETC pformatetc, LPFORMATETC pformatetcOut)
{
    //
    //  This is the simplest implemtation. It means we always return
    // the data in the format requested.
    //
    return ResultFromScode(DATA_S_SAMEFORMATETC);
}

HRESULT CScrapData::SetData(LPFORMATETC pformatetc, STGMEDIUM  * pmedium, BOOL fRelease)
{
    return E_FAIL;
}

HRESULT CScrapData::EnumFormatEtc(DWORD dwDirection, LPENUMFORMATETC * ppenumFormatEtc)
{
    if (dwDirection!=DATADIR_GET) {
        return E_NOTIMPL; // Not supported (as documented)
    }

    if (_ccf==0) {
        return E_UNEXPECTED;
    }

    FORMATETC * pfmt = (FORMATETC*)LocalAlloc(LPTR, sizeof(FORMATETC)*_ccf);
    if (!pfmt) {
        return E_OUTOFMEMORY;
    }

    static const FORMATETC s_fmteInit =
    {
          0,
          (DVTARGETDEVICE __RPC_FAR *)NULL,
          DVASPECT_CONTENT,
          -1,
          TYMED_HGLOBAL         // HGLOBAL except CF_EMBEDDEDOBJECT/SCRAPOBJECT
     };

    //
    // Fills FORMATETC for each clipboard format.
    //
    for (INT i=0; i<_ccf; i++)
    {
        pfmt[i] = s_fmteInit;
        pfmt[i].cfFormat = (CLIPFORMAT)_acf[i];

        if (_acf[i]==CF_EMBEDDEDOBJECT || _acf[i]==CF_SCRAPOBJECT) {
            pfmt[i].tymed = TYMED_ISTORAGE;
        } else {
            switch(_acf[i])
            {
            case CF_METAFILEPICT:
                pfmt[i].tymed = TYMED_MFPICT;
                break;

            case CF_ENHMETAFILE:
                pfmt[i].tymed = TYMED_ENHMF;
                break;

            case CF_BITMAP:
            case CF_PALETTE:
                pfmt[i].tymed = TYMED_GDI;
                break;
            }
        }
    }

    HRESULT hres = SHCreateStdEnumFmtEtc(_ccf, pfmt, ppenumFormatEtc);
    LocalFree((HLOCAL)pfmt);

    return hres;
}

HRESULT CScrapData::DAdvise(FORMATETC * pFormatetc, DWORD advf, LPADVISESINK pAdvSink, DWORD * pdwConnection)
{
    return ResultFromScode(OLE_E_ADVISENOTSUPPORTED);
}

HRESULT CScrapData::DUnadvise(DWORD dwConnection)
{
    return ResultFromScode(OLE_E_ADVISENOTSUPPORTED);
}

HRESULT CScrapData::EnumDAdvise(LPENUMSTATDATA * ppenumAdvise)
{
    return ResultFromScode(OLE_E_ADVISENOTSUPPORTED);
}


//===========================================================================
// CScrapData : Member functions (virtual IPersistFile)
//===========================================================================

HRESULT CScrapData::GetClassID(LPCLSID lpClassID)
{
    *lpClassID = CLSID_CScrapData;
    return S_OK;
}

HRESULT CScrapData::IsDirty(void)
{
    return S_FALSE;     // meaningless (read only)
}

HRESULT CScrapData::Load(LPCOLESTR pwszFile, DWORD grfMode)
{
    //
    // Close all the storage (if there is any) and reset flags.
    //
    _CloseStorage(TRUE);

    //
    // Copy the new file name and open storage to update the flags.
    //
#ifdef UNICODE
    lstrcpyn(_szPath, pwszFile, ARRAYSIZE(_szPath));
#else
    WideCharToMultiByte(CP_ACP, 0, pwszFile, -1, _szPath, ARRAYSIZE(_szPath), NULL, NULL);
#endif
    HRESULT hres = _OpenStorage();
    _FillCFArray();

    //
    // Close all the storage, so that we can move/delete.
    //
    _CloseStorage(FALSE);

    return hres;
}

HRESULT CScrapData::Save(LPCOLESTR pwszFile, BOOL fRemember)
{
    return E_FAIL;      // read only
}

HRESULT CScrapData::SaveCompleted(LPCOLESTR pwszFile)
{
    return S_OK;
}

HRESULT CScrapData::GetCurFile(LPOLESTR *lplpszFileName)
{
    return E_NOTIMPL;   // nobody needs it
}

HRESULT CScrapData_CreateInstance(LPUNKNOWN * ppunk)
{
//
//  This test code is unrelated to the scrap itself. It just verifies that
// CLSID_ShellLink is correctly registered.
//
#ifdef DEBUG
    LPUNKNOWN punk = NULL;
    HRESULT hres = CoCreateInstance(CLSID_ShellLink, NULL,
                    CLSCTX_INPROC, IID_IShellLink, (LPVOID*)&punk);
    DebugMsg(DM_TRACE, TEXT("###############################################"));
    DebugMsg(DM_TRACE, TEXT("CoCreateInstance returned %x"), hres);
    DebugMsg(DM_TRACE, TEXT("###############################################"));
    if (SUCCEEDED(hres)) {
        punk->Release();
    }
#endif

    CScrapData* pscd = new CScrapData();
    if (pscd) {
        *ppunk = (LPDATAOBJECT)pscd;
        return S_OK;
    }
    return E_OUTOFMEMORY;
}
