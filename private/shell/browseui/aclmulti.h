/* Copyright 1996 Microsoft */

#ifndef _ACLMULTI_H_
#define _ACLMULTI_H_

//
// CACLMulti is an AutoComplete List that simply contains
// other AutoComplete Lists.  Objects are added via IObjMgr
// methods, and each IEnumString method is
// just passed on to each sub list in turn.
//

class CACLMulti
                : public IEnumString
                , public IObjMgr
                , public IACList
{
public:
    //////////////////////////////////////////////////////
    // Public Interfaces
    //////////////////////////////////////////////////////
    
    // *** IUnknown ***
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppvObj);

    // *** IEnumString ***
    virtual STDMETHODIMP Next(ULONG celt, LPOLESTR *rgelt, ULONG *pceltFetched);
    virtual STDMETHODIMP Skip(ULONG celt);
    virtual STDMETHODIMP Reset(void);
    virtual STDMETHODIMP Clone(IEnumString **ppenum);

    // *** IObjMgr ***
    virtual STDMETHODIMP Append(IUnknown *punk);
    virtual STDMETHODIMP Remove(IUnknown *punk);

    // *** IACList ***
    virtual STDMETHODIMP Expand(LPCOLESTR pszExpand);

protected:
    // Constructor / Destructor (protected so we can't create on stack)
    CACLMulti(void);
    ~CACLMulti(void);

    // Instance creator
    friend HRESULT CACLMulti_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi);
    friend HRESULT CACLMulti_Create(IEnumString **ppenum, CACLMulti * paclMultiToCopy);

    // Private variables
    DWORD   _cRef;          // COM reference count
    int     _iSubList;      // Current sublist for Next() operations
    HDSA    _hdsa;          // HDSA of sublists

    // Private methods
    static int _FreeListItem(LPVOID p, LPVOID d);
};

#endif // _ACLMULTI_H_
