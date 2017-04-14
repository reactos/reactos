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

CBandSiteBase::CBandSiteBase()
{
    fBandsCount = 0;
    fBandsAllocated = 0;
    fBands = NULL;
    fRebarWindow = NULL;
}

UINT CBandSiteBase::GetBandID(struct BandObject *Band)
{
    return (UINT)(Band - fBands);
}

struct CBandSiteBase::BandObject *CBandSiteBase::GetBandByID(DWORD dwBandID)
{
    if ((LONG)dwBandID >= fBandsAllocated)
        return NULL;

    if (fBands[dwBandID].DeskBand == NULL)
        return NULL;

    return &fBands[dwBandID];
}

void CBandSiteBase::FreeBand(struct BandObject *Band)
{
    ATLASSERT(Band->DeskBand != NULL);
    ATLASSERT(Band->OleWindow != NULL);
    ATLASSERT(Band->WndEvtHandler != NULL);
    Band->DeskBand->Release();
    Band->OleWindow->Release();
    Band->WndEvtHandler->Release();
    memset(Band, 0, sizeof(*Band));
    fBandsCount--;
}

DWORD CBandSiteBase::GetBandSiteViewMode()
{
    DWORD                                   dwStyle;

    /* FIXME: What about DBIF_VIEWMODE_FLOATING and DBIF_VIEWMODE_TRANSPARENT? */
    dwStyle = GetWindowLongPtr(fRebarWindow, GWL_STYLE);

    if (dwStyle & CCS_VERT)
        return DBIF_VIEWMODE_VERTICAL;
    else
        return DBIF_VIEWMODE_NORMAL;
}

VOID CBandSiteBase::BuildRebarBandInfo(struct BandObject *Band, REBARBANDINFOW *prbi)
{
    ZeroMemory(prbi, sizeof(*prbi));
    prbi->cbSize = sizeof(*prbi);

    prbi->fMask = RBBIM_ID;
    prbi->wID = GetBandID(Band);

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
        if (Band->dbi.dwModeFlags & DBIMF_NOGRIPPER)
            prbi->fStyle |= RBBS_NOGRIPPER;
        if (Band->dbi.dwModeFlags & DBIMF_ALWAYSGRIPPER)
            prbi->fStyle |= RBBS_GRIPPERALWAYS;
    }

    if ((Band->dbi.dwMask & (DBIM_BKCOLOR | DBIM_MODEFLAGS)) == (DBIM_BKCOLOR | DBIM_MODEFLAGS) &&
        (Band->dbi.dwModeFlags & DBIMF_BKCOLOR))
    {
        prbi->fMask |= RBBIM_COLORS;
        prbi->clrFore = (COLORREF)(COLOR_WINDOWTEXT + 1);
        prbi->clrBack = Band->dbi.crBkgnd;
    }
}

HRESULT CBandSiteBase::UpdateSingleBand(struct BandObject *Band)
{
    REBARBANDINFOW                          rbi;
    DWORD                                   dwViewMode;
    UINT                                    uBand;
    HRESULT                                 hRet;

    ZeroMemory(&Band->dbi, sizeof(Band->dbi));
    Band->dbi.dwMask = DBIM_MINSIZE | DBIM_MAXSIZE | DBIM_INTEGRAL |
        DBIM_ACTUAL | DBIM_TITLE | DBIM_MODEFLAGS | DBIM_BKCOLOR;

    dwViewMode = GetBandSiteViewMode();

    hRet = Band->DeskBand->GetBandInfo((DWORD)GetBandID(Band), dwViewMode, &Band->dbi);
    if (SUCCEEDED(hRet))
    {
        BuildRebarBandInfo(Band, &rbi);
        if (SUCCEEDED(Band->OleWindow->GetWindow(&rbi.hwndChild)) &&
            rbi.hwndChild != NULL)
        {
            rbi.fMask |= RBBIM_CHILD;
            WARN ("ReBar band uses child window 0x%p\n", rbi.hwndChild);
        }

        uBand = (UINT)SendMessageW(fRebarWindow, RB_IDTOINDEX, (WPARAM)rbi.wID, 0);
        if (uBand != (UINT)-1)
        {
            if (!SendMessageW(fRebarWindow, RB_SETBANDINFOW, (WPARAM)uBand, reinterpret_cast<LPARAM>(&rbi)))
            {
                WARN("Failed to update the rebar band!\n");
            }
        }
        else
            WARN("Failed to map rebar band id to index!\n");

    }

    return hRet;
}

HRESULT CBandSiteBase::UpdateAllBands()
{
    LONG                                    i;
    HRESULT                                 hRet;

    for (i = 0; i < fBandsAllocated; i++)
    {
        if (fBands[i].DeskBand != NULL)
        {
            hRet = UpdateSingleBand(&fBands[i]);
            if (FAILED_UNEXPECTEDLY(hRet))
                return hRet;
        }
    }

    return S_OK;
}

HRESULT CBandSiteBase::UpdateBand(DWORD dwBandID)
{
    struct BandObject                       *Band;

    Band = GetBandByID(dwBandID);
    if (Band == NULL)
        return E_FAIL;

    return UpdateSingleBand(Band);
}

struct CBandSiteBase::BandObject *CBandSiteBase::GetBandFromHwnd(HWND hwnd)
{
    HRESULT                                 hRet;
    HWND                                    hWndBand;
    LONG                                    i;

    for (i = 0; i < fBandsAllocated; i++)
    {
        if (fBands[i].DeskBand != NULL)
        {
            ASSERT(fBands[i].OleWindow);

            hWndBand = NULL;
            hRet = fBands[i].OleWindow->GetWindow(&hWndBand);
            if (SUCCEEDED(hRet) && hWndBand == hwnd)
                return &fBands[i];
        }
    }

    return NULL;
}

CBandSiteBase::~CBandSiteBase()
{

    TRACE("destroying %p\n", this);

    if (fRebarWindow != NULL)
    {
        DestroyWindow(fRebarWindow);
        fRebarWindow = NULL;
    }

    if (fBands != NULL)
    {
        for (INT i = 0; i < fBandsAllocated; i++)
        {
            if (fBands[i].DeskBand != NULL)
                FreeBand(&fBands[i]);
        }
        CoTaskMemFree(fBands);
        fBands = NULL;
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

    if (punk == NULL || fRebarWindow == NULL)
        return E_FAIL;

    hRet = punk->QueryInterface(IID_PPV_ARG(IDeskBand, &DeskBand));
    if (!SUCCEEDED(hRet) || DeskBand == NULL)
        goto Cleanup;
    hRet = punk->QueryInterface(IID_PPV_ARG(IObjectWithSite, &ObjWithSite));
    if (!SUCCEEDED(hRet) || ObjWithSite == NULL)
        goto Cleanup;
    hRet = punk->QueryInterface(IID_PPV_ARG(IOleWindow, &OleWindow));
    if (!SUCCEEDED(hRet) || OleWindow == NULL)
        goto Cleanup;
    hRet = punk->QueryInterface(IID_PPV_ARG(IWinEventHandler, &WndEvtHandler));
    if (!SUCCEEDED(hRet) || WndEvtHandler == NULL)
        goto Cleanup;

    hRet = S_OK;
    if (fBandsAllocated > fBandsCount)
    {
        /* Search for a free band object */
        for (INT i = 0; i < fBandsAllocated; i++)
        {
            if (fBands[i].DeskBand == NULL)
            {
                NewBand = &fBands[i];
                break;
            }
        }
    }
    else if (fBandsAllocated > 0)
    {
        ASSERT (fBands != NULL);

        /* Reallocate the band object array */
        NewAllocated = fBandsAllocated + 8;
        if (NewAllocated > 0xFFFF)
            NewAllocated = 0xFFFF;
        if (NewAllocated == fBandsAllocated)
        {
            hRet = E_OUTOFMEMORY;
            goto Cleanup;
        }


        NewBand = static_cast<struct BandObject *>(CoTaskMemAlloc(NewAllocated * sizeof(struct BandObject)));
        if (NewBand == NULL)
        {
            hRet = E_OUTOFMEMORY;
            goto Cleanup;
        }

        /* Copy the old array */
        memcpy(NewBand, fBands, fBandsAllocated * sizeof(struct BandObject));

        /* Initialize the added bands */
        memset(&NewBand[fBandsAllocated], 0, (NewAllocated - fBandsAllocated) * sizeof(struct BandObject));

        fBandsAllocated = NewAllocated;
        CoTaskMemFree(fBands);
        fBands = NewBand;
    }
    else
    {
        ASSERT(fBands == NULL);
        ASSERT(fBandsAllocated == 0);
        ASSERT(fBandsCount == 0);

        /* Allocate new array */
        fBands = static_cast<struct BandObject *>(CoTaskMemAlloc(8 * sizeof(struct BandObject)));
        if (fBands == NULL)
        {
            hRet = E_OUTOFMEMORY;
            goto Cleanup;
        }

        /* Initialize the added bands */
        memset(fBands, 0, 8 * sizeof(struct BandObject));

        fBandsAllocated += 8;
        NewBand = &fBands[0];
    }

    if (SUCCEEDED(hRet))
    {
        ASSERT(NewBand != NULL);

        fBandsCount++;
        NewBand->DeskBand = DeskBand.Detach();
        NewBand->OleWindow = OleWindow.Detach();
        NewBand->WndEvtHandler = WndEvtHandler.Detach();

        /* Create the ReBar band */
        hRet = ObjWithSite->SetSite(static_cast<IOleWindow *>(this));
        if (SUCCEEDED(hRet))
        {
            uBand = 0xffffffff;
            if (SUCCEEDED(UpdateSingleBand(NewBand)))
            {
                if (NewBand->dbi.dwMask & DBIM_MODEFLAGS)
                {
                    if (NewBand->dbi.dwModeFlags & DBIMF_ADDTOFRONT)
                        uBand = 0;
                }
            }

            BuildRebarBandInfo(NewBand, &rbi);

            if (SUCCEEDED(NewBand->OleWindow->GetWindow(&rbi.hwndChild)) &&
                rbi.hwndChild != NULL)
            {
                rbi.fMask |= RBBIM_CHILD;
                WARN ("ReBar band uses child window 0x%p\n", rbi.hwndChild);
            }

            if (!SendMessageW(fRebarWindow, RB_INSERTBANDW, (WPARAM)uBand, reinterpret_cast<LPARAM>(&rbi)))
                return E_FAIL;

            hRet = (HRESULT)((USHORT)GetBandID(NewBand));
        }
        else
        {
            WARN("IBandSite::AddBand(): Call to IDeskBand::SetSite() failed: %x\n", hRet);

            /* Remove the band from the ReBar control */
            BuildRebarBandInfo(NewBand, &rbi);
            uBand = (UINT)SendMessageW(fRebarWindow, RB_IDTOINDEX, (WPARAM)rbi.wID, 0);
            if (uBand != (UINT)-1)
            {
                if (!SendMessageW(fRebarWindow, RB_DELETEBAND, (WPARAM)uBand, 0))
                {
                    ERR("Failed to delete band!\n");
                }
            }
            else
                ERR("Failed to map band id to index!\n");

            FreeBand(NewBand);

            hRet = E_FAIL;
            /* goto Cleanup; */
        }
    }
Cleanup:
    return hRet;
}

HRESULT STDMETHODCALLTYPE CBandSiteBase::EnumBands(UINT uBand, DWORD *pdwBandID)
{
    DWORD                                   i;

    TRACE("(%p, %u, %p)\n", this, uBand, pdwBandID);

    if (uBand == 0xffffffff)
        return (UINT)fBandsCount;

    if (uBand >= (UINT)fBandsCount)
        return E_FAIL;

    for (i = 0; i < (DWORD)fBandsAllocated; i++)
    {
        if (fBands[i].DeskBand != NULL)
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

    Band = GetBandByID(dwBandID);
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

    Band = GetBandByID(dwBandID);
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

    if (fRebarWindow == NULL)
        return E_FAIL;

    Band = GetBandByID(dwBandID);
    if (Band == NULL)
        return E_FAIL;

    uBand = (UINT)SendMessageW(fRebarWindow, RB_IDTOINDEX, (WPARAM)GetBandID(Band), 0);
    if (uBand != (UINT)-1)
    {
        if (!SendMessageW(fRebarWindow, RB_DELETEBAND, (WPARAM)uBand, 0))
        {
            ERR("Could not delete band!\n");
        }
    }
    else
        ERR("Could not map band id to index!\n");

    FreeBand(Band);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CBandSiteBase::GetBandObject(DWORD dwBandID, REFIID riid, VOID **ppv)
{
    struct BandObject                       *Band;

    TRACE("(%p, %u, %s, %p)\n", this, dwBandID, debugstr_guid(&riid), ppv);

    Band = GetBandByID(dwBandID);
    if (Band == NULL)
    {
        *ppv = NULL;
        return E_FAIL;
    }

    return Band->DeskBand->QueryInterface(riid, ppv);
}

HRESULT STDMETHODCALLTYPE CBandSiteBase::SetBandSiteInfo(const BANDSITEINFO *pbsinfo)
{
    FIXME("(%p, %p)\n", this, pbsinfo);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBandSiteBase::GetBandSiteInfo(BANDSITEINFO *pbsinfo)
{
    FIXME("(%p, %p)\n", this, pbsinfo);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBandSiteBase::OnWinEvent(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plrResult)
{
    struct BandObject                       *Band;

    TRACE("(%p, %p, %u, %p, %p, %p)\n", this, hWnd, uMsg, wParam, lParam, plrResult);

    *plrResult = 0;
    if (fRebarWindow == NULL)
        return E_FAIL;

    if (hWnd == fRebarWindow)
    {
        /* FIXME: Just send the message? */
        *plrResult = SendMessageW(hWnd, uMsg, wParam, lParam);
        return S_OK;
    }

    Band = GetBandFromHwnd(hWnd);
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

    if (fRebarWindow == NULL)
        return E_FAIL;

    Band = GetBandFromHwnd(hWnd);
    if (Band != NULL)
        return S_OK;

    return S_FALSE;
}

HRESULT STDMETHODCALLTYPE CBandSiteBase::GetWindow(HWND *phWnd)
{
    TRACE("(%p, %p)\n", this, phWnd);

    *phWnd = fRebarWindow;
    if (fRebarWindow != NULL)
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

    fOleWindow.Release();

    hRet = pUnk->QueryInterface(IID_PPV_ARG(IOleWindow, &fOleWindow));
    if (FAILED_UNEXPECTEDLY(hRet))
        return E_FAIL;

    hRet = fOleWindow->GetWindow(&hWndParent);
    if (FAILED_UNEXPECTEDLY(hRet))
        return E_FAIL;

    style = WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | RBS_VARHEIGHT | RBS_AUTOSIZE |
        RBS_BANDBORDERS | CCS_NODIVIDER | CCS_NORESIZE | CCS_NOPARENTALIGN;

    fRebarWindow = CreateWindowExW(WS_EX_TOOLWINDOW,
                                   REBARCLASSNAMEW,
                                   NULL,
                                   style,
                                   0, 0, 0, 0,
                                   hWndParent,
                                   NULL,
                                   _AtlBaseModule.GetModuleInstance(),
                                   NULL);
    if (fRebarWindow == NULL)
    {
        fOleWindow.Release();
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

    if (fRebarWindow == NULL)
        return E_FAIL;

    dwStyle = dwPrevStyle = GetWindowLongPtr(fRebarWindow, GWL_STYLE);
    if (dwMode & DBIF_VIEWMODE_VERTICAL)
        dwStyle |= CCS_VERT;

    if (dwMode & ~DBIF_VIEWMODE_VERTICAL)
        FIXME("IDeskBarClient::SetModeDBC() unhandled modes: %x\n", dwStyle & ~DBIF_VIEWMODE_VERTICAL);

    if (dwStyle != dwPrevStyle)
    {
        SetWindowLongPtr(fRebarWindow, GWL_STYLE, dwPrevStyle);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CBandSiteBase::UIActivateDBC(DWORD dwState)
{
    TRACE("(%p, %x)\n", this, dwState);

    if (fRebarWindow == NULL)
        return E_FAIL;

    ShowWindow(fRebarWindow, (dwState & DBC_SHOW) ? SW_SHOW : SW_HIDE);
    FIXME("IDeskBarClient::UIActivateDBC() Properly notify bands?\n");
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

    if (fRebarWindow == NULL)
        return E_FAIL;

    if (IsEqualIID(*pguidCmdGroup, IID_IDeskBand))
    {
        switch (nCmdID)
        {
            case DBID_BANDINFOCHANGED:
                if (pvaIn == NULL)
                    hRet = UpdateAllBands();
                else
                {
                    /* Update a single band */
                    if (pvaIn->n1.n2.vt == VT_I4)
                        hRet = UpdateBand(pvaIn->n1.n2.n3.lVal);
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
HRESULT WINAPI CBandSite_CreateInstance(LPUNKNOWN pUnkOuter, REFIID riid, void **ppv)
{
    return CBandSite::_CreatorClass::CreateInstance(pUnkOuter, riid, ppv);
}
