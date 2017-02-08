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

extern DWORD WINAPI BrowserThreadProc(LPVOID lpThreadParameter);

/*************************************************************************
 * InitOCHostClass				[BROWSEUI.101]
 */
extern "C" void WINAPI InitOCHostClass(long param8)
{
    // forwards to shdocvw
}

/*************************************************************************
 * SHOpenFolderWindow			[BROWSEUI.102]
 */
extern "C" long WINAPI SHOpenFolderWindow(IEThreadParamBlock *param8)
{
    return 0;
}

/*************************************************************************
 * SHCreateSavedWindows			[BROWSEUI.105]
 * Called to recreate explorer windows from previous session
 */
extern "C" void WINAPI SHCreateSavedWindows()
{
}

/*************************************************************************
 * SHCreateFromDesktop			[BROWSEUI.106]
 * parameter is a FolderInfo
 */
extern "C" long WINAPI SHCreateFromDesktop(long param8)
{
    return -1;
}

/*************************************************************************
 * SHExplorerParseCmdLine		[BROWSEUI.107]
 */
extern "C" long WINAPI SHExplorerParseCmdLine(LPCTSTR commandLine)
{
    return -1;
}

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
 * SHCreateIETHREADPARAM		[BROWSEUI.123]
 */
extern "C" IEThreadParamBlock *WINAPI SHCreateIETHREADPARAM(
    long param8, long paramC, IUnknown *param10, IUnknown *param14)
{
    IEThreadParamBlock                      *result;

    result = (IEThreadParamBlock *)LocalAlloc(LMEM_ZEROINIT, 256);
    if (result == NULL)
        return NULL;
    result->offset0 = param8;
    result->offset8 = paramC;
    result->offsetC = param10;
    if (param10 != NULL)
        param10->AddRef();
    result->offset14 = param14;
    if (param14 != NULL)
        param14->AddRef();
    return result;
}

/*************************************************************************
 * SHCloneIETHREADPARAM			[BROWSEUI.124]
 */
extern "C" IEThreadParamBlock *WINAPI SHCloneIETHREADPARAM(IEThreadParamBlock *param)
{
    IEThreadParamBlock                      *result;

    result = (IEThreadParamBlock *)LocalAlloc(LMEM_FIXED, 256);
    if (result == NULL)
        return NULL;
    memcpy(result, param, 0x40 * 4);
    if (result->directoryPIDL != NULL)
        result->directoryPIDL = ILClone(result->directoryPIDL);
    if (result->offset7C != NULL)
        result->offset7C = ILClone(result->offset7C);
    if (result->offset80 != NULL)
        result->offset80 = ILClone(result->offset80);
    if (result->offset70 != NULL)
        result->offset70->AddRef();
#if 0
    if (result->offsetC != NULL)
        result->offsetC->Method2C();
#endif
    return result;
}

/*************************************************************************
 * SHParseIECommandLine			[BROWSEUI.125]
 */
extern "C" long WINAPI SHParseIECommandLine(long param8, long paramC)
{
    return -1;
}

/*************************************************************************
 * SHDestroyIETHREADPARAM		[BROWSEUI.126]
 */
extern "C" void WINAPI SHDestroyIETHREADPARAM(IEThreadParamBlock *param)
{
    if (param == NULL)
        return;
    if (param->directoryPIDL != NULL)
        ILFree(param->directoryPIDL);
    if (param->offset7C != NULL)
        ILFree(param->offset7C);
    if ((param->offset4 & 0x80000) == 0 && param->offset80 != NULL)
        ILFree(param->offset80);
    if (param->offset14 != NULL)
        param->offset14->Release();
    if (param->offset70 != NULL)
        param->offset70->Release();
    if (param->offset78 != NULL)
        param->offset78->Release();
    if (param->offsetC != NULL)
        param->offsetC->Release();
    if (param->offsetF8 != NULL)
        param->offsetF8->Release();
    LocalFree(param);
}

/*************************************************************************
 * SHOnCWMCommandLine			[BROWSEUI.127]
 */
extern "C" HRESULT WINAPI SHOnCWMCommandLine(long param8)
{
    return E_NOTIMPL;
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

// 75FA56C1h
// (pidl, 0, -1, 1)
// this function should handle creating a new process if needed, but I'm leaving that out for now
// this function always opens a new window - it does NOT check for duplicates
/*************************************************************************
 * SHOpenNewFrame				[BROWSEUI.103]
 */
extern "C" HRESULT WINAPI SHOpenNewFrame(LPITEMIDLIST pidl, IUnknown *paramC, long param10, long param14)
{
    IEThreadParamBlock                      *parameters;
    HANDLE                                  threadHandle;
    DWORD                                   threadID;

    parameters = SHCreateIETHREADPARAM(0, 1, paramC, NULL);
    if (parameters == NULL)
    {
        ILFree(pidl);
        return E_OUTOFMEMORY;
    }
    if (paramC != NULL)
        parameters->offset10 = param10;
    parameters->directoryPIDL = pidl;
    parameters->offset4 = param14;
    threadHandle = CreateThread(NULL, 0x10000, BrowserThreadProc, parameters, 0, &threadID);
    if (threadHandle != NULL)
    {
        CloseHandle(threadHandle);
        return S_OK;
    }
    SHDestroyIETHREADPARAM(parameters);
    return E_FAIL;
}
