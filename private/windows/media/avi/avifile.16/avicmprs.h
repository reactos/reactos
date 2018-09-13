/*	-	-	-	-	-	-	-	-	*/

/*
**	Copyright (C) Microsoft Corporation 1993. All rights reserved.
*/

/*	-	-	-	-	-	-	-	-	*/

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

#ifndef _INC_COMPMAN
typedef HANDLE HIC;     /* Handle to a Insatlable Compressor */
#endif

#ifdef __cplusplus
class FAR CAVICmpStream {
public:
    static HRESULT Create(IUnknown FAR* pUnknownOuter, REFIID riid, LPVOID FAR* ppv);
    CAVICmpStream(IUnknown FAR* pUnknownOuter, IUnknown FAR* FAR* ppUnknown);
    HRESULT SetUpCompression();
private:
implementations:
    implement CUnknownImpl : IUnknown {
    public:
	_StdClassImplementations(CUnknownImpl);
	CUnknownImpl(CAVICmpStream FAR* pAVIStream);
    private:
	CAVICmpStream FAR*	m_pAVIStream;
	ULONG	m_refs;
    };
    implement CAVICmpStreamImpl : IAVIStream {
    public:
	_StdClassImplementations(CAVICmpStreamImpl);
	CAVICmpStreamImpl(CAVICmpStream FAR* pAVIStream);
	~CAVICmpStreamImpl();
	STDMETHODIMP Create      (THIS_ LPARAM lParam1, LPARAM lParam2);
	STDMETHODIMP Info        (THIS_ AVISTREAMINFO FAR * psi, LONG lSize);
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
    private:
	void ReadPalette(LONG lPos, LONG lPal, LPRGBQUAD prgb);
	// private functions here?
	CAVICmpStream FAR*	m_pAVIStream;
    };
public:
    CUnknownImpl	m_Unknown;
    CAVICmpStreamImpl	m_AVIStream;
    LONG ICCrunch(LPBITMAPINFOHEADER lpbi, LPVOID lp);
    void ResetInst(void);
    
public:
    IUnknown FAR*	m_pUnknownOuter;

    // AVIStream Instance data
    AVISTREAMINFO     avistream;      // stream info
    PAVISTREAM		pavi;
    PGETFRAME		pgf;
    LONG		lFrameCurrent;
    HIC			hic;
    LPBITMAPINFOHEADER	lpbiC;
    LPVOID		lpC;
    LPBITMAPINFOHEADER	lpbiU;
    LPVOID		lpU;
    LPBITMAPINFOHEADER	lpFormat;
    LONG		cbFormat;
    LPBITMAPINFOHEADER	lpFormatOrig;
    LONG		cbFormatOrig;
    DWORD		dwKeyFrameEvery;
    DWORD		fccIC;
    DWORD		dwICFlags;
    BOOL		fPad;
    LPVOID		lpHandler;
    LONG		cbHandler;
    DWORD		dwMaxSize;
    
    DWORD		dwQualityLast;
    LONG		lLastKeyFrame;
    DWORD		dwSaved;
    DWORD		m_ckid;
    DWORD		m_dwFlags;
};
#endif

DEFINE_AVIGUID(CLSID_AVICmprsStream,           0x00020001, 0, 0);

