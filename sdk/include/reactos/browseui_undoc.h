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

#ifndef __BROWSEUI_UNDOC_H
#define __BROWSEUI_UNDOC_H

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

// Name is IETHREADPARAM according to symbols / mangled function names
#ifdef _WIN64
typedef struct IEThreadParamBlock
{
    long                            offset0;  // 0x00
    UCHAR gap4[4];
    DWORD                   dwFlags;          // 0x08
    long                            offset8;  // 0x0c
    IUnknown*                       offsetC;  // 0x10
    long                            offset10; // 0x18
    char padding1[4];                         // 0x1c
    IUnknown*                       offset14; // 0x20
    LPITEMIDLIST            directoryPIDL;    // 0x28
    WCHAR awsz_30[21];                        // 0x30
    WCHAR awszEventName[21];                  // 0x5A
    char padding2[4];                         // 0x84
    IUnknown*                       offset70; // 0x88
    long                            offset74; // 0x90 unknown contents
    char padding3[4];                         // 0x94
    IUnknown*                       offset78; // 0x98
    LPITEMIDLIST                    offset7C; // 0xa0
    LPITEMIDLIST                    offset80; // 0xa8
    LONG                            offset84; // 0xb0
    LONG                            offset88; // 0xb4
    LONG                            offset8C; // 0xb8
    LONG                            offset90; // 0xbc
    LONG                            offset94; // 0xc0
    LONG                            offset98; // 0xc4
    LONG                            offset9C; // 0xc8
    LONG                            offsetA0; // 0xcc
    char field_B4[52];
    LONG                            offsetD8; // 0x104
    UCHAR gap108[24];
    DWORD dword120;
    DWORD dword124;
    IUnknown*                       offsetF8; // 0x128 instance explorer
    UCHAR byteflags_130;
} IE_THREAD_PARAM_BLOCK, * PIE_THREAD_PARAM_BLOCK;
#else
typedef struct IEThreadParamBlock
{
    long                            offset0;
    DWORD                   dwFlags;
    long                            offset8;
    IUnknown                      * offsetC;
    long                            offset10;
    IUnknown                      * offset14;
    LPITEMIDLIST            directoryPIDL;      // 0x18
    char                            offset1C[0x70-0x1C];    // unknown contents -- 0x1C..0x6c
    IUnknown                      * offset70;
    long                            offset74;        // unknown contents
    IUnknown                      * offset78;
    LPITEMIDLIST                    offset7C;
    LPITEMIDLIST                    offset80;
    LONG                            offset84;
    LONG                            offset88;
    LONG                            offset8C;
    LONG                            offset90;
    LONG                            offset94;
    LONG                            offset98;
    LONG                            offset9C;
    LONG                            offsetA0;
    char                            offsetA4[0xD8-0xA4];    // unknown contents -- 0xA4..0xD8
    LONG                            offsetD8;
    char                            offsetDC[0xF8-0xDC];    // unknown contents -- 0xDC..0xF8
    IUnknown                      * offsetF8;        // instance explorer
    LONG                            offsetFC;        // unknown contents
} IE_THREAD_PARAM_BLOCK, *PIE_THREAD_PARAM_BLOCK;
#endif

typedef struct ExplorerCommandLineParseResults
{
    LPWSTR                  strPath;
    // TODO: PIDLIST_ABSOLUTE?
    LPITEMIDLIST            pidlPath;
    DWORD                   dwFlags;
    int                     nCmdShow;
    DWORD                   offset10_18;
    DWORD                   offset14_1C;
    DWORD                   offset18_20;
    DWORD                   offset1C_24;
    // TODO: PIDLIST_ABSOLUTE?
    LPITEMIDLIST            pidlRoot;
    CLSID                   clsid;
    GUID                    guidInproc;
    // TODO: 'ULONG                   Padding[0x100];'?
} EXPLORER_CMDLINE_PARSE_RESULTS, *PEXPLORER_CMDLINE_PARSE_RESULTS;

#define SH_EXPLORER_CMDLINE_FLAG_ONE      0x00000001
#define SH_EXPLORER_CMDLINE_FLAG_S        0x00000002
// unknown/unused                         0x00000004
#define SH_EXPLORER_CMDLINE_FLAG_E        0x00000008
// unknown/unused                         0x00000010
// unknown/unused                         0x00000020
#define SH_EXPLORER_CMDLINE_FLAG_SELECT   0x00000040
#define SH_EXPLORER_CMDLINE_FLAG_EMBED    0x00000080
// unknown/unused                         0x00000100
#define SH_EXPLORER_CMDLINE_FLAG_IDLIST   0x00000200
#define SH_EXPLORER_CMDLINE_FLAG_INPROC   0x00000400
// unknown/unused                         0x00000800
#define SH_EXPLORER_CMDLINE_FLAG_NOUI     0x00001000
// unknown/unused                         0x00002000
#define SH_EXPLORER_CMDLINE_FLAG_N        0x00004000
// unknown/unused                         0x00008000
// unknown/unused                         0x00010000
#define SH_EXPLORER_CMDLINE_FLAG_SEPARATE 0x00020000
// unknown/unused                         0x00040000
// unknown/unused                         0x00080000
// unknown/unused                         0x00100000
// unknown/unused                         0x00200000
// unknown/unused                         0x00400000
// unknown/unused                         0x00800000
// unknown/unused                         0x01000000
#define SH_EXPLORER_CMDLINE_FLAG_STRING   0x02000000

#define WM_EXPLORER_OPEN_NEW_WINDOW (WM_USER+11)
#define WM_EXPLORER_1037 (WM_USER+13)

void WINAPI InitOCHostClass(long param8);
long WINAPI SHOpenFolderWindow(PIE_THREAD_PARAM_BLOCK parameters);
void WINAPI SHCreateSavedWindows(void);
BOOL WINAPI SHCreateFromDesktop(_In_ PEXPLORER_CMDLINE_PARSE_RESULTS parseResults);
UINT_PTR WINAPI SHExplorerParseCmdLine(_Out_ PEXPLORER_CMDLINE_PARSE_RESULTS pInfo);
void WINAPI UEMRegisterNotify(long param8, long paramC);
HRESULT WINAPI SHCreateBandForPidl(LPCITEMIDLIST param8, IUnknown *paramC, BOOL param10);
HRESULT WINAPI SHPidlFromDataObject(IDataObject *param8, long *paramC, long param10, FILEDESCRIPTORW *param14);
long WINAPI IDataObject_GetDeskBandState(long param8);
PIE_THREAD_PARAM_BLOCK WINAPI SHCreateIETHREADPARAM(long param8, long paramC, IUnknown *param10, IUnknown *param14);
PIE_THREAD_PARAM_BLOCK WINAPI SHCloneIETHREADPARAM(PIE_THREAD_PARAM_BLOCK param);
long WINAPI SHParseIECommandLine(long param8, long paramC);
void WINAPI SHDestroyIETHREADPARAM(PIE_THREAD_PARAM_BLOCK param);
BOOL WINAPI SHOnCWMCommandLine(HANDLE hSharedInfo);
LPITEMIDLIST WINAPI Channel_GetFolderPidl(void);
IUnknown *WINAPI ChannelBand_Create(LPITEMIDLIST pidl);
HRESULT WINAPI Channels_SetBandInfoSFB(IUnknown *param8);
HRESULT WINAPI IUnknown_SetBandInfoSFB(IUnknown *param8, long paramC);
HRESULT WINAPI Channel_QuickLaunch(void);
HRESULT WINAPI SHGetNavigateTarget(long param8, long paramC, long param10, long param14);
HRESULT WINAPI GetInfoTip(IUnknown *param8, long paramC, LPTSTR *param10, long cchMax);
HRESULT WINAPI SHEnumClassesOfCategories(ULONG cImplemented, CATID *pImplemented, ULONG cRequired, CATID *pRequired, IEnumGUID **out);
HRESULT WINAPI SHWriteClassesOfCategories(long param8, long paramC, long param10, long param14, long param18, long param1C, long param20);
BOOL WINAPI SHIsExplorerBrowser(void);
HRESULT WINAPI SHOpenNewFrame(LPITEMIDLIST pidl, IUnknown *paramC, long param10, DWORD dwFlags);


#define INTERFACE IACLCustomMRU
DECLARE_INTERFACE_IID_(IACLCustomMRU, IUnknown, "F729FC5E-8769-4F3E-BDB2-D7B50FD2275B")
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, void **ppv) PURE;
    STDMETHOD_(ULONG, AddRef) (THIS)PURE;
    STDMETHOD_(ULONG, Release) (THIS)PURE;

    // *** IACLCustomMRU specific methods ***
    STDMETHOD(Initialize) (THIS_ LPCWSTR pwszMRURegKey, DWORD dwMax) PURE;
    STDMETHOD(AddMRUString) (THIS_ LPCWSTR pwszEntry) PURE;
};
#undef INTERFACE

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* __BROWSEUI_UNDOC_H */
