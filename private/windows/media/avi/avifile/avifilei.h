#include "extra.h"
#include "fileshar.h"
#include "aviidx.h"
#include "buffer.h"
#include <ole2.h>
/*	-	-	-	-	-	-	-	-	*/

/*
**	Copyright (C) Microsoft Corporation 1993-1995. All rights reserved.
*/

/*	-	-	-	-	-	-	-	-	*/


#define	CFactoryImpl	CI	// Can't handle long exported names
#define	CAVIFileImpl	CF	// Can't handle long exported names
#define	CAVIStreamImpl	CS	// Can't handle long exported names

/* Remove warning of using object during initialization. */
#pragma warning(disable:4355)

#ifndef OLESTR	    // work with old OLE headers
typedef char      OLECHAR;
typedef LPSTR     LPOLESTR;
typedef LPCSTR    LPCOLESTR;
#define OLESTR(str) str
#endif


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
    ~CAVIStream();
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
	STDMETHODIMP Info        (THIS_ AVISTREAMINFOW FAR * psi, LONG lSize);
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
#ifdef _WIN32
	STDMETHODIMP SetInfo(AVISTREAMINFOW FAR *lpInfo, LONG cbInfo);
#else
	STDMETHODIMP Reserved1            (THIS);
	STDMETHODIMP Reserved2            (THIS);
	STDMETHODIMP Reserved3            (THIS);
	STDMETHODIMP Reserved4            (THIS);
	STDMETHODIMP Reserved5            (THIS);
#endif
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

#ifdef CUSTOMMARSHAL
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
#endif	// CUSTOMMARSHAL

public:
    CUnknownImpl	m_Unknown;
    CAVIStreamImpl	m_AVIStream;
#ifdef CUSTOMMARSHAL
    CMarshalImpl	m_Marshal;
#endif
    CStreamingImpl	m_Streaming;

public:
    IUnknown FAR*	m_pUnknownOuter;

    // AVIStream Instance data
    AVISTREAMINFOW             avistream;      // stream info
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

    STDMETHODIMP OpenInternal(DWORD mode);

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
#ifndef _WIN32
	STDMETHODIMP Open		    (THIS_
					 LPCTSTR szFile,
					 UINT mode);
#endif
	STDMETHODIMP Info                 (THIS_
					 AVIFILEINFOW FAR * pfi,
					 LONG lSize);
	STDMETHODIMP GetStream            (THIS_
					 PAVISTREAM FAR * ppStream,
					 DWORD fccType,
					 LONG lParam);
	STDMETHODIMP CreateStream         (THIS_
					 PAVISTREAM FAR * ppStream,
					 AVISTREAMINFOW FAR * psi);
#ifndef _WIN32
	STDMETHODIMP Save                 (THIS_
					 LPCTSTR szFile,
					 AVICOMPRESSOPTIONS FAR *lpOptions,
					 AVISAVECALLBACK lpfnCallback);
#endif
	STDMETHODIMP WriteData            (THIS_
					 DWORD ckid,
					 LPVOID lpData,
					 LONG cbData);
	STDMETHODIMP ReadData             (THIS_
					 DWORD ckid,
					 LPVOID lpData,
					 LONG FAR *lpcbData);
	STDMETHODIMP EndRecord            (THIS);
#ifdef _WIN32
	STDMETHODIMP DeleteStream            (THIS_
					 DWORD fccType,
					 LONG lParam);

#else
	STDMETHODIMP Reserved1            (THIS);
	STDMETHODIMP Reserved2            (THIS);
	STDMETHODIMP Reserved3            (THIS);
	STDMETHODIMP Reserved4            (THIS);
	STDMETHODIMP Reserved5            (THIS);
#endif
    private:
	// private functions here?
	CAVIFile FAR*	m_pAVIFile;
    };

#ifdef CUSTOMMARSHAL
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
#endif	// CUSTOMMARSHAL

    struct CPersistStorageImpl : IPersistStorage {
    public:
	CPersistStorageImpl(CAVIFile FAR* pAVIFile);
	STDMETHODIMP QueryInterface(REFIID riid, LPVOID FAR* ppv);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();

	// *** IPersist methods ***
	STDMETHODIMP GetClassID (LPCLSID lpClassID);

	// *** IPersistStorage methods ***
	STDMETHODIMP IsDirty ();
	STDMETHODIMP InitNew (LPSTORAGE pStg);
	STDMETHODIMP Load (LPSTORAGE pStg);
	STDMETHODIMP Save (LPSTORAGE pStgSave, BOOL fSameAsLoad);
	STDMETHODIMP SaveCompleted (LPSTORAGE pStgNew);
	STDMETHODIMP HandsOffStorage ();
	CAVIFile FAR*	m_pAVIFile;
    };

    struct CPersistFileImpl : IPersistFile {
    public:
	CPersistFileImpl(CAVIFile FAR* pAVIFile);
	STDMETHODIMP QueryInterface(REFIID riid, LPVOID FAR* ppv);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();

	// *** IPersist methods ***
	STDMETHODIMP GetClassID (LPCLSID lpClassID);

	// *** IPersistFile methods ***
	STDMETHODIMP IsDirty ();
	STDMETHODIMP Load (LPCOLESTR lpszFileName, DWORD grfMode);
	STDMETHODIMP Save (LPCOLESTR lpszFileName, BOOL fRemember);
	STDMETHODIMP SaveCompleted (LPCOLESTR lpszFileName);
	STDMETHODIMP GetCurFile (LPOLESTR FAR * lplpszFileName);
	
	CAVIFile FAR*	m_pAVIFile;
    };

public:
    CUnknownImpl	m_Unknown;
    CAVIFileImpl	m_AVIFile;
#ifdef CUSTOMMARSHAL
    CMarshalImpl	m_Marshal;
#endif
    CPersistStorageImpl	m_PersistS;
    CPersistFileImpl	m_PersistF;

public:
    IUnknown FAR*	m_pUnknownOuter;

    //
    //  AVIFile instance data
    //
    MainAVIHeader FARSTRUCT     avihdr;         // file info
    LONG			lHeaderSize;
    TCHAR			achFile[260];
    DWORD			mode;
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

#ifdef _WIN32
    CRITICAL_SECTION		m_critsec;
#endif
};

// this class can be used to provide thread locking by declaring
// an automatic instance on the stack. The non-win32 class does nothing
class FAR CLock {

#ifdef _WIN32

private:
    LPCRITICAL_SECTION pcritsec;

public:
    CLock(CAVIFile FAR* pfile)
    {
	pcritsec = &pfile->m_critsec;
	EnterCriticalSection(pcritsec);
    };

    ~CLock()
    {
	if (pcritsec != NULL) {	    // Because we may explicitly leave before
				    // the automatic instance is destroyed
	    LeaveCriticalSection(pcritsec);
	}
    };

    // Normally we enter/leave the critical section automatically by
    // creating an automatic instance of the Class, and letting C++ call
    // the destructor when the instance goes out of scope.  Exit is
    // provided to allow the user to explicitly release the critsec.
    void Exit()
    {
	LPCRITICAL_SECTION ptmp = pcritsec;
	pcritsec = NULL;
	if (ptmp != NULL) {
	    LeaveCriticalSection(ptmp);
	}
    };
#else
public:
    CLock(CAVIFile FAR* pfile)
    {
    };
    ~CLock()
    {
    };
    void Exit()
    {
    };
#endif
};

#ifdef _WIN32
// for C files
#define EnterCrit(pfile)	(EnterCriticalSection(&pfile->m_critsec))
#define LeaveCrit(p)		(LeaveCriticalSection(&pfile->m_critsec))

#else
#define EnterCrit(p)
#define LeaveCrit(p)

#endif


/*
** The usage counter keeps track of the overall usage of objects based on
** implementations provided by the component. This allows one to determine
** when the implementation is no longer in use.
*/

extern UINT	uUseCount;
extern BOOL	fLocked;

/*	-	-	-	-	-	-	-	-	*/


DEFINE_AVIGUID(CLSID_ACMCmprs,		0x0002000F, 0, 0);

