//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       delaydll.h
//
//  Contents:   precompiled headers for delayed dll 
//
//  Classes:
//
//  Functions:
//
//  History:    12-22-95   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
#ifndef DELAY_DLL_H
#define DELAY_DLL_H

#include "oautdll.hxx"

extern COleAutDll   g_OleAutDll;

#pragma warning(disable:4005)
#define VariantClear            g_OleAutDll.VariantClear 
#define VariantInit             g_OleAutDll.VariantInit
#define VariantCopy             g_OleAutDll.VariantCopy
#define VariantChangeType       g_OleAutDll.VariantChangeType
#define SysAllocStringByteLen   g_OleAutDll.SysAllocStringByteLen
#define SysAllocString          g_OleAutDll.SysAllocString
#define SysStringByteLen        g_OleAutDll.SysStringByteLen
#define SysFreeString           g_OleAutDll.SysFreeString
#define LoadTypeLib             g_OleAutDll.LoadTypeLib
#pragma warning(default:4005)

#endif
