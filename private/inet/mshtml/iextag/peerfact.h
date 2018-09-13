// CPeerFactory.h : Declaration of the CPeerFactory

#ifndef __PEERFACT_H_
#define __PEERFACT_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CPeerFactory

class ATL_NO_VTABLE CPeerFactory : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CPeerFactory, &CLSID_PeerFactory>,
	public IElementBehaviorFactory
{
public:
    //
    // construction / destruction
    //

	CPeerFactory();
	~CPeerFactory();

    //
    // IElementBehaviorFactory
    //

	STDMETHOD(FindBehavior)(
        BSTR bstrName, BSTR bstrUrl, IElementBehaviorSite * pSite, IElementBehavior ** ppPeer);

    //
    // macros
    //

DECLARE_REGISTRY_RESOURCEID(IDR_PEERFACTORY)

BEGIN_COM_MAP(CPeerFactory)
	COM_INTERFACE_ENTRY(IElementBehaviorFactory)
END_COM_MAP()

};

#endif //__PEERFACT_H_
