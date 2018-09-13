//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// tooltip.h 
//
//   Tool tip interface for items.
//
//   History:
//
//       4/21/97  edwardp   Created.
//
////////////////////////////////////////////////////////////////////////////////

//
// Check for previous includes of this file.
//

#ifndef _TOOLTIP_H_

#define _TOOLTIP_H_

//
// Class definition for the item context menu class.
//

class CQueryInfo : public IQueryInfo
{
//
// Methods
//

public:

    // Constructor
    CQueryInfo(PCDFITEMIDLIST pcdfidl,
             IXMLElementCollection* pIXMLElementCollection);
    CQueryInfo(IXMLElement* pIXMLElement, BOOL fHasSubItems);

    // IUnknown
    STDMETHODIMP         QueryInterface(REFIID, void **);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IQueryInfo methods.
    STDMETHODIMP GetInfoTip(DWORD dwFlags, WCHAR **ppwszTip);
    STDMETHODIMP GetInfoFlags(DWORD *pdwFlags);
private:

    // Destructor.
    ~CQueryInfo(void);

//
// Member variables.
//

private:

    ULONG           m_cRef;
    IXMLElement*    m_pIXMLElement;
    BOOL            m_fHasSubItems;
};


#endif // _TOOLTIP_H_
