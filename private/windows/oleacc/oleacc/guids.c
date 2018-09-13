// Copyright (c) 1996-1999 Microsoft Corporation

// ===========================================================================
// File: G U I D S . C
// 
// Used to define all MSAA GUIDs for OLEACC.  By compiling this file w/o 
// precompiled headers, we are allowing the MSAA GUIDs to be defined and stored
// in OLEACC.DLLs data or code segments.  This is necessary for OLEACC.DLL to 
// be built.
// 
// Copyright 1995-1996 Microsoft Corporation.  All Rights Reserved.
// Microsoft Confidential.
// ===========================================================================

// disable warnings to placate compiler wjen compiling included ole headers
#pragma warning(disable:4201)	// allows nameless structs and unions
#pragma warning(disable:4514)	// don't care when unreferenced inline functions are removed
#pragma warning(disable:4706)	// we are allowed to assign within a conditional
#pragma warning(disable:4214)	// ignore nonstandard extensions
#pragma warning(disable:4115)	// named type definition in parenthesis

#include <objbase.h>

#include <initguid.h>

//=--------------------------------------------------------------------------=
// GUIDs
//=--------------------------------------------------------------------------=

DEFINE_GUID(LIBID_Accessibility,	0x1ea4dbf0, 0x3c3b, 0x11cf, 0x81, 0x0c, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);
DEFINE_GUID(IID_IAccessible,		0x618736e0, 0x3c3d, 0x11cf, 0x81, 0x0c, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);
DEFINE_GUID(IID_IAccessibleHandler, 0x03022430, 0xABC4, 0x11d0, 0xBD, 0xE2, 0x00, 0xAA, 0x00, 0x1A, 0x19, 0x53);



