//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       siteguid.h
//
//  Contents:   extern references for site guids and manifest constants
//              for dispids
//
//----------------------------------------------------------------------------

//
// Reserve 10000 GUID entries beginning 3050f160-98b5-11cf-bb82-00aa00bdce0b
// and ending at 30c38c70-98b5-11cf-bb82-00aa00bdce0b.
//
// See the next available GUID in ...\src\guids.txt
// Check procedure for using next available GUID with F3 procedures handbook
//

#ifndef I_SITEGUID_H_
#define I_SITEGUID_H_
#pragma INCMSG("--- Beg 'siteguid.h'")

// Use PUBLIC_GUID for GUIDs used outside FORMS3.DLL.
// Use PRIVATE_GUID for all other GUIDS.

#ifndef PUBLIC_GUID
#define PUBLIC_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8);
#endif

#ifndef PRIVATE_GUID
#define PRIVATE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8);
#endif

PUBLIC_GUID(CLSID_CTextSite,  0x3050f35e, 0x98b5, 0x11cf, 0xbb, 0x82, 0x00, 0xaa, 0x00, 0xbd, 0xce, 0x0b)
PUBLIC_GUID(CLSID_CTextEdit,  0x3050f330, 0x98b5, 0x11cf, 0xbb, 0x82, 0x00, 0xaa, 0x00, 0xbd, 0xce, 0x0b)
PUBLIC_GUID(CLSID_CRange,     0x3050f234, 0x98b5, 0x11cf, 0xbb, 0x82, 0x00, 0xaa, 0x00, 0xbd, 0xce, 0x0b)
PUBLIC_GUID(CLSID_CElement,   0x3050f233, 0x98b5, 0x11cf, 0xbb, 0x82, 0x00, 0xaa, 0x00, 0xbd, 0xce, 0x0b)
PUBLIC_GUID(CLSID_CStyle,     0x3050f499, 0x98b5, 0x11cf, 0xbb, 0x82, 0x00, 0xaa, 0x00, 0xbd, 0xce, 0x0b)
PUBLIC_GUID(CLSID_CElementCollection,   0x3050f627, 0x98b5, 0x11cf, 0xbb, 0x82, 0x00, 0xaa, 0x00, 0xbd, 0xce, 0x0b)

PRIVATE_GUID(CLSID_CMarkup,        0x3050F4FB, 0x98B5, 0x11CF, 0xBB, 0x82, 0x00, 0xAA, 0x00, 0xBD, 0xCE, 0x0B)
PRIVATE_GUID(CLSID_CTreeNode,      0x3050F432, 0x98B5, 0x11CF, 0xBB, 0x82, 0x00, 0xAA, 0x00, 0xBD, 0xCE, 0x0B)
PRIVATE_GUID(CLSID_CMarkupPointer, 0x3050f4a5, 0x98B5, 0x11CF, 0xBB, 0x82, 0x00, 0xAA, 0x00, 0xBD, 0xCE, 0x0B)

// ********************** DO NOT REMOVE the below GUID. **********************
//
// IE4 shipped the interface IHTMLControlElement with the same GUID as IControl
// from forms^3 this is of course bad.  To resolve this problem Trident's
// GUID for IHTMLControlElement has changed however, the old GUID remembered in
// the QI for CSite to return IHTMLControlElement.  The only side affect is that
// using the old GUID will not marshall the interface correctly only the new
// GUID has the correct marshalling code.

// {04598fc6-866c-11cf-ab7c-00aa00c08fcf}
PRIVATE_GUID(IID_IControl, 0x04598fc6, 0x866c, 0x11cf, 0xab, 0x7c, 0x00, 0xaa, 0x00, 0xc0, 0x8f, 0xcf);

#pragma INCMSG("--- End 'siteguid.h'")
#else
#pragma INCMSG("*** Dup 'siteguid.h'")
#endif
