//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// globals.cpp 
//
//   Contains *all* of the global variables used in cdfview.  Globals shouldn't
//   be declared elsewhere.
//
//   History:
//
//       3/16/97  edwardp   Created.
//
////////////////////////////////////////////////////////////////////////////////

//
// Includes
//

#include "stdinc.h"

//
// Global variables.
//

// Remove shared memory segment.  This must be removed so w2k can get a c2
// security rating.  It should no longer be rquired because the cache is 
// going to be limited to one item.
//
//#pragma data_seg("SharedData")
DWORD               g_dwCacheCount = 0;
//#pragma data_seg()

HINSTANCE           g_msxmlInst = NULL;
#ifndef UNIX
/* Unix does not use webcheck */
HINSTANCE           g_webcheckInst = NULL;
#endif /* UNIX */
HINSTANCE           g_hinst   = NULL;
ULONG               g_cDllRef = 0;
PCACHEITEM          g_pCache  = NULL;
CRITICAL_SECTION    g_csCache;
TCHAR               g_szModuleName[MAX_PATH];

const GUID  CLSID_CDFVIEW =
{0xf39a0dc0, 0x9cc8, 0x11d0, {0xa5, 0x99, 0x0, 0xc0, 0x4f, 0xd6, 0x44, 0x33}};
// {f39a0dc0-9cc8-11d0-a599-00c04fd64433}

const GUID  CLSID_CDFINI =
{0xf3aa0dc0, 0x9cc8, 0x11d0, {0xa5, 0x99, 0x0, 0xc0, 0x4f, 0xd6, 0x44, 0x34}};
// {f3aa0dc0-9cc8-11d0-a599-00c04fd64434}

const GUID  CLSID_CDFICONHANDLER =
{0xf3ba0dc0, 0x9cc8, 0x11d0, {0xa5, 0x99, 0x0, 0xc0, 0x4f, 0xd6, 0x44, 0x35}};
// {f3ba0dc0-9cc8-11d0-a599-00c04fd64435}

const GUID  CLSID_CDFMENUHANDLER =
{0xf3da0dc0, 0x9cc8, 0x11d0, {0xa5, 0x99, 0x0, 0xc0, 0x4f, 0xd6, 0x44, 0x37}};
// {f3da0dc0-9cc8-11d0-a599-00c04fd64437}

const GUID  CLSID_CDFPROPPAGES =
{0xf3ea0dc0, 0x9cc8, 0x11d0, {0xa5, 0x99, 0x0, 0xc0, 0x4f, 0xd6, 0x44, 0x38}};
// 

const TCHAR c_szChannel[] = TEXT("Channel");
const TCHAR c_szCDFURL[] = TEXT("CDFURL");
const TCHAR c_szHotkey[] = TEXT("Hotkey");
const TCHAR c_szDesktopINI[] = TEXT("desktop.ini");
const TCHAR c_szScreenSaverURL[] = TEXT("ScreenSaverURL");
const WCHAR c_szPropCrawlActualSize[] = L"ActualSizeKB";
const WCHAR c_szPropStatusString[] = L"StatusString";
const WCHAR c_szPropCompletionTime[] = L"CompletionTime";

//  From Plus! tab code
const TCHAR c_szHICKey[] = TEXT("Control Panel\\Desktop\\WindowMetrics"); // show icons using highest possible colors
const TCHAR c_szHICVal[] = TEXT("Shell Icon BPP"); // (4 if the checkbox is false, otherwise 16, don't set it to anything else)

