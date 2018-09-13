//*******************************************************************************************
//
// Filename : Pch.h
//	
//				Common header file 
//
// Copyright (c) 1994 - 1996 Microsoft Corporation. All rights reserved
//
//*******************************************************************************************

#define STRICT 
#include <windows.h>

#include <windowsx.h>
#include <shlobj.h>
#include <shlobjp.h>
#include <shlguid.h>
#include <shlwapi.h>

#define ARRAYSIZE(a)    (sizeof(a)/sizeof(a[0]))
#define FIELDOFFSET(type, field)    ((int)(&((type NEAR*)1)->field)-1)
#define SIZEOF(a)                   sizeof(a)

