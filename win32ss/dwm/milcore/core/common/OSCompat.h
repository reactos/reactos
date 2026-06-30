// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------

//
//  Abstract:        Declarations for OS compatibility routines
//-----------------------------------------------------------------------------

#pragma once

//+----------------------------------------------------------------------------
//
//  Function:
//      OSSupportsUpdateLayeredWindowIndirect
//
//  Synopsis:
//      Return true if OS supports UpdateLayeredWindowIndirect
//
//-----------------------------------------------------------------------------

bool
OSSupportsUpdateLayeredWindowIndirect(
    );

//+----------------------------------------------------------------------------
//
//  Function:  UpdateLayeredWindowEx
//
//  Synopsis:  Call UpdateLayeredWindow or UpdateLayeredWindowIndirect as
//             required by parameters.  If UpdateLayeredWindowIndirect is
//             needed (ULW_EX_NORESIZE requested), but not available return
//             HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND).  prcDirty is ignored
//             when UpdateLayeredWindowIndirect is not available.
//
//-----------------------------------------------------------------------------
HRESULT
UpdateLayeredWindowEx(
    __in HWND hWnd,
    __in_opt HDC hdcDst,
    __in_ecount_opt(1) CONST POINT *pptDst,
    __in_ecount_opt(1) CONST SIZE *psize,
    __in_opt HDC hdcSrc,
    __in_ecount_opt(1) CONST POINT *pptSrc,
    COLORREF crKey,
    __in_ecount_opt(1)CONST BLENDFUNCTION *pblend,
    DWORD dwFlags,
    __in_ecount_opt(1) CONST RECT *prcDirty
    );


//+------------------------------------------------------------------------
//
// Class:       CDisableWow64FsRedirection
//
// Synopsis:    Class that will disable Wow64 FS redirection and make sure
//              it's reverted.  Makes a best effort & doesn't return
//              any errors.
//
//              Won't do anything if the app isn't running under Wow64.
//
//              NOTE should be used on the stack.
//
//-------------------------------------------------------------------------

class CDisableWow64FsRedirection
{
    PVOID m_pOldValue;
    HRESULT m_hr;

public:
    CDisableWow64FsRedirection();
    ~CDisableWow64FsRedirection();
};

