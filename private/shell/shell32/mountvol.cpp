//   Implements Mounted Volume

#include "shellprv.h"
#include "clsobj.h"

#ifdef WINNT            

UINT g_cfMountedVolume = 0;

class CMountedVolume : public IMountedVolume, IDataObject
{
public:
    //IUnknown methods
    STDMETHODIMP QueryInterface(REFIID,void **);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
    
    //IDataObject methods
    STDMETHODIMP GetData(LPFORMATETC pformatetcIn, LPSTGMEDIUM pmedium);
    STDMETHODIMP GetDataHere(LPFORMATETC pformatetc, LPSTGMEDIUM pmedium);
    STDMETHODIMP QueryGetData(LPFORMATETC pformatetcIn);
    STDMETHODIMP GetCanonicalFormatEtc(LPFORMATETC pformatetc, LPFORMATETC pformatetcOut);
    STDMETHODIMP SetData(FORMATETC *pformatetc, STGMEDIUM *pmedium, BOOL fRelease);
    STDMETHODIMP EnumFormatEtc(DWORD dwDirection, LPENUMFORMATETC *ppenumFormatEtc);
    STDMETHODIMP DAdvise(FORMATETC *pFormatetc, DWORD advf, LPADVISESINK pAdvSink, DWORD *pdwConnection);
    STDMETHODIMP DUnadvise(DWORD dwConnection);
    STDMETHODIMP EnumDAdvise(LPENUMSTATDATA *ppenumAdvise);

    // IMountedVolume methods
    STDMETHODIMP Initialize(LPCWSTR pcszMountPoint);
protected:
    CMountedVolume();
    ~CMountedVolume();

    friend HRESULT CMountedVolume_CreateInstance(IUnknown* pUnkOuter, REFIID riid, void **ppv);

private:
    LONG _cRef;
    TCHAR _szMountPoint[MAX_PATH];
};

//constructor/destructor and related functions

CMountedVolume::CMountedVolume() :
    _cRef(1)
{
    _szMountPoint[0]=(TEXT('\0'));
    DllAddRef();
}

CMountedVolume::~CMountedVolume()
{
    DllRelease();
}

STDAPI CMountedVolume_CreateInstance(IUnknown* pUnkOuter, REFIID riid, void **ppv)
{
    HRESULT hres = E_OUTOFMEMORY;
    *ppv = NULL;
    
    if (!g_cfMountedVolume)
        g_cfMountedVolume = RegisterClipboardFormat(CFSTR_MOUNTEDVOLUME);

    // aggregation checking is handled in class factory
    CMountedVolume* pMountedVolume=new CMountedVolume();
    if (pMountedVolume)
    {
        hres = pMountedVolume->QueryInterface(riid, ppv);
        pMountedVolume->Release();
    }

    return hres;
}

//IUnknown handling

STDMETHODIMP CMountedVolume::QueryInterface(REFIID riid,void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CMountedVolume, IDataObject),
        QITABENT(CMountedVolume, IMountedVolume),
        { 0 },
    };
    return QISearch(this, qit, riid, ppvObj);
}

STDMETHODIMP_(ULONG) CMountedVolume::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CMountedVolume::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

//IDataObject handling
STDMETHODIMP CMountedVolume::GetData(LPFORMATETC pformatetcIn, LPSTGMEDIUM pmedium)
{
    HRESULT hres = E_FAIL;

    //make sure IMountedVolume::Initialize was called
    if (TEXT('\0') != _szMountPoint[0])
    {
        pmedium->hGlobal = NULL;
        pmedium->pUnkForRelease = NULL;
        pmedium->tymed = TYMED_HGLOBAL;

        if ((g_cfMountedVolume == pformatetcIn->cfFormat) && (TYMED_HGLOBAL & pformatetcIn->tymed))
        {
            pmedium->hGlobal = GlobalAlloc(GPTR, (MAX_PATH + 1) * SIZEOF(TCHAR) + SIZEOF(DROPFILES));

            if (pmedium->hGlobal)
            {
                LPDROPFILES pdf = (LPDROPFILES)pmedium->hGlobal;
                LPTSTR pszMountPoint = (LPTSTR)(pdf + 1);
                pdf->pFiles = SIZEOF(DROPFILES);
                ASSERT(pdf->pt.x==0);
                ASSERT(pdf->pt.y==0);
                ASSERT(pdf->fNC==FALSE);
                ASSERT(pdf->fWide==FALSE);
        #ifdef UNICODE
                pdf->fWide = TRUE;
        #endif
                //do the copy
                lstrcpy(pszMountPoint, _szMountPoint);
                hres = NOERROR;     // success
            }
            else
                hres = E_OUTOFMEMORY;
        }
    }
    else
        ASSERTMSG(0, "IMountedVolume::Initialize was NOT called prior to IMountedVolume::GetData");

    return hres;
}

STDMETHODIMP CMountedVolume::QueryGetData(LPFORMATETC pformatetcIn)
{
    HRESULT hres = S_FALSE;

    if ((g_cfMountedVolume == pformatetcIn->cfFormat) && (TYMED_HGLOBAL & pformatetcIn->tymed))
        hres = NOERROR;

    return hres;
}

STDMETHODIMP CMountedVolume::SetData(FORMATETC *pformatetc, STGMEDIUM *pmedium, BOOL fRelease)
{
    return E_NOTIMPL;
}

STDMETHODIMP CMountedVolume::GetDataHere(LPFORMATETC pformatetc, LPSTGMEDIUM pmedium )
{
    return E_NOTIMPL;
}

STDMETHODIMP CMountedVolume::GetCanonicalFormatEtc(LPFORMATETC pformatetc, LPFORMATETC pformatetcOut)
{
    return E_NOTIMPL;
}

STDMETHODIMP CMountedVolume::EnumFormatEtc(DWORD dwDirection, LPENUMFORMATETC *ppenumFormatEtc)
{
    return S_FALSE;
}

STDMETHODIMP CMountedVolume::DAdvise(FORMATETC *pFormatetc, DWORD advf, LPADVISESINK pAdvSink, DWORD *pdwConnection)
{
    return OLE_E_ADVISENOTSUPPORTED;
}

STDMETHODIMP CMountedVolume::DUnadvise(DWORD dwConnection)
{
    return OLE_E_ADVISENOTSUPPORTED;
}

STDMETHODIMP CMountedVolume::EnumDAdvise(LPENUMSTATDATA *ppenumAdvise)
{
    return OLE_E_ADVISENOTSUPPORTED;
}

// IMountedVolume methods
STDMETHODIMP CMountedVolume::Initialize(LPCWSTR pcszMountPoint)
{
    lstrcpy(_szMountPoint, pcszMountPoint);
    PathAddBackslash(_szMountPoint);

    return NOERROR;
}

#endif //WINNT