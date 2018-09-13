/* Copyright 1996 Microsoft */

#ifndef _ACLHIST_H_
#define _ACLHIST_H_

class CACLHistory
                : public IEnumString
{
public:
    //////////////////////////////////////////////////////
    // Public Interfaces
    //////////////////////////////////////////////////////
    
    // *** IUnknown ***
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);

    // *** IEnumString ***
    virtual STDMETHODIMP Next(ULONG celt, LPOLESTR *rgelt, ULONG *pceltFetched);
    virtual STDMETHODIMP Skip(ULONG celt);
    virtual STDMETHODIMP Reset(void);
    virtual STDMETHODIMP Clone(IEnumString **ppenum);

protected:
    // Constructor / Destructor (protected so we can't create on stack)
    CACLHistory(void);
    ~CACLHistory(void);

    // Instance creator
    friend HRESULT CACLHistory_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi);

    // Private variables
    DWORD               _cRef;              // COM reference count
    IUrlHistoryStg*     _puhs;              // URL History storage
    IEnumSTATURL*       _pesu;              // URL enumerator
    LPOLESTR            _pwszAlternate;     // Alternate string
    HDSA                _hdsaAlternateData; // Contains alternate mappings

    // Private functions
    void _CreateAlternateData(void);
    void _CreateAlternateItem(LPCTSTR pszUrl);
    void _SetAlternateItem(LPCTSTR pszUrl);
    void _AddAlternateDataItem(LPCTSTR pszProtocol, LPCTSTR pszDomain, BOOL fMoveSlashes);
    void _ReadAndSortHistory(void);
    static int _FreeAlternateDataItem(LPVOID p, LPVOID d);
};

#endif // _ACLHIST_H_ 
