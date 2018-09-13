#include "shellprv.h"

#include "ftascstr.h" //for now, until CoCreateInstance
#include "ftassoc.h" //for now, until CoCreate IAssocInfo
#include "ftenum.h"

#define EHKCR_NONE      0
#define EHKCR_EXT       1
#define EHKCR_PROGID    2

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//  CFTEnumAssocInfo

///////////////////////////////////////////////////////////////////////////////
// Contructor / Destructor

CFTEnumAssocInfo::CFTEnumAssocInfo() : _cRef(1)
{
    //DLLAddRef();
}

CFTEnumAssocInfo::~CFTEnumAssocInfo()
{
    //DLLRelease();
}

///////////////////////////////////////////////////////////////////////////////
// IUnknown methods

HRESULT CFTEnumAssocInfo::QueryInterface(REFIID riid, PVOID* ppv)
{
    //nothing for now
    return E_NOTIMPL;
}

ULONG CFTEnumAssocInfo::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CFTEnumAssocInfo::Release()
{
    if (InterlockedDecrement(&_cRef) > 0)
        return _cRef;
    delete this;
    return 0;
}
///////////////////////////////////////////////////////////////////////////////
// IEnum methods

HRESULT CFTEnumAssocInfo::Init(ASENUM asenumFlags, LPTSTR pszStr, 
                               AIINIT aiinitFlags)
{
    HRESULT hres = E_INVALIDARG;

    if (((ASENUM_PROGID & asenumFlags) && !(ASENUM_EXT & asenumFlags)) ||
        (!(ASENUM_PROGID & asenumFlags) && (ASENUM_EXT & asenumFlags)) ||
        (ASENUM_ACTION & asenumFlags) )
    {
        hres = S_OK;

        _asenumFlags = asenumFlags;
        _aiinitFlags = aiinitFlags;

        if (pszStr)
            StrCpyN(_szInitStr, pszStr, ARRAYSIZE(_szInitStr));
        else
            _szInitStr[0] = 0;
    }

    return hres;
}

HRESULT CFTEnumAssocInfo::Next(IAssocInfo** ppAI)
{
    HRESULT hres = E_FAIL;
    TCHAR szStr[MAX_FTMAX];
    DWORD cchStr = ARRAYSIZE(szStr);
    AIINIT aiinitFlags = 0;

    *szStr = 0;

    switch(_aiinitFlags)
    {
        // We go through the registry
        case AIINIT_NONE:
        {
            switch(_asenumFlags & ASENUM_MAINMASK)
            {
                case ASENUM_EXT:
                    hres = _EnumHKCR(_asenumFlags, szStr, &cchStr);
                    aiinitFlags = AIINIT_EXT;
                    break;

                case ASENUM_PROGID:
                    hres = _EnumHKCR(_asenumFlags, szStr, &cchStr);
                    aiinitFlags = AIINIT_PROGID;
                    break;

                default:
                    hres = E_INVALIDARG;
                    break;
            }
            break;
        }
        // In theory, we go through the value linked to a progID
        case AIINIT_PROGID:
        {
            switch(_asenumFlags & ASENUM_MAINMASK)
            {
                case ASENUM_EXT:
                    hres = _EnumHKCR(_asenumFlags, szStr, &cchStr);
                    aiinitFlags = AIINIT_EXT;
                    break;

                case ASENUM_ACTION:
                    hres = _EnumProgIDActions(szStr, &cchStr);
                    break;

                default:
                    hres = E_INVALIDARG;
                    break;
            }
            break;
        }
        default:
            hres = E_INVALIDARG;
            break;
    }

    if (SUCCEEDED(hres) && (S_FALSE != hres))
    {
        if (*szStr)
        {
            *ppAI = new CFTAssocInfo();

            if (*ppAI)
            {
                if (ASENUM_ACTION != (_asenumFlags & ASENUM_MAINMASK))
                    hres = (*ppAI)->Init(aiinitFlags, szStr);
                else
                    hres = (*ppAI)->InitComplex(AIINIT_PROGID, _szInitStr, AIINIT_ACTION, szStr);
            }
            else
                hres = E_OUTOFMEMORY;
        }
        else
            hres = E_FAIL;
    }

    return hres;
}

// This beast goes through the HKCR reg key and check that the
// key meets the criteria of dwFlags (mostly extension vs progID)
HRESULT CFTEnumAssocInfo::_EnumHKCR(ASENUM asenumFlags, LPTSTR pszStr, 
                                    DWORD* pcchStr)
{
    HRESULT hres = E_FAIL;
    BOOL fNext = TRUE;

    while (fNext)
    {
        // This will mean "no more item"
        hres = S_FALSE;

        DWORD cchStr = *pcchStr;

        LONG lRes = RegEnumKeyEx(HKEY_CLASSES_ROOT, _dwIndex, pszStr, &cchStr, NULL, NULL,
            NULL, NULL);

        ++_dwIndex;

        if (lRes != ERROR_NO_MORE_ITEMS)
        {
            if (TEXT('*') != *pszStr)
            {
                if (!_EnumKCRStop(asenumFlags, pszStr))
                {
                    if (!_EnumKCRSkip(asenumFlags, pszStr))
                    {
                        hres = S_OK;
                        fNext = FALSE;
                    }
                }
                else
                {
                    hres = S_FALSE;
                    fNext = FALSE;
                }
            }
        }
        else
        {
            fNext = FALSE;
        }
    }

    // Did we found the first ext?
    if (!_fFirstExtFound && SUCCEEDED(hres) && (S_FALSE != hres) && (TEXT('.') == *pszStr))
    {
        // Yes
        _fFirstExtFound = TRUE;
    }

    return hres;
}

HRESULT CFTEnumAssocInfo::_EnumProgIDActions(LPTSTR pszStr, DWORD* pcchStr)
{
    // 5 for "shell"
    TCHAR szSubKey[MAX_PROGID + 5 + 1];
    HRESULT hres = S_OK;
    HKEY hKey = NULL;

    StrCpyN(szSubKey, _szInitStr, MAX_PROGID);
    StrCat(szSubKey, TEXT("\\shell"));

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CLASSES_ROOT, szSubKey, 0, KEY_READ, &hKey))
    {
        LONG lRes = RegEnumKeyEx(hKey, _dwIndex, pszStr, pcchStr, NULL,
            NULL, NULL, NULL);

        if (ERROR_SUCCESS !=lRes)
        {
            if (ERROR_NO_MORE_ITEMS == lRes)
                hres = S_FALSE;
            else
                hres = E_FAIL;
        }
        // BUGBUG
#if 0
        else
        {
            TCHAR szNiceText[MAX_ACTIONDESCR];
            LONG lNiceText = ARRAYSIZE(szNiceText);
#endif
            ++_dwIndex;

#if 0
            // Check if there is nice text for the action
            if ((ERROR_SUCCESS == SHRegQueryValue(hKey, pszStr, szNiceText, 
                &lNiceText)) && (lNiceText > SIZEOF(TCHAR)))
            {
                StrCpyN(pszStr, szNiceText, ARRAYSIZE(pszStr));
            }
        }
#endif

        RegCloseKey(hKey);
    }

    return hres;
}

///////////////////////////////////////////////////////////////////////////////
// Helpers

BOOL CFTEnumAssocInfo::_EnumKCRSkip(DWORD asenumFlags, LPTSTR pszExt)
{
    BOOL fRet = FALSE;

    if (AIINIT_NONE == _aiinitFlags)
    {
        //BUGBUG: remove theis CFTAssocStore creation
        CFTAssocStore* pAssocStore = NULL;
        //BUGBUG: end

        // Do we want the Exts?
        if (!(ASENUM_EXT & asenumFlags))
        {
            // No
            // Is the first char a '.'?
            if (TEXT('.') == *pszExt)
            {
                // Yes, skip this one
                fRet = TRUE;
            }
        }
        else
        {
            // Yes
            // Is the first char a '.'?
            if (TEXT('.') != *pszExt)
            {
                // No, skip it
                fRet = TRUE;
            }
        }

        // we want to skip all the ext having explorer.exe as the executable for
        // their default verb.
        if ((ASENUM_NOEXPLORERSHELLACTION & asenumFlags) && !fRet)
        {
            IQueryAssociations* pQA = NULL;

            ASSERT(ASENUM_EXT & asenumFlags);

            HRESULT hres = AssocCreate(CLSID_QueryAssociations, IID_IQueryAssociations,
                (LPVOID*)&pQA);

            if (SUCCEEDED(hres))
            {
                WCHAR szwExt[MAX_EXT];
                DWORD cchExt = ARRAYSIZE(szwExt);

                SHTCharToUnicode(pszExt, szwExt, ARRAYSIZE(szwExt));

                hres = pQA->Init(0, szwExt, NULL, NULL);
        
                if (SUCCEEDED(hres))
                {
                    WCHAR szwExec[MAX_APPFRIENDLYNAME];
                    DWORD cchExec = ARRAYSIZE(szwExec);

                    hres = pQA->GetString(ASSOCF_VERIFY,
                        ASSOCSTR_EXECUTABLE, NULL, szwExec, &cchExec);

                    if (!StrCmpIW(PathFindFileNameW(szwExec), L"explorer.exe"))
                    {
                        fRet = TRUE;
                    }
                }
                pQA->Release();
            }
        }

        if ((ASENUM_NOEXCLUDED & asenumFlags) && !fRet)
        {
            IAssocInfo* pAI = NULL;
            HRESULT hres = E_FAIL;

            //BUGBUG: remove theis CFTAssocStore creation
            if (!pAssocStore)
                pAssocStore = new CFTAssocStore();
            //BUGBUG: end

            ASSERT(ASENUM_EXT & asenumFlags);

            if (pAssocStore)
                hres = pAssocStore->GetAssocInfo(pszExt, AIINIT_EXT, &pAI);
            
            if (SUCCEEDED(hres))
            {
                hres = pAI->GetBOOL(AIBOOL_EXCLUDE, &fRet);

                pAI->Release();
            }
        }

        if ((ASENUM_NOEXE & asenumFlags) && !fRet)
        {
            ASSERT(ASENUM_EXT & asenumFlags);

            fRet = PathIsExe(pszExt);
        }

        if ((ASENUM_ASSOC_YES & asenumFlags) &&
            (ASENUM_ASSOC_ALL != (ASENUM_ASSOC_ALL & asenumFlags)) && !fRet)
        {
            IAssocInfo* pAI = NULL;
            HRESULT hres = E_FAIL;

            //BUGBUG: remove theis CFTAssocStore creation
            if (!pAssocStore)
                pAssocStore = new CFTAssocStore();
            //BUGBUG: end
            
            ASSERT(ASENUM_EXT & asenumFlags);

            if (pAssocStore)
                hres = pAssocStore->GetAssocInfo(pszExt, AIINIT_EXT, &pAI);
            
            if (SUCCEEDED(hres))
            {
                BOOL fExtAssociated = FALSE;
                hres = pAI->GetBOOL(AIBOOL_EXTASSOCIATED, &fExtAssociated);

                fRet = (fExtAssociated ? FALSE : TRUE);

                pAI->Release();
            }
        }

        if ((ASENUM_ASSOC_NO & asenumFlags) &&
            (ASENUM_ASSOC_ALL != (ASENUM_ASSOC_ALL & asenumFlags)) && !fRet)
        {
            IAssocInfo* pAI = NULL;
            HRESULT hres = E_FAIL;

            //BUGBUG: remove theis CFTAssocStore creation
            if (!pAssocStore)
                pAssocStore = new CFTAssocStore();
            //BUGBUG: end
            
            ASSERT(ASENUM_EXT & asenumFlags);

            if (pAssocStore)
                hres = pAssocStore->GetAssocInfo(pszExt, AIINIT_EXT, &pAI);
            
            if (SUCCEEDED(hres))
            {
                hres = pAI->GetBOOL(AIBOOL_EXTASSOCIATED, &fRet);

                pAI->Release();
            }
        }

        if ((ASENUM_SHOWONLY & asenumFlags) && !fRet)
        {
            IAssocInfo* pAI = NULL;
            HRESULT hres = E_FAIL;

            //BUGBUG: remove theis CFTAssocStore creation
            if (!pAssocStore)
                pAssocStore = new CFTAssocStore();
            //BUGBUG: end

            ASSERT(ASENUM_PROGID & asenumFlags);

            // I know pszExt is not an Extension but a progID...
            if (pAssocStore)
                hres = pAssocStore->GetAssocInfo(pszExt, AIINIT_PROGID, &pAI);
            
            if (SUCCEEDED(hres))
            {
                BOOL fTmpRet = FALSE;

                hres = pAI->GetBOOL(AIBOOL_SHOW, &fTmpRet);

                // If it has the show flag (FTA_Show), we don't skip it, so
                // invert the fTmpRet
                fRet = fTmpRet ? FALSE : TRUE;

                pAI->Release();
            }
        }

        //BUGBUG: remove theis CFTAssocStore creation
        if (pAssocStore)
            delete pAssocStore;
        //BUGBUG: end
    }
    else
    {
        if (AIINIT_PROGID == _aiinitFlags)
        {
            fRet = TRUE;
            // Do we want the Exts?
            if (ASENUM_EXT & asenumFlags)
            {
                DWORD dwType = 0;
                TCHAR szProgID[MAX_PROGID];
                DWORD cbProgID = ARRAYSIZE(szProgID) * sizeof(TCHAR);

                LONG lRes = SHGetValue(HKEY_CLASSES_ROOT, pszExt, NULL,
                    &dwType, szProgID, &cbProgID);

                if (ERROR_SUCCESS == lRes)
                {
                    // Does it have the same progID?
                    if (!lstrcmpi(szProgID, _szInitStr))
                    {
                        // Yes, don't skip
                        fRet = FALSE;
                    }
                }
            }
        }
    }
    
    return fRet;
}

BOOL CFTEnumAssocInfo::_EnumKCRStop(DWORD asenumFlags, LPTSTR pszExt)
{
    BOOL fRet = FALSE;

    // If we want only the extensions, and the first char is not a '.', then stop
    if (ASENUM_EXT & asenumFlags)
    {
        // Don't go out if we haven't found the first extension
        if ((TEXT('.') != *pszExt) && _fFirstExtFound)
            fRet = TRUE;
    }
        
    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
// Non-implemented IEnum methods

HRESULT CFTEnumAssocInfo::Clone(IEnumAssocInfo* pEnum)
{
    // Will never be implemented
    return E_FAIL;
}

HRESULT CFTEnumAssocInfo::Skip(DWORD dwSkip)
{
    return E_NOTIMPL;
}

HRESULT CFTEnumAssocInfo::Reset()
{
    return E_NOTIMPL;
}
