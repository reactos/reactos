
#ifndef _URLENUM_H_
#define _URLENUM_H



class CEnumString : public IEnumString
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

public:
    // Constructor-destructor, don't create this on the stack. 
    CEnumString();
    ~CEnumString();

    // This is a very simple class where all the strings should be added
    // before any enumeration occurs. 
    HRESULT AddString(LPCWSTR lpsz);

private:
    CRefCount m_ref;

    struct ListStr 
    {
        LPTSTR lpsz;
        ListStr * pListNext;
    };

    ListStr * pFirst;
    ListStr * pLast;
    ListStr * pCurrent;
};

#endif  

    