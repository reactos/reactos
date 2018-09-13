#include "extra.h"
#include "fileshar.h"
#include "aviidx.h"
#include "buffer.h"
/*	-	-	-	-	-	-	-	-	*/

/*
**	Copyright (C) Microsoft Corporation 1993. All rights reserved.
*/

/*	-	-	-	-	-	-	-	-	*/

#define	CFactoryImpl	CI	// Can't handle long exported names
#define	CAVIFileImpl	CF	// Can't handle long exported names
#define	CAVIStreamImpl	CS	// Can't handle long exported names

/* Remove warning of using object during initialization. */
#pragma warning(disable:4355)

/*	-	-	-	-	-	-	-	-	*/

#define	implement	struct
#define	implementations	private

/*
** _StdClassImplementations
** Defines the standard implementations for a class object.
*/

#define	_StdClassImplementations(Impl)	\
	STDMETHODIMP QueryInterface(REFIID riid, LPVOID FAR* ppv);	\
	STDMETHODIMP_(ULONG) AddRef();	\
	STDMETHODIMP_(ULONG) Release()


/*	-	-	-	-	-	-	-	-	*/

class FAR CAVIFileCF {
public:
    static HRESULT Create(const CLSID FAR& rclsid, REFIID riid, LPVOID FAR* ppv);
private:
    CAVIFileCF(const CLSID FAR& rclsid, IUnknown FAR* FAR* ppUnknown);
implementations:
    implement CUnknownImpl : IUnknown {
    public:
	_StdClassImplementations(CUnknownImpl);
	CUnknownImpl(CAVIFileCF FAR* pAVIFileCF);
    private:
	CAVIFileCF FAR*	m_pAVIFileCF;
	ULONG	m_refs;
    };
    implement CFactoryImpl : IClassFactory {
    public:
	_StdClassImplementations(CFactoryImpl);
	CFactoryImpl(CAVIFileCF FAR* pAVIFileCF);
	STDMETHODIMP CreateInstance(IUnknown FAR* pUnknownOuter, REFIID riid, LPVOID FAR* ppv);
	STDMETHODIMP LockServer(BOOL fLock);
    private:
	CAVIFileCF FAR*	m_pAVIFileCF;
    };
public:
    CUnknownImpl	m_Unknown;
    CFactoryImpl	m_Factory;
public:
    CLSID	m_clsid;
};

/*	-	-	-	-	-	-	-	-	*/

class FAR CAVIFile;

class FAR CAVIStream {
public:
    CAVIStream(IUnknown FAR* pUnknownOuter, IUnknown FAR* FAR* ppUnknown);
private:
implementations:
    implement CUnknownImpl : IUnknown {
    public:
	_StdClassImplementations(CUnknownImpl);
	CUnknownImpl(CAVIStream FAR* pAVIStream);
    private:
	CAVIStream FAR*	m_pAVIStream;
	ULONG	m_refs;
    };
    implement CAVIStreamImpl : IAVIStream {
    public:
	_StdClassImplementations(CAVIStreamImpl);
	CAVIStreamImpl(CAVIStream FAR* pAVIStream);
	~CAVIStreamImpl();
	STDMETHODIMP Create      (THIS_ LPARAM lParam1, LPARAM lParam2);
	STDMETHODIMP Info        (THIS_ AVISTREAMINFO FAR * psi, LONG lSize);
        STDMETHODIMP_(LONG) FindSample(THIS_ LONG lPos, LONG lFlags);
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
    private:
	void ReadPalette(LONG lPos, LONG lPal, LPRGBQUAD prgb);
	// private functions here?
	CAVIStream FAR*	m_pAVIStream;
    };
    implement CStreamingImpl : IAVIStreaming {
    public:
	_StdClassImplementations(CStreamingImpl);
	CStreamingImpl(CAVIStream FAR* pAVIStream);
	~CStreamingImpl();
	STDMETHODIMP Begin (THIS_
			  LONG  lStart,	
			  LONG  lEnd,	
			  LONG  lRate);
	STDMETHODIMP End   (THIS);
    private:
	// private functions here?
	CAVIStream FAR*	m_pAVIStream;
    };
    struct CMarshalImpl : IMarshal {
    public:
	CMarshalImpl(CAVIStream FAR* pAVIStream);
	STDMETHODIMP QueryInterface(REFIID riid, LPVOID FAR* ppv);
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
	CAVIStream FAR*	m_pAVIStream;
    };
public:
    CUnknownImpl	m_Unknown;
    CAVIStreamImpl	m_AVIStream;
    CMarshalImpl	m_Marshal;
    CStreamingImpl	m_Streaming;
    
public:
    IUnknown FAR*	m_pUnknownOuter;

    // AVIStream Instance data
    AVISTREAMINFO             avistream;      // stream info
    CAVIFile FAR *		pfile;
    int				iStream;

    PAVISTREAM                  paviBase;

    //
    //  stream instance data
    //
    HSHFILE                     hshfile;        // file I/O

    LONG                        lPal;           // last palette change
    RGBQUAD                     argbq[256];     // current palette

    LPVOID                      lpFormat;       // stream format
    LONG                        cbFormat;

    LPVOID                      lpData;         // stream handler data
    LONG                        cbData;

    EXTRA			extra;
    
    PBUFSYSTEM                  pb;

    BOOL                        fInit;

    PSTREAMINDEX                psx;
};

/*	-	-	-	-	-	-	-	-	*/
#define MAXSTREAMS		64


class FAR CAVIFile {
public:
    static HRESULT Create(IUnknown FAR* pUnknownOuter, REFIID riid, LPVOID FAR* ppv);
private:
    CAVIFile(IUnknown FAR* pUnknownOuter, IUnknown FAR* FAR* ppUnknown);
implementations:
    implement CUnknownImpl : IUnknown {
    public:
	_StdClassImplementations(CUnknownImpl);
	CUnknownImpl(CAVIFile FAR* pAVIFile);
    private:
	CAVIFile FAR*	m_pAVIFile;
	ULONG	m_refs;
    };
    implement CAVIFileImpl : IAVIFile {
    public:
	_StdClassImplementations(CAVIFileImpl);
	CAVIFileImpl(CAVIFile FAR* pAVIFile);
	~CAVIFileImpl();
	STDMETHODIMP Open		    (THIS_
					 LPCSTR szFile,
					 UINT mode);
	STDMETHODIMP Info                 (THIS_
					 AVIFILEINFO FAR * pfi,
					 LONG lSize);
	STDMETHODIMP GetStream            (THIS_
					 PAVISTREAM FAR * ppStream,
					 DWORD fccType,
					 LONG lParam);
	STDMETHODIMP CreateStream         (THIS_
					 PAVISTREAM FAR * ppStream,
					 AVISTREAMINFO FAR * psi);
	STDMETHODIMP Save                 (THIS_
					 LPCSTR szFile,
					 AVICOMPRESSOPTIONS FAR *lpOptions,
					 AVISAVECALLBACK lpfnCallback);
	STDMETHODIMP WriteData            (THIS_
					 DWORD ckid,
					 LPVOID lpData,
					 LONG cbData);
	STDMETHODIMP ReadData             (THIS_
					 DWORD ckid,
					 LPVOID lpData,
					 LONG FAR *lpcbData);
	STDMETHODIMP EndRecord            (THIS);
	STDMETHODIMP Reserved1            (THIS);
	STDMETHODIMP Reserved2            (THIS);
	STDMETHODIMP Reserved3            (THIS);
	STDMETHODIMP Reserved4            (THIS);
	STDMETHODIMP Reserved5            (THIS);
    private:
	// private functions here?
	CAVIFile FAR*	m_pAVIFile;
    };
    struct CMarshalImpl : IMarshal {
    public:
	CMarshalImpl(CAVIFile FAR* pAVIFile);
	STDMETHODIMP QueryInterface(REFIID riid, LPVOID FAR* ppv);
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
	CAVIFile FAR*	m_pAVIFile;
    };
public:
    CUnknownImpl	m_Unknown;
    CAVIFileImpl	m_AVIFile;
    CMarshalImpl	m_Marshal;
public:
    IUnknown FAR*	m_pUnknownOuter;
    
    //
    //  AVIFile instance data
    //
    MainAVIHeader FARSTRUCT     avihdr;         // file info
    LONG			lHeaderSize;
    char			achFile[260];
    UINT			mode;
    HSHFILE                     hshfile;          // file I/O
    LONG			lDataListStart;
    BOOL			fInRecord;
    LONG			lRecordIndex;
    MMCKINFO			ckRecord;
    LONG			lWriteLoc;
    EXTRA			extra;
    BOOL			fDirty;
    CAVIStream FAR *            ps[MAXSTREAMS];

    PAVIINDEX                   px;         // the index
    PBUFSYSTEM                  pb;
};


/*
** The usage counter keeps track of the overall usage of objects based on
** implementations provided by the component. This allows one to determine
** when the implementation is no longer in use.
*/

extern UINT	uUseCount;
extern BOOL	fLocked;

/*	-	-	-	-	-	-	-	-	*/


DEFINE_AVIGUID(CLSID_AVIFile,           0x00020000, 0, 0);
