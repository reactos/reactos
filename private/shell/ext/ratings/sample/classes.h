//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1996                    **
//*********************************************************************

class CSampleObtainRating : public IObtainRating
{
private:
	UINT m_cRef;

public:
	CSampleObtainRating();
	~CSampleObtainRating();
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, void **ppvObj);
    STDMETHOD_(ULONG,AddRef) (THIS);
    STDMETHOD_(ULONG,Release) (THIS);

	STDMETHOD(ObtainRating) (THIS_ LPCTSTR pszTargetUrl, HANDLE hAbortEvent,
							 IMalloc *pAllocator, LPSTR *ppRatingOut);

	STDMETHOD_(ULONG,GetSortOrder) (THIS);
};


class CSampleClassFactory : public IClassFactory
{
public:
	STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
	STDMETHODIMP_(ULONG) AddRef(void);
	STDMETHODIMP_(ULONG) Release(void);
	STDMETHODIMP CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObject);
	STDMETHODIMP LockServer(BOOL fLock);
};
