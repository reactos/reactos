/*	-	-	-	-	-	-	-	-	*/

class FAR CUnMarshal : IMarshal {
public:
    static HRESULT Create(IUnknown FAR* pUnknownOuter, const IID FAR& riid, void FAR* FAR* ppv);
    
private:
    CUnMarshal(IUnknown FAR* pUnknownOuter, IUnknown FAR* FAR* ppUnknown);
    
public:
    STDMETHODIMP QueryInterface(const IID FAR& riid, void FAR* FAR* ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();
	    
    // *** IMarshal methods ***
    STDMETHODIMP GetUnmarshalClass (THIS_ REFIID riid, LPVOID pv, 
			DWORD dwDestContext, LPVOID pvDestContext,
			DWORD mshlflags, LPCLSID pCid);
    STDMETHODIMP GetMarshalSizeMax (THIS_ REFIID riid, LPVOID pv, 
			DWORD dwDestContext, LPVOID pvDestContext,
			DWORD mshlflags, LPDWORD pSize);
    STDMETHODIMP MarshalInterface (THIS_ LPSTREAM pStm, REFIID riid,
			LPVOID pv, DWORD dwDestContext, LPVOID pvDestContext,
			DWORD mshlflags);
    STDMETHODIMP UnmarshalInterface (THIS_ LPSTREAM pStm, REFIID riid,
			LPVOID FAR* ppv);
    STDMETHODIMP ReleaseMarshalData (THIS_ LPSTREAM pStm);
    STDMETHODIMP DisconnectObject (THIS_ DWORD dwReserved);

    IUnknown FAR*	m_pUnknownOuter;
    ULONG	m_refs;
};

