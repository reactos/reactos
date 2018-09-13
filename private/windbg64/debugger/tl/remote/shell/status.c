/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    status.c

Abstract:

    This file implements the ui feedback class. 
    This is to let the user know what the current 
    status is.

Author:

    Carlos Klapp (a-caklap) 1-July-1998

Environment:

    User Mode

--*/

#include "precomp.h"
#pragma hdrstop



extern BOOL     fConnected;



CWindbgrmFeedback g_CWindbgrmFeedback;



CWindbgrmFeedback::
CWindbgrmFeedback()
{
    ZeroMemory(m_rgpsz, sizeof(m_rgpsz));
    ZeroMemory(m_rgsize, sizeof(m_rgsize));
    ZeroMemory(m_rgbChanged, sizeof(m_rgbChanged));

    m_pszClientName = NULL;

    m_bCurrentlyPainting = FALSE;
    m_bCurrentlyUpdating = FALSE;

    // Basically we are going to use the semaphore as
    // a mutex
    InitializeCriticalSection(&m_cs);
}


CWindbgrmFeedback::
~CWindbgrmFeedback()
{
    DeleteCriticalSection(&m_cs);

    for (int i=0; i<MAX_NUM_STRINGS; i++) {
        if (m_rgpsz[i]) {
            free(m_rgpsz[i]);
        }
    }
}


void
CWindbgrmFeedback::
SetTL(
    CIndiv_TL_RM_WKSP * pCTl
    )
{
    EnterCriticalSection(&m_cs);

    Assert(pCTl);

    m_CTl.Duplicate(*pCTl);

    LeaveCriticalSection(&m_cs);
}


void
CWindbgrmFeedback::
SetClientName(
    PSTR pszClientName
    )
{
    EnterCriticalSection(&m_cs);

    if (m_pszClientName) {
        free(m_pszClientName);
        m_pszClientName = NULL;
    }
    m_pszClientName = _strdup(pszClientName);

    LeaveCriticalSection(&m_cs);
}


void
CWindbgrmFeedback::
HelperSetString(
    HDC     hdc,
    PSTR &  pszNewText,
    PSTR &  pszOldText,
    SIZE &  sizeTextExtent,
    BOOL &  bChanged
    )
{
    Assert(hdc);
    Assert(pszNewText);

    if (pszOldText && !strcmp(pszOldText, pszNewText)) {
        bChanged = FALSE;
        free(pszNewText);
    } else {
        SIZE sizeOld = {0};
        SIZE sizeNew = {0};

        bChanged = TRUE;

        // Make sure we also erase the previous info.
        if (pszOldText) {
            GetTextExtentPoint32(hdc, pszOldText, 
                strlen(pszOldText), &sizeOld);
        }
        GetTextExtentPoint32(hdc, pszNewText, 
            strlen(pszNewText), &sizeNew);

        sizeTextExtent.cx = max(sizeNew.cx, sizeOld.cx);
        sizeTextExtent.cy = max(sizeNew.cy, sizeOld.cy);

        if (pszOldText) {
            free(pszOldText);
        }
        pszOldText = pszNewText;
    }
}


void
CWindbgrmFeedback::
UpdateInfo(
    HWND hwnd
    )
{
    EnterCriticalSection(&m_cs);

    if (m_bCurrentlyUpdating || m_bCurrentlyPainting) {
        LeaveCriticalSection(&m_cs);
        return;
    }

    m_bCurrentlyUpdating = TRUE;

    PSTR pszNewText = NULL;
    int nIdx;
    int nYPos;
    HDC hdc = GetDC(hwnd);
    
    Assert(hdc);
    
    nIdx = 0;
    pszNewText = WKSP_DynaLoadStringWithArgs(g_hInst,   
        fConnected ? IDS_Status_Connected : IDS_Status_Not_Connected);
    HelperSetString(hdc, pszNewText, m_rgpsz[nIdx],
        m_rgsize[nIdx], m_rgbChanged[nIdx]);
    
    nIdx = 1;
    pszNewText = WKSP_DynaLoadStringWithArgs(g_hInst,   
        IDS_Status_Debuggee_Info, 
        m_pszClientName ? m_pszClientName : "");
    HelperSetString(hdc, pszNewText, m_rgpsz[nIdx],
        m_rgsize[nIdx], m_rgbChanged[nIdx]);
    
    nIdx = 2;
    pszNewText = WKSP_DynaLoadStringWithArgs(g_hInst,   
        IDS_Status_Transport_Layer, 
        m_CTl.m_pszDescription, m_CTl.m_pszDll, m_CTl.m_pszParams);
    HelperSetString(hdc, pszNewText, m_rgpsz[nIdx],
        m_rgsize[nIdx], m_rgbChanged[nIdx]);
    
    // Invalidate regions of the screen
    for (nYPos = 0, nIdx = 0; nIdx < MAX_NUM_STRINGS; nIdx++) {
        if (m_rgbChanged[nIdx]) {
            RECT rc = {0};
            
            rc.top = nYPos;
            rc.bottom = nYPos + m_rgsize[nIdx].cy;
            rc.right = m_rgsize[nIdx].cx;
            
            InvalidateRect(hwnd, &rc, TRUE);
        }            
        nYPos += m_rgsize[nIdx].cy;
    }
    
    ReleaseDC(hwnd, hdc);

    m_bCurrentlyUpdating = FALSE;

    LeaveCriticalSection(&m_cs);
}


void
CWindbgrmFeedback::
PaintConnectionStatus(
    HWND hwnd, 
    HDC hdc
    )
{
    EnterCriticalSection(&m_cs);

    Assert(hwnd);
    Assert(hdc);

    if (m_bCurrentlyUpdating || m_bCurrentlyPainting) {
        LeaveCriticalSection(&m_cs);
        return;
    }

    m_bCurrentlyPainting = TRUE;

    int nIdx;
    int nYPos;

    // Invalidate regions of the screen
    for (nYPos = 0, nIdx = 0; nIdx < MAX_NUM_STRINGS; nIdx++) {
        m_rgbChanged[nIdx] = FALSE;
        TextOut(hdc, 0, nYPos, m_rgpsz[nIdx], strlen(m_rgpsz[nIdx]));
        nYPos += m_rgsize[nIdx].cy;
    }

    m_bCurrentlyPainting = FALSE;

    LeaveCriticalSection(&m_cs);
}


