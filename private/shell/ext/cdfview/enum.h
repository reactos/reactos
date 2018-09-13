//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// enum.h 
//
//   The definition of the cdf enumerator.
//
//   History:
//
//       3/17/97  edwardp   Created.
//
////////////////////////////////////////////////////////////////////////////////

//
// Check for previous includes of this file.
//

#ifndef _ENUM_H_

#define _ENUM_H_

//
// Class definition for the cdf enumerator class.
//

class CCdfEnum : public IEnumIDList
{
//
// Methods
//

public:

    // Constructor
    CCdfEnum(IXMLElementCollection* pIXMLElementCollection,
             DWORD fEnumerateFlags, PCDFITEMIDLIST pcdfidlFolder);

    // IUnknown
    STDMETHODIMP         QueryInterface(REFIID, void **);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IEnumIDList
    STDMETHODIMP Next(ULONG celt, LPITEMIDLIST *rgelt, ULONG *pceltFetched);
    STDMETHODIMP Skip(ULONG celt);
    STDMETHODIMP Reset(void);
    STDMETHODIMP Clone(IEnumIDList **ppenum);
 
private:

    // Destructor
    ~CCdfEnum(void);

    // Helper methods
    LPITEMIDLIST NextCdfidl(void);
    HRESULT      GetNextCdfElement(IXMLElement** ppIXMLElement,PULONG pnIndex);
    inline BOOL  IsCorrectType(IXMLElement* pIXMLElement);

//
// Member variables.
//

private:

    ULONG                   m_cRef;
    IXMLElementCollection*  m_pIXMLElementCollection;
    DWORD                   m_fEnumerate;
    ULONG                   m_nCurrentItem;
    PCDFITEMIDLIST          m_pcdfidlFolder;
    BOOL                    m_fReturnedFolderPidl;
};


#endif _ENUM_H_
