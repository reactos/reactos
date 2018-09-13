/*	-	-	-	-	-	-	-	-	*/

/*
**	Copyright (C) Microsoft Corporation 1993. All rights reserved.
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

public:
    IUnknown FAR*	m_pUnknownOuter;
    
    //
    //  AVIFile instance data
    //
    AVIFILEINFO FARSTRUCT	avihdr;         // file info
    ULONG			m_refs;
    PAVISTREAM NEAR *		aps;
};



