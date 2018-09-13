#ifndef _ENUMCP_H_
#define _ENUMCP_H_

#ifdef  __cplusplus
//
//  CEnumCodePage declaration with IEnumCodePage Interface
//
class CEnumCodePage : public IEnumCodePage
{
    MIMECONTF       dwMimeSource;

public:
    // IUnknown methods
    virtual STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);

    // IEnumCodePage methods
    virtual STDMETHODIMP Clone(IEnumCodePage **ppEnumCodePage);
    virtual STDMETHODIMP Next(ULONG celt, PMIMECPINFO rgcpInfo, ULONG *pceltFetched);
    virtual STDMETHODIMP Reset(void);
    virtual STDMETHODIMP Skip(ULONG celt);

    // Constructor & Destructor
    CEnumCodePage(DWORD grfFlags, LANGID LangId, MIMECONTF dwSource);
    ~CEnumCodePage();

protected:
    int _cRef;
    int _iCur;
    DWORD   _dwLevel;
    LANGID  _LangId;
};

//
//  CEnumRfc1766 declaration with IEnumRfc1766 Interface
//
class CEnumRfc1766 : public IEnumRfc1766
{
    MIMECONTF   dwMimeSource;

public:
    // IUnknown methods
    virtual STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);

    // IEnumCodePage methods
    virtual STDMETHODIMP Clone(IEnumRfc1766 **ppEnumRfc1766);
    virtual STDMETHODIMP Next(ULONG celt, PRFC1766INFO rgRfc1766Info, ULONG *pceltFetched);
    virtual STDMETHODIMP Reset(void);
    virtual STDMETHODIMP Skip(ULONG celt);

    // Constructor & Destructor
    CEnumRfc1766(MIMECONTF dwSource, LANGID LangId);
    ~CEnumRfc1766();

protected:
    LANGID  _LangID;
    int     _cRef;
    UINT    _uCur;
};

class CEnumScript : public IEnumScript
{
public:
    // IUnknown methods
    virtual STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);

    // IEnumScript methods
    virtual STDMETHODIMP Clone(IEnumScript **ppEnumScript);
    virtual STDMETHODIMP Next(ULONG celt, PSCRIPTINFO rgScriptInfo, ULONG *pceltFetched);
    virtual STDMETHODIMP Reset(void);
    virtual STDMETHODIMP Skip(ULONG celt);

    // Constructor & Destructor
    CEnumScript(DWORD grfFlags, LANGID LangId, MIMECONTF dwSource);
    ~CEnumScript();

protected:
    int     _cRef;
    UINT    _uCur;
    LANGID  _LangId;
    DWORD   _dwLevel;
};


#endif  // __cplusplus

typedef struct tagRFC1766INFOA
{
    LCID    lcid;
    char    szRfc1766[MAX_RFC1766_NAME];
    char    szLocaleName[MAX_LOCALE_NAME];
} RFC1766INFOA, *PRFC1766INFOA;

#endif  // _ENUMCP_H_
