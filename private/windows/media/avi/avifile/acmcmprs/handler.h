
/*	-	-	-	-	-	-	-	-	*/

/*
**	Copyright (C) Microsoft Corporation 1993. All rights reserved.
*/

/*	-	-	-	-	-	-	-	-	*/

/*	-	-	-	-	-	-	-	-	*/

#define	implement	struct
#define	implementations	private

/*
** _StdClassImplementations
** Defines the standard implementations for a class object.
*/

#define	_StdClassImplementations(Impl)	\
	STDMETHODIMP QueryInterface(const IID FAR& riid, void FAR* FAR* ppv);	\
	STDMETHODIMP_(ULONG) AddRef();	\
	STDMETHODIMP_(ULONG) Release()

/*	-	-	-	-	-	-	-	-	*/

class FAR CAVIFileCF : IClassFactory {
public:
    static HRESULT Create(const CLSID FAR& rclsid, const IID FAR& riid, void FAR* FAR* ppv);
private:
    CAVIFileCF(const CLSID FAR& rclsid, IUnknown FAR* FAR* ppUnknown);
    _StdClassImplementations(CFactoryImpl);
    CFactoryImpl(CAVIFileCF FAR* pAVIFileCF);
    STDMETHODIMP CreateInstance(IUnknown FAR* pUnknownOuter, const IID FAR& riid, void FAR* FAR* ppv);
    STDMETHODIMP LockServer(BOOL fLock);

    ULONG	m_refs;
    CLSID	m_clsid;
};


#include "mmreg.h"
#include "msacm.h"

/*	-	-	-	-	-	-	-	-	*/

class FAR CACMCmpStream : IAVIStream{
public:
    static HRESULT MakeInst(IUnknown FAR* pUnknownOuter, const IID FAR& riid, void FAR* FAR* ppv);
    LONG SetUpCompression();
private:
    CACMCmpStream(IUnknown FAR* pUnknownOuter, IUnknown FAR* FAR* ppUnknown);
    public:
    _StdClassImplementations(CAVIStreamImpl);
    STDMETHODIMP Create      (THIS_ LPARAM lParam1, LPARAM lParam2);
    STDMETHODIMP Info        (THIS_ AVISTREAMINFO FAR * psi, LONG lSize);
    STDMETHODIMP_(LONG) FindSample (THIS_ LONG lPos, LONG lFlags);
    STDMETHODIMP ReadFormat  (THIS_ LONG lPos,
			    LPVOID lpFormat, LONG FAR *cbFormat);
    STDMETHODIMP SetFormat   (THIS_ LONG lPos,
			    LPVOID lpFormat, LONG cbFormat);
    STDMETHODIMP Read        (THIS_ LONG lStart, LONG lSamples,
			    LPVOID lpBuffer, LONG cbBuffer,
			    LONG FAR * plBytes, LONG FAR * plSamples);
    STDMETHODIMP Write       (THIS_ LONG lStart, LONG lSamples,
			      LPVOID lpBuffer, LONG cbBuffer, DWORD dwFlags,
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

public:
    IUnknown FAR*	m_pUnknownOuter;

    // Instance data
    ULONG		m_refs;
    AVISTREAMINFO     m_avistream;      // stream info
    PAVISTREAM		m_pavi;
    HACMSTREAM		m_hs;

    LPWAVEFORMATEX	m_lpFormat;
    LONG		m_cbFormat;
    LPWAVEFORMATEX	m_lpFormatC;
    LONG		m_cbFormatC;
};

/*	-	-	-	-	-	-	-	-	*/

/*
** The usage counter keeps track of the overall usage of objects based on
** implementations provided by the component. This allows one to determine
** when the implementation is no longer in use.
*/

extern UINT	uUseCount;
extern BOOL	fLocked;
extern HINSTANCE ghInst;


/*	-	-	-	-	-	-	-	-	*/


DEFINE_AVIGUID(CLSID_AVIWaveFileReader,           0x00020003, 0, 0);


/* resource ids */
#define	IDS_CNVTERR	100
#define	IDS_ACMERR	101


/*----------------------------------------------------------------------*/


