#include "priv.h"
#include "atl.h"
#include "nscoc.h"
#include "srchasst.h"

//ATL support
CComModule _Module;         // ATL module object

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_ShellFavoritesNameSpace, CShellFavoritesNameSpace)
    OBJECT_ENTRY(CLSID_SearchAssistantOC, CSearchAssistantOC)
END_OBJECT_MAP()

STDAPI_(void) AtlInit(HINSTANCE hinst)
{
    _Module.Init(ObjectMap, hinst);
}

STDAPI_(void) AtlTerm()
{
    _Module.Term();
}

STDAPI AtlGetClassObject(REFCLSID rclsid, REFIID riid, void **ppv)
{
    HRESULT hr = _Module.GetClassObject(rclsid, riid, ppv);

#ifdef DEBUG
    //this object gets freed on DLL_PROCESS_DETACH, which happens AFTER the
    // mem leak check happens on exit.
    if (SUCCEEDED(hr))
    {
        _ASSERTE(_Module.m_pObjMap != NULL);
        _ATL_OBJMAP_ENTRY* pEntry = _Module.m_pObjMap;

        while (pEntry->pclsid != NULL)
        {
            if (InlineIsEqualGUID(rclsid, *pEntry->pclsid))
            {
                ASSERT(pEntry->pCF);
                remove_from_memlist(pEntry->pCF);
                break;
            }
            pEntry++;
        }
    }
#endif

    return hr;
}

STDAPI_(LONG) AtlGetLockCount()
{
    return _Module.GetLockCount();
}

