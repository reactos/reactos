/*
 * ReactOS shlwapi
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

#ifndef __SHLWAPI_UNDOC_H
#define __SHLWAPI_UNDOC_H

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

struct IEThreadParamBlock
{
	long							offset0;
	long							offset4;
	long							offset8;
	IUnknown						*offsetC;
	long							offset10;
	IUnknown						*offset14;
	LPITEMIDLIST					directoryPIDL;
	char							filler1[84];	// unknown contents
	IUnknown						*offset70;
	long							filler2;		// unknown contents
	IUnknown						*offset78;
	LPITEMIDLIST					offset7C;
	LPITEMIDLIST					offset80;
	char							filler3[116];	// unknown contents
	IUnknown						*offsetF8;		// instance explorer
	long							filler4;		// unknown contents
};

void WINAPI InitOCHostClass(long param8);
long WINAPI SHOpenFolderWindow(IEThreadParamBlock *param8);
void WINAPI SHCreateSavedWindows(void);
long WINAPI SHCreateFromDesktop(long param8);
long WINAPI SHExplorerParseCmdLine(LPCTSTR commandLine);
void WINAPI UEMRegisterNotify(long param8, long paramC);
HRESULT WINAPI SHCreateBandForPidl(LPCITEMIDLIST param8, IUnknown *paramC, BOOL param10);
HRESULT WINAPI SHPidlFromDataObject(IDataObject *param8, long *paramC, long param10, FILEDESCRIPTORW *param14);
long WINAPI IDataObject_GetDeskBandState(long param8);
IEThreadParamBlock *WINAPI SHCreateIETHREADPARAM(long param8, long paramC, IUnknown *param10, IUnknown *param14);
IEThreadParamBlock *WINAPI SHCloneIETHREADPARAM(IEThreadParamBlock *param);
long WINAPI SHParseIECommandLine(long param8, long paramC);
void WINAPI SHDestroyIETHREADPARAM(IEThreadParamBlock *param);
HRESULT WINAPI SHOnCWMCommandLine(long param8);
LPITEMIDLIST WINAPI Channel_GetFolderPidl(void);
IUnknown *WINAPI ChannelBand_Create(LPITEMIDLIST pidl);
HRESULT WINAPI Channels_SetBandInfoSFB(IUnknown *param8);
HRESULT WINAPI IUnknown_SetBandInfoSFB(IUnknown *param8, long paramC);
HRESULT WINAPI Channel_QuickLaunch(void);
HRESULT WINAPI SHGetNavigateTarget(long param8, long paramC, long param10, long param14);
HRESULT WINAPI GetInfoTip(IUnknown *param8, long paramC, LPTSTR *param10, long cchMax);
HRESULT WINAPI SHEnumClassesOfCategories(long param8, long paramC, long param10, long param14, long param18);
HRESULT WINAPI SHWriteClassesOfCategories(long param8, long paramC, long param10, long param14, long param18, long param1C, long param20);
BOOL WINAPI SHIsExplorerBrowser();
HRESULT WINAPI SHOpenNewFrame(LPITEMIDLIST pidl, IUnknown *paramC, long param10, long param14);

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* __SHLWAPI_UNDOC_H */
