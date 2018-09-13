#include <windows.h>
#include <ole2.h>
#include <olectl.h>

#include <atlbase.h>

//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>

#include <commctrl.h>
#include <gpedit.h>
#include <initguid.h>
#include <mmc.h>

class CSnapIn;

#include "resource.h"
#include "util.h"
#include "snapin.h"
#include "dataobj.h"
#include "dialogs.h"

//
// Global variables
//

extern LONG g_cRefThisDll;
extern HINSTANCE g_hInstance;

//
// Functions to create class factories
//

HRESULT CreateComponentDataClassFactory (REFCLSID rclsid, REFIID riid, LPVOID* ppv);


extern const GUID CLSID_SnapIn;
extern const TCHAR szNodeType[];
extern const GUID NodeType;
