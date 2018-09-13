//+-------------------------------------------------------------------------
//
//  Copyright (C) Microsoft, 1997
//
//  File:       IDeskBand.cpp
//
//  Contents:   CPreviewBand::IDeskBand methods
//
//  History:    7-24-97  Davepl  Created
//
//--------------------------------------------------------------------------

#include "stdafx.h"
#include "PreviewBand.h"

//
// IObjectWithSite::GetBandInfo for CPreviewBand
//

const int CPreviewBand::m_MIN_SIZE_X = 20;
const int CPreviewBand::m_MIN_SIZE_Y = 20;

STDMETHODIMP CPreviewBand::GetBandInfo(DWORD dwBandID, DWORD dwViewMode, DESKBANDINFO* pdbi)
{
    if(NULL == pdbi)
    {
        return E_INVALIDARG;
    }
    else
    {
        m_dwBandID = dwBandID;
        m_dwViewMode = dwViewMode;

        if(pdbi->dwMask & DBIM_MINSIZE)
        {
            pdbi->ptMinSize.x = m_MIN_SIZE_X;
            pdbi->ptMinSize.y = m_MIN_SIZE_Y;
        }

        if(pdbi->dwMask & DBIM_MAXSIZE)
        {
            pdbi->ptMaxSize.x = -1;
            pdbi->ptMaxSize.y = -1;
        }

        if(pdbi->dwMask & DBIM_INTEGRAL)
        {
            pdbi->ptIntegral.x = 1;
            pdbi->ptIntegral.y = 1;
        }

        if(pdbi->dwMask & DBIM_ACTUAL)
        {
            pdbi->ptActual.x = 0;
            pdbi->ptActual.y = 0;
        }

        if(pdbi->dwMask & DBIM_TITLE)
        {
            #ifdef UNICODE
                LoadStringW(pdbi->wszTitle, IDS_BANDNAME);
            #else
                CHAR szAnsi[MAX_PATH];
                LoadStringA(_Module.m_hInstResource, IDS_BANDNAME, szAnsi, MAX_PATH);
                MultiByteToWideChar(CP_ACP, 0, szAnsi, MAX_PATH, pdbi->wszTitle, MAX_PATH);
            #endif
        }

        if(pdbi->dwMask & DBIM_MODEFLAGS)
        {
            pdbi->dwModeFlags = DBIMF_VARIABLEHEIGHT;
        }
   
        if(pdbi->dwMask & DBIM_BKCOLOR)
        {
            //Use the default background color by removing this flag.
            pdbi->dwMask &= ~DBIM_BKCOLOR;
        }

        return S_OK;
    }
}
