//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1995.
//
//  File:       sdetails.cxx
//
//  Contents:   Implementation of IShellDetails
//
//  History:    13-Dec-95    BruceFo     Created
//
//----------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

#include "sdetails.hxx"
#include "shares.h"
#include "shares.hxx"
#include "resource.h"
#include "util.hxx"

//////////////////////////////////////////////////////////////////////////////

//
// Define the columns that we know about...
//

struct COL_INFO
{
    UINT idString;
    int  fmt;
    UINT cxChar;
};

const COL_INFO c_ColumnHeaders1[] =
{
    {IDS_NAME,    LVCFMT_LEFT, 25},
    {IDS_COMMENT, LVCFMT_LEFT, 30},
};

const COL_INFO c_ColumnHeaders2[] =
{
    {IDS_NAME,    LVCFMT_LEFT, 25},
    {IDS_COMMENT, LVCFMT_LEFT, 30},
    {IDS_PATH,    LVCFMT_LEFT, 30},
    {IDS_MAXUSES, LVCFMT_LEFT, 15},
    {IDS_SERVICE, LVCFMT_LEFT, 30},
};

//////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CSharesSD::GetDetailsOf(
    LPCITEMIDLIST pidl,
    UINT iColumn,
    LPSHELLDETAILS lpDetails
    )
{
    switch (m_level)
    {
    case 1: return GetDetailsOf1(pidl, iColumn, lpDetails);
    case 2: return GetDetailsOf2(pidl, iColumn, lpDetails);
    default: appAssert(FALSE); return E_FAIL;
    }
}

STDMETHODIMP
CSharesSD::GetDetailsOf1(
    LPCITEMIDLIST pidl,
    UINT iColumn,
    LPSHELLDETAILS lpDetails
    )
{
    if (iColumn >= ICOL1_MAX)
    {
        return E_NOTIMPL;
    }

    HRESULT hr;
    LPIDSHARE pids = (LPIDSHARE)pidl;
    if (NULL == pids)
    {
        hr = STRRETLoadString(c_ColumnHeaders1[iColumn].idString, &lpDetails->str);
        if (FAILED(hr))
        {
            return hr;
        }

        lpDetails->fmt    = c_ColumnHeaders1[iColumn].fmt;
        lpDetails->cxChar = c_ColumnHeaders1[iColumn].cxChar;
        return NOERROR;
    }

    appAssert(Share_GetLevel(pids) == 1);

    switch (iColumn)
    {
    case ICOL1_NAME:
        hr = STRRETCopy(Share_GetName(pids), &lpDetails->str);
        if (FAILED(hr))
        {
            return hr;
        }
        break;

    case ICOL1_COMMENT:
        hr = STRRETCopy(Share_GetComment(pids), &lpDetails->str);
        if (FAILED(hr))
        {
            return hr;
        }
        break;
	}

    return NOERROR;
}

STDMETHODIMP
CSharesSD::GetDetailsOf2(
    LPCITEMIDLIST pidl,
    UINT iColumn,
    LPSHELLDETAILS lpDetails
    )
{
    if (iColumn >= ICOL2_MAX)
    {
        return E_NOTIMPL;
    }

    HRESULT hr;
    LPIDSHARE pids = (LPIDSHARE)pidl;
    if (NULL == pids)
    {
        hr = STRRETLoadString(c_ColumnHeaders2[iColumn].idString, &lpDetails->str);
        if (FAILED(hr))
        {
            return hr;
        }

        lpDetails->fmt    = c_ColumnHeaders2[iColumn].fmt;
        lpDetails->cxChar = c_ColumnHeaders2[iColumn].cxChar;
        return NOERROR;
    }

    BYTE bService = Share_GetService(pids);
    DWORD dwServiceCount = 0;
    if (bService & SHARE_SERVICE_SMB)  ++dwServiceCount;
    if (bService & SHARE_SERVICE_SFM)  ++dwServiceCount;
    if (bService & SHARE_SERVICE_FPNW) ++dwServiceCount;
    appAssert(dwServiceCount > 0);
    appAssert(dwServiceCount == 1);	// until we implement merging shares

    switch (iColumn)
    {
    case ICOL2_NAME:
        hr = STRRETCopy(Share_GetName(pids), &lpDetails->str);
        if (FAILED(hr))
        {
            return hr;
        }
        break;

    case ICOL2_COMMENT:
        if (dwServiceCount > 1)
        {
			// nothing!
        	lpDetails->str.uType = STRRET_CSTR;
        	lpDetails->str.cStr[0] = '\0';
        }
        else
        {
			// SMB is only service with a comment field
            if (bService & SHARE_SERVICE_SMB)
            {
        		hr = STRRETCopy(Share_GetComment(pids), &lpDetails->str);
        		if (FAILED(hr))
        		{
            		return hr;
        		}
            }
        }
        break;

    case ICOL2_PATH:
        hr = STRRETCopy(Share_GetPath(pids), &lpDetails->str);
        if (FAILED(hr))
        {
            return hr;
        }
        break;

    case ICOL2_MAXUSES:
    {
        if (dwServiceCount > 1)
        {
			// nothing!
        	lpDetails->str.uType = STRRET_CSTR;
        	lpDetails->str.cStr[0] = '\0';
        }
        else
        {
        	DWORD maxuses = 0;
            if (bService & SHARE_SERVICE_SMB)
            {
				maxuses = pids->maxUses;
            }
            if (bService & SHARE_SERVICE_SFM)
            {
				maxuses = pids->sfmMaxUses;
            }
            if (bService & SHARE_SERVICE_FPNW)
            {
				maxuses = pids->fpnwMaxUses;
            }
			appAssert(0 != maxuses);

			// BUGBUG: assuming max uses is 0xffffffff for all services!
            if (maxuses == SHI_USES_UNLIMITED)
            {
                hr = STRRETLoadString(IDS_UNLIMITED, &lpDetails->str);
                if (FAILED(hr))
                {
                    return hr;
                }
            }
            else
            {
                TCHAR szTemp[MAX_PATH];
                wsprintf(szTemp, TEXT("%d"), maxuses);
                hr = STRRETCopy(szTemp, &lpDetails->str);
                if (FAILED(hr))
                {
                    return hr;
                }
            }
        }
        break;
    }

    case ICOL2_SERVICE:
    {
        if (bService & SHARE_SERVICE_SMB)
        {
            hr = STRRETLoadString(IDS_SERVICE_SMB, &lpDetails->str);
        }
        if (bService & SHARE_SERVICE_SFM)
        {
            hr = STRRETLoadString(IDS_SERVICE_SFM, &lpDetails->str);
        }
        if (bService & SHARE_SERVICE_FPNW)
        {
            hr = STRRETLoadString(IDS_SERVICE_FPNW, &lpDetails->str);
        }

        if (FAILED(hr))
        {
            return hr;
        }
        break;
    }

    }

    return NOERROR;
}

STDMETHODIMP
CSharesSD::ColumnClick(
    UINT iColumn
    )
{
    ShellFolderView_ReArrange(m_hwnd, iColumn);
    return NOERROR;
}
