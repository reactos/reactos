/*++

  shmenu.cpp

  this is for IShellMenu and related stuff.  eventually all
  of the fsmenu.c functionality should be in here

--*/




class CFMDropTarget : public IDropTarget 
{
public:

    CFMDropTarget();
    ~CFMDropTarget();

    HRESULT Init (
        HWND hwnd, 
        IShellFolder *psf, 
        LPITEMIDLIST pidl,
        DWORD dwFlags);

    // IUnknown methods

    virtual STDMETHODIMP  QueryInterface(REFIID riid, PVOID *ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);

    STDMETHODIMP DragEnter(
        IDataObject *pdata,
        DWORD grfKeyState,
        POINTL pt,
        DWORD *pdwEffect)
    {return _pdrop->DragEnter(pdata, grfKeyState, pt, pdwEffect);}
    
    STDMETHODIMP DragOver( 
        DWORD grfKeyState,
        POINTL pt,
        DWORD *pdwEffect)
    {return _pdrop->DragOver(grfKeyState, pt, pdwEffect);}
    
    STDMETHODIMP DragLeave( void)
    {return _pdrop->DragLeave();}
    
    STDMETHODIMP  Drop( 
        IDataObject *pDataObj,
        DWORD grfKeyState,
        POINTL pt,
        DWORD *pdwEffect)
    {return _pdrop->Drop(pDataObj, grfKeyState, pt, pdwEffect);}

private:

    ULONG _cRef;
    IShellFolder *_psf;     //  the psf to use...
    LPITEMIDLIST _pidl;
    DWORD _dwFlags;
    IDropTarget *_pdrop;      //  the actual droptarget


}

CFMDropTarget :: CFMDropTarget ()
{
    _cRef = 1;
    DllAddRef();
}

CFMDropTarget :: ~CFMDropTarget ()
{
    SAFERELEASE(_psf);
    if(pidl)
        ILFree(pidl);
    SAFERELEASE(_pdrop);
    DllRelease();
}

HRESULT
CFMDropTarget :: QueryInterface(REFIID riid, PVOID *ppvObj)
{
    HRESULT hr = E_NOINTERFACE;


    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IDropTarget))
    {
        AddRef();
        *ppvObj = (LPVOID) SAFECAST(this, IDropTarget*);
        hr = S_OK;

    }

    return hr;
}


ULONG
CFMDropTarget :: AddRef(void)
{

    _cRef++;

    return _cRef;

}

ULONG
CFMDropTarget :: Release(void)
{
    ASSERT (_cRef > 0);

    cRef--;

    if (!cRef)
    {
        //time to go bye bye
        delete this;
        return 0;
    }

    return cRef;

}

// BUGBUGZEKEL we are just using the psf here...we need to support more
HRESULT Init (
        HWND hwnd, 
        IShellFolder *psf, 
        LPITEMIDLIST pidl,
        DWORD dwFlags)
{
    HRESULT hr = E_INVALIDARG;

    if(psf)
        hr = psf->QueryInterface(IID_IShellFolder, (LPVOID *) &_psf);

    _pidl = ILClone(pidl);
    _dwFlags = dwFlags;

    if(SUCCEEDED(hr) && _psf && _pidl)
    {
        hr = _psf->CreateViewObject(hwnd, IID_IDropTarget, (LPVOID*) &_pdrop);
    }

    return hr;
}

//BUGBUGZEKEL right now this doesnt support ordering, and assumes that you 
//want to drop right onto the current menu.  this is just a start.
//pidl and dwFlags are just dummy params
HRESULT
CFMDropTarget_CreateAndInit(
                            HWND hwnd, 
                            IShellFolder *psf, 
                            LPITEMIDLIST pidl,
                            DWORD dwFlags,
                            LPVOID *ppvObj)
{
    HRESULT hr = E_OUTOFMEMORY;
    CFMDropTarget *pdt;

    ASSERT(ppvObj)
    if(ppvObj)
        *ppvObj = NULL;
    else
        return E_INVALIDARG;


    pdt = new CFMDropTargetNULL;

    if (pdt)
    {
        hr = pdt->Init(hwnd, psf, pidl, dwFlags);

        if (SUCCEEDED(hr))
            *ppvObj= SAFECAST(pdt, IDropTarget * );
        else
            pdt->Release();
    }
    
    return hr;
}

    if (psf)
    {


        hr = psf->QueryInterface(IID_IShellFolder, (LPVOID *) &psfMine);

        if(SUCCEEDED(hr) && psfMine)
        {


    }


#if 0  // BUGBUGZEKEL
    {
        if(pmgoi->dwFlags & (MNGO_TOPGAP | MNGO_BOTTOMGAP))
        {
            //then we need to use the current psf as the droptarget
            // and the pidl is just a marker
        }
        else
        {
            //  we need to use the pidl's psf as the droptarget if possible
                DWORD dwAttr = SFGAO_DROPTARGET;
                hr = psf->lpVtbl->GetAttributesOf(1, (LPCITEMIDLIST*)&pfmi->pidl, &dwAttr);
                if (SUCCEEDED(hres) && (dwAttr & SFGAO_DROPTARGET))
                {
                    hr = psf->lpVtbl->GetUIObjectOf(hwnd, 1, (LPCITEMIDLIST*)&pfmi->pidl,
                                              IID_IDropTarget, NULL, (LPVOID*)&_pdropgtCur);
                }
        }
#endif