//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// globals.h 
//
//   Contains defines for all of the global variables used in cdfview.
//
//   History:
//
//       3/16/97  edwardp   Created.
//
////////////////////////////////////////////////////////////////////////////////

//
// Check for previous includes of this file.
//

#ifndef _GLOBALS_H_

#define _GLOBALS_H_

//
// Extern global variable declarations.
//

extern DWORD               g_dwCacheCount;
extern HINSTANCE           g_msxmlInst;
#ifndef UNIX
/* Unix does not use webcheck */
extern HINSTANCE           g_webcheckInst;
#endif /* UNIX */
extern HINSTANCE           g_hinst;
extern ULONG               g_cDllRef;
extern PCACHEITEM          g_pCache;
extern CRITICAL_SECTION    g_csCache;
extern TCHAR               g_szModuleName[MAX_PATH];
extern BOOL                g_bRunningOnNT;

extern const GUID   CLSID_CDFVIEW;
extern const GUID   CLSID_CDFINI;
extern const GUID   CLSID_CDFICONHANDLER;
extern const GUID   CLSID_CDFMENUHANDLER;
extern const GUID   CLSID_CDFPROPPAGES;

extern const TCHAR c_szChannel[];
extern const TCHAR c_szCDFURL[];
extern const TCHAR c_szHotkey[];
extern const TCHAR c_szDesktopINI[];
extern const TCHAR c_szScreenSaverURL[];

extern const WCHAR c_szPropCrawlActualSize[];
extern const WCHAR c_szPropStatusString[];
extern const WCHAR c_szPropCompletionTime[];

extern const TCHAR c_szHICKey[];
extern const TCHAR c_szHICVal[];

//
// Trace flag definitions
//

#define TF_CDFPARSE       0x00000010
#define TF_CDFICON        0x00000020
#define TF_CDFLOGO        0x00000040
#define TF_CDFENUM        0x00000080
#define TF_OBJECTS        0x00000100
#define TF_GLEAM          0x00000200
#define TF_THUNK          0x00000400

#endif // _GLOBALS_H_
