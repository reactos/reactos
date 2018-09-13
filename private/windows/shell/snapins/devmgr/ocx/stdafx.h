//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       stdafx.h
//
//--------------------------------------------------------------------------

// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#define VC_EXTRALEAN        // Exclude rarely-used stuff from Windows headers

#include <afxctl.h>         // MFC support for OLE Controls

// Delete the two includes below if you do not wish to use the MFC
//  database classes
#ifndef _UNICODE
#include <afxdb.h>          // MFC database classes
#include <afxdao.h>         // MFC DAO database classes
#endif //_UNICODE

#include <mmc.h>

extern const IID IID_ITreeViewPrivate;
extern const IID IID_IMMCSnapin;
