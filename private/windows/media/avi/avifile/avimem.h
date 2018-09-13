/*	-	-	-	-	-	-	-	-	*/

/*
**	Copyright (C) Microsoft Corporation 1993 - 1995. All rights reserved.
*/

#ifdef __cplusplus
class FAR CAVIMemStream : public IAVIStream {
public:
    CAVIMemStream();
public:
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
#ifdef _WIN32
    STDMETHODIMP SetInfo(AVISTREAMINFOW FAR *lpInfo, LONG cbInfo);
#else
    STDMETHODIMP Reserved1            (THIS);
    STDMETHODIMP Reserved2            (THIS);
    STDMETHODIMP Reserved3            (THIS);
    STDMETHODIMP Reserved4            (THIS);
    STDMETHODIMP Reserved5            (THIS);
#endif

public:
    ULONG	m_refs;

    LPVOID	m_lpMemory;

    LPVOID	m_lpFormat;
    LONG	m_cbFormat;
    LPVOID	m_lpData;
    LONG	m_cbData;

    AVISTREAMINFOW     m_avistream;      // stream info
};
#endif

#ifdef __cplusplus
extern "C"
#endif

