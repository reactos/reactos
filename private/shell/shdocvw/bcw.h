/****************************************************************************
bcw.h

  Owner: Srinik
  Copyright (c) 1995 Microsoft Corporation
  
    This header file for BCW class which implements wrappers for IBindCtx 
    and IRunningObjectTable. We use this object to trick the moniker binding
    code to create a new instance of the object (that the moniker is 
    referring to) instead connecting to already running instance. 
****************************************************************************/

#ifndef BCW_H
#define BCW_H

/****************************************************************************
BCW_ROT is the IRunningObjectTable imlementation of BCW_ROT.
****************************************************************************/

class BCW_ROT: public IRunningObjectTable
{ 
    friend class BCW;
public:
    BCW_ROT(); 
    ~BCW_ROT();
    
private:
    BOOL_PTR FInitROTPointer(void);
    
private:
    // *** IUnknown methods ***
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
    
    // *** IRunningObjectTable methods ***
    STDMETHODIMP Register(DWORD grfFlags, IUnknown *punkObject,
        IMoniker *pmkObjectName, DWORD *pdwRegister)
    {
        if (m_piROT == NULL)
            return E_FAIL;
        
        return m_piROT->Register(grfFlags, punkObject, pmkObjectName, pdwRegister);
    }
    
    STDMETHODIMP Revoke(DWORD dwRegister)
    {
        if (m_piROT == NULL)
            return E_FAIL;
        
        return m_piROT->Revoke(dwRegister);
    }
    
    STDMETHODIMP IsRunning(IMoniker *pmkObjectName)
    {
        // Trick the moniker binding code into thinking that the object is not 
        // running. This way it will try to create a new instance of the object.
        // REVIEW: we may want to check the pmkObjectName, and if is not the one
        // that we are concerned with,then we may want to delegate the call.
        return S_FALSE;
    }
    
    STDMETHODIMP GetObject(IMoniker *pmkObjectName,IUnknown **ppunkObject)
    {
        // Trick the moniker binding code into thinking that the object is not 
        // running. This way it will try to create a new instance of the object.
        // REVIEW: we may want to check the pmkObjectName, and if is not the one
        // that we are concerned with,then we may want to delegate the call.
        return MK_E_UNAVAILABLE;
    }
    
    STDMETHODIMP NoteChangeTime(DWORD dwRegister, FILETIME *pfiletime)
    {
        if  (m_piROT == NULL)
            return E_FAIL;
        
        return m_piROT->NoteChangeTime(dwRegister, pfiletime);
    }
    
    STDMETHODIMP GetTimeOfLastChange(IMoniker *pmkObjectName,  FILETIME *pfiletime)
    {
        if (m_piROT == NULL)
            return E_FAIL;
        
        return m_piROT->GetTimeOfLastChange(pmkObjectName, pfiletime);
    }
    
    STDMETHODIMP EnumRunning(IEnumMoniker **ppenumMoniker)
    {
        if (m_piROT == NULL)
            return E_FAIL;
        
        return m_piROT->EnumRunning(ppenumMoniker);
    }
    
private:
    /* Return back pointer to containing BCW object. */
    inline BCW* PBCW();
    IRunningObjectTable * m_piROT;
#ifdef DEBUG
    Debug(ULONG m_cRef); 
#endif
};


/****************************************************************************
Declaration of BCW. This class implements IBindCtx and IRunningObjectTable
This is class is used to manipulate the binding process, such that the 
moniker binding code will create a new instance of the object instead of 
binding to the existing instance
****************************************************************************/

class BCW: public IBindCtx
{ 
    friend class BCW_ROT;
    
public:
    BCW(IBindCtx * pibc); 
    ~BCW();
    
    static IBindCtx * Create(IBindCtx * pibc);
    
private:
    // *** IUnknown methods ***
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
    
    // *** IBindCtx methods ***
    STDMETHODIMP RegisterObjectBound(IUnknown *punk)
    {   return m_pibc->RegisterObjectBound(punk); }
    
    STDMETHODIMP RevokeObjectBound(IUnknown *punk)
    {   return m_pibc->RevokeObjectBound(punk); }
    
    STDMETHODIMP ReleaseBoundObjects(void)
    {   return m_pibc->ReleaseBoundObjects(); }
    
    STDMETHODIMP SetBindOptions(BIND_OPTS *pbindopts)
    {   return m_pibc->SetBindOptions(pbindopts); }
    
    STDMETHODIMP GetBindOptions(BIND_OPTS *pbindopts)
    {   return m_pibc->GetBindOptions(pbindopts); }
    
    STDMETHODIMP GetRunningObjectTable(IRunningObjectTable **pprot)
    {   
        if (pprot == NULL)
            return E_INVALIDARG;
        
        *pprot = (IRunningObjectTable *) &m_ROT;
        ((IUnknown *) *pprot)->AddRef();
        return NOERROR;
    }
    
    STDMETHODIMP RegisterObjectParam(LPOLESTR pszKey, IUnknown *punk)
    {   return m_pibc->RegisterObjectParam(pszKey, punk); }
    
    STDMETHODIMP GetObjectParam(LPOLESTR pszKey, IUnknown **ppunk)
    {   return m_pibc->GetObjectParam(pszKey, ppunk); }
    
    STDMETHODIMP EnumObjectParam(IEnumString **ppenum)
    {   return m_pibc->EnumObjectParam(ppenum); }
    
    STDMETHODIMP RevokeObjectParam(LPOLESTR pszKey)
    {   return m_pibc->RevokeObjectParam(pszKey); }
    
private:
    BCW_ROT     m_ROT;      // IRunningObjectTable implementation
    DWORD       m_cObjRef;
    IBindCtx *  m_pibc;
};

#endif  // BCW_H 
