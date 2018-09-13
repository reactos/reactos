/*	-	-	-	-	-	-	-	-	*/

/*
**	Copyright (C) Microsoft Corporation 1993 - 1995. All rights reserved.
*/

/*	-	-	-	-	-	-	-	-	*/

/*
** _StdClassImplementations
** Defines the standard implementations for a class object.
*/

#define	_StdClassImplementations(Impl)	\
	STDMETHODIMP QueryInterface(const IID FAR& riid, void FAR* FAR* ppv);	\
	STDMETHODIMP_(ULONG) AddRef();	\
	STDMETHODIMP_(ULONG) Release()


/*	-	-	-	-	-	-	-	-	*/

class FAR CFakeFile : IAVIFile {
public:
    CFakeFile(int nStreams, PAVISTREAM FAR * papStreams);

    _StdClassImplementations(CUnknownImpl);

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
    STDMETHODIMP DeleteStream         (THIS_
				     DWORD fccType,
				     LONG lParam);
#else
    STDMETHODIMP Reserved1            (THIS);
    STDMETHODIMP Reserved2            (THIS);
    STDMETHODIMP Reserved3            (THIS);
    STDMETHODIMP Reserved4            (THIS);
    STDMETHODIMP Reserved5            (THIS);
#endif

public:
    IUnknown FAR*	m_pUnknownOuter;

    //
    //  AVIFile instance data
    //
    AVIFILEINFOW FARSTRUCT	avihdr;         // file info
    ULONG			m_refs;
    PAVISTREAM NEAR *		aps;
};



