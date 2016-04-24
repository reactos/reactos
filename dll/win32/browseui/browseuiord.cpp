/*
 * ReactOS browseui
 *
 * Copyright 2009 Andrew Hill <ash77 at domain reactos.org>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "precomp.h"

/*************************************************************************
 * InitOCHostClass				[BROWSEUI.101]
 */
extern "C" void WINAPI InitOCHostClass(long param8)
{
    // forwards to shdocvw
}

/*************************************************************************
 * SHCreateSavedWindows			[BROWSEUI.105]
 * Called to recreate explorer windows from previous session
 */
extern "C" void WINAPI SHCreateSavedWindows()
{
}

/*************************************************************************
 * SHExplorerParseCmdLine		[BROWSEUI.107]
 */
/****** MOVED TO parsecmdline.cpp ******/

/*************************************************************************
 * UEMRegisterNotify			[BROWSEUI.118]
 */
extern "C" void WINAPI UEMRegisterNotify(long param8, long paramC)
{
}

/*************************************************************************
 * SHCreateBandForPidl			[BROWSEUI.120]
 */
extern "C" HRESULT WINAPI SHCreateBandForPidl(LPCITEMIDLIST param8, IUnknown *paramC, BOOL param10)
{
    return E_NOTIMPL;
}

/*************************************************************************
 * SHPidlFromDataObject			[BROWSEUI.121]
 */
extern "C" HRESULT WINAPI SHPidlFromDataObject(IDataObject *param8, long *paramC, long param10, FILEDESCRIPTORW *param14)
{
    return E_NOTIMPL;
}

/*************************************************************************
 * IDataObject_GetDeskBandState	[BROWSEUI.122]
 */
extern "C" long WINAPI IDataObject_GetDeskBandState(long param8)
{
    return -1;
}

/*************************************************************************
 * SHParseIECommandLine			[BROWSEUI.125]
 */
extern "C" long WINAPI SHParseIECommandLine(long param8, long paramC)
{
    return -1;
}

/*************************************************************************
 * Channel_GetFolderPidl		[BROWSEUI.128]
 */
extern "C" LPITEMIDLIST WINAPI Channel_GetFolderPidl()
{
    return NULL;
}

/*************************************************************************
 * ChannelBand_Create			[BROWSEUI.129]
 */
extern "C" IUnknown *WINAPI ChannelBand_Create(LPITEMIDLIST pidl)
{
    return NULL;
}

/*************************************************************************
 * Channels_SetBandInfoSFB		[BROWSEUI.130]
 */
extern "C" HRESULT WINAPI Channels_SetBandInfoSFB(IUnknown *param8)
{
    return E_NOTIMPL;
}

/*************************************************************************
 * IUnknown_SetBandInfoSFB		[BROWSEUI.131]
 */
extern "C" HRESULT WINAPI IUnknown_SetBandInfoSFB(IUnknown *param8, long paramC)
{
    return E_NOTIMPL;
}

/*************************************************************************
 * Channel_QuickLaunch			[BROWSEUI.133]
 */
extern "C" HRESULT WINAPI Channel_QuickLaunch()
{
    return E_NOTIMPL;
}

/*************************************************************************
 * SHGetNavigateTarget			[BROWSEUI.134]
 */
extern "C" HRESULT WINAPI SHGetNavigateTarget(long param8, long paramC, long param10, long param14)
{
    return E_NOTIMPL;
}

/*************************************************************************
 * GetInfoTip					[BROWSEUI.135]
 */
extern "C" HRESULT WINAPI GetInfoTip(IUnknown *param8, long paramC, LPTSTR *param10, long cchMax)
{
    return E_NOTIMPL;
}

/*************************************************************************
 * SHEnumClassesOfCategories	[BROWSEUI.136]
 */
extern "C" HRESULT WINAPI SHEnumClassesOfCategories(long param8, long paramC, long param10, long param14, long param18)
{
    return E_NOTIMPL;
}

/*************************************************************************
 * SHWriteClassesOfCategories	[BROWSEUI.137]
 */
extern "C" HRESULT WINAPI SHWriteClassesOfCategories(long param8, long paramC, long param10,
    long param14, long param18, long param1C, long param20)
{
    return E_NOTIMPL;
}

/*************************************************************************
 * SHIsExplorerBrowser			[BROWSEUI.138]
 */
extern "C" BOOL WINAPI SHIsExplorerBrowser()
{
    return TRUE;
}
