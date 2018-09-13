//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1995-1998               **
//*********************************************************************

//
//	HTMLGUID.H - GUID definition for HTML viewer object
//

#ifndef _HTMLGUID_H_
#define _HTMLGUID_H_

// GUID for HTML viewer is: {25336920-03F9-11cf-8FD0-00AA00686F13}
DEFINE_GUID(CLSID_HTMLViewer, 0x25336920, 0x3f9, 0x11cf, 0x8f, 0xd0, 0x0, 0xaa, 0x0, 0x68, 0x6f, 0x13);

// GUID for BSCB proxy is: {25336922-03F9-11cf-8FD0-00AA00686F13}
DEFINE_GUID(CLSID_HTMLBSCBProxy, 0x25336922, 0x3f9, 0x11cf, 0x8f, 0xd0, 0x0, 0xaa, 0x0, 0x68, 0x6f, 0x13);

// The GUID used to identify the TypeLib of the HTML Page
// {71BC8840-60BB-11cf-8B97-00AA00476DA6}
DEFINE_GUID(GUID_PageTL,
0x71bc8840, 0x60bb, 0x11cf, 0x8b, 0x97, 0x0, 0xaa, 0x0, 0x47, 0x6d, 0xa6);

// The GUID used to identify the Primary dispinterface of the HTML Page
// {71BC8841-60BB-11cf-8B97-00AA00476DA6}
DEFINE_GUID(IID_PageProps,
0x71bc8841, 0x60bb, 0x11cf, 0x8b, 0x97, 0x0, 0xaa, 0x0, 0x47, 0x6d, 0xa6);

// The GUID used to identify the Event dispinterface of the HTML Page
// The page events are currently commented out but will be added later
// so I grabbed a guid for the events now.
// {71BC8842-60BB-11cf-8B97-00AA00476DA6}
DEFINE_GUID(IID_PageEvents,
0x71bc8842, 0x60bb, 0x11cf, 0x8b, 0x97, 0x0, 0xaa, 0x0, 0x47, 0x6d, 0xa6);

// The GUID used to identify the coclass of the HTML Page
// {71BC8843-60BB-11cf-8B97-00AA00476DA6}
DEFINE_GUID(CLSID_Page,
0x71bc8843, 0x60bb, 0x11cf, 0x8b, 0x97, 0x0, 0xaa, 0x0, 0x47, 0x6d, 0xa6);


#endif // _HTMLGUID_H_
