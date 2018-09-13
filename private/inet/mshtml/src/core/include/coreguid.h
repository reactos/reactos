//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       coreguid.h
//
//  Contents:   extern references for forms guids
//
//----------------------------------------------------------------------------

// Please check \forms3\src\site\include\siteguid.h for information about GUID

#ifndef I_COREGUID_H_
#define I_COREGUID_H_
#pragma INCMSG("--- Beg 'coreguid.h'")

// BUGBUG delete these. should be getting from public headers.
EXTERN_C const GUID CGID_ShellDocView;
EXTERN_C const GUID IID_IBrowseControl;
EXTERN_C const GUID IID_ITargetFrame2;
EXTERN_C const GUID CGID_MSHTML;

// Use PUBLIC_GUID for GUIDs used outside FORMS3.DLL.
// Use PRIVATE_GUID for all other GUIDS.

#ifndef PUBLIC_GUID
#define PUBLIC_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8);
#endif

#ifndef PRIVATE_GUID
#define PRIVATE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8);
#endif

// GUID from classic MSHTML
PRIVATE_GUID(CGID_IWebBrowserPriv, 0xED016940L,0xBD5B,0x11cf, 0xBA,0x4E,0x00,0xC0,0x4F,0xD7,0x08,0x16)

// private GUID used by IOleCommandTarget support in CBaseBag
//
PRIVATE_GUID(CGID_DATAOBJECTEXEC, 0x3050f3e4L,0x98b5,0x11cf,0xbb,0x82,0x00,0xaa,0x00,0xbd,0xce,0x0b)

// VARIANT conversion interface exposed by script engines (VBScript/JScript).
PUBLIC_GUID(SID_VariantConversion,  0x1f101481, 0xbccd, 0x11d0, 0x93, 0x36, 0x0, 0xa0, 0xc9, 0xd, 0xca, 0xa9)

// Service GUID to return a pointer to the scoped obect, used in IObjectIdentity impls
PUBLIC_GUID(SID_ELEMENT_SCOPE_OBJECT, 0x3050f408,0x98b5,0x11cf,0xbb,0x82,0x00,0xaa,0x00,0xbd,0xce,0x0b)

// CLSID to create the default recalc engine
PUBLIC_GUID(CLSID_CRecalcEngine, 0x3050f499, 0x98b5, 0x11cf, 0xbb, 0x82, 0x00, 0xaa, 0x00, 0xbd, 0xce, 0x0b)

// CLSID for CBase (needed to allow the document to go from a IUnknown to a CBase
PUBLIC_GUID(CLSID_CBase, 0x3050f49a, 0x98b5, 0x11cf, 0xbb, 0x82, 0x00, 0xaa, 0x00, 0xbd, 0xce, 0x0b)


// ********************** DO NOT REMOVE the below GUID. **********************
//
// IE4 shipped the interface IHTMLControlElementEvents with the same GUID as IControlEvents
// from forms^3 this is of course bad.  To resolve this problem Trident's
// GUID for IHTMLControlElementEvents has changed however, the old GUID remembered in
// the FindConnectionPt.  The only side affect is that using the old GUID will not marshall
// the interface correctly only the new GUID has the correct marshalling code.

// {9A4BBF53-4E46-101B-8BBD-00AA003E3B29}
PRIVATE_GUID(IID_IControlEvents, 0x9A4BBF53, 0x4E46, 0x101B, 0x8B, 0xBD, 0x00, 0xAA, 0x00, 0x3E, 0x3B, 0x29)

#pragma INCMSG("--- End 'coreguid.h'")
#else
#pragma INCMSG("*** Dup 'coreguid.h'")
#endif
