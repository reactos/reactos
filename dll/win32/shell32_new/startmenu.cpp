/*
 *	Start menu object
 *
 *	Copyright 2009 Andrew Hill
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

#include <precomp.h>
#include <tchar.h>
#include <atlbase.h>
#include <atlcom.h>
#include <atlwin.h>
#include "startmenu.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell32start);

CStartMenuCallback::CStartMenuCallback()
{
}

CStartMenuCallback::~CStartMenuCallback()
{
}

HRESULT STDMETHODCALLTYPE CStartMenuCallback::SetSite(IUnknown *pUnkSite)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CStartMenuCallback::GetSite(REFIID riid, void **ppvSite)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CStartMenuCallback::CallbackSM(LPSMDATA psmd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return E_NOTIMPL;
}

CMenuBandSite::CMenuBandSite()
{
}

CMenuBandSite::~CMenuBandSite()
{
}

HRESULT STDMETHODCALLTYPE CMenuBandSite::GetWindow(HWND *phwnd)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMenuBandSite::ContextSensitiveHelp(BOOL fEnterMode)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMenuBandSite::SetDeskBarSite(IUnknown *punkSite)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMenuBandSite::SetModeDBC(DWORD dwMode)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMenuBandSite::UIActivateDBC(DWORD dwState)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMenuBandSite::GetSize(DWORD dwWhich, LPRECT prc)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMenuBandSite::UIActivateIO(BOOL fActivate, LPMSG lpMsg)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMenuBandSite::HasFocusIO()
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMenuBandSite::TranslateAcceleratorIO(LPMSG lpMsg)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMenuBandSite::OnFocusChangeIS(IUnknown *punkObj, BOOL fSetFocus)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMenuBandSite::OnWinEvent(HWND paramC, UINT param10, WPARAM param14, LPARAM param18, LRESULT *param1C)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMenuBandSite::IsWindowOwner(HWND paramC)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMenuBandSite::QueryService(REFGUID guidService, REFIID riid, void **ppvObject)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMenuBandSite::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[  ], OLECMDTEXT *pCmdText)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMenuBandSite::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMenuBandSite::AddBand(IUnknown *punk)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMenuBandSite::EnumBands(UINT uBand, DWORD *pdwBandID)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMenuBandSite::QueryBand(DWORD dwBandID, IDeskBand **ppstb, DWORD *pdwState, LPWSTR pszName, int cchName)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMenuBandSite::SetBandState(DWORD dwBandID, DWORD dwMask, DWORD dwState)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMenuBandSite::RemoveBand(DWORD dwBandID)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMenuBandSite::GetBandObject(DWORD dwBandID, REFIID riid, VOID **ppv)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMenuBandSite::SetBandSiteInfo(const BANDSITEINFO *pbsinfo)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMenuBandSite::GetBandSiteInfo(BANDSITEINFO *pbsinfo)
{
	return E_NOTIMPL;
}
