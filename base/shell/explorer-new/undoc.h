#pragma once

/*
 * Undocumented stuff
 */

/* IMenuDeskBar provides the band site toolbars menu */
static const CLSID CLSID_HACK_IShellBandSiteMenu = {0xECD4FC4E,0x521C,0x11D0,{0xB7,0x92,0x00,0xA0,0xC9,0x03,0x12,0xE1}};
#define CLSID_IShellBandSiteMenu CLSID_HACK_IShellBandSiteMenu

static const GUID IID_HACK_IBandSiteStreamCallback = {0xD1E7AFEA,0x6A2E,0x11D0,{0x8C,0x78,0x00,0xC0,0x4F,0xD9,0x18,0xB4}};
#define IID_IBandSiteStreamCallback IID_HACK_IBandSiteStreamCallback

static const GUID CLSID_HACK_StartMenu = {0x4622AD11,0xFF23,0x11D0,{0x8D,0x34,0x00,0xA0,0xC9,0x0F,0x27,0x19}};
#define CLSID_StartMenu CLSID_HACK_StartMenu
static const GUID CLSID_HACK_PersonalStartMenu = {0x3F6953F0,0x5359,0x47FC,{0xBD,0x99,0x9F,0x2C,0xB9,0x5A,0x62,0xFD}};
#define CLSID_PersonalStartMenu CLSID_HACK_PersonalStartMenu

static const GUID IID_HACK_IMenuBand = {0x568804CD,0xCBD7,0x11D0,{0x98,0x16,0x00,0xC0,0x4F,0xD9,0x19,0x72}};
#define IID_IMenuBand IID_HACK_IMenuBand

#define WM_GETISHELLBROWSER (WM_USER+7)
BOOL WINAPI SetShellWindow(HWND);
BOOL WINAPI SetShellWindowEx(HWND, HWND);
BOOL WINAPI RegisterShellHook(HWND, DWORD);
IStream* WINAPI SHGetViewStream(LPCITEMIDLIST, DWORD, LPCTSTR, LPCTSTR, LPCTSTR);
BOOL WINAPI SHIsEmptyStream(IStream*);

#define MRU_BINARY  0x1
#define MRU_CACHEWRITE  0x2

HANDLE WINAPI CreateMRUListW(LPCREATEMRULISTW);
HANDLE WINAPI CreateMRUListA(LPCREATEMRULISTA);
INT WINAPI AddMRUData(HANDLE,LPCVOID,DWORD);
INT WINAPI FindMRUData(HANDLE,LPCVOID,DWORD,LPINT);
VOID WINAPI FreeMRUList(HANDLE);

#define DC_NOSENDMSG 0x2000
BOOL WINAPI DrawCaptionTempA(HWND,HDC,const RECT*,HFONT,HICON,LPCSTR,UINT);
BOOL WINAPI DrawCaptionTempW(HWND,HDC,const RECT*,HFONT,HICON,LPCWSTR,UINT);

#ifdef UNICODE
#define CreateMRUList   CreateMRUListW
#define DrawCaptionTemp DrawCaptionTempW
#else
#define CreateMRUList   CreateMRUListA
#define DrawCaptionTemp DrawCaptionTempA
#endif

EXTERN_C const GUID CLSID_RebarBandSite;

HRESULT WINAPI SHInvokeDefaultCommand(HWND,IShellFolder*,LPCITEMIDLIST);

HRESULT WINAPI SHPropertyBag_ReadPOINTL(IPropertyBag*,LPCWSTR,POINTL*);
