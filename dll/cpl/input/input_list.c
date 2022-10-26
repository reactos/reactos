/*
* PROJECT:         input.dll
* FILE:            dll/cpl/input/input_list.c
* PURPOSE:         input.dll
* PROGRAMMERS:     Dmitry Chapyshev (dmitry@reactos.org)
*                  Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
*/

#include "input_list.h"
#define NOTHING

typedef struct
{
    PWCHAR FontName;
    PWCHAR SubFontName;
} MUI_SUBFONT;

#include "../../../base/setup/lib/muifonts.h"

BOOL UpdateRegistryForFontSubstitutes(MUI_SUBFONT *pSubstitutes)
{
    DWORD cbData;
    HKEY hKey;
    static const WCHAR pszKey[] =
        L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\FontSubstitutes";

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, pszKey, 0, KEY_ALL_ACCESS, &hKey) != ERROR_SUCCESS)
        return FALSE;

    /* Overwrite only */
    for (; pSubstitutes->FontName; ++pSubstitutes)
    {
        cbData = (lstrlenW(pSubstitutes->SubFontName) + 1) * sizeof(WCHAR);
        RegSetValueExW(hKey, pSubstitutes->FontName, 0,
            REG_SZ, (LPBYTE)pSubstitutes->SubFontName, cbData);
    }

    RegCloseKey(hKey);
    return TRUE;
}

VOID GetSystemLibraryPath(LPWSTR pszPath, INT cchPath, LPCWSTR pszFileName)
{
    WCHAR szSysDir[MAX_PATH];
    GetSystemDirectoryW(szSysDir, ARRAYSIZE(szSysDir));
    StringCchPrintfW(pszPath, cchPath, L"%s\\%s", szSysDir, pszFileName);
}

BOOL
InputList_SetFontSubstitutes(LCID dwLocaleId)
{
    MUI_SUBFONT *pSubstitutes;
    WORD wLangID, wPrimaryLangID, wSubLangID;

    wLangID = LANGIDFROMLCID(dwLocaleId);
    wPrimaryLangID = PRIMARYLANGID(wLangID);
    wSubLangID = SUBLANGID(wLangID);

    /* FIXME: Add more if necessary */
    switch (wPrimaryLangID)
    {
        default:
            pSubstitutes = LatinFonts;
            break;
        case LANG_AZERI:
        case LANG_BELARUSIAN:
        case LANG_BULGARIAN:
        case LANG_KAZAK:
        case LANG_RUSSIAN:
        case LANG_SERBIAN:
        case LANG_TATAR:
        case LANG_UKRAINIAN:
        case LANG_UZBEK:
            pSubstitutes = CyrillicFonts;
            break;
        case LANG_GREEK:
            pSubstitutes = GreekFonts;
            break;
        case LANG_HEBREW:
            pSubstitutes = HebrewFonts;
            break;
        case LANG_CHINESE:
            switch (wSubLangID)
            {
                case SUBLANG_CHINESE_SIMPLIFIED:
                case SUBLANG_CHINESE_SINGAPORE:
                    pSubstitutes = ChineseSimplifiedFonts;
                    break;
                case SUBLANG_CHINESE_TRADITIONAL:
                case SUBLANG_CHINESE_HONGKONG:
                case SUBLANG_CHINESE_MACAU:
                    pSubstitutes = ChineseTraditionalFonts;
                    break;
                default:
                    pSubstitutes = NULL;
                    DebugBreak();
                    break;
            }
            break;
        case LANG_JAPANESE:
            pSubstitutes = JapaneseFonts;
            break;
        case LANG_KOREAN:
            pSubstitutes = KoreanFonts;
            break;
        case LANG_ARABIC:
        case LANG_ARMENIAN:
        case LANG_BENGALI:
        case LANG_FARSI:
        case LANG_GEORGIAN:
        case LANG_GUJARATI:
        case LANG_HINDI:
        case LANG_KONKANI:
        case LANG_MARATHI:
        case LANG_PUNJABI:
        case LANG_SANSKRIT:
        case LANG_TAMIL:
        case LANG_TELUGU:
        case LANG_THAI:
        case LANG_URDU:
        case LANG_VIETNAMESE:
            pSubstitutes = UnicodeFonts;
            break;
    }

    if (pSubstitutes)
    {
        UpdateRegistryForFontSubstitutes(pSubstitutes);
        return TRUE;
    }
    return FALSE;
}

static INPUT_LIST_NODE *_InputList = NULL;


static INPUT_LIST_NODE*
InputList_AppendNode(VOID)
{
    INPUT_LIST_NODE *pCurrent;
    INPUT_LIST_NODE *pNew;

    pNew = (INPUT_LIST_NODE*)malloc(sizeof(INPUT_LIST_NODE));
    if (pNew == NULL)
        return NULL;

    ZeroMemory(pNew, sizeof(INPUT_LIST_NODE));

    if (_InputList == NULL) /* Empty? */
    {
        _InputList = pNew;
        return pNew;
    }

    /* Find last node */
    for (pCurrent = _InputList; pCurrent->pNext; pCurrent = pCurrent->pNext)
    {
        NOTHING;
    }

    /* Add to the end */
    pCurrent->pNext = pNew;
    pNew->pPrev = pCurrent;

    return pNew;
}


static VOID
InputList_RemoveNode(INPUT_LIST_NODE *pNode)
{
    INPUT_LIST_NODE *pCurrent = pNode;

    if (_InputList == NULL)
        return;

    if (pCurrent != NULL)
    {
        INPUT_LIST_NODE *pNext = pCurrent->pNext;
        INPUT_LIST_NODE *pPrev = pCurrent->pPrev;

        free(pCurrent->pszIndicator);
        free(pCurrent);

        if (pNext != NULL)
            pNext->pPrev = pPrev;

        if (pPrev != NULL)
            pPrev->pNext = pNext;
        else
            _InputList = pNext;
    }
}


VOID
InputList_Destroy(VOID)
{
    INPUT_LIST_NODE *pCurrent;
    INPUT_LIST_NODE *pNext;

    if (_InputList == NULL)
        return;

    for (pCurrent = _InputList; pCurrent; pCurrent = pNext)
    {
        pNext = pCurrent->pNext;

        free(pCurrent->pszIndicator);
        free(pCurrent);
    }

    _InputList = NULL;
}


static BOOL
InputList_PrepareUserRegistry(PHKEY phPreloadKey, PHKEY phSubstKey)
{
    BOOL bResult = FALSE;
    HKEY hKey;

    *phPreloadKey = *phSubstKey = NULL;

    if (RegOpenKeyExW(HKEY_CURRENT_USER,
                      L"Keyboard Layout",
                      0,
                      KEY_ALL_ACCESS,
                      &hKey) == ERROR_SUCCESS)
    {
        RegDeleteKeyW(hKey, L"Preload");
        RegDeleteKeyW(hKey, L"Substitutes");

        RegCloseKey(hKey);
    }

    if (RegCreateKeyW(HKEY_CURRENT_USER, L"Keyboard Layout", &hKey) == ERROR_SUCCESS &&
        RegCreateKeyW(hKey, L"Preload", phPreloadKey) == ERROR_SUCCESS &&
        RegCreateKeyW(hKey, L"Substitutes", phSubstKey) == ERROR_SUCCESS)
    {
        bResult = TRUE;
    }

    if (hKey)
        RegCloseKey(hKey);

    return bResult;
}

static BOOL
InputList_FindPreloadKLID(HKEY hPreloadKey, DWORD dwKLID)
{
    DWORD dwNumber, dwType, cbValue;
    WCHAR szNumber[16], szValue[KL_NAMELENGTH], szKLID[KL_NAMELENGTH];

    StringCchPrintfW(szKLID, ARRAYSIZE(szKLID), L"%08x", dwKLID);

    for (dwNumber = 1; dwNumber <= 1000; ++dwNumber)
    {
        StringCchPrintfW(szNumber, ARRAYSIZE(szNumber), L"%u", dwNumber);

        cbValue = ARRAYSIZE(szValue) * sizeof(WCHAR);
        if (RegQueryValueExW(hPreloadKey, szNumber, NULL, &dwType,
                             (LPBYTE)szValue, &cbValue) != ERROR_SUCCESS)
        {
            break;
        }

        if (dwType != REG_SZ)
            continue;

        szValue[ARRAYSIZE(szValue) - 1] = 0;
        if (_wcsicmp(szKLID, szValue) == 0)
            return TRUE;
    }

    return FALSE;
}

static BOOL
InputList_WriteSubst(HKEY hSubstKey, DWORD dwPhysicalKLID, DWORD dwLogicalKLID)
{
    DWORD cbValue;
    WCHAR szLogicalKLID[KL_NAMELENGTH], szPhysicalKLID[KL_NAMELENGTH];

    StringCchPrintfW(szLogicalKLID, ARRAYSIZE(szLogicalKLID), L"%08x", dwLogicalKLID);
    StringCchPrintfW(szPhysicalKLID, ARRAYSIZE(szPhysicalKLID), L"%08x", dwPhysicalKLID);

    cbValue = (wcslen(szPhysicalKLID) + 1) * sizeof(WCHAR);
    return RegSetValueExW(hSubstKey, szLogicalKLID, 0, REG_SZ, (LPBYTE)szPhysicalKLID,
                          cbValue) == ERROR_SUCCESS;
}

static DWORD
InputList_DoSubst(HKEY hPreloadKey, HKEY hSubstKey,
                  DWORD dwPhysicalKLID, DWORD dwLogicalKLID)
{
    DWORD iTrial;
    BOOL bSubstNeeded = (dwPhysicalKLID != dwLogicalKLID) || (HIWORD(dwPhysicalKLID) != 0);

    for (iTrial = 1; iTrial <= 1000; ++iTrial)
    {
        if (!InputList_FindPreloadKLID(hPreloadKey, dwLogicalKLID)) /* Not found? */
        {
            if (bSubstNeeded)
            {
                /* Write now */
                InputList_WriteSubst(hSubstKey, dwPhysicalKLID, dwLogicalKLID);
            }
            return dwLogicalKLID;
        }

        bSubstNeeded = TRUE;

        /* Calculate the next logical KLID */
        if (!IS_SUBST_KLID(dwLogicalKLID))
        {
            dwLogicalKLID |= SUBST_MASK;
        }
        else
        {
            WORD wLow = LOWORD(dwLogicalKLID);
            WORD wHigh = HIWORD(dwLogicalKLID);
            dwLogicalKLID = MAKELONG(wLow, wHigh + 1);
        }
    }

    return 0;
}

static VOID
InputList_AddInputMethodToUserRegistry(
    HKEY hPreloadKey,
    HKEY hSubstKey,
    DWORD dwNumber,
    INPUT_LIST_NODE *pNode)
{
    WCHAR szNumber[32], szLogicalKLID[KL_NAMELENGTH];
    DWORD dwPhysicalKLID, dwLogicalKLID, cbValue;
    HKL hKL = pNode->hkl;

    if (IS_IME_HKL(hKL)) /* IME? */
    {
        /* Do not substitute the IME KLID */
        dwLogicalKLID = dwPhysicalKLID = HandleToUlong(hKL);
    }
    else
    {
        /* Substitute the KLID if necessary */
        dwPhysicalKLID = pNode->pLayout->dwKLID;
        dwLogicalKLID = pNode->pLocale->dwId;
        dwLogicalKLID = InputList_DoSubst(hPreloadKey, hSubstKey, dwPhysicalKLID, dwLogicalKLID);
    }

    /* Write the Preload value (number |--> logical KLID) */
    StringCchPrintfW(szNumber, ARRAYSIZE(szNumber), L"%lu", dwNumber);
    StringCchPrintfW(szLogicalKLID, ARRAYSIZE(szLogicalKLID), L"%08x", dwLogicalKLID);
    cbValue = (wcslen(szLogicalKLID) + 1) * sizeof(WCHAR);
    RegSetValueExW(hPreloadKey,
                   szNumber,
                   0,
                   REG_SZ,
                   (LPBYTE)szLogicalKLID,
                   cbValue);

    if ((pNode->wFlags & INPUT_LIST_NODE_FLAG_ADDED) ||
        (pNode->wFlags & INPUT_LIST_NODE_FLAG_EDITED))
    {
        UINT uFlags = KLF_SUBSTITUTE_OK | KLF_NOTELLSHELL;
        if (pNode->wFlags & INPUT_LIST_NODE_FLAG_DEFAULT)
            uFlags |= KLF_REPLACELANG;

        pNode->hkl = LoadKeyboardLayoutW(szLogicalKLID, uFlags);
    }
}


/*
 * Writes any changes in input methods to the registry
 */
BOOL
InputList_Process(VOID)
{
    INPUT_LIST_NODE *pCurrent;
    DWORD dwNumber;
    BOOL bRet = FALSE;
    HKEY hPreloadKey, hSubstKey;

    if (!InputList_PrepareUserRegistry(&hPreloadKey, &hSubstKey))
    {
        if (hPreloadKey)
            RegCloseKey(hPreloadKey);
        if (hSubstKey)
            RegCloseKey(hSubstKey);
        return FALSE;
    }

    /* Process DELETED and EDITED entries */
    for (pCurrent = _InputList; pCurrent != NULL; pCurrent = pCurrent->pNext)
    {
        if ((pCurrent->wFlags & INPUT_LIST_NODE_FLAG_DELETED) ||
            (pCurrent->wFlags & INPUT_LIST_NODE_FLAG_EDITED))
        {
            /* Only unload the DELETED and EDITED entries */
            if (UnloadKeyboardLayout(pCurrent->hkl))
            {
                /* But the EDITED entries are used later */
                if (pCurrent->wFlags & INPUT_LIST_NODE_FLAG_DELETED)
                {
                    InputList_RemoveNode(pCurrent);
                }
            }
        }
    }

    /* Add the DEFAULT entry and set font substitutes */
    for (pCurrent = _InputList; pCurrent != NULL; pCurrent = pCurrent->pNext)
    {
        if (pCurrent->wFlags & INPUT_LIST_NODE_FLAG_DELETED)
            continue;

        if (pCurrent->wFlags & INPUT_LIST_NODE_FLAG_DEFAULT)
        {
            bRet = InputList_SetFontSubstitutes(pCurrent->pLocale->dwId);
            InputList_AddInputMethodToUserRegistry(hPreloadKey, hSubstKey, 1, pCurrent);

            /* Activate the DEFAULT entry */
            ActivateKeyboardLayout(pCurrent->hkl, KLF_RESET);
            break;
        }
    }

    /* Add entries except DEFAULT to registry */
    dwNumber = 2;
    for (pCurrent = _InputList; pCurrent != NULL; pCurrent = pCurrent->pNext)
    {
        if (pCurrent->wFlags & INPUT_LIST_NODE_FLAG_DELETED)
            continue;
        if (pCurrent->wFlags & INPUT_LIST_NODE_FLAG_DEFAULT)
            continue;

        InputList_AddInputMethodToUserRegistry(hPreloadKey, hSubstKey, dwNumber, pCurrent);

        ++dwNumber;
    }

    /* Remove ADDED and EDITED flags */
    for (pCurrent = _InputList; pCurrent != NULL; pCurrent = pCurrent->pNext)
    {
        pCurrent->wFlags &= ~(INPUT_LIST_NODE_FLAG_ADDED | INPUT_LIST_NODE_FLAG_EDITED);
    }

    /* Change the default keyboard language */
    if (SystemParametersInfoW(SPI_SETDEFAULTINPUTLANG, 0, &pCurrent->hkl, 0))
    {
        DWORD dwRecipients = BSM_ALLDESKTOPS | BSM_APPLICATIONS;

        BroadcastSystemMessageW(BSF_POSTMESSAGE,
                                &dwRecipients,
                                WM_INPUTLANGCHANGEREQUEST,
                                INPUTLANGCHANGE_SYSCHARSET,
                                (LPARAM)pCurrent->hkl);
    }

    /* Retry to delete (in case of failure to delete the default keyboard) */
    for (pCurrent = _InputList; pCurrent != NULL; pCurrent = pCurrent->pNext)
    {
        if (pCurrent->wFlags & INPUT_LIST_NODE_FLAG_DELETED)
        {
            UnloadKeyboardLayout(pCurrent->hkl);
            InputList_RemoveNode(pCurrent);
        }
    }

    RegCloseKey(hPreloadKey);
    RegCloseKey(hSubstKey);
    return bRet;
}


BOOL
InputList_Add(LOCALE_LIST_NODE *pLocale, LAYOUT_LIST_NODE *pLayout)
{
    WCHAR szIndicator[MAX_STR_LEN];
    INPUT_LIST_NODE *pInput = NULL;

    if (pLocale == NULL || pLayout == NULL)
    {
        return FALSE;
    }

    for (pInput = _InputList; pInput != NULL; pInput = pInput->pNext)
    {
        if (pInput->wFlags & INPUT_LIST_NODE_FLAG_DELETED)
            continue;

        if (pInput->pLocale == pLocale && pInput->pLayout == pLayout)
        {
            return FALSE; /* Already exists */
        }
    }

    pInput = InputList_AppendNode();
    pInput->wFlags = INPUT_LIST_NODE_FLAG_ADDED;
    pInput->pLocale = pLocale;
    pInput->pLayout = pLayout;

    if (GetLocaleInfoW(LOWORD(pInput->pLocale->dwId),
                       LOCALE_SABBREVLANGNAME | LOCALE_NOUSEROVERRIDE,
                       szIndicator,
                       ARRAYSIZE(szIndicator)))
    {
        size_t len = wcslen(szIndicator);

        if (len > 0)
        {
            szIndicator[len - 1] = 0;
            pInput->pszIndicator = _wcsdup(szIndicator);
        }
    }

    return TRUE;
}


VOID
InputList_SetDefault(INPUT_LIST_NODE *pNode)
{
    INPUT_LIST_NODE *pCurrent;

    if (pNode == NULL)
        return;

    for (pCurrent = _InputList; pCurrent != NULL; pCurrent = pCurrent->pNext)
    {
        if (pCurrent == pNode)
        {
            pCurrent->wFlags |= INPUT_LIST_NODE_FLAG_DEFAULT;
        }
        else
        {
            pCurrent->wFlags &= ~INPUT_LIST_NODE_FLAG_DEFAULT;
        }
    }
}

INPUT_LIST_NODE *
InputList_FindNextDefault(INPUT_LIST_NODE *pNode)
{
    INPUT_LIST_NODE *pCurrent;

    for (pCurrent = pNode->pNext; pCurrent; pCurrent = pCurrent->pNext)
    {
        if (pCurrent->wFlags & INPUT_LIST_NODE_FLAG_DELETED)
            continue;

        return pCurrent;
    }

    for (pCurrent = pNode->pPrev; pCurrent; pCurrent = pCurrent->pPrev)
    {
        if (pCurrent->wFlags & INPUT_LIST_NODE_FLAG_DELETED)
            continue;

        return pCurrent;
    }

    return NULL;
}

/*
 * It marks the input method for deletion, but does not delete it directly.
 * To apply the changes using InputList_Process()
 */
BOOL
InputList_Remove(INPUT_LIST_NODE *pNode)
{
    BOOL ret = FALSE;
    BOOL bRemoveNode = FALSE;

    if (pNode == NULL)
        return FALSE;

    if (pNode->wFlags & INPUT_LIST_NODE_FLAG_ADDED)
    {
        /*
         * If the input method has been added to the list, but not yet written
         * in the registry, then simply remove it from the list
         */
        bRemoveNode = TRUE;
    }
    else
    {
        pNode->wFlags |= INPUT_LIST_NODE_FLAG_DELETED;
    }

    if (pNode->wFlags & INPUT_LIST_NODE_FLAG_DEFAULT)
    {
        INPUT_LIST_NODE *pCurrent = InputList_FindNextDefault(pNode);
        if (pCurrent)
            pCurrent->wFlags |= INPUT_LIST_NODE_FLAG_DEFAULT;

        pNode->wFlags &= ~INPUT_LIST_NODE_FLAG_DEFAULT;
        ret = TRUE; /* default input is changed */
    }

    if (bRemoveNode)
    {
        InputList_RemoveNode(pNode);
    }

    return ret;
}

BOOL
InputList_RemoveByLang(LANGID wLangId)
{
    BOOL ret = FALSE;
    INPUT_LIST_NODE *pCurrent;

Retry:
    for (pCurrent = _InputList; pCurrent; pCurrent = pCurrent->pNext)
    {
        if (pCurrent->wFlags & INPUT_LIST_NODE_FLAG_DELETED)
            continue;

        if (LOWORD(pCurrent->pLocale->dwId) == wLangId)
        {
            if (InputList_Remove(pCurrent))
                ret = TRUE; /* default input is changed */
            goto Retry;
        }
    }

    return ret;
}

VOID
InputList_Create(VOID)
{
    INT iLayoutCount, iIndex;
    WCHAR szIndicator[MAX_STR_LEN];
    INPUT_LIST_NODE *pInput;
    HKL *pLayoutList, hklDefault;

    SystemParametersInfoW(SPI_GETDEFAULTINPUTLANG, 0, &hklDefault, 0);

    iLayoutCount = GetKeyboardLayoutList(0, NULL);
    pLayoutList = (HKL*) malloc(iLayoutCount * sizeof(HKL));

    if (!pLayoutList || GetKeyboardLayoutList(iLayoutCount, pLayoutList) <= 0)
    {
        free(pLayoutList);
        return;
    }

    for (iIndex = 0; iIndex < iLayoutCount; ++iIndex)
    {
        HKL hKL = pLayoutList[iIndex];
        LOCALE_LIST_NODE *pLocale = LocaleList_GetByHkl(hKL);
        LAYOUT_LIST_NODE *pLayout = LayoutList_GetByHkl(hKL);
        if (!pLocale || !pLayout)
            continue;

        pInput = InputList_AppendNode();
        pInput->pLocale = pLocale;
        pInput->pLayout = pLayout;
        pInput->hkl     = hKL;

        if (pInput->hkl == hklDefault) /* Default HKL? */
        {
            pInput->wFlags |= INPUT_LIST_NODE_FLAG_DEFAULT;
            hklDefault = NULL; /* No more default item */
        }

        /* Get abbrev language name */
        szIndicator[0] = 0;
        if (GetLocaleInfoW(LOWORD(pInput->pLocale->dwId),
                           LOCALE_SABBREVLANGNAME | LOCALE_NOUSEROVERRIDE,
                           szIndicator,
                           ARRAYSIZE(szIndicator)))
        {
            size_t len = wcslen(szIndicator);
            if (len > 0)
            {
                szIndicator[len - 1] = 0;
                pInput->pszIndicator = _wcsdup(szIndicator);
            }
        }
    }

    free(pLayoutList);
}

static INT InputList_Compare(INPUT_LIST_NODE *pNode1, INPUT_LIST_NODE *pNode2)
{
    INT nCompare = _wcsicmp(pNode1->pszIndicator, pNode2->pszIndicator);
    if (nCompare != 0)
        return nCompare;

    return _wcsicmp(pNode1->pLayout->pszName, pNode2->pLayout->pszName);
}

VOID InputList_Sort(VOID)
{
    INPUT_LIST_NODE *pList = _InputList;
    INPUT_LIST_NODE *pNext, *pPrev;
    INPUT_LIST_NODE *pMinimum, *pNode;

    _InputList = NULL;

    while (pList)
    {
        /* Find the minimum node */
        pMinimum = NULL;
        for (pNode = pList; pNode; pNode = pNext)
        {
            pNext = pNode->pNext;

            if (pMinimum == NULL)
            {
                pMinimum = pNode;
            }
            else if (InputList_Compare(pNode, pMinimum) < 0)
            {
                pMinimum = pNode;
            }
        }

        // Remove pMinimum from pList
        pNext = pMinimum->pNext;
        pPrev = pMinimum->pPrev;
        if (pNext)
            pNext->pPrev = pPrev;
        if (pPrev)
            pPrev->pNext = pNext;
        else
            pList = pNext;

        // Append pMinimum to _InputList
        if (!_InputList)
        {
            pMinimum->pPrev = pMinimum->pNext = NULL;
            _InputList = pMinimum;
        }
        else
        {
            /* Find last node */
            for (pNode = _InputList; pNode->pNext; pNode = pNode->pNext)
            {
                NOTHING;
            }

            /* Add to the end */
            pNode->pNext = pMinimum;
            pMinimum->pPrev = pNode;
            pMinimum->pNext = NULL;
        }
    }
}

INT
InputList_GetAliveCount(VOID)
{
    INPUT_LIST_NODE *pNode;
    INT nCount = 0;

    for (pNode = _InputList; pNode; pNode = pNode->pNext)
    {
        if (pNode->wFlags & INPUT_LIST_NODE_FLAG_DELETED)
            continue;

        ++nCount;
    }

    return nCount;
}

INPUT_LIST_NODE*
InputList_GetFirst(VOID)
{
    return _InputList;
}
