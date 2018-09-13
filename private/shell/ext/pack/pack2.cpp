#include "priv.h"
#include "privcpp.h"

#define CPP_FUNCTIONS
// #include <crtfree.h>

UINT    g_cfFileContents;
UINT    g_cfFileDescriptor;
UINT    g_cfObjectDescriptor;
UINT    g_cfEmbedSource;
UINT    g_cfFileNameW;

INT     g_cxIcon;
INT     g_cyIcon;
INT     g_cxArrange;
INT     g_cyArrange;
HFONT   g_hfontTitle;

static TCHAR szUserType[] = TEXT("Package");
static TCHAR szDefTempFile[] = TEXT("PKG");

CPackage::CPackage() : 
    _cRef(1)
{
    DebugMsg(DM_TRACE, "pack - CPackage() called.");
    g_cRefThisDll++;
    
    ASSERT(_cf == 0);
    ASSERT(_panetype == NOTHING);
    ASSERT(_pEmbed == NULL);
    ASSERT(_pCml == NULL);
    ASSERT(_fLoaded == FALSE);
    
    ASSERT(_lpszContainerApp == NULL);
    ASSERT(_lpszContainerObj == NULL);
    
    ASSERT(_fIsDirty == FALSE);
    
    ASSERT(_pIStorage == NULL);       
    ASSERT(_pstmFileContents == NULL);
    ASSERT(_pstm == NULL);
    
    ASSERT(_pIPersistStorage == NULL);
    ASSERT(_pIDataObject == NULL);
    ASSERT(_pIOleObject == NULL);    
    ASSERT(_pIViewObject2 == NULL);
    ASSERT(_pIAdviseSink == NULL);
    ASSERT(_pIRunnableObject == NULL);
        
    ASSERT(_pIDataAdviseHolder == NULL);
    ASSERT(_pIOleAdviseHolder == NULL);
    ASSERT(_pIOleClientSite == NULL);
    
    ASSERT(_pViewSink == NULL);
    ASSERT(_dwViewAspects == 0);
    ASSERT(_dwViewAdvf == 0);

    ASSERT(_cVerbs == 0);
    ASSERT(_nCurVerb == 0);
    ASSERT(_pVerbs == NULL);
    ASSERT(_pcm == NULL);
}


CPackage::~CPackage()
{
    DebugMsg(DM_TRACE, "pack - ~CPackage() called.");
   
    // We should never be destroyed unless our ref count is zero.
    ASSERT(_cRef == 0);
    
    g_cRefThisDll--;
    
    // Destroy our interfaces...
    //
    delete _pIOleObject;
    delete _pIViewObject2;
    delete _pIDataObject;
    delete _pIPersistStorage;
    delete _pIAdviseSink;
    delete _pIRunnableObject;

    // Destroy the packaged file structure...
    //
    DestroyIC();
    
    // we destroy depending on which type of object we had packaged
    switch(_panetype)
    {
    case PEMBED:
        if (_pEmbed->pszTempName) {
            DeleteFile(_pEmbed->pszTempName);
            delete _pEmbed->pszTempName;
        }
        delete _pEmbed;
        break;
        
    case CMDLINK:
        delete _pCml;
        break;

    }
    
    // Release Advise pointers...
    //
    if (_pIDataAdviseHolder)
        _pIDataAdviseHolder->Release();
    if (_pIOleAdviseHolder)
        _pIOleAdviseHolder->Release();
    if (_pIOleClientSite)
        _pIOleClientSite->Release();


    // Release Storage pointers...
    //
    if (_pIStorage)
        _pIStorage->Release();
    if (_pstmFileContents)
        _pstmFileContents->Release();
    if (_pstm)
        _pstm->Release();
    
    delete _lpszContainerApp;
    delete _lpszContainerObj;

    ReleaseContextMenu();
    if (NULL != _pVerbs)
    {
        for (ULONG i = 0; i < _cVerbs; i++)
        {
            delete _pVerbs[i].lpszVerbName;
        }
        delete _pVerbs;
    }
    
    DebugMsg(DM_TRACE,"CPackage being destroyed. _cRef == %d",_cRef);
}

HRESULT CPackage::Init() 
{
    // 
    // initializes parts of a package object that have a potential to fail
    // return:  S_OK            -- everything initialized
    //          E_FAIL          -- error in initialzation
    //          E_OUTOFMEMORY   -- out of memory
    //
    
    DebugMsg(DM_TRACE, "pack - Init() called.");

    // Initialize all the interfaces...
    //
    if (!(_pIOleObject        = new CPackage_IOleObject(this)))
        return E_OUTOFMEMORY;
    if (!(_pIViewObject2      = new CPackage_IViewObject2(this)))
        return E_OUTOFMEMORY;
    if (!(_pIDataObject       = new CPackage_IDataObject(this)))
        return E_OUTOFMEMORY;
    if (!(_pIPersistStorage   = new CPackage_IPersistStorage(this)))
        return E_OUTOFMEMORY;
    if (!(_pIPersistFile      = new CPackage_IPersistFile(this)))
        return E_OUTOFMEMORY;
    if (!(_pIAdviseSink       = new CPackage_IAdviseSink(this)))
        return E_OUTOFMEMORY;
    if (!(_pIRunnableObject   = new CPackage_IRunnableObject(this)))
        return E_OUTOFMEMORY;
    
    // Get some system metrics that we'll need later...
    //
    LOGFONT lf;
    SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(lf), &lf, FALSE);
    SystemParametersInfo(SPI_ICONHORIZONTALSPACING, 0, &g_cxArrange, FALSE);
    SystemParametersInfo(SPI_ICONVERTICALSPACING, 0, &g_cyArrange, FALSE);
    g_cxIcon = GetSystemMetrics(SM_CXICON);
    g_cyIcon = GetSystemMetrics(SM_CYICON);
    g_hfontTitle = CreateFontIndirect(&lf);
    
    // register some clipboard formats that we support...
    //
    g_cfFileContents    = RegisterClipboardFormat(CFSTR_FILECONTENTS);
    g_cfFileDescriptor  = RegisterClipboardFormat(CFSTR_FILEDESCRIPTOR);
    g_cfObjectDescriptor= RegisterClipboardFormat(CFSTR_OBJECTDESCRIPTOR);
    g_cfEmbedSource     = RegisterClipboardFormat(CFSTR_EMBEDSOURCE);
    g_cfFileNameW       = RegisterClipboardFormat(TEXT("FileNameW"));
    
    // Initialize a generic icon
    _lpic = IconCreate();
    IconRefresh();
   

    return S_OK;
}

////////////////////////////////////////////////////////////////////////
//
// IUnknown Methods...
//
////////////////////////////////////////////////////////////////////////

HRESULT CPackage::QueryInterface(REFIID iid, void ** ppvObj)
{
    DebugMsg(DM_TRACE, "pack - QueryInterface() called.");
    
    if (iid == IID_IUnknown) {
        DebugMsg(DM_TRACE, "         getting IID_IUnknown");
        *ppvObj = (void *)this;
    }
    else if (iid == IID_IOleObject) {
        DebugMsg(DM_TRACE, "         getting IID_IOleObject");
        *ppvObj = (void *)_pIOleObject;
    }
    else if ((iid == IID_IViewObject2) || (iid == IID_IViewObject)) {
        DebugMsg(DM_TRACE, "         getting IID_IViewObject");
        *ppvObj = (void *)_pIViewObject2;
    }
    else if (iid == IID_IDataObject) {
        DebugMsg(DM_TRACE, "         getting IID_IDataObject");
        *ppvObj = (void *)_pIDataObject;
    }
    else if ((iid == IID_IPersistStorage) || (iid == IID_IPersist)) {
        DebugMsg(DM_TRACE, "         getting IID_IPersistStorage");
        *ppvObj = (void *)_pIPersistStorage;
    }
    else if (iid == IID_IPersistFile) {
        DebugMsg(DM_TRACE, "         getting IID_IPersistFile");
        *ppvObj = (void *)_pIPersistFile;
    }
    else if (iid == IID_IAdviseSink) {
        DebugMsg(DM_TRACE, "         getting IID_IAdviseSink");
        *ppvObj = (void *)_pIAdviseSink;
    }
    else if (iid == IID_IRunnableObject) {
        DebugMsg(DM_TRACE, "         getting IID_IRunnableObject");
        *ppvObj = (void *)_pIRunnableObject;
    }
    else if (iid == IID_IEnumOLEVERB)
    {
        DebugMsg(DM_TRACE, "         getting IID_IEnumOLEVERB");
        *ppvObj = (IEnumOLEVERB*) this;
    }
    else {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    ((LPUNKNOWN)*ppvObj)->AddRef();
    return S_OK;
}

ULONG CPackage::AddRef()
{
    _cRef++;
    return _cRef;
}

ULONG CPackage::Release()
{
    _cRef--;
    if (_cRef > 0)
        return _cRef;

    delete this;
    return 0;
}

HRESULT CPackage_CreateInstnace(LPUNKNOWN * ppunk)
{
    DebugMsg(DM_TRACE, "pack - CreateInstance called");
    
    *ppunk = NULL;              // null the out param
    
    CPackage* pPack = new CPackage;
    if (!pPack)  
        return E_OUTOFMEMORY;
    
    if (FAILED(pPack->Init())) {
        delete pPack;
        return E_OUTOFMEMORY;
    }

    *ppunk = pPack;
    return S_OK;
}

STDMETHODIMP CPackage::Next(ULONG celt, OLEVERB* rgVerbs, ULONG* pceltFetched)
{
    HRESULT hr;
    if (NULL != rgVerbs)
    {
        if (1 == celt)
        {
            if (_nCurVerb < _cVerbs)
            {
                ASSERT(NULL != _pVerbs);
                *rgVerbs = _pVerbs[_nCurVerb];
                if ((NULL != _pVerbs[_nCurVerb].lpszVerbName) &&
                    (NULL != (rgVerbs->lpszVerbName = (LPWSTR) CoTaskMemAlloc(
                        (lstrlenW(_pVerbs[_nCurVerb].lpszVerbName) + 1) * SIZEOF(WCHAR)))))
                {
                    StrCpyW(rgVerbs->lpszVerbName, _pVerbs[_nCurVerb].lpszVerbName);
                }
                _nCurVerb++;
                hr = S_OK;
            }
            else
            {
                hr = S_FALSE;
            }
            if (NULL != pceltFetched)
            {
                *pceltFetched = (S_OK == hr) ? 1 : 0;
            }
        }
        else if (NULL != pceltFetched)
        {
            int cVerbsToCopy = min(celt, _cVerbs - _nCurVerb);
            if (cVerbsToCopy > 0)
            {
                ASSERT(NULL != _pVerbs);
                CopyMemory(rgVerbs, &(_pVerbs[_nCurVerb]), cVerbsToCopy * sizeof(OLEVERB));
                for (int i = 0; i < cVerbsToCopy; i++)
                {
                    if ((NULL != _pVerbs[_nCurVerb + i].lpszVerbName) &&
                        (NULL != (rgVerbs[i].lpszVerbName = (LPWSTR) CoTaskMemAlloc(
                            (lstrlenW(_pVerbs[_nCurVerb + i].lpszVerbName) + 1) * SIZEOF(WCHAR)))))
                    {
                        StrCpyW(rgVerbs[i].lpszVerbName, _pVerbs[_nCurVerb + i].lpszVerbName);
                    }
                }
                _nCurVerb += cVerbsToCopy;
            }
            *pceltFetched = (ULONG) cVerbsToCopy;
            hr = (celt == (ULONG) cVerbsToCopy) ? S_OK : S_FALSE;
        }
        else
        {
            hr = E_INVALIDARG;
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }
    return hr;
}

STDMETHODIMP CPackage::Skip(ULONG celt)
{
    if (_nCurVerb + celt > _cVerbs)
    {
        // there aren't enough elements, go to the end and return S_FALSE
        _nCurVerb = _cVerbs;
        return S_FALSE;
    }
    else
    {
        _nCurVerb += celt;
        return S_OK;
    }
}

STDMETHODIMP CPackage::Reset()
{
    _nCurVerb = 0;
    return S_OK;
}

STDMETHODIMP CPackage::Clone(IEnumOLEVERB** ppEnum)
{
    if (NULL != ppEnum)
    {
        *ppEnum = NULL;
    }
    return E_NOTIMPL;
}

///////////////////////////////////////////////////////////////////
//
// Package helper functions
//
///////////////////////////////////////////////////////////////////

HRESULT  CPackage::EmbedInitFromFile(LPTSTR lpFileName, BOOL fInitFile) 
{
    //
    // get's the file size of the packaged file and set's the name 
    // of the packaged file if fInitFile == TRUE.
    // return:  S_OK    -- initialized ok
    //          E_FAIL  -- error initializing file
    //
    
    DWORD dwSize;

    
    // if this is the first time we've been called, then we need to allocate
    // memory for the _pEmbed structure
    if (_pEmbed == NULL) 
    {
        _pEmbed = new EMBED;
        if (_pEmbed)
        {
            _pEmbed->pszTempName = NULL;
            _pEmbed->hTask = NULL;
            _pEmbed->poo = NULL;
            _pEmbed->fIsOleFile = TRUE;
        }
    }

    if (_pEmbed == NULL)
        return E_OUTOFMEMORY;

    
    // open the file to package...
    //
    HANDLE fh = CreateFile(lpFileName, GENERIC_READ, FILE_SHARE_READWRITE, 
            NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL); 
    if (fh == INVALID_HANDLE_VALUE) 
    {
        DWORD dwError = GetLastError();
        return E_FAIL;
    }

    _panetype = PEMBED;
    
    // Get the size of the file
    _pEmbed->fd.nFileSizeLow = GetFileSize(fh, &dwSize);
    if (_pEmbed->fd.nFileSizeLow == 0xFFFFFFFF) 
    {
        DWORD dwError = GetLastError();
        return E_FAIL;
    }
    ASSERT(dwSize == 0);
    _pEmbed->fd.nFileSizeHigh = 0L;
    _pEmbed->fd.dwFlags = FD_FILESIZE;

    // We only want to set the filename if this is the file to be packaged.
    // If it's only a temp file that we're reloading (fInitFile == FALSE) then
    // don't bother setting the filename.
    //
    if (fInitFile) 
    {
        lstrcpy(_pEmbed->fd.cFileName,lpFileName);
        DestroyIC();
        _lpic = IconCreateFromFile(lpFileName);
        if (_pIDataAdviseHolder)
            _pIDataAdviseHolder->SendOnDataChange(_pIDataObject,0, NULL);
        if (_pViewSink)
            _pViewSink->OnViewChange(_dwViewAspects,_dwViewAdvf);
    }

    _fIsDirty = TRUE;
    CloseHandle(fh);
    return S_OK;
}    

HRESULT CPackage::CmlInitFromFile(LPTSTR lpFileName, BOOL fUpdateIcon) 
{
    // if this is the first time we've been called, then we need to allocate
    // memory for the _pCml structure
    if (_pCml == NULL) 
    {
        _pCml = new CML;
        if (_pCml)
        {
            // we don't use this, but an old packager accessing us might.
            _pCml->fCmdIsLink = FALSE;
        }
    }

    if (_pCml == NULL)
        return E_OUTOFMEMORY;

    _panetype = CMDLINK;
    lstrcpy(_pCml->szCommandLine,lpFileName);
    _fIsDirty = TRUE;
    
    if (fUpdateIcon)
    {
        DestroyIC();
        _lpic = IconCreateFromFile(lpFileName);
    
        if (_pIDataAdviseHolder)
            _pIDataAdviseHolder->SendOnDataChange(_pIDataObject,0, NULL);
    
        if (_pViewSink)
            _pViewSink->OnViewChange(_dwViewAspects,_dwViewAdvf);
    }
    return S_OK;
}    


HRESULT CPackage::InitFromPackInfo(LPPACKAGER_INFO lppi)
{
    HRESULT hr;
    DWORD   dwFileAttributes = GetFileAttributes(lppi->szFilename);
    
    // Ok, we need to test whether the user tried to package a folder
    // instead of a file.  If s/he did, then we'll just create a link
    // to that folder instead of an embedded file.
    //

    if ((dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
    {
        hr = CmlInitFromFile(lppi->szFilename, FALSE);
    }
    else
    {
        // we pass FALSE here, because we don't want to write the icon
        // information.
        //      
        hr = EmbedInitFromFile(lppi->szFilename, FALSE);
        lstrcpy(_pEmbed->fd.cFileName,lppi->szFilename);
        _panetype = PEMBED;
    }

    // set the icon information
    lstrcpy(_lpic->szIconPath,lppi->szIconPath);
    _lpic->iDlgIcon = lppi->iIcon;
    lstrcpy(_lpic->szIconText,lppi->szLabel);
    IconRefresh();

    // we need to tell the client we want to be saved...it should be smart
    // enough to do it anyway, but we can't take any chances.
    if (_pIOleClientSite)
        _pIOleClientSite->SaveObject();

    return hr;
}    

HRESULT CPackage::CreateTempFileName()
{
    ASSERT(NULL != _pEmbed);
    TCHAR szDefPath[MAX_PATH];
    if (_pEmbed->pszTempName)
    {
        return S_OK;
    }
    else if (GetTempPath(ARRAYSIZE(szDefPath), szDefPath))
    {
        LPTSTR pszFile;
        if ((NULL != _lpic) && (TEXT('\0') != _lpic->szIconText[0]))
        {
            pszFile = _lpic->szIconText;
        }
        else
        {
            pszFile = PathFindFileName(_pEmbed->fd.cFileName);
        }
        PathAppend(szDefPath, pszFile);
        if (PathFileExists(szDefPath))
        {
            TCHAR szOriginal[MAX_PATH];
            lstrcpy(szOriginal, szDefPath);
            PathYetAnotherMakeUniqueName(szDefPath, szOriginal, NULL, NULL);
        }
        
        _pEmbed->pszTempName = new TCHAR[lstrlen(szDefPath) + 1];
        if (!_pEmbed->pszTempName) 
        {
            DebugMsg(DM_TRACE,"            couldn't alloc memory for pszTempName!!");
            return E_OUTOFMEMORY;
        }    
        lstrcpy(_pEmbed->pszTempName, szDefPath);
        return S_OK;
    }
    else
    {
        DebugMsg(DM_TRACE,"            couldn't get temp path!!");
        return E_FAIL;
    }
}

HRESULT CPackage::CreateTempFile() 
{
    //
    // used to create a temporary file that holds the file contents of the
    // packaged file.  the old packager used to keep the packaged file in 
    // memory which is just a total waste.  so, being as we're much more 
    // efficient, we create a temp file whenever someone wants to do something
    // with our contents.  we initialze the temp file from the original file
    // to package or our persistent storage depending on whether we are a new
    // package or a loaded package
    // return:  S_OK    -- temp file created
    //          E_FAIL  -- error creating temp file
    //
    
    DebugMsg(DM_TRACE,"            CreateTempFile() called.");

    HRESULT hr = CreateTempFileName();
    if (FAILED(hr))
    {
        return hr;
    }

    if (PathFileExists(_pEmbed->pszTempName))
    {
        DebugMsg(DM_TRACE,"            already have a temp file!!");
        return S_OK;
    }
    
    // if we weren't loaded from a storage then we're in the process of
    // creating a package, and should be able to copy the packaged file
    // to create a temp file
    //
    if (!_fLoaded) 
    {
        if (!(CopyFile(_pEmbed->fd.cFileName, _pEmbed->pszTempName, FALSE))) 
        {
            DebugMsg(DM_TRACE,"            couldn't copy file!!");
            return E_FAIL;
        }
    }
    else 
    {
        TCHAR szTempFile[MAX_PATH];
        // copy the file name because _pEmbed may get re-created below,
        // but we want to hold on to this filename:
        lstrcpy(szTempFile, _pEmbed->pszTempName);
        
        // if we have a valid stream, but not a file contents stream, 
        // it's because we went hands off and didn't know where to put
        // the seek pointer to init the filecontents stream.  so, we
        // call PackageReadFromStream to create the FileContents stream
        //
        if (_pstm && !_pstmFileContents) 
        {
            if (FAILED(PackageReadFromStream(_pstm))) 
            {
                DebugMsg(DM_TRACE,"            couldn't read from stream!!");
                return E_FAIL;
            }
        }
        
        IStream* pstm;
        _pstmFileContents->Clone(&pstm);  // we don't want to move the seek
                                          // pointer on our FileContents stream
        
        if (FAILED(CopyStreamToFile(pstm, szTempFile))) 
        {
            DebugMsg(DM_TRACE,"            couldn't copy from stream!!");
            pstm->Release();
            return E_FAIL;
        }
        else
        {
            ASSERT(_pEmbed);
            delete _pEmbed->pszTempName;
            if (NULL != (_pEmbed->pszTempName = new TCHAR[lstrlen(szTempFile) + 1]))
            {
                lstrcpy(_pEmbed->pszTempName, szTempFile);
            }
            else
            {
                return E_OUTOFMEMORY;
            }
        }
        pstm->Release();
    }

    
    // whenever we create a tempfile we are activating the contents which 
    // means we are dirty until we get a save message
    return S_OK;
}



///////////////////////////////////////////////////////////////////////
//
// Data Transfer Functions
//
///////////////////////////////////////////////////////////////////////

HRESULT CPackage::GetFileDescriptor(LPFORMATETC pFE, LPSTGMEDIUM pSTM) 
{
    FILEGROUPDESCRIPTOR *pfgd;

    
    DebugMsg(DM_TRACE,"            Getting File Descriptor");

    // we only support HGLOBAL at this time
    //
    if (!(pFE->tymed & TYMED_HGLOBAL)) {
        DebugMsg(DM_TRACE,"            does not support HGLOBAL!");
        return DATA_E_FORMATETC;
    }

    //// Copy file descriptor to HGLOBAL ///////////////////////////
    //
    pSTM->tymed = TYMED_HGLOBAL;
    
    // render the file descriptor 
    if (!(pfgd = (FILEGROUPDESCRIPTOR *)GlobalAlloc(GPTR,
        sizeof(FILEGROUPDESCRIPTOR))))
        return E_OUTOFMEMORY;

    pSTM->hGlobal = pfgd;
    
    pfgd->cItems = 1;

    switch(_panetype) 
    {
        case PEMBED:
            pfgd->fgd[0] = _pEmbed->fd;
            GetDisplayName(pfgd->fgd[0].cFileName, _pEmbed->fd.cFileName);
            break;

        case CMDLINK:
            // the label for the package will serve as the filename for the
            // shortcut we're going to create.
            lstrcpy(pfgd->fgd[0].cFileName, _lpic->szIconText);
            // BUGBUG: harcoded use of .lnk extension!!
            lstrcat(pfgd->fgd[0].cFileName, TEXT(".lnk"));

            // we want to add the little arrow to the shortcut.
            pfgd->fgd[0].dwFlags = FD_LINKUI;
            break;
    }
    return S_OK;
}

HRESULT CPackage::GetFileContents(LPFORMATETC pFE, LPSTGMEDIUM pSTM) 
{
    void *  lpvDest = NULL;
    DWORD   dwSize;
    HANDLE  hFile = NULL;
    DWORD   cb;
    HRESULT hr = E_FAIL;
    
    DebugMsg(DM_TRACE,"            Getting File Contents");
    
    //// Copy file contents to ISTREAM ///////////////////////////
    // 
    // NOTE: Hopefully, everyone using our object supports TYMED_ISTREAM,
    // otherwise we could get some really slow behavior.  We might later
    // want to implement TYMED_ISTORAGE as well and shove our file contents
    // into a single stream named CONTENTS.
    //
    if (pFE->tymed & TYMED_ISTREAM) {
        DebugMsg(DM_TRACE,"            using TYMED_ISTREAM");
        pSTM->tymed = TYMED_ISTREAM;

        switch (_panetype) {
            case PEMBED:
                if (_pstmFileContents)
                    hr = _pstmFileContents->Clone(&pSTM->pstm);
                else 
                    return E_FAIL;
                break;

            case CMDLINK:
                hr = CreateStreamOnHGlobal(NULL, TRUE, &pSTM->pstm);
                if (SUCCEEDED(hr))
                {
                    hr = CreateShortcutOnStream(pSTM->pstm);
                    if (FAILED(hr))
                    {
                        pSTM->pstm->Release();
                        pSTM->pstm = NULL;
                    }
                }
                break;
        }
        return hr;
    }
    
    //// Copy file contents to HGLOBAL ///////////////////////////
    //
    // NOTE: This is really icky and could potentially be very slow if
    // somebody decides to package really large files.  Hopefully, 
    // everyone should be able to get the info it wants through TYMED_ISTREAM,
    // but this is here as a common denominator
    //
    if (pFE->tymed & TYMED_HGLOBAL) {
        DebugMsg(DM_TRACE,"            using TYMED_HGLOBAL");
        pSTM->tymed = TYMED_HGLOBAL;

        if (_panetype == CMDLINK) {
            DebugMsg(DM_TRACE,
                "    H_GLOBAL not supported for CMDLINK");
            return DATA_E_FORMATETC;
        }
        
        dwSize = _pEmbed->fd.nFileSizeLow;
        
        // caller is responsible for freeing this memory, even if we fail.
        if (!(lpvDest = GlobalAlloc(GPTR, dwSize))) {
            DebugMsg(DM_TRACE,"            out o memory!!");
            return E_OUTOFMEMORY;
        }
        pSTM->hGlobal = lpvDest;
        
        // This will reinitialize our FileContents stream if we had to get
        // rid of it.  For instance, we have to get rid of all our storage
        // pointers in HandsOffStorage, but there's no need to reinit our 
        // FileContents stream unless we need it again.
        //
        if (_pstm && !_pstmFileContents)
            PackageReadFromStream(_pstm);
        
        if (_pstmFileContents) {
            IStream* pstm;
            hr = _pstmFileContents->Clone(&pstm);
            if (FAILED(hr))
                return hr;
            hr = pstm->Read(lpvDest, dwSize, &cb);
            pstm->Release();
            if (FAILED(hr))
                return hr;
        }
        else
            return E_FAIL;
        
        if (FAILED(hr) || cb != dwSize) {
            DebugMsg(DM_TRACE,"            error reading from stream!!");
            return E_FAIL;
        }
        return hr;
    }
    
    return DATA_E_FORMATETC;
}

HRESULT CPackage::GetMetafilePict(LPFORMATETC pFE, LPSTGMEDIUM pSTM)
{
    LPMETAFILEPICT      lpmfpict;
    RECT                rcTemp;
    LPIC                lpic = _lpic;
    HDC                 hdcMF = NULL;
    HFONT               hfont = NULL;
    
    
    DebugMsg(DM_TRACE,"            Getting MetafilePict");
    
    if (!(pFE->tymed & TYMED_MFPICT)) {
        DebugMsg(DM_TRACE,"            does not support MFPICT!");
        return DATA_E_FORMATETC;
    }
    pSTM->tymed = TYMED_MFPICT;
    
    // Allocate memory for the metafilepict and get a pointer to it
    // NOTE: the caller is responsible for freeing this memory, even on fail
    //
    if (!(pSTM->hMetaFilePict = GlobalAlloc(GPTR, sizeof(METAFILEPICT))))
        return E_OUTOFMEMORY;
    lpmfpict = (LPMETAFILEPICT)pSTM->hMetaFilePict;
        
    // Create the metafile
    if (!(hdcMF = CreateMetaFile(NULL))) 
        return E_OUTOFMEMORY;
    
    // Initializae the metafile
    SetWindowOrgEx(hdcMF, 0, 0, NULL);
    SetWindowExtEx(hdcMF, lpic->rc.right - 1, lpic->rc.bottom - 1, NULL);

    SetRect(&rcTemp, 0, 0, lpic->rc.right,lpic->rc.bottom);
    hfont = SelectFont(hdcMF, g_hfontTitle);
    
    // Center the icon
    DrawIcon(hdcMF, (rcTemp.right - g_cxIcon) / 2, 0, lpic->hDlgIcon);
    
    // Center the text below the icon
    SetBkMode(hdcMF, TRANSPARENT);
    SetTextAlign(hdcMF, TA_CENTER);
    TextOut(hdcMF, rcTemp.right / 2, g_cxIcon + 1, lpic->szIconText,
            lstrlen(lpic->szIconText));
    
    if (hfont)
        SelectObject(hdcMF, hfont);
    
    // Map to device independent coordinates
    rcTemp.right =
        MulDiv((rcTemp.right - rcTemp.left), HIMETRIC_PER_INCH, DEF_LOGPIXELSX);
    rcTemp.bottom =
        MulDiv((rcTemp.bottom - rcTemp.top), HIMETRIC_PER_INCH, DEF_LOGPIXELSY);

    // Finish filling in the metafile header
    lpmfpict->mm = MM_ANISOTROPIC;
    lpmfpict->xExt = rcTemp.right;
    lpmfpict->yExt = rcTemp.bottom;
    lpmfpict->hMF = CloseMetaFile(hdcMF);
    
    return S_OK;
}


HRESULT CPackage::GetObjectDescriptor(LPFORMATETC pFE, LPSTGMEDIUM pSTM) 
{
    LPOBJECTDESCRIPTOR lpobj;
    DWORD   dwFullUserTypeNameLen;
    
    DebugMsg(DM_TRACE,"            Getting Object Descriptor");

    // we only support HGLOBAL at this time
    //
    if (!(pFE->tymed & TYMED_HGLOBAL)) {
        DebugMsg(DM_TRACE,"            does not support HGLOBAL!");
        return DATA_E_FORMATETC;
    }

    //// Copy file descriptor to HGLOBAL ///////////////////////////

    dwFullUserTypeNameLen = 0; //lstrlen(szUserType) + 1;
    pSTM->tymed = TYMED_HGLOBAL;

    if (!(lpobj = (OBJECTDESCRIPTOR *)GlobalAlloc(GPTR,
        sizeof(OBJECTDESCRIPTOR)+dwFullUserTypeNameLen)))
        return E_OUTOFMEMORY;

    pSTM->hGlobal = lpobj;
    
    lpobj->cbSize       = sizeof(OBJECTDESCRIPTOR)+dwFullUserTypeNameLen;
    lpobj->clsid        = CLSID_CPackage;
    lpobj->dwDrawAspect = DVASPECT_CONTENT|DVASPECT_ICON;
    _pIOleObject->GetMiscStatus(DVASPECT_CONTENT|DVASPECT_ICON,&(lpobj->dwStatus));
    lpobj->dwFullUserTypeName = 0L; //sizeof(OBJECTDESCRIPTOR);
    lpobj->dwSrcOfCopy = 0L;

    // lstrcpy((LPTSTR)lpobj+lpobj->dwFullUserTypeName, szUserType);
    return S_OK;
}


/////////////////////////////////////////////////////////////////////////
//
// Stream I/O Functions
//
/////////////////////////////////////////////////////////////////////////

HRESULT CPackage::PackageReadFromStream(IStream* pstm)
{
    //
    // initialize the package object from a stream
    // return:  s_OK   - package properly initialized
    //          E_FAIL - error initializing package
    //
    
    WORD  w;
    DWORD dw;
    
    DebugMsg(DM_TRACE, "pack - PackageReadFromStream called.");

    // read in the package size, which we don't really need, but the old 
    // packager puts it there.
    if (FAILED(pstm->Read(&dw, sizeof(dw), NULL)))
        return E_FAIL;

    // NOTE: Ok, this is really dumb.  The old packager allowed the user
    // to create packages without giving them icons or labels, which
    // in my opinion is just dumb, it should have at least created a default
    // icon and shoved it in the persistent storage...oh well...
    //     So if the appearance type comes back as NOTHING ( == 0)
    // then we just won't read any icon information.
    
    // read in the appearance type
    pstm->Read(&w, sizeof(w), NULL);
    
    // read in the icon information
    if (w == (WORD)ICON)
    {
        if (FAILED(IconReadFromStream(pstm))) 
        {
            DebugMsg(DM_TRACE,"         error reading icon info!!");
            return E_FAIL;
        }
    }
    else if (w == (WORD)PICTURE)
    {
        DebugMsg(DM_TRACE, "         old Packager Appearance, not supported!!");
        // NOTE: Ideally, we could just ignore the appearance and continue, but to
        // do so, we'll need to know how much information to skip over before continuing
        // to read from the stream
        ShellMessageBox(g_hinst,
                        NULL,
                        MAKEINTRESOURCE(IDS_OLD_FORMAT_ERROR),
                        MAKEINTRESOURCE(IDS_APP_TITLE),
                        MB_OK | MB_ICONERROR | MB_TASKMODAL);
        return E_FAIL;
    }
    
    // read in the contents type
    pstm->Read(&w, sizeof(w), NULL);

    _panetype = (PANETYPE)w;
    
    switch((PANETYPE)w)
    {
    case PEMBED:
        // read in the contents information
        return EmbedReadFromStream(pstm);

    case CMDLINK:
        // read in the contents information
        return CmlReadFromStream(pstm); 

    default:
        return E_FAIL;
    }
}

//
// read the icon info from a stream
// return:  S_OK   -- icon read correctly
//          E_FAIL -- error reading icon
//
HRESULT CPackage::IconReadFromStream(IStream* pstm) 
{
    LPIC lpic = IconCreate();
    if (lpic)
    {
        CHAR szTemp[MAX_PATH];
        StringReadFromStream(pstm, szTemp, ARRAYSIZE(szTemp));
        SHAnsiToTChar(szTemp, lpic->szIconText, ARRAYSIZE(lpic->szIconText));
        
        StringReadFromStream(pstm, szTemp, ARRAYSIZE(szTemp));
        SHAnsiToTChar(szTemp, lpic->szIconPath, ARRAYSIZE(lpic->szIconPath));
        
        WORD wDlgIcon;
        pstm->Read(&wDlgIcon, sizeof(wDlgIcon), NULL);
        lpic->iDlgIcon = (INT) wDlgIcon;
        GetCurrentIcon(lpic);
        IconCalcSize(lpic);
    }

    DestroyIC();
    _lpic = lpic;

    return lpic ? S_OK : E_FAIL;
}

HRESULT CPackage::EmbedReadFromStream(IStream* pstm) 
{
    //
    // reads embedded file contents from a stream
    // return:  S_OK   - contents read succesfully
    //          E_FAIL - error reading contents
    //
    
    DWORD dwSize;
    CHAR  szFileName[MAX_PATH];
    
    DebugMsg(DM_TRACE, "pack - EmbedReadFromStream called.");
    
    pstm->Read(&dwSize, sizeof(dwSize), NULL);  // get string size
    pstm->Read(szFileName, dwSize, NULL);       // get string
    pstm->Read(&dwSize, sizeof(dwSize), NULL);  // get file size

    // we don't do anything with the file contents here, because anything
    // we do could be a potentially expensive operation.  so, we just clone
    // the stream and hold onto it for future use.
    
    if (_pstmFileContents) 
        _pstmFileContents->Release();
        
    if (FAILED(pstm->Clone(&_pstmFileContents)))
        return E_FAIL;

    if (_pEmbed) {
        if (_pEmbed->pszTempName) {
            DeleteFile(_pEmbed->pszTempName);
            delete _pEmbed->pszTempName;
        }
        delete _pEmbed;
    }

    _pEmbed = new EMBED;
    if (NULL != _pEmbed)
    {
        _pEmbed->fd.dwFlags = FD_FILESIZE;
        _pEmbed->fd.nFileSizeLow = dwSize;
        _pEmbed->fd.nFileSizeHigh = 0;
        SHAnsiToTChar(szFileName, _pEmbed->fd.cFileName, ARRAYSIZE(_pEmbed->fd.cFileName));
        DebugMsg(DM_TRACE,"         %s\n\r         %d",_pEmbed->fd.cFileName,_pEmbed->fd.nFileSizeLow);
        return S_OK;
    }
    else
    {
        return E_OUTOFMEMORY;
    }
}


HRESULT CPackage::CmlReadFromStream(IStream* pstm)
{
    //
    // reads command line contents from a stream
    // return:  S_OK   - contents read succesfully
    //          E_FAIL - error reading contents
    //

    WORD w;
    CHAR  szCmdLink[CBCMDLINKMAX];
    
    DebugMsg(DM_TRACE, "pack - CmlReadFromStream called.");

    // read in the fCmdIsLink and the command line string
    if (FAILED(pstm->Read(&w, sizeof(w), NULL)))    
        return E_FAIL;
    StringReadFromStream(pstm, szCmdLink, ARRAYSIZE(szCmdLink));

    if (_pCml != NULL)
        delete _pCml;
    
    _pCml = new CML;
    SHAnsiToTChar(szCmdLink, _pCml->szCommandLine, ARRAYSIZE(_pCml->szCommandLine));
    
    return S_OK;
}    
    

HRESULT CPackage::PackageWriteToStream(IStream* pstm)
{
    //
    // write the package object to a stream
    // return:  s_OK   - package properly written
    //          E_FAIL - error writing package
    //
    
    WORD w;
    DWORD cb = 0L;
    DWORD dwSize;
      
    DebugMsg(DM_TRACE, "pack - PackageWriteToStream called.");

    // write out a DWORD where the package size will go
    if (FAILED(pstm->Write(&cb, sizeof(DWORD), NULL)))
        return E_FAIL;
    
    // write out the appearance type
    w = (WORD)ICON;
    if (FAILED(pstm->Write(&w, sizeof(WORD), NULL)))
        return E_FAIL;
    cb += 2*sizeof(WORD);       // for appearance type and contents type
    
    // write out the icon information
    if (FAILED(IconWriteToStream(pstm,&dwSize))) 
    {
        DebugMsg(DM_TRACE,"         error writing icon info!!");
        return E_FAIL;
    }
    cb += dwSize;

    // write out the contents type
    w = (WORD)_panetype;
    if (FAILED(pstm->Write(&_panetype, sizeof(WORD), NULL)))
        return E_FAIL;

    switch(_panetype) 
    {
        case PEMBED:
            
            // write out the contents information
            if (FAILED(EmbedWriteToStream(pstm,&dwSize))) 
            {
                DebugMsg(DM_TRACE,"         error writing embed info!!");
                return E_FAIL;
            }
            cb += dwSize;
            break;

        case CMDLINK:
            // write out the contents information
            if (FAILED(CmlWriteToStream(pstm,&dwSize))) {
                DebugMsg(DM_TRACE,"         error writing cml info!!");
                return E_FAIL;
            }
            cb += dwSize;
            break;
    }

    
    LARGE_INTEGER li = {0, 0};
    if (FAILED(pstm->Seek(li, STREAM_SEEK_SET, NULL)))
        return E_FAIL;
    if (FAILED(pstm->Write(&cb, sizeof(DWORD), NULL)))
        return E_FAIL;
    
    return S_OK;
}


//
// write the icon to a stream
// return:  s_OK   - icon properly written
//          E_FAIL - error writing icon
//
HRESULT CPackage::IconWriteToStream(IStream* pstm, DWORD *pdw)
{
    DWORD cb = 0;
    CHAR szTemp[MAX_PATH];
    SHTCharToAnsi(_lpic->szIconText, szTemp, ARRAYSIZE(szTemp));
    HRESULT hr = StringWriteToStream(pstm, szTemp, &cb);
    if (SUCCEEDED(hr))
    {
        SHTCharToAnsi(_lpic->szIconPath, szTemp, ARRAYSIZE(szTemp));
        hr = StringWriteToStream(pstm, szTemp, &cb);
        if (SUCCEEDED(hr))
        {
            DWORD dwWrite;
            WORD wDlgIcon = (WORD) _lpic->iDlgIcon;
            hr = pstm->Write(&wDlgIcon, sizeof(wDlgIcon), &dwWrite);
            if (SUCCEEDED(hr))
            {
                cb += dwWrite;
                if (pdw)
                    *pdw = cb;
            }
        }
    }
    return hr;
}

//
// write embedded file contents to a stream
// return:  S_OK   - contents written succesfully
//          E_FAIL - error writing contents
//
HRESULT CPackage::EmbedWriteToStream(IStream* pstm, DWORD *pdw)
{
    DWORD cb = 0;
    CHAR szTemp[MAX_PATH];
    SHTCharToAnsi(_pEmbed->fd.cFileName, szTemp, ARRAYSIZE(szTemp));
    DWORD dwSize = lstrlenA(szTemp) + 1;
    HRESULT hr = pstm->Write(&dwSize, sizeof(dwSize), &cb);
    if (SUCCEEDED(hr))
    {
        DWORD dwWrite;
        hr = StringWriteToStream(pstm, szTemp, &dwWrite);
        if (SUCCEEDED(hr))
        {
            cb += dwWrite;
            hr = pstm->Write(&_pEmbed->fd.nFileSizeLow, sizeof(_pEmbed->fd.nFileSizeLow), &dwWrite);
            if (SUCCEEDED(hr))
            {
                cb += dwWrite;

                // we want to make sure our file contents stream always points to latest 
                // saved file contents
                //
                // NOTE: If we're not saving to our loaded stream, we shouldn't keep a
                // pointer to it, because we're in a SaveAs situation, and we don't want
                // to be hanging onto pointers to other peoples streams.
                //
                if (_pstmFileContents && _pstm == pstm)
                {
                    _pstmFileContents->Release();
                    pstm->Clone(&_pstmFileContents);
                }

                // This is for screwy apps, like MSWorks that ask us to save ourselves 
                // before they've even told us to initialize ourselves.  
                //
                if (_pEmbed->fd.cFileName[0])
                {
                    hr = CopyFileToStream(_pEmbed->pszTempName, pstm);
                    if (SUCCEEDED(hr))
                    {
                        cb += _pEmbed->fd.nFileSizeLow;
                    }
                }
                if (pdw)
                    *pdw = cb;
            }
        }
    }
    return hr;
}

//
// write embedded file contents to a stream
// return:  S_OK   - contents written succesfully
//          E_FAIL - error writing contents
//
HRESULT CPackage::CmlWriteToStream(IStream* pstm, DWORD *pdw)
{
    DWORD cb = 0;
    WORD w = _pCml->fCmdIsLink;
    
    DebugMsg(DM_TRACE, "pack - CmlWriteToStream called.");

    if (FAILED(pstm->Write(&w, sizeof(w), NULL)))
        return E_FAIL;                                   // write fCmdIsLink
    cb += sizeof(w);      // for fCmdIsLink

    CHAR szTemp[MAX_PATH];
    SHTCharToAnsi(_pCml->szCommandLine, szTemp, ARRAYSIZE(szTemp));
    HRESULT hres = StringWriteToStream(pstm, szTemp, &cb);
    if (FAILED(hres))
        return hres;                                   // write command link

    // return the number of bytes written in the outparam    
    if (pdw)
        *pdw = cb;
    
    return S_OK;
}


HRESULT CPackage::CreateShortcutOnStream(IStream* pstm)
{
    HRESULT hr;
    IShellLink *psl;
    TCHAR szArgs[CBCMDLINKMAX - MAX_PATH];
    TCHAR szPath[MAX_PATH];
    
    hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
        IID_IShellLink, (void **)&psl);
    if (SUCCEEDED(hr))
    {
        IPersistStream *pps;

        lstrcpy(szPath,_pCml->szCommandLine);
        PathSeparateArgs(szPath, szArgs);

        psl->SetPath(szPath);
        psl->SetIconLocation(_lpic->szIconPath, _lpic->iDlgIcon);
        psl->SetShowCmd(SW_SHOW);
        psl->SetArguments(szArgs);
        
        hr = psl->QueryInterface(IID_IPersistStream, (void **)&pps);
        if (SUCCEEDED(hr))
        {
            hr = pps->Save(pstm,TRUE);
            pps->Release();
        }
        psl->Release();
    }
    
    LARGE_INTEGER li = {0,0};
    pstm->Seek(li,STREAM_SEEK_SET,NULL);

    return hr;
}

HRESULT CPackage::InitVerbEnum(OLEVERB* pVerbs, ULONG cVerbs)
{
    if (NULL != _pVerbs)
    {
        for (ULONG i = 0; i < _cVerbs; i++)
        {
            delete _pVerbs[i].lpszVerbName;
        }
        delete _pVerbs;
    }
    _pVerbs = pVerbs;
    _cVerbs = cVerbs;
    _nCurVerb = 0;
    return (NULL != pVerbs) ? S_OK : E_FAIL;
}

VOID CPackage::ReleaseContextMenu()
{
    if (NULL != _pcm)
    {
        _pcm->Release();
        _pcm = NULL;
    }
}

HRESULT CPackage::GetContextMenu(IContextMenu** ppcm)
{
    HRESULT hr = E_FAIL;
    ASSERT(NULL != ppcm);
    if (NULL != _pcm)
    {
        _pcm->AddRef();
        *ppcm = _pcm;
        hr = S_OK;
    }
    else if ((PEMBED == _panetype) || (CMDLINK == _panetype))
    {
        if (PEMBED == _panetype)
        {
            hr = CreateTempFileName();
        }
        else
        {
            hr = S_OK;
        }
        if (SUCCEEDED(hr))
        {
            LPITEMIDLIST pidl = SHSimpleIDListFromPath((PEMBED == _panetype) ?
                                                        _pEmbed->pszTempName :
                                                        _pCml->szCommandLine);
            if (NULL != pidl)
            {
                IShellFolder* psf;
                LPCITEMIDLIST pidlChild;
                if (SUCCEEDED(hr = SHBindToIDListParent(pidl, IID_IShellFolder, (void **)&psf, &pidlChild)))
                {
                    hr = psf->GetUIObjectOf(NULL, 1, &pidlChild, IID_IContextMenu, NULL, (void**) &_pcm);
                    if (SUCCEEDED(hr))
                    {
                        _pcm->AddRef();
                        *ppcm = _pcm;
                    }
                    psf->Release();
                }
                ILFree(pidl);
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }
    return hr;
}

HRESULT CPackage::IconRefresh()
{
    // we refresh the icon.  typically, this will be called the first time
    // the package is created to load the new icon and calculate how big
    // it should be.  this will also be called after we edit the package,
    // since the user might have changed the icon.
    
    // First, load the appropriate icon.  We'll load the icon specified by
    // lpic->szIconPath and lpic->iDlgIcon if possible, otherwise we'll just
    // use the generic packager icon.
    //
    GetCurrentIcon(_lpic);

    // Next, we need to have the icon recalculate its size, since it's text
    // might have changed, causing it to get bigger or smaller.
    //
    IconCalcSize(_lpic);

    // Next, notify our containers that our view has changed.
    if (_pIDataAdviseHolder)
        _pIDataAdviseHolder->SendOnDataChange(_pIDataObject,0, NULL);
    if (_pViewSink)
        _pViewSink->OnViewChange(_dwViewAspects,_dwViewAdvf);

    // Set our dirty flag
    _fIsDirty = TRUE;

    return S_OK;
}

    
int CPackage::RunWizard()
{
    PACKAGER_INFO packInfo;
    HRESULT hr;
    
    PackWiz_CreateWizard(NULL, &packInfo);
    
    if (*packInfo.szFilename == TEXT('\0'))
    {
        ShellMessageBox(g_hinst,
                        NULL,
                        MAKEINTRESOURCE(IDS_CREATE_ERROR),
                        MAKEINTRESOURCE(IDS_APP_TITLE),
                        MB_ICONERROR | MB_TASKMODAL | MB_OK);
        return E_FAIL;
    }

    InitFromPackInfo(&packInfo);

    hr = OleSetClipboard(_pIDataObject);
    if (FAILED(hr))
    {
        ShellMessageBox(g_hinst,
                        NULL,
                        MAKEINTRESOURCE(IDS_COPY_ERROR),
                        MAKEINTRESOURCE(IDS_APP_TITLE),
                        MB_ICONERROR | MB_TASKMODAL | MB_OK);
        return -1;
    }

    // we need to do this.  our OleUninitialze call at the end, free the
    // libarary and our dataobject on the clipboard unless we flush
    // the clipboard.
    
    hr = OleFlushClipboard();
    if (FAILED(hr))
    {
        ShellMessageBox(g_hinst,
                        NULL,
                        MAKEINTRESOURCE(IDS_COPY_ERROR),
                        MAKEINTRESOURCE(IDS_APP_TITLE),
                        MB_ICONERROR | MB_TASKMODAL | MB_OK);
        return -1;
    }
    
    ShellMessageBox(g_hinst,
                    NULL,
                    MAKEINTRESOURCE(IDS_COPY_COMPLETE),
                    MAKEINTRESOURCE(IDS_APP_TITLE),
                    MB_ICONINFORMATION | MB_TASKMODAL | MB_OK);
    
    return 0;
}

void CPackage::DestroyIC()
{
    if (_lpic)
    {
        if (_lpic->hDlgIcon)
            DestroyIcon(_lpic->hDlgIcon);
        
        GlobalFree(_lpic);
    }
}

STDAPI_(BOOL) PackWizRunFromExe()
{
    OleInitialize(NULL);

    CPackage *pPackage = new CPackage;
    if (pPackage)
    {
        pPackage->Init();
        pPackage->RunWizard();
        pPackage->Release();
    }
    
    OleUninitialize();
    return 0;
}
