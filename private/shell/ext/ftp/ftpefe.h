/*****************************************************************************\
    FILE: ftpefe.h
\*****************************************************************************/

#ifndef _FTPEFE_H
#define _FTPEFE_H


/*****************************************************************************\
    CLASS: CFtpEfe

    DESCRIPTION:
        The stuff that tracks the state of an enumeration.
\*****************************************************************************/

class CFtpEfe           : public IEnumFORMATETC
{
public:
    //////////////////////////////////////////////////////
    // Public Interfaces
    //////////////////////////////////////////////////////
    
    // *** IUnknown ***
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    
    // *** IEnumFORMATETC ***
    virtual STDMETHODIMP Next(ULONG celt, FORMATETC * rgelt, ULONG *pceltFetched);
    virtual STDMETHODIMP Skip(ULONG celt);
    virtual STDMETHODIMP Reset(void);
    virtual STDMETHODIMP Clone(IEnumFORMATETC **ppenum);

public:
    // Friend Functions
    friend HRESULT CFtpEfe_Create(DWORD dwSize, FORMATETC rgfe[], STGMEDIUM rgstg[], CFtpObj * pfo, IEnumFORMATETC ** ppenum);

protected:
    // Private Member Variables
    int                     m_cRef;

    DWORD                   m_dwIndex;           // Current Item in the m_hdsaFormatEtc list
    DWORD                   m_dwExtraIndex;      // Current Item in the m_pfo->m_hdsaSetData list
    HDSA                    m_hdsaFormatEtc;     // pointer to the array 
    CFtpObj *               m_pfo;               // pointer to the parent IDataObject impl that has the list of extra data from ::SetData.


    CFtpEfe(DWORD dwSize, FORMATETC rgfe[], STGMEDIUM rgstg[], CFtpObj * pfo);
    CFtpEfe(DWORD dwSize, HDSA hdsaFormatEtc, CFtpObj * pfo, DWORD dwIndex);
    ~CFtpEfe(void);

    // Public Member Functions
    HRESULT _NextOne(FORMATETC * pfetc);

    
    // Friend Functions
    friend HRESULT CFtpEfe_Create(DWORD dwSize, HDSA m_hdsaFormatEtc, DWORD dwIndex, CFtpObj * pfo, IEnumFORMATETC ** ppenum);
    friend HRESULT CFtpEfe_Create(DWORD dwSize, FORMATETC rgfe[], STGMEDIUM rgstg[], CFtpObj * pfo, CFtpEfe ** ppfefe);
};

#endif // _FTPEFE_H
