#include "shellprv.h"
#include "ftascstr.h"
#include "ftassoc.h" //for now, until use CoCreateInstance
#include "ftenum.h" //for now, until use CoCreateInstance

HRESULT CFTAssocStore::_hresAccess = -1;

HRESULT CFTAssocStore::EnumAssocInfo(ASENUM asenumFlags, LPTSTR pszStr, 
		AIINIT aiinitFlags, IEnumAssocInfo** ppEnum)
{
    //for now
    *ppEnum = new CFTEnumAssocInfo();

    if (*ppEnum)
    {
        (*ppEnum)->Init(asenumFlags, pszStr, aiinitFlags);
    }

    return (*ppEnum) ? S_OK : E_OUTOFMEMORY;
}

HRESULT CFTAssocStore::GetAssocInfo(LPTSTR pszStr, AIINIT aiinitFlags, IAssocInfo** ppAI)
{
    HRESULT hres = E_FAIL;

	*ppAI = new CFTAssocInfo();

	if (*ppAI)
		hres = (*ppAI)->Init(aiinitFlags, pszStr);
	else
		hres = E_OUTOFMEMORY;

	return hres;
}

HRESULT CFTAssocStore::GetComplexAssocInfo(LPTSTR pszStr1, AIINIT aiinitFlags1, 
	LPTSTR pszStr2, AIINIT aiinitFlags2, IAssocInfo** ppAI)
{
    HRESULT hres = E_FAIL;

	*ppAI = new CFTAssocInfo();

	if (*ppAI)
		hres = (*ppAI)->InitComplex(aiinitFlags1, pszStr1, aiinitFlags2, pszStr2);
	else
		hres = E_OUTOFMEMORY;

	return hres;
}

HRESULT CFTAssocStore::CheckAccess()
{
#if 0
    // Should be something like this but it does not work

    if (-1 == _hresAccess)
    {
        HKEY hkey;

        _hresAccess = S_FALSE;

        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CLASSES_ROOT, NULL, 0,
            KEY_CREATE_SUB_KEY | KEY_READ | KEY_SET_VALUE | KEY_QUERY_VALUE, &hkey))
        {
            _hresAccess = S_OK;

            RegCloseKey(hkey);
        }
    }
#endif

    if (-1 == _hresAccess)
    {
        TCHAR szGUID[] = TEXT("{A4BFEC7C-F821-11d2-86BE-0000F8757D7E}");
        DWORD dwDisp = 0;
        int cTry = 0;
        HKEY hkey;

        _hresAccess = S_FALSE;

        // we want to try this only two times
        while ((S_FALSE == _hresAccess) && (cTry <= 1))
        {
            ++cTry;

            // we try to write a GUID to HKCR and delete it
            if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_CLASSES_ROOT, szGUID, 0, NULL, REG_OPTION_NON_VOLATILE,
                MAXIMUM_ALLOWED, NULL, &hkey, &dwDisp))
            {
                // Did we really created a new key?
                if (REG_CREATED_NEW_KEY == dwDisp)
                {
                    // yes
                    RegCloseKey(hkey);

                    if (ERROR_SUCCESS == RegDeleteKey(HKEY_CLASSES_ROOT, szGUID))
                    {
                        _hresAccess = S_OK;
                    }
                }
                else
                {
                    // No, there was already one, maybe we crashed right in the middle of this fct
                    // some other time in the past

                    // delete the key and try again
                    RegDeleteKey(HKEY_CLASSES_ROOT, szGUID);
                }
            }
        }
    }

    return _hresAccess;
}
///////////////////////////////////////////////////////////////////////////////
//
CFTAssocStore::CFTAssocStore()
{
    _hresCoInit = SHCoInitialize();

    //DLLAddRef();
}

CFTAssocStore::~CFTAssocStore()
{
    //DLLRelease();
    SHCoUninitialize(_hresCoInit);
}

//IUnknown methods
HRESULT CFTAssocStore::QueryInterface(REFIID riid, PVOID* ppv)
{
#if 0
    static const QITAB qit[] = {
      QITABENT(CFTAssocStore, IAssocStore),
      { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
#endif

    if (IsEqualIID(riid, IID_IUnknown))
        *ppv = static_cast<IUnknown*>(this);
    else
        *ppv = static_cast<IAssocStore*>(this);

    return S_OK;
}

ULONG CFTAssocStore::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CFTAssocStore::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;
    delete this;
    return 0;
}