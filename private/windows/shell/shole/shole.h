//Prevent windows.h from pulling in OLE 1
#define _INC_OLE

#include <windows.h>
#include <stdlib.h>
#include <shlobj.h>
#include <shellapi.h>
#include <shlobjp.h>
#include <ole2ver.h>
#include <debug.h>

#define SAVE_OBJECTDESCRIPTOR
#define FIX_ROUNDTRIP

#define CCF_CACHE_GLOBAL        32
#define CCF_CACHE_CLSID         32
#define CCF_RENDER_CLSID        32
#define CCFCACHE_TOTAL  (CC_FCACHE_GLOBAL+CCF_CACHE_CLSID+CCF_RENDER_CLSID)


HRESULT CScrapData_CreateInstance(LPUNKNOWN * ppunk);

//
// global variables
//
extern UINT g_cRefThisDll;              // per-instance
extern HINSTANCE g_hinst;
extern "C" const WCHAR c_wszContents[];
extern "C" const WCHAR c_wszDescriptor[];

#define HINST_THISDLL g_hinst



#define CFID_EMBEDDEDOBJECT     0
#define CFID_OBJECTDESCRIPTOR   1
#define CFID_LINKSRCDESCRIPTOR  2
#define CFID_RICHTEXT           3
#define CFID_SCRAPOBJECT        4
#define CFID_MAX                5

#define CF_EMBEDDEDOBJECT       _GetClipboardFormat(CFID_EMBEDDEDOBJECT)
#define CF_OBJECTDESCRIPTOR     _GetClipboardFormat(CFID_OBJECTDESCRIPTOR)
#define CF_LINKSRCDESCRIPTOR    _GetClipboardFormat(CFID_LINKSRCDESCRIPTOR)
#define CF_RICHTEXT             _GetClipboardFormat(CFID_RICHTEXT)
#define CF_SCRAPOBJECT          _GetClipboardFormat(CFID_SCRAPOBJECT)

UINT _GetClipboardFormat(UINT id);
void DisplayError(HWND hwndOwner, HRESULT hres, UINT idsMsg, LPCTSTR szFileName);
