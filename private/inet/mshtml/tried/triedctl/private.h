//	private.h
//	Copyright (c)1997-1999 Microsoft Corporation, All Rights Reserved
//	Private interfaces for DHTMLED
// Include this in the file which used <initguid.h>

#ifndef __DHTMLED_PRIVATE_H__
#define __DHTMLED_PRIVATE_H__


typedef interface IInterconnector IInterconnector;
typedef interface IProtocolInfoConnector IProtocolInfoConnector;


DEFINE_GUID(IID_IInterconnector, 0x9499F420,0xCE86,0x11d1,0x8C,0xD3,0x00,0xA0,0xC9,0x59,0xBC,0x0A);

MIDL_INTERFACE("9499F420-CE86-11d1-8CD3-00A0C959BC0A")
IInterconnector : public IUnknown
{
	public:
    STDMETHOD(GetInterconnector)( SIZE_T* pProxyFrame );
    STDMETHOD(GetCtlWnd)( SIZE_T* pWndCD );
	STDMETHOD(MakeDirty)( DISPID dispid );
};



DEFINE_GUID(IID_IProtocolInfoConnector, 0x5ADEA280,0xC2CD,0x11d1,0x8C,0xCB,0x00,0xA0,0xC9,0x59,0xBC,0x0A);

MIDL_INTERFACE("5ADEA280-C2CD-11d1-8CCB-00A0C959BC0A")
IProtocolInfoConnector : public IUnknown
{
	public:
     STDMETHOD(SetProxyFrame)( SIZE_T* pProxyFrame);
    
};


DEFINE_GUID(CLSID_DHTMLEdProtocol, 0xF6E34E90,0xC032,0x11d1, 0x8C,0xCB,0x00,0xA0,0xC9,0x59,0xBC,0x0A);

DEFINE_GUID(IID_IMultiLanguage2Correct, 0xDCCFC164,0x2B38,0x11d2, 0xB7,0xEC,0x00,0xC0,0x4F,0x8F,0x5D,0x9A);

#endif //__DHTMLED_PRIVATE_H__

// End of private.h
