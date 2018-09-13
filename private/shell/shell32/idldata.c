#include "shellprv.h"
#pragma  hdrstop

#include "bookmk.h"

#include "datautil.h"


// External prototypes
HIDA HIDA_Create2(LPVOID pida, UINT cb);

CLIPFORMAT g_acfIDLData[ICF_MAX] = { CF_HDROP, 0 };

#define RCF(x)  (CLIPFORMAT) RegisterClipboardFormat(x)

STDAPI_(void) IDLData_InitializeClipboardFormats(void)
{
    if (g_cfHIDA == 0)
    {
        g_cfHIDA                       = RCF(CFSTR_SHELLIDLIST);
        g_cfOFFSETS                    = RCF(CFSTR_SHELLIDLISTOFFSET);
        g_cfNetResource                = RCF(CFSTR_NETRESOURCES);
        g_cfFileContents               = RCF(CFSTR_FILECONTENTS);         // "FileContents"
        g_cfFileGroupDescriptorA       = RCF(CFSTR_FILEDESCRIPTORA);      // "FileGroupDescriptor"
        g_cfFileGroupDescriptorW       = RCF(CFSTR_FILEDESCRIPTORW);      // "FileGroupDescriptor"
        g_cfPrivateShellData           = RCF(CFSTR_SHELLIDLISTP);
        g_cfFileName                   = RCF(CFSTR_FILENAMEA);            // "FileName"
        g_cfFileNameW                  = RCF(CFSTR_FILENAMEW);            // "FileNameW"
        g_cfFileNameMap                = RCF(CFSTR_FILENAMEMAP);          // "FileNameMap"
        g_cfFileNameMapW               = RCF(CFSTR_FILENAMEMAPW);         // "FileNameMapW"
        g_cfPrinterFriendlyName        = RCF(CFSTR_PRINTERGROUP);
        g_cfHTML                       = RCF(TEXT("HTML Format"));
        g_cfPreferredDropEffect        = RCF(CFSTR_PREFERREDDROPEFFECT);  // "Preferred DropEffect"
        g_cfPerformedDropEffect        = RCF(CFSTR_PERFORMEDDROPEFFECT);  // "Performed DropEffect"
        g_cfLogicalPerformedDropEffect = RCF(CFSTR_LOGICALPERFORMEDDROPEFFECT); // "Logical Performed DropEffect"
        g_cfPasteSucceeded             = RCF(CFSTR_PASTESUCCEEDED);       // "Paste Succeeded"
        g_cfShellURL                   = RCF(CFSTR_SHELLURL);             // "Uniform Resource Locator"
        g_cfInDragLoop                 = RCF(CFSTR_INDRAGLOOP);           // "InShellDragLoop"
        g_cfDragContext                = RCF(CFSTR_DRAGCONTEXT);          // "DragContext"
        g_cfTargetCLSID                = RCF(CFSTR_TARGETCLSID);          // "TargetCLSID", who the drag drop went to
        g_cfEmbeddedObject             = RCF(TEXT("Embedded Object"));
        g_cfObjectDescriptor           = RCF(TEXT("Object Descriptor"));
        g_cfNotRecyclable        = RCF(TEXT("NotRecyclable"));      // This object is not recyclable in the recycle bin.
    }
}


//===========================================================================
// CIDLData : Class definition (for subclass)
//===========================================================================

#define MAX_FORMATS     ICF_MAX

typedef struct _IDLData // idt
{
    IDataObject dtobj;
    IAsyncOperation ao;

    UINT _cRef;
    IDataObject *_pdtInner;
    IUnknown *_punkThread;
    BOOL _fEnumFormatCalled;    // TRUE once called.
    BOOL _fDidAsynchStart;

    FORMATETC _fmte[MAX_FORMATS];
    STGMEDIUM _medium[MAX_FORMATS];
} CIDLData;


STDMETHODIMP CIDLData_QueryInterface(IDataObject * pdtobj, REFIID riid, void **ppvObj)
{
    CIDLData * this = IToClass(CIDLData, dtobj, pdtobj);

    if (IsEqualIID(riid, &IID_IDataObject) || IsEqualIID(riid, &IID_IUnknown))
    {
        *ppvObj = &this->dtobj;
    }
    else if (IsEqualIID(riid, &IID_IAsyncOperation))
    {
        *ppvObj = &this->ao;
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }
    InterlockedIncrement(&this->_cRef);
    return S_OK;
}

STDMETHODIMP_(ULONG) CIDLData_AddRef(IDataObject *pdtobj)
{
    CIDLData * this = IToClass(CIDLData, dtobj, pdtobj);
    return InterlockedIncrement(&this->_cRef);
}

STDMETHODIMP_(ULONG) CIDLData_Release(IDataObject *pdtobj)
{
    CIDLData * this = IToClass(CIDLData, dtobj, pdtobj);
    int i;

    if (InterlockedDecrement(&this->_cRef))
        return this->_cRef;

    for (i = 0; i < MAX_FORMATS; i++)
    {
        if (this->_medium[i].hGlobal)
            ReleaseStgMedium(&this->_medium[i]);
    }

    if (this->_pdtInner)
        this->_pdtInner->lpVtbl->Release(this->_pdtInner);

    if (this->_punkThread)
        this->_punkThread->lpVtbl->Release(this->_punkThread);

    LocalFree((HLOCAL)this);
    return 0;
}

STDMETHODIMP CIDLData_GetData(IDataObject * pdtobj, LPFORMATETC pformatetcIn, LPSTGMEDIUM pmedium)
{
    CIDLData * this = IToClass(CIDLData, dtobj, pdtobj);
    HRESULT hres = E_INVALIDARG;
    int i;

    pmedium->hGlobal = NULL;
    pmedium->pUnkForRelease = NULL;

    for (i = 0; i < MAX_FORMATS; i++)
    {
        if ((this->_fmte[i].cfFormat == pformatetcIn->cfFormat) &&
            (this->_fmte[i].tymed & pformatetcIn->tymed) &&
            (this->_fmte[i].dwAspect == pformatetcIn->dwAspect))
        {
            *pmedium = this->_medium[i];

            if (pmedium->hGlobal)
            {
                // Indicate that the caller should not release hmem.
                if (pmedium->tymed == TYMED_HGLOBAL)
                {
                    InterlockedIncrement(&this->_cRef);
                    pmedium->pUnkForRelease = (IUnknown*)&this->dtobj;
                    return S_OK;
                }

                //if the type is stream  then clone the stream.
                
                if (pmedium->tymed == TYMED_ISTREAM)
                {
                    hres = CreateStreamOnHGlobal(NULL, TRUE, &(pmedium->pstm));

                    if (SUCCEEDED(hres))
                    {
                        STATSTG stat;

                        //Get the Current Stream size
                         hres = this->_medium[i].pstm->lpVtbl->Stat(this->_medium[i].pstm, &stat,STATFLAG_NONAME);

                         if (SUCCEEDED(hres))
                         {
                            //Seek the source stream to  the beginning.
                            this->_medium[i].pstm->lpVtbl->Seek(this->_medium[i].pstm, g_li0, STREAM_SEEK_SET, NULL);

                            //Copy the entire source into the destination. Since the destination stream is created using 
                            //CreateStreamOnHGlobal, it seek pointer is at the beginning.
                            hres = this->_medium[i].pstm->lpVtbl->CopyTo(this->_medium[i].pstm, pmedium->pstm, stat.cbSize, NULL,NULL );
                            
                            //Before returning Set the destination seek pointer back at the beginning.
                            pmedium->pstm->lpVtbl->Seek(pmedium->pstm, g_li0, STREAM_SEEK_SET, NULL);

                            // If this medium has a punk for release, make sure to add ref that...
                            pmedium->pUnkForRelease = this->_medium[i].pUnkForRelease;
                            if (pmedium->pUnkForRelease)
                                pmedium->pUnkForRelease->lpVtbl->AddRef(pmedium->pUnkForRelease);


                            //Hoooh its done. 
                            return hres;

                         }
                         else
                         {
                             hres = E_OUTOFMEMORY;
                         }

                    }
                }
                
            }
        }
    }

    if (hres == E_INVALIDARG && this->_pdtInner) 
    {
        hres = this->_pdtInner->lpVtbl->GetData(this->_pdtInner, pformatetcIn, pmedium);
    }

    return hres;
}

STDMETHODIMP CIDLData_GetDataHere(IDataObject * pdtobj, LPFORMATETC pformatetc, LPSTGMEDIUM pmedium )
{
    CIDLData * this = IToClass(CIDLData, dtobj, pdtobj);
    HRESULT hres = E_NOTIMPL;

#ifdef DEBUG
    if (pformatetc->cfFormat<CF_MAX) 
    {
        TraceMsg(TF_IDLIST, "CIDLData_GetDataHere called with %x,%x,%x",
                 pformatetc->cfFormat, pformatetc->tymed, pmedium->tymed);
    }
    else 
    {
        TCHAR szName[256];

        GetClipboardFormatName(pformatetc->cfFormat, szName, ARRAYSIZE(szName));
        TraceMsg(TF_IDLIST, "CIDLData_GetDataHere called with %s,%x,%x",
                 szName, pformatetc->tymed, pmedium->tymed);
    }
#endif

    if (this->_pdtInner)
        hres = this->_pdtInner->lpVtbl->GetDataHere(this->_pdtInner, pformatetc, pmedium);

    return hres;
}

STDMETHODIMP CIDLData_QueryGetData(IDataObject * pdtobj, LPFORMATETC pformatetcIn)
{
    CIDLData * this = IToClass(CIDLData, dtobj, pdtobj);
    HRESULT hres;
    int i;

#ifdef DEBUG
    if (pformatetcIn->cfFormat<CF_MAX) 
    {
        TraceMsg(TF_IDLIST, "CIDLData_QueryGetData called with %x,%x",
                             pformatetcIn->cfFormat, pformatetcIn->tymed);
    }
    else 
    {
        TCHAR szName[256];
        GetClipboardFormatName(pformatetcIn->cfFormat, szName, ARRAYSIZE(szName));
        TraceMsg(TF_IDLIST, "CIDLData_QueryGetData called with %s,%x",
                             szName, pformatetcIn->tymed);
    }
#endif

    for (i = 0; i < MAX_FORMATS; i++)
    {
        if ((this->_fmte[i].cfFormat == pformatetcIn->cfFormat) &&
            (this->_fmte[i].tymed & pformatetcIn->tymed) &&
            (this->_fmte[i].dwAspect == pformatetcIn->dwAspect))
            return S_OK;
    }

    hres = S_FALSE;
    if (this->_pdtInner)
        hres = this->_pdtInner->lpVtbl->QueryGetData(this->_pdtInner, pformatetcIn);
    return hres;
}

STDMETHODIMP CIDLData_GetCanonicalFormatEtc(IDataObject *pdtobj, LPFORMATETC pformatetc, LPFORMATETC pformatetcOut)
{
    // This is the simplest implemtation. It means we always return
    // the data in the format requested.
    return DATA_S_SAMEFORMATETC;
}

STDMETHODIMP CIDLData_SetData(IDataObject *pdtobj, FORMATETC *pformatetc, STGMEDIUM *pmedium, BOOL fRelease)
{
    CIDLData * this = IToClass(CIDLData, dtobj, pdtobj);
    HRESULT hres;

    ASSERT(pformatetc->tymed == pmedium->tymed);

    if (fRelease)
    {
        int i;

        // first add it if that format is already present
        // on a NULL medium (render on demand)
        for (i = 0; i < MAX_FORMATS; i++)
        {
            if ((this->_fmte[i].cfFormat == pformatetc->cfFormat) &&
                (this->_fmte[i].tymed    == pformatetc->tymed) &&
                (this->_fmte[i].dwAspect == pformatetc->dwAspect))
            {
                //
                // We are simply adding a format, ignore.
                //
                if (pmedium->hGlobal == NULL)
                    return S_OK;

                // if we are set twice on the same object
                if (this->_medium[i].hGlobal)
                    ReleaseStgMedium(&this->_medium[i]);

                this->_medium[i] = *pmedium;
                return S_OK;
            }
        }

        //
        //  This is a new clipboard format.  Give the inner a chance first.
        //  This is important for formats like "Performed DropEffect" and
        //  "TargetCLSID", which are used by us to communicate information
        //  into the data object.
        //
        if (this->_pdtInner == NULL ||
            FAILED(hres = this->_pdtInner->lpVtbl->SetData(this->_pdtInner, pformatetc, pmedium, fRelease)))
        {
            // Inner object doesn't want it; let's keep it ourselves
            // now look for a free slot
            for (i = 0; i < MAX_FORMATS; i++)
            {
                if (this->_fmte[i].cfFormat == 0)
                {
                    // found a free slot
                    this->_medium[i] = *pmedium;
                    this->_fmte[i] = *pformatetc;
                    return S_OK;
                }
            }
            // fixed size table
            hres = E_OUTOFMEMORY;
        }
    }
    else
    {
        if (this->_pdtInner)
            hres = this->_pdtInner->lpVtbl->SetData(this->_pdtInner, pformatetc, pmedium, fRelease);
        else
            hres = E_INVALIDARG;
    }

    return hres;
}

STDMETHODIMP CIDLData_EnumFormatEtc(IDataObject *pdtobj, DWORD dwDirection, IEnumFORMATETC **ppenumFormatEtc)
{
    CIDLData * this = IToClass(CIDLData, dtobj, pdtobj);
    UINT cfmt;

    //
    // If this is the first time, build the format list by calling
    // QueryGetData with each clipboard format.
    //
    if (!this->_fEnumFormatCalled)
    {
        UINT ifmt;
        FORMATETC fmte = { 0, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
        STGMEDIUM medium = { TYMED_HGLOBAL, NULL, NULL };
        for (ifmt = 0; ifmt < ICF_MAX; ifmt++)
        {
            fmte.cfFormat = g_acfIDLData[ifmt];
            if (pdtobj->lpVtbl->QueryGetData(pdtobj, &fmte) == S_OK) 
            {
                pdtobj->lpVtbl->SetData(pdtobj, &fmte, &medium, TRUE);
            }
        }
        this->_fEnumFormatCalled = TRUE;
    }

    // Get the number of formatetc
    for (cfmt = 0; cfmt < MAX_FORMATS; cfmt++)
    {
        if (this->_fmte[cfmt].cfFormat == 0)
            break;
    }

    return SHCreateStdEnumFmtEtcEx(cfmt, this->_fmte, this->_pdtInner, ppenumFormatEtc);
}

STDMETHODIMP CIDLData_Advise(IDataObject *pdtobj, FORMATETC * pFormatetc, DWORD advf, LPADVISESINK pAdvSink, DWORD *pdwConnection)
{
    return OLE_E_ADVISENOTSUPPORTED;
}

STDMETHODIMP CIDLData_Unadvise(IDataObject *pdtobj, DWORD dwConnection)
{
    return OLE_E_ADVISENOTSUPPORTED;
}

STDMETHODIMP CIDLData_EnumAdvise(IDataObject *pdtobj, LPENUMSTATDATA *ppenumAdvise)
{
    return OLE_E_ADVISENOTSUPPORTED;
}

const IDataObjectVtbl c_CIDLDataVtbl = {
    CIDLData_QueryInterface, 
    CIDLData_AddRef, 
    CIDLData_Release,
    CIDLData_GetData,
    CIDLData_GetDataHere,
    CIDLData_QueryGetData,
    CIDLData_GetCanonicalFormatEtc,
    CIDLData_SetData,
    CIDLData_EnumFormatEtc,
    CIDLData_Advise,
    CIDLData_Unadvise,
    CIDLData_EnumAdvise
};


// *** IAsyncOperation methods ***

STDMETHODIMP CIDLData_AO_QueryInterface(IAsyncOperation *pao, REFIID riid, void **ppvObj)
{
    CIDLData * this = IToClass(CIDLData, ao, pao);
    return CIDLData_QueryInterface(&this->dtobj, riid, ppvObj);
}

STDMETHODIMP_(ULONG) CIDLData_AO_AddRef(IAsyncOperation *pao)
{
    CIDLData * this = IToClass(CIDLData, ao, pao);
    return CIDLData_AddRef(&this->dtobj);
}

STDMETHODIMP_(ULONG) CIDLData_AO_Release(IAsyncOperation *pao)
{
    CIDLData * this = IToClass(CIDLData, ao, pao);
    return CIDLData_Release(&this->dtobj);
}

HRESULT CIDLData_SetAsyncMode(IAsyncOperation *pao, BOOL fDoOpAsync)
{ 
    return E_NOTIMPL;
}

HRESULT CIDLData_GetAsyncMode(IAsyncOperation *pao, BOOL *pfIsOpAsync)
{
    CIDLData * this = IToClass(CIDLData, ao, pao);
    if (this->_punkThread || IsMainShellProcess())
    {
        *pfIsOpAsync = TRUE;
    }
    else
    {
        *pfIsOpAsync = FALSE;
    }
    return S_OK;
}
  
HRESULT CIDLData_StartOperation(IAsyncOperation *pao, IBindCtx * pbc)
{
    CIDLData * this = IToClass(CIDLData, ao, pao);
    this->_fDidAsynchStart = TRUE;
    return S_OK;
}
  
HRESULT CIDLData_InOperation(IAsyncOperation *pao, BOOL * pfInAsyncOp)
{
    CIDLData * this = IToClass(CIDLData, ao, pao);
    if (this->_fDidAsynchStart)
    {
        *pfInAsyncOp = TRUE;
    }
    else
    {
        *pfInAsyncOp = FALSE;
    }
    return S_OK;
}
  
HRESULT CIDLData_EndOperation(IAsyncOperation *pao, HRESULT hResult, IBindCtx * pbc, DWORD dwEffects)
{
    CIDLData * this = IToClass(CIDLData, ao, pao);
    this->_fDidAsynchStart = FALSE;
    return S_OK;
}
  
const IAsyncOperationVtbl c_CIDLAsyncOperationVtbl = {
    CIDLData_AO_QueryInterface, 
    CIDLData_AO_AddRef, 
    CIDLData_AO_Release,
    CIDLData_SetAsyncMode,
    CIDLData_GetAsyncMode,
    CIDLData_StartOperation,
    CIDLData_InOperation,
    CIDLData_EndOperation,
};


//
// Create an instance of CIDLData with specified Vtable pointer.
//
HRESULT CIDLData_CreateInstance(const IDataObjectVtbl *lpVtbl, IDataObject **ppdtobj, IDataObject *pdtInner)
{
    CIDLData *pidt = (void*)LocalAlloc(LPTR, SIZEOF(CIDLData));
    if (pidt)
    {
        pidt->dtobj.lpVtbl = lpVtbl ? lpVtbl : &c_CIDLDataVtbl;
        pidt->ao.lpVtbl = &c_CIDLAsyncOperationVtbl;
        pidt->_cRef = 1;
        pidt->_pdtInner = pdtInner;
        if (pdtInner)
            pdtInner->lpVtbl->AddRef(pdtInner);

        SHGetThreadRef(&pidt->_punkThread);

        *ppdtobj = &pidt->dtobj;

        return S_OK;
    }
    else
    {
        *ppdtobj = NULL;
        return E_OUTOFMEMORY;
    }
}


//
// Create an instance of CIDLData with specified Vtable pointer.
//
HRESULT CIDLData_CreateFromIDArray3(const IDataObjectVtbl *lpVtbl,
                                    LPCITEMIDLIST pidlFolder,
                                    UINT cidl, LPCITEMIDLIST apidl[],
                                    IDataObject *pdtInner,
                                    IDataObject **ppdtobj)
{
    HRESULT hres = CIDLData_CreateInstance(lpVtbl, ppdtobj, pdtInner);
    if (SUCCEEDED(hres))
    {
        // allow empty array to be passed in
        if (apidl)
        {
            HIDA hida = HIDA_Create(pidlFolder, cidl, apidl);
            if (hida)
            {
#if 0 // not yet
                // QueryGetData/SetData on HDROP before calling DataObj_SetGlobal with
                // HIDA to insure that CF_HDROP will come first in the enumerator
                FORMATETC fmte = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
                STGMEDIUM medium = { TYMED_HGLOBAL, NULL, NULL };
                if ((*ppdtobj)->lpVtbl->QueryGetData(*ppdtobj, &fmte) == S_OK) 
                {
                    (*ppdtobj)->lpVtbl->SetData(*ppdtobj, &fmte, &medium, TRUE);
                }
#endif // 0                
                IDLData_InitializeClipboardFormats(); // init our registerd formats

                hres = DataObj_SetGlobal(*ppdtobj, g_cfHIDA, hida);
                if (FAILED(hres))
                    goto SetFailed;
            }
            else
            {
                hres = E_OUTOFMEMORY;
SetFailed:
                (*ppdtobj)->lpVtbl->Release(*ppdtobj);
                *ppdtobj = NULL;
            }
        }
    }
    return hres;
}

HRESULT CIDLData_CreateFromIDArray2(const IDataObjectVtbl *lpVtbl,
                                    LPCITEMIDLIST pidlFolder,
                                    UINT cidl, LPCITEMIDLIST apidl[],
                                    IDataObject **ppdtobj)
{
    return CIDLData_CreateFromIDArray3(lpVtbl, pidlFolder, cidl, apidl, NULL, ppdtobj);
}
//
// Create an instance of CIDLData with default Vtable pointer.
//
STDAPI CIDLData_CreateFromIDArray(LPCITEMIDLIST pidlFolder, UINT cidl, LPCITEMIDLIST apidl[], IDataObject **ppdtobj)
{
    return CIDLData_CreateFromIDArray3(NULL, pidlFolder, cidl, apidl, NULL, ppdtobj);
}
