#include <windows.h>
#include <reactos/debug.h>

#define ATLAPI_(x) x WINAPI
#define ATLAPI ATLAPI_(HRESULT)

struct _ATL_CATMAP_ENTRY;
typedef struct _ATL_PROPMAP_ENTRY ATL_PROPMAP_ENTRY;
typedef struct _ATL_MODULE _ATL_MODULE;

BOOL
WINAPI
AtlWaitWithMessageLoop(
   HANDLE hEvent)
{
    UNIMPLEMENTED;
	return FALSE;
}

ATLAPI
AtlSetErrorInfo(
    const CLSID *pclsid,
    LPCOLESTR lpszDesc,
    DWORD dwHelpID,
    LPCOLESTR lpszHelpFile,
    const IID *piid,
    HRESULT hRes,
    HINSTANCE hInst)
{
    UNIMPLEMENTED;
	return E_NOTIMPL;
}

ATLAPI_(LPDEVMODEA)
AtlDevModeW2A(LPDEVMODEA lpDevModeA, LPDEVMODEW lpDevModeW)
{
    UNIMPLEMENTED;
	return NULL;
}

ATLAPI_(DWORD)
AtlGetVersion(void* pReserved)
{
    UNIMPLEMENTED;
	return 0;
}

ATLAPI_(int)
AtlAxDialogBoxW(
    HINSTANCE hInstance,
    LPCWSTR lpTemplateName,
    HWND hWndParent,
    DLGPROC lpDialogProc,
    LPARAM dwInitParam)
{
    UNIMPLEMENTED;
	return 0;
}

ATLAPI_(int)
AtlAxDialogBoxA(
    HINSTANCE hInstance,
    LPCSTR lpTemplateName,
    HWND hWndParent,
    DLGPROC lpDialogProc,
    LPARAM dwInitParam)
{
    UNIMPLEMENTED;
	return 0;
}

ATLAPI_(int)
AtlRegisterClassCategoriesHelper(
    const CLSID *pclsid,
    const struct _ATL_CATMAP_ENTRY* pEntry,
    BOOL arg3)
{
    UNIMPLEMENTED;
	return 0;
}

ATLAPI
AtlIPersistPropertyBag_Load(
    LPPROPERTYBAG pPropBag,
    LPERRORLOG pErrorLog,
    const ATL_PROPMAP_ENTRY* pMap,
    void* pThis,
    IUnknown* pUnk)
{
    UNIMPLEMENTED;
	return E_NOTIMPL;
}

ATLAPI
AtlIPersistPropertyBag_Save(
    LPPROPERTYBAG pPropBag,
    BOOL fClearDirty,
    BOOL fSaveAllProperties,
    const ATL_PROPMAP_ENTRY* pMap,
    void* pThis,
    IUnknown* pUnk)
{
    UNIMPLEMENTED;
	return E_NOTIMPL;
}

ATLAPI
AtlGetObjectSourceInterface(
   IUnknown* punkObj,
   GUID* plibid,
   IID* piid,
   unsigned short* pdwMajor,
   unsigned short* pdwMinor)
{
    UNIMPLEMENTED;
	return E_NOTIMPL;
}

ATLAPI
AtlModuleUnregisterTypeLib(
   _ATL_MODULE* pM,
   LPCOLESTR lpszIndex)
{
    UNIMPLEMENTED;
	return E_NOTIMPL;
}

