//+------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       STDAFX.CXX
//
//  Contents:   Source file that includes ATL standard files
//
//-------------------------------------------------------------------------
#include "headers.hxx"
#include "stdafx.h"

#pragma warning(disable:4189)   /* local variable is initialized but not referenced */

#ifdef _ATL_STATIC_REGISTRY
#include <statreg.h>
#include <statreg.cpp>
#endif

#include <atlimpl.cpp>
#include <atlctl.cpp>
#include <atlwin.cpp>

#ifdef _M_IA64

//$WIN64: Don't know why _WndProcThunkProc isn't defined 

extern "C" LRESULT CALLBACK _WndProcThunkProc(HWND, UINT, WPARAM, LPARAM )
{
    return 0;
}

#endif
