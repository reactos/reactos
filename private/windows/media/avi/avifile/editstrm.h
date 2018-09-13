DEFINE_AVIGUID(CLSID_EditStream,        0x0002000A, 0, 0);

struct FAR ICEditStreamInternal : public IUnknown
{
    STDMETHOD(GetInternalPointer)(LPVOID FAR * ppInternal) = 0;
};

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

typedef struct {
    PAVISTREAM	    pavi;
    LONG	    lStart;
    LONG	    lLength;
    LONG	    unused;	// pad to power of two in size....
    RECT	    rcSource;
    RECT	    rcDest;
} EDIT, FAR * LPEDIT;

class FAR CEditStream : public virtual IAVIStream,
			public virtual IAVIEditStream,
			public virtual ICEditStreamInternal
		       
#ifdef CUSTOMMARSHAL
			, public virtual IMarshal
#endif
{
public:
    static CEditStream FAR * NewEditStream(PAVISTREAM psSource);
    
    STDMETHODIMP QueryInterface(const IID FAR& riid, void FAR* FAR* ppv);	\
    STDMETHODIMP_(ULONG) AddRef();	\
    STDMETHODIMP_(ULONG) Release();
    STDMETHODIMP Create      (THIS_ LPARAM lParam1, LPARAM lParam2);
    STDMETHODIMP Info        (THIS_ AVISTREAMINFOW FAR * psi, LONG lSize);
    STDMETHODIMP_(LONG)  FindSample (THIS_ LONG lPos, LONG lFlags);
    STDMETHODIMP ReadFormat  (THIS_ LONG lPos,
			    LPVOID lpFormat, LONG FAR *cbFormat);
    STDMETHODIMP SetFormat   (THIS_ LONG lPos,
			    LPVOID lpFormat, LONG cbFormat);
    STDMETHODIMP Read        (THIS_ LONG lStart, LONG lSamples,
			    LPVOID lpBuffer, LONG cbBuffer,
			    LONG FAR * plBytes, LONG FAR * plSamples);
    STDMETHODIMP Write       (THIS_ LONG lStart, LONG lSamples,
			      LPVOID lpBuffer, LONG cbBuffer,
			      DWORD dwFlags,
			      LONG FAR *plSampWritten,
			      LONG FAR *plBytesWritten);
    STDMETHODIMP Delete      (THIS_ LONG lStart, LONG lSamples);
    STDMETHODIMP ReadData    (THIS_ DWORD fcc, LPVOID lp, LONG FAR *lpcb);
    STDMETHODIMP WriteData   (THIS_ DWORD fcc, LPVOID lp, LONG cb);
    STDMETHODIMP Reserved1            (THIS);
    STDMETHODIMP Reserved2            (THIS);
    STDMETHODIMP Reserved3            (THIS);
    STDMETHODIMP Reserved4            (THIS);
    STDMETHODIMP Reserved5            (THIS);

    STDMETHODIMP Cut(LONG FAR *plStart, LONG FAR *plLength, PAVISTREAM FAR * ppResult);
    STDMETHODIMP Copy(LONG FAR *plStart, LONG FAR *plLength, PAVISTREAM FAR * ppResult);
    STDMETHODIMP Paste(LONG FAR *plPos, LONG FAR *plLength, PAVISTREAM pstream, LONG lStart, LONG lLength);
    STDMETHODIMP Clone(PAVISTREAM FAR *ppResult);
    STDMETHODIMP SetInfo(AVISTREAMINFOW FAR *lpInfo, LONG cbInfo);
    
    static HRESULT NewInstance(IUnknown FAR* pUnknownOuter,
			       REFIID riid,
			       LPVOID FAR* ppv);

    STDMETHODIMP GetInternalPointer(LPVOID FAR * ppv);
#ifdef CUSTOMMARSHAL
    // *** IMarshal methods ***
    BOOL CanMarshalSimply();

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
#endif	// CUSTOMMARSHAL
    
private:
    CEditStream() {
	cedits = 0;
	maxedits = 0;
	edits = NULL;
	pgf = NULL;
	psgf = NULL;
	fFullFrames = FALSE;
    };
    
    HRESULT ResolveEdits(LONG lPos,
		  PAVISTREAM FAR *ppavi, LONG FAR *plPos,
		  LONG FAR *pl, BOOL fAllowEnd);
    HRESULT PossiblyRemoveEdit(LONG l);
    HRESULT AllocEditSpace(LONG l, LONG cNew);
    LPBITMAPINFOHEADER NEAR PASCAL CEditStream::CallGetFrame(
						      PAVISTREAM p,
						      LONG l);
    void CheckEditList();
    
public:
    ULONG			ulRefCount;
    //
    // instance data
    //
    AVISTREAMINFOW		sinfo;    
    LONG    			cedits;
    LONG    			maxedits;
    BOOL			fFullFrames;
    EDIT _huge *		edits;

    //
    // cached PGETFRAME
    PGETFRAME			pgf;
    PAVISTREAM			psgf;
    LPBITMAPINFOHEADER		lpbiLast;
};
