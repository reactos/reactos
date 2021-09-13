/*
 *  Rebar band site
 *
 *  Copyright 2007  Hervé Poussineau
 *  Copyright 2009  Andrew Hill
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "shellbars.h"

#ifndef ASSERT
#define ASSERT(cond) \
    if (!(cond)) \
        ERR ("ASSERTION %s AT %s:%d FAILED!\n", #cond, __FILE__, __LINE__)
#endif


/*
TODO:
    ** Fix tasks band gripper not appearing when quick launch is added
    ** Fix hiding grippers in locked mode
    ** The context menu should include the menu of both the site and the band
    ** The chevron should be shown only when needed
*/

CBandSiteBase::CBandSiteBase():
    m_cBands(0),
    m_cBandsAllocated(0),
    m_bands(NULL),
    m_hwndRebar(NULL),
    m_dwState(0),
    m_dwStyle(0)
{
}

UINT CBandSiteBase::_GetBandID(struct BandObject *Band)
{
    return (UINT)(Band - m_bands);
}

struct CBandSiteBase::BandObject *CBandSiteBase::_GetBandByID(DWORD dwBandID)
{
    if ((LONG)dwBandID >= m_cBandsAllocated)
        return NULL;

    if (m_bands[dwBandID].DeskBand == NULL)
        return NULL;

    return &m_bands[dwBandID];
}

void CBandSiteBase::_FreeBand(struct BandObject *Band)
{
    ATLASSERT(Band->DeskBand != NULL);
    ATLASSERT(Band->OleWindow != NULL);
    ATLASSERT(Band->WndEvtHandler != NULL);
    Band->DeskBand->Release();
    Band->OleWindow->Release();
    Band->WndEvtHandler->Release();
    memset(Band, 0, sizeof(*Band));
    m_cBands--;
}

DWORD CBandSiteBase::_GetViewMode()
{
    DWORD                                   dwStyle;

    /* FIXME: What about DBIF_VIEWMODE_FLOATING and DBIF_VIEWMODE_TRANSPARENT? */
    dwStyle = GetWindowLongPtr(m_hwndRebar, GWL_STYLE);

    if (dwStyle & CCS_VERT)
        return DBIF_VIEWMODE_VERTICAL;
    else
        return DBIF_VIEWMODE_NORMAL;
}

VOID CBandSiteBase::_BuildBandInfo(struct BandObject *Band, REBARBANDINFOW *prbi)
{
    ZeroMemory(prbi, sizeof(*prbi));
    prbi->cbSize = sizeof(*prbi);

    prbi->fMask = RBBIM_ID;
    prbi->wID = _GetBandID(Band);

    if (Band->dbi.dwMask & DBIM_MINSIZE)
    {
        prbi->fMask |= RBBIM_CHILDSIZE;
        prbi->cxMinChild = Band->dbi.ptMinSize.x;
        prbi->cyMinChild = Band->dbi.ptMinSize.y;
    }

    if (Band->dbi.dwMask & DBIM_MAXSIZE)
    {
        prbi->fMask |= RBBIM_CHILDSIZE;
        prbi->cyMaxChild = Band->dbi.ptMaxSize.y;
    }

    if ((Band->dbi.dwMask & (DBIM_INTEGRAL | DBIM_MODEFLAGS)) == (DBIM_INTEGRAL | DBIM_MODEFLAGS) &&
        (Band->dbi.dwModeFlags & DBIMF_VARIABLEHEIGHT))
    {
        prbi->fMask |= RBBIM_CHILDSIZE;
        prbi->cyIntegral = Band->dbi.ptIntegral.y;
    }

    if (Band->dbi.dwMask & DBIM_ACTUAL)
    {
        prbi->fMask |= RBBIM_IDEALSIZE | RBBIM_SIZE | RBBIM_CHILDSIZE;
        prbi->cxIdeal = Band->dbi.ptActual.x;
        prbi->cx = Band->dbi.ptActual.x;
        prbi->cyChild = Band->dbi.ptActual.y;
    }

    if (Band->dbi.dwMask & DBIM_TITLE)
    {
        prbi->fMask |= RBBIM_TEXT;
        prbi->lpText = Band->dbi.wszTitle;
        prbi->cch = wcslen(Band->dbi.wszTitle);
    }

    if (Band->dbi.dwMask & DBIM_MODEFLAGS)
    {
        prbi->fMask |= RBBIM_STYLE;

        if (Band->dbi.dwModeFlags & DBIMF_FIXED)
            prbi->fStyle |= RBBS_FIXEDSIZE | RBBS_NOGRIPPER;
        if (Band->dbi.dwModeFlags & DBIMF_FIXEDBMP)
            prbi->fStyle |= RBBS_FIXEDBMP;
        if (Band->dbi.dwModeFlags & DBIMF_VARIABLEHEIGHT)
            prbi->fStyle |= RBBS_VARIABLEHEIGHT;
        if (Band->dbi.dwModeFlags & DBIMF_DEBOSSED)
            prbi->fStyle |= RBBS_CHILDEDGE;
        if (Band->dbi.dwModeFlags & DBIMF_USECHEVRON)
            prbi->fStyle |= RBBS_USECHEVRON;
        if (Band->dbi.dwModeFlags & DBIMF_BREAK)
            prbi->fStyle |= RBBS_BREAK;
        if (Band->dbi.dwModeFlags & DBIMF_TOPALIGN)
            prbi->fStyle |= RBBS_TOPALIGN;
        if ((Band->dbi.dwModeFlags & DBIMF_NOGRIPPER) || (m_dwStyle & BSIS_NOGRIPPER))
            prbi->fStyle |= RBBS_NOGRIPPER;
        if ((Band->dbi.dwModeFlags & DBIMF_ALWAYSGRIPPER) || (m_dwStyle & BSIS_ALWAYSGRIPPER))
            prbi->fStyle |= RBBS_GRIPPERALWAYS;
    }

    if (Band->bHiddenTitle  || (m_dwStyle & BSIS_NOCAPTION))
    {
        prbi->fMask |= RBBIM_STYLE;
        prbi->fStyle |= RBBS_HIDETITLE;
    }

    if ((Band->dbi.dwMask & (DBIM_BKCOLOR | DBIM_MODEFLAGS)) == (DBIM_BKCOLOR | DBIM_MODEFLAGS) &&
        (Band->dbi.dwModeFlags & DBIMF_BKCOLOR))
    {
        prbi->fMask |= RBBIM_COLORS;
        prbi->clrFore = (COLORREF)(COLOR_WINDOWTEXT + 1);
        prbi->clrBack = Band->dbi.crBkgnd;
    }
}

HRESULT CBandSiteBase::_UpdateBand(struct BandObject *Band)
{
    REBARBANDINFOW                          rbi;
    DWORD                                   dwViewMode;
    UINT                                    uBand;
    HRESULT                                 hRet;

    ZeroMemory(&Band->dbi, sizeof(Band->dbi));
    Band->dbi.dwMask = DBIM_MINSIZE | DBIM_MAXSIZE | DBIM_INTEGRAL |
        DBIM_ACTUAL | DBIM_TITLE | DBIM_MODEFLAGS | DBIM_BKCOLOR;

    dwViewMode = _GetViewMode();

    hRet = Band->DeskBand->GetBandInfo((DWORD)_GetBandID(Band), dwViewMode, &Band->dbi);
    if (SUCCEEDED(hRet))
    {
        _BuildBandInfo(Band, &rbi);
        if (SUCCEEDED(Band->OleWindow->GetWindow(&rbi.hwndChild)) &&
            rbi.hwndChild != NULL)
        {
            rbi.fMask |= RBBIM_CHILD;
            WARN ("ReBar band uses child window 0x%p\n", rbi.hwndChild);
        }

        uBand = (UINT)SendMessageW(m_hwndRebar, RB_IDTOINDEX, (WPARAM)rbi.wID, 0);
        if (uBand != (UINT)-1)
        {
            if (!SendMessageW(m_hwndRebar, RB_SETBANDINFOW, (WPARAM)uBand, reinterpret_cast<LPARAM>(&rbi)))
            {
                WARN("Failed to update the rebar band!\n");
            }
        }
        else
            WARN("Failed to map rebar band id to index!\n");

    }

    return hRet;
}

HRESULT CBandSiteBase::_UpdateAllBands()
{
    LONG                                    i;
    HRESULT                                 hRet;

    for (i = 0; i < m_cBandsAllocated; i++)
    {
        if (m_bands[i].DeskBand != NULL)
        {
            hRet = _UpdateBand(&m_bands[i]);
            if (FAILED_UNEXPECTEDLY(hRet))
                return hRet;
        }
    }

    return S_OK;
}

HRESULT CBandSiteBase::_UpdateBand(DWORD dwBandID)
{
    struct BandObject                       *Band;

    Band = _GetBandByID(dwBandID);
    if (Band == NULL)
        return E_FAIL;

    return _UpdateBand(Band);
}

HRESULT CBandSiteBase::_IsBandDeletable(DWORD dwBandID)
{
    CComPtr<IBandSite> pbs;

    /* Use QueryInterface so that we get the outer object in case we have one */
    HRESULT hr = this->QueryInterface(IID_PPV_ARG(IBandSite, &pbs));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    DWORD dwState;
    hr = pbs->QueryBand(dwBandID, NULL, &dwState, NULL, NULL);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    return ((dwState & BSSF_UNDELETEABLE) != 0) ? S_FALSE : S_OK;
}

HRESULT CBandSiteBase::_OnContextMenu(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plrResult)
{
    /* Find the index fo the band that was clicked */
    int x = GET_X_LPARAM(lParam);
    int y = GET_Y_LPARAM(lParam);

    RBHITTESTINFO htInfo = {{x, y}};
    ScreenToClient(m_hwndRebar, &htInfo.pt);
    int iBand = SendMessageW(m_hwndRebar, RB_HITTEST, 0, (LPARAM)&htInfo);
    if (iBand < 0)
    {
        /* FIXME: what to do here? */
        return S_OK;
    }

    /* Now get the id of the band that was clicked */
    REBARBANDINFOW bandInfo = {sizeof(bandInfo), RBBIM_ID};
    SendMessageW(m_hwndRebar, RB_GETBANDINFOW, htInfo.iBand, (LPARAM)&bandInfo);

    /* Finally get the band */
    DWORD dwBandID = bandInfo.wID;
    struct BandObject *Band = _GetBandByID(dwBandID);
    if (Band == NULL)
        return E_FAIL;

    HMENU hMenu = CreatePopupMenu();
    if (hMenu == NULL)
        return E_OUTOFMEMORY;

    /* Try to load the menu of the band */
    UINT idBandLast = 0;
    CComPtr<IContextMenu> pcm;
    HRESULT hr = Band->DeskBand->QueryInterface(IID_PPV_ARG(IContextMenu, &pcm));
    if (SUCCEEDED(hr))
    {
        hr = pcm->QueryContextMenu(hMenu, 0, 0, UINT_MAX, CMF_NORMAL);
        if (SUCCEEDED(hr))
        {
            idBandLast = HRESULT_CODE(hr);
        }
    }

    if (!(m_dwStyle & BSIS_LOCKED))
    {
        /* Load the static part of the menu */
        HMENU hMenuStatic = LoadMenuW(GetModuleHandleW(L"browseui.dll"), MAKEINTRESOURCEW(IDM_BAND_MENU));

        if (hMenuStatic)
        {
            Shell_MergeMenus(hMenu, hMenuStatic, UINT_MAX, 0, UINT_MAX, MM_DONTREMOVESEPS | MM_SUBMENUSHAVEIDS);

            ::DestroyMenu(hMenuStatic);

            hr = _IsBandDeletable(dwBandID);
            if (FAILED_UNEXPECTEDLY(hr))
                return hr;

            /* Remove the close item if it is not deletable */
            if (hr == S_FALSE || (Band->dbi.dwModeFlags & DBIMF_UNDELETEABLE) != 0)
                DeleteMenu(hMenu, IDM_BAND_CLOSE, MF_BYCOMMAND);

            if ((Band->dbi.dwMask & DBIM_TITLE) == 0)
                DeleteMenu(hMenu, IDM_BAND_TITLE, MF_BYCOMMAND);
            else
                CheckMenuItem(hMenu, IDM_BAND_TITLE, Band->bHiddenTitle ? MF_UNCHECKED : MF_CHECKED);
        }
    }

    /* TODO: Query the menu of our site */

    UINT uCommand = ::TrackPopupMenuEx(hMenu, TPM_RETURNCMD, x, y, m_hwndRebar, NULL);
    if (uCommand < idBandLast)
    {
        CMINVOKECOMMANDINFO cmi = { sizeof(cmi), 0, m_hwndRebar, MAKEINTRESOURCEA(uCommand)};
        hr = pcm->InvokeCommand(&cmi);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;
    }
    else
    {
        if (uCommand == IDM_BAND_TITLE)
        {
            Band->bHiddenTitle = !Band->bHiddenTitle;

            hr = _UpdateBand(dwBandID);
            if (FAILED_UNEXPECTEDLY(hr))
                return hr;
        }
        else if(uCommand == IDM_BAND_CLOSE)
        {
            hr = RemoveBand(dwBandID);
            if (FAILED_UNEXPECTEDLY(hr))
                return hr;
        }
    }

    return S_OK;
}

struct CBandSiteBase::BandObject *CBandSiteBase::_GetBandFromHwnd(HWND hwnd)
{
    HRESULT                                 hRet;
    HWND                                    hWndBand;
    LONG                                    i;

    for (i = 0; i < m_cBandsAllocated; i++)
    {
        if (m_bands[i].DeskBand != NULL)
        {
            ASSERT(m_bands[i].OleWindow);

            hWndBand = NULL;
            hRet = m_bands[i].OleWindow->GetWindow(&hWndBand);
            if (SUCCEEDED(hRet) && hWndBand == hwnd)
                return &m_bands[i];
        }
    }

    return NULL;
}

CBandSiteBase::~CBandSiteBase()
{

    TRACE("destroying %p\n", this);

    if (m_hwndRebar != NULL)
    {
        DestroyWindow(m_hwndRebar);
        m_hwndRebar = NULL;
    }

    if (m_bands != NULL)
    {
        for (INT i = 0; i < m_cBandsAllocated; i++)
        {
            if (m_bands[i].DeskBand != NULL)
                _FreeBand(&m_bands[i]);
        }
        CoTaskMemFree(m_bands);
        m_bands = NULL;
    }
}

HRESULT STDMETHODCALLTYPE CBandSiteBase::AddBand(IUnknown *punk)
{
    LONG                                    NewAllocated;
    struct BandObject                       *NewBand = NULL;
    CComPtr<IDeskBand>                      DeskBand;
    CComPtr<IObjectWithSite>                ObjWithSite;
    CComPtr<IOleWindow>                     OleWindow;
    CComPtr<IWinEventHandler>               WndEvtHandler;
    REBARBANDINFOW                          rbi;
    HRESULT                                 hRet;
    UINT                                    uBand;

    TRACE("(%p, %p)\n", this, punk);

    if (punk == NULL || m_hwndRebar == NULL)
        return E_FAIL;

    hRet = punk->QueryInterface(IID_PPV_ARG(IDeskBand, &DeskBand));
    if (FAILED_UNEXPECTEDLY(hRet))
        return hRet;

    hRet = punk->QueryInterface(IID_PPV_ARG(IObjectWithSite, &ObjWithSite));
    if (FAILED_UNEXPECTEDLY(hRet))
        return hRet;

    hRet = punk->QueryInterface(IID_PPV_ARG(IOleWindow, &OleWindow));
    if (FAILED_UNEXPECTEDLY(hRet))
        return hRet;

    hRet = punk->QueryInterface(IID_PPV_ARG(IWinEventHandler, &WndEvtHandler));
    if (FAILED_UNEXPECTEDLY(hRet))
        return hRet;

    if (m_cBandsAllocated > m_cBands)
    {
        /* Search for a free band object */
        for (INT i = 0; i < m_cBandsAllocated; i++)
        {
            if (m_bands[i].DeskBand == NULL)
            {
                NewBand = &m_bands[i];
                break;
            }
        }
    }
    else if (m_cBandsAllocated > 0)
    {
        ASSERT (m_bands != NULL);

        /* Reallocate the band object array */
        NewAllocated = m_cBandsAllocated + 8;
        if (NewAllocated > 0xFFFF)
            NewAllocated = 0xFFFF;
        if (NewAllocated == m_cBandsAllocated)
        {
            return E_OUTOFMEMORY;
        }


        NewBand = static_cast<struct BandObject *>(CoTaskMemAlloc(NewAllocated * sizeof(struct BandObject)));
        if (NewBand == NULL)
        {
            return E_OUTOFMEMORY;
        }

        /* Copy the old array */
        memcpy(NewBand, m_bands, m_cBandsAllocated * sizeof(struct BandObject));

        /* Initialize the added bands */
        memset(&NewBand[m_cBandsAllocated], 0, (NewAllocated - m_cBandsAllocated) * sizeof(struct BandObject));

        m_cBandsAllocated = NewAllocated;
        CoTaskMemFree(m_bands);
        m_bands = NewBand;
    }
    else
    {
        ASSERT(m_bands == NULL);
        ASSERT(m_cBandsAllocated == 0);
        ASSERT(m_cBands == 0);

        /* Allocate new array */
        m_bands = static_cast<struct BandObject *>(CoTaskMemAlloc(8 * sizeof(struct BandObject)));
        if (m_bands == NULL)
        {
            return E_OUTOFMEMORY;
        }

        /* Initialize the added bands */
        memset(m_bands, 0, 8 * sizeof(struct BandObject));

        m_cBandsAllocated += 8;
        NewBand = &m_bands[0];
    }

    ASSERT(NewBand != NULL);

    m_cBands++;
    NewBand->DeskBand = DeskBand.Detach();
    NewBand->OleWindow = OleWindow.Detach();
    NewBand->WndEvtHandler = WndEvtHandler.Detach();

    /* Create the ReBar band */
    hRet = ObjWithSite->SetSite(static_cast<IOleWindow *>(this));
    if (SUCCEEDED(hRet))
    {
        uBand = 0xffffffff;
        if (SUCCEEDED(_UpdateBand(NewBand)))
        {
            if (NewBand->dbi.dwMask & DBIM_MODEFLAGS)
            {
                if (NewBand->dbi.dwModeFlags & DBIMF_ADDTOFRONT)
                    uBand = 0;
            }
        }

        _BuildBandInfo(NewBand, &rbi);

        if (SUCCEEDED(NewBand->OleWindow->GetWindow(&rbi.hwndChild)) &&
            rbi.hwndChild != NULL)
        {
            rbi.fMask |= RBBIM_CHILD;
            WARN ("ReBar band uses child window 0x%p\n", rbi.hwndChild);
        }

        if (!SendMessageW(m_hwndRebar, RB_INSERTBANDW, (WPARAM)uBand, reinterpret_cast<LPARAM>(&rbi)))
            return E_FAIL;

        hRet = (HRESULT)((USHORT)_GetBandID(NewBand));

        _UpdateAllBands();
    }
    else
    {
        WARN("IBandSite::AddBand(): Call to IDeskBand::SetSite() failed: %x\n", hRet);

        /* Remove the band from the ReBar control */
        _BuildBandInfo(NewBand, &rbi);
        uBand = (UINT)SendMessageW(m_hwndRebar, RB_IDTOINDEX, (WPARAM)rbi.wID, 0);
        if (uBand != (UINT)-1)
        {
            if (!SendMessageW(m_hwndRebar, RB_DELETEBAND, (WPARAM)uBand, 0))
            {
                ERR("Failed to delete band!\n");
            }
        }
        else
            ERR("Failed to map band id to index!\n");

        _FreeBand(NewBand);

        hRet = E_FAIL;
    }

    return hRet;
}

HRESULT STDMETHODCALLTYPE CBandSiteBase::EnumBands(UINT uBand, DWORD *pdwBandID)
{
    DWORD                                   i;

    TRACE("(%p, %u, %p)\n", this, uBand, pdwBandID);

    if (uBand == 0xffffffff)
        return (UINT)m_cBands;

    if (uBand >= (UINT)m_cBands)
        return E_FAIL;

    for (i = 0; i < (DWORD)m_cBandsAllocated; i++)
    {
        if (m_bands[i].DeskBand != NULL)
        {
            if (uBand == 0)
            {
                *pdwBandID = i;
                return S_OK;
            }

            uBand--;
        }
    }

    return E_FAIL;
}

HRESULT STDMETHODCALLTYPE CBandSiteBase::QueryBand(DWORD dwBandID, IDeskBand **ppstb,
    DWORD *pdwState, LPWSTR pszName, int cchName)
{
    struct BandObject                       *Band;

    TRACE("(%p, %u, %p, %p, %p, %d)\n", this, dwBandID, ppstb, pdwState, pszName, cchName);

    Band = _GetBandByID(dwBandID);
    if (Band == NULL)
        return E_FAIL;

    if (ppstb != NULL)
    {
        Band->DeskBand->AddRef();
        *ppstb = Band->DeskBand;
    }

    if (pdwState != NULL)
    {
        FIXME("IBandSite::QueryBand() requests band state!\n");
        *pdwState = 0;
    }

    if (pszName != NULL && cchName > 0)
    {
        FIXME("IBandSite::QueryBand() requests band name!\n");
        pszName[0] = 0;
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CBandSiteBase::SetBandState(DWORD dwBandID, DWORD dwMask, DWORD dwState)
{
    struct BandObject                       *Band;

    TRACE("(%p, %u, %x, %x)\n", this, dwBandID, dwMask, dwState);

    Band = _GetBandByID(dwBandID);
    if (Band == NULL)
        return E_FAIL;

    FIXME("Stub\n");
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBandSiteBase::RemoveBand(DWORD dwBandID)
{
    struct BandObject                       *Band;
    UINT                                    uBand;

    TRACE("(%p, %u)\n", this, dwBandID);

    if (m_hwndRebar == NULL)
        return E_FAIL;

    Band = _GetBandByID(dwBandID);
    if (Band == NULL)
        return E_FAIL;

    uBand = (UINT)SendMessageW(m_hwndRebar, RB_IDTOINDEX, (WPARAM)_GetBandID(Band), 0);
    if (uBand != (UINT)-1)
    {
        if (!SendMessageW(m_hwndRebar, RB_DELETEBAND, (WPARAM)uBand, 0))
        {
            ERR("Could not delete band!\n");
        }
    }
    else
        ERR("Could not map band id to index!\n");

    _FreeBand(Band);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CBandSiteBase::GetBandObject(DWORD dwBandID, REFIID riid, VOID **ppv)
{
    struct BandObject                       *Band;

    TRACE("(%p, %u, %s, %p)\n", this, dwBandID, debugstr_guid(&riid), ppv);

    Band = _GetBandByID(dwBandID);
    if (Band == NULL)
    {
        *ppv = NULL;
        return E_FAIL;
    }

    return Band->DeskBand->QueryInterface(riid, ppv);
}

HRESULT STDMETHODCALLTYPE CBandSiteBase::SetBandSiteInfo(const BANDSITEINFO *pbsinfo)
{
    if (!pbsinfo)
        return E_INVALIDARG;

    if ((pbsinfo->dwMask & BSIM_STATE))
        m_dwState = pbsinfo->dwState;
    if ((pbsinfo->dwMask & BSIM_STYLE))
        m_dwStyle = pbsinfo->dwStyle;

    _UpdateAllBands();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CBandSiteBase::GetBandSiteInfo(BANDSITEINFO *pbsinfo)
{
    if (!pbsinfo)
        return E_INVALIDARG;

    if ((pbsinfo->dwMask & BSIM_STATE))
        pbsinfo->dwState = m_dwState;
    if ((pbsinfo->dwMask & BSIM_STYLE))
        pbsinfo->dwStyle = m_dwStyle;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CBandSiteBase::OnWinEvent(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plrResult)
{
    struct BandObject                       *Band;

    TRACE("(%p, %p, %u, %p, %p, %p)\n", this, hWnd, uMsg, wParam, lParam, plrResult);

    if (plrResult)
        *plrResult = 0;
    if (m_hwndRebar == NULL)
        return E_FAIL;

    if (uMsg == WM_CONTEXTMENU)
    {
        HRESULT hr = _OnContextMenu(hWnd, uMsg, wParam, lParam, plrResult);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        return S_OK;
    }
    else if (uMsg == WM_COMMAND && lParam)
    {
        hWnd = reinterpret_cast<HWND>(lParam);
    }
    else if (uMsg == WM_NOTIFY)
    {
        NMHDR* pnmhdr = reinterpret_cast<NMHDR*>(lParam);
        if (pnmhdr->hwndFrom != m_hwndRebar)
        {
            hWnd = pnmhdr->hwndFrom;
        }
        else
        {
            for (int i = 0; i < m_cBandsAllocated; i++)
            {
                if (m_bands[i].WndEvtHandler && m_bands[i].OleWindow)
                {
                    HWND hwndBand;
                    if (SUCCEEDED(m_bands[i].OleWindow->GetWindow(&hwndBand)))
                    {
                        m_bands[i].WndEvtHandler->OnWinEvent(hwndBand, uMsg, wParam, lParam, plrResult);
                    }
                }
            }
            return S_OK;
        }
    }

    Band = _GetBandFromHwnd(hWnd);
    if (Band != NULL)
    {
        return Band->WndEvtHandler->OnWinEvent(hWnd, uMsg, wParam, lParam, plrResult);
    }

    return E_FAIL;
}

HRESULT STDMETHODCALLTYPE CBandSiteBase::IsWindowOwner(HWND hWnd)
{
    struct BandObject                       *Band;

    TRACE("(%p, %p)\n", this, hWnd);

    if (m_hwndRebar == NULL)
        return E_FAIL;

    Band = _GetBandFromHwnd(hWnd);
    if (Band != NULL)
        return S_OK;

    return S_FALSE;
}

HRESULT STDMETHODCALLTYPE CBandSiteBase::GetWindow(HWND *phWnd)
{
    TRACE("(%p, %p)\n", this, phWnd);

    *phWnd = m_hwndRebar;
    if (m_hwndRebar != NULL)
        return S_OK;

    return E_FAIL;
}

HRESULT STDMETHODCALLTYPE CBandSiteBase::ContextSensitiveHelp(BOOL fEnterMode)
{
    FIXME("(%p, %d)\n", this, fEnterMode);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBandSiteBase::SetDeskBarSite(IUnknown *pUnk)
{
    HWND                                    hWndParent;
    HRESULT                                 hRet;
    DWORD                                   style;

    TRACE("(%p, %p)\n", this, pUnk);

    m_site = NULL;

    hRet = pUnk->QueryInterface(IID_PPV_ARG(IOleWindow, &m_site));
    if (FAILED_UNEXPECTEDLY(hRet))
        return E_FAIL;

    hRet = m_site->GetWindow(&hWndParent);
    if (FAILED_UNEXPECTEDLY(hRet))
        return E_FAIL;

    style = WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | RBS_VARHEIGHT | RBS_AUTOSIZE |
        RBS_BANDBORDERS | CCS_NODIVIDER | /*CCS_NORESIZE |*/ CCS_NOPARENTALIGN;

    m_hwndRebar = CreateWindowExW(WS_EX_TOOLWINDOW,
                                   REBARCLASSNAMEW,
                                   NULL,
                                   style,
                                   0, 0, 0, 0,
                                   hWndParent,
                                   NULL,
                                   _AtlBaseModule.GetModuleInstance(),
                                   NULL);
    if (m_hwndRebar == NULL)
    {
        m_site = NULL;
        WARN("IDeskbarClient::SetDeskBarSite() failed to create ReBar control!\n");
        return E_FAIL;
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CBandSiteBase::SetModeDBC(DWORD dwMode)
{
    LONG                                    dwStyle;
    LONG                                    dwPrevStyle;

    TRACE("(%p, %x)\n", this, dwMode);

    if (m_hwndRebar == NULL)
        return E_FAIL;

    dwStyle = dwPrevStyle = GetWindowLongPtr(m_hwndRebar, GWL_STYLE);
    if (dwMode & DBIF_VIEWMODE_VERTICAL)
        dwStyle |= CCS_VERT;

    if (dwMode & ~DBIF_VIEWMODE_VERTICAL)
        FIXME("IDeskBarClient::SetModeDBC() unhandled modes: %x\n", dwStyle & ~DBIF_VIEWMODE_VERTICAL);

    if (dwStyle != dwPrevStyle)
    {
        SetWindowLongPtr(m_hwndRebar, GWL_STYLE, dwPrevStyle);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CBandSiteBase::UIActivateDBC(DWORD dwState)
{
    TRACE("(%p, %x)\n", this, dwState);

    if (m_hwndRebar == NULL)
        return E_FAIL;

    ShowWindow(m_hwndRebar, (dwState & DBC_SHOW) ? SW_SHOW : SW_HIDE);
    //FIXME: Properly notify bands?
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CBandSiteBase::GetSize(DWORD unknown1, LPRECT unknown2)
{
    FIXME("(%p, %x, %p)\n", this, unknown1, unknown2);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBandSiteBase::QueryStatus(const GUID *pguidCmdGroup,
    DWORD cCmds, OLECMD *prgCmds, OLECMDTEXT *pCmdText)
{
    FIXME("(%p, %p, %u, %p, %p)\n", this, pguidCmdGroup, cCmds, prgCmds, pCmdText);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBandSiteBase::Exec(const GUID *pguidCmdGroup, DWORD nCmdID,
    DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut)
{
    HRESULT                                 hRet = S_OK;

    TRACE("(%p, %p, %u, %u, %p, %p)\n", this, pguidCmdGroup, nCmdID, nCmdExecOpt, pvaIn, pvaOut);

    if (m_hwndRebar == NULL)
        return E_FAIL;

    if (IsEqualIID(*pguidCmdGroup, IID_IDeskBand))
    {
        switch (nCmdID)
        {
            case DBID_BANDINFOCHANGED:
                if (pvaIn == NULL)
                    hRet = _UpdateAllBands();
                else
                {
                    /* Update a single band */
                    if (pvaIn->n1.n2.vt == VT_I4)
                        hRet = _UpdateBand(pvaIn->n1.n2.n3.lVal);
                    else
                        hRet = E_FAIL;
                }
                break;

            case DBID_SHOWONLY:
            case DBID_MAXIMIZEBAND:
            case DBID_PUSHCHEVRON:
                FIXME("IOleCommandTarget::Exec(): Unsupported command ID %d\n", nCmdID);
                return E_NOTIMPL;
            default:
                return E_FAIL;
        }
        return hRet;
    }
    else
        WARN("IOleCommandTarget::Exec(): Unsupported command group GUID\n");

    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBandSiteBase::UIActivateIO(BOOL fActivate, LPMSG lpMsg)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBandSiteBase::HasFocusIO()
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBandSiteBase::TranslateAcceleratorIO(LPMSG lpMsg)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBandSiteBase::OnFocusChangeIS(struct IUnknown *paramC, int param10)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBandSiteBase::QueryService(REFGUID guidService, REFIID riid, void **ppvObject)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBandSiteBase::GetClassID(CLSID *pClassID)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBandSiteBase::IsDirty()
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBandSiteBase::Load(IStream *pStm)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBandSiteBase::Save(IStream *pStm, BOOL fClearDirty)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBandSiteBase::GetSizeMax(ULARGE_INTEGER *pcbSize)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBandSiteBase::DragEnter(
    IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBandSiteBase::DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBandSiteBase::DragLeave()
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBandSiteBase::Drop(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBandSiteBase::LoadFromStreamBS(IStream *, const GUID &, void **)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBandSiteBase::SaveToStreamBS(IUnknown *, IStream *)
{
    return E_NOTIMPL;
}

extern "C"
HRESULT WINAPI RSHELL_CBandSite_CreateInstance(LPUNKNOWN pUnkOuter, REFIID riid, void **ppv)
{
    return CBandSite::_CreatorClass::CreateInstance(pUnkOuter, riid, ppv);
}
