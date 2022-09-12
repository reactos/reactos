/*
* PROJECT:         input.dll
* FILE:            dll/cpl/input/input_list.c
* PURPOSE:         input.dll
* PROGRAMMERS:     Dmitry Chapyshev (dmitry@reactos.org)
*                  Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
*/

#include "input_list.h"

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

    hKey = NULL;
    RegOpenKeyExW(HKEY_LOCAL_MACHINE, pszKey, 0, KEY_ALL_ACCESS, &hKey);
    if (hKey == NULL)
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

    pCurrent = _InputList;

    pNew = (INPUT_LIST_NODE*)malloc(sizeof(INPUT_LIST_NODE));
    if (pNew == NULL)
        return NULL;

    ZeroMemory(pNew, sizeof(INPUT_LIST_NODE));

    if (pCurrent == NULL)
    {
        _InputList = pNew;
    }
    else
    {
        while (pCurrent->pNext != NULL)
        {
            pCurrent = pCurrent->pNext;
        }

        pNew->pPrev = pCurrent;
        pCurrent->pNext = pNew;
    }

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

    if (_InputList == NULL)
        return;

    pCurrent = _InputList;

    while (pCurrent != NULL)
    {
        INPUT_LIST_NODE *pNext = pCurrent->pNext;

        free(pCurrent->pszIndicator);
        free(pCurrent);

        pCurrent = pNext;
    }

    _InputList = NULL;
}

static BOOL
InputList_PrepareUserRegistry(VOID)
{
    BOOL ret = FALSE;
    HKEY hLayoutKey = NULL, hPreloadKey = NULL, hSubstKey = NULL;

    if (RegOpenKeyExW(HKEY_CURRENT_USER,
                      L"Keyboard Layout",
                      0,
                      KEY_ALL_ACCESS,
                      &hLayoutKey) == ERROR_SUCCESS)
    {
        RegDeleteKeyW(hLayoutKey, L"Preload");
        RegDeleteKeyW(hLayoutKey, L"Substitutes");
        RegCloseKey(hLayoutKey);
        hLayoutKey = NULL;
    }

    if (RegCreateKeyW(HKEY_CURRENT_USER, L"Keyboard Layout", &hLayoutKey) == ERROR_SUCCESS &&
        RegCreateKeyW(hLayoutKey, L"Preload", &hPreloadKey) == ERROR_SUCCESS &&
        RegCreateKeyW(hLayoutKey, L"Substitutes", &hSubstKey) == ERROR_SUCCESS)
    {
        ret = TRUE;
    }

    if (hSubstKey)
        RegCloseKey(hSubstKey);
    if (hPreloadKey)
        RegCloseKey(hPreloadKey);
    if (hLayoutKey)
        RegCloseKey(hLayoutKey);

    return ret;
}

static BOOL
InputList_FindPreloadKLID(HKEY hPreloadKey, DWORD dwKLID)
{
    DWORD dwNumber, dwType, cbValue;
    WCHAR szNumber[16], szValue[16], szKLID[16];

    StringCchPrintfW(szKLID, ARRAYSIZE(szKLID), L"%08X", dwKLID);

    for (dwNumber = 1; dwNumber <= 0xFF; ++dwNumber)
    {
        StringCchPrintfW(szNumber, ARRAYSIZE(szNumber), L"%u", dwNumber);

        cbValue = ARRAYSIZE(szValue) * sizeof(WCHAR);
        if (!RegQueryValueExW(hPreloadKey, szNumber, NULL, &dwType, (LPBYTE)szValue, &cbValue))
            break;

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
    WCHAR szLogicalKLID[16], szPhysicalKLID[16];

    StringCchPrintfW(szLogicalKLID, ARRAYSIZE(szLogicalKLID), L"%08X", dwLogicalKLID);
    StringCchPrintfW(szPhysicalKLID, ARRAYSIZE(szPhysicalKLID), L"%08X", dwPhysicalKLID);

    cbValue = (wcslen(szPhysicalKLID) + 1) * sizeof(WCHAR);
    return RegSetValueExW(hSubstKey, szLogicalKLID, 0, REG_SZ, (LPBYTE)szPhysicalKLID,
                          cbValue) == ERROR_SUCCESS;
}

static DWORD
InputList_DoSubst(HKEY hPreloadKey, HKEY hSubstKey, DWORD dwPhysicalKLID)
{
    DWORD iTrial;
    DWORD dwLogicalKLID = LOWORD(dwPhysicalKLID);

    for (iTrial = 0; iTrial <= 0xFF; ++iTrial)
    {
        if (!InputList_FindPreloadKLID(hPreloadKey, dwLogicalKLID)) /* Not found? */
        {
            /* Write now */
            InputList_WriteSubst(hSubstKey, dwPhysicalKLID, dwLogicalKLID);
            return dwLogicalKLID;
        }

        /* Calculate the next logical KLID */
        if (!IS_SUBST_KLID(dwLogicalKLID))
        {
            dwLogicalKLID |= SUBST_MASK;
        }
        else
        {
            WORD wLow = LOWORD(dwLogicalKLID), wHigh = HIWORD(dwLogicalKLID);
            dwLogicalKLID = MAKELONG(wLow, wHigh + 1);
        }
    }

    return 0;
}

static BOOL
InputList_AddInputMethodToUserRegistry(
    HKEY hPreloadKey,
    HKEY hSubstKey,
    DWORD dwNumber,
    INPUT_LIST_NODE *pNode)
{
    WCHAR szNumber[MAX_PATH], szPreload[MAX_PATH];
    DWORD dwPhysicalKLID, dwLogicalKLID, cbValue;
    HKL hKL = pNode->hkl;
    BOOL ret;

    if (IS_IME_HKL(hKL)) /* IME? */
    {
        /* Don't substitute the IME KLIDs */
        dwLogicalKLID = dwPhysicalKLID = HandleToUlong(hKL);
    }
    else
    {
        /* Substitute the KLID if necessary */
        dwPhysicalKLID = pNode->pLayout->dwKLID;
        dwLogicalKLID = InputList_DoSubst(hPreloadKey, hSubstKey, dwPhysicalKLID);
    }

    /* Write the Preload value (number |--> logical KLID) */
    StringCchPrintfW(szNumber, ARRAYSIZE(szNumber), L"%lu", dwNumber);
    StringCchPrintfW(szPreload, ARRAYSIZE(szPreload), L"%08X", dwLogicalKLID);
    cbValue = (wcslen(szPreload) + 1) * sizeof(WCHAR);
    ret = (RegSetValueExW(hPreloadKey,
                          szNumber,
                          0,
                          REG_SZ,
                          (LPBYTE)szPreload,
                          cbValue) == ERROR_SUCCESS);

    if ((pNode->wFlags & INPUT_LIST_NODE_FLAG_ADDED) ||
        (pNode->wFlags & INPUT_LIST_NODE_FLAG_EDITED))
    {
        pNode->hkl = LoadKeyboardLayoutW(szPreload, KLF_SUBSTITUTE_OK | KLF_NOTELLSHELL);
    }

    return ret;
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

    if (RegOpenKeyExW(HKEY_CURRENT_USER,
                      L"Keyboard Layout\\Preload",
                      0,
                      KEY_READ | KEY_WRITE,
                      &hPreloadKey) != ERROR_SUCCESS)
    {
        return FALSE;
    }

    if (RegOpenKeyExW(HKEY_CURRENT_USER,
                      L"Keyboard Layout\\Substitutes",
                      0,
                      KEY_READ | KEY_WRITE,
                      &hSubstKey) != ERROR_SUCCESS)
    {
        RegCloseKey(hPreloadKey);
        return FALSE;
    }

    /* Process deleted and edited input methods */
    for (pCurrent = _InputList; pCurrent != NULL; pCurrent = pCurrent->pNext)
    {
        if ((pCurrent->wFlags & INPUT_LIST_NODE_FLAG_DELETED) ||
            (pCurrent->wFlags & INPUT_LIST_NODE_FLAG_EDITED))
        {
            if (UnloadKeyboardLayout(pCurrent->hkl))
            {
                /* Only unload the edited input method, but does not delete it from the list */
                if (!(pCurrent->wFlags & INPUT_LIST_NODE_FLAG_EDITED))
                {
                    InputList_RemoveNode(pCurrent);
                }
            }
        }
    }

    InputList_PrepareUserRegistry();

    /* Find default input method */
    for (pCurrent = _InputList; pCurrent != NULL; pCurrent = pCurrent->pNext)
    {
        if (pCurrent->wFlags & INPUT_LIST_NODE_FLAG_DEFAULT)
        {
            bRet = InputList_SetFontSubstitutes(pCurrent->pLocale->dwId);
            InputList_AddInputMethodToUserRegistry(hPreloadKey, hSubstKey, 1, pCurrent);
            break;
        }
    }

    if (SystemParametersInfoW(SPI_SETDEFAULTINPUTLANG, 0, &pCurrent->hkl, 0))
    {
        DWORD dwRecipients = BSM_ALLCOMPONENTS | BSM_ALLDESKTOPS;

        BroadcastSystemMessageW(BSF_POSTMESSAGE,
                                &dwRecipients,
                                WM_INPUTLANGCHANGEREQUEST,
                                0,
                                (LPARAM)pCurrent->hkl);
    }

    /* Add methods to registry */
    dwNumber = 2;
    for (pCurrent = _InputList; pCurrent != NULL; pCurrent = pCurrent->pNext)
    {
        if (pCurrent->wFlags & INPUT_LIST_NODE_FLAG_DEFAULT)
            continue;

        InputList_AddInputMethodToUserRegistry(hPreloadKey, hSubstKey, dwNumber, pCurrent);

        ++dwNumber;
    }

    RegCloseKey(hPreloadKey);
    RegCloseKey(hSubstKey);
    return bRet;
}


BOOL
InputList_Add(LOCALE_LIST_NODE *pLocale, LAYOUT_LIST_NODE *pLayout)
{
    WCHAR szIndicator[MAX_STR_LEN];
    INPUT_LIST_NODE *pInput;

    if (pLocale == NULL || pLayout == NULL)
    {
        return FALSE;
    }

    for (pInput = _InputList; pInput != NULL; pInput = pInput->pNext)
    {
        if (pInput->pLocale == pLocale && pInput->pLayout == pLayout)
        {
            return FALSE;
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


/*
 * It marks the input method for deletion, but does not delete it directly.
 * To apply the changes using InputList_Process()
 */
VOID
InputList_Remove(INPUT_LIST_NODE *pNode)
{
    BOOL bRemoveNode = FALSE;

    if (pNode == NULL)
        return;

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
        pNode->wFlags = INPUT_LIST_NODE_FLAG_DELETED;
    }

    if (pNode->wFlags & INPUT_LIST_NODE_FLAG_DEFAULT)
    {
        if (pNode->pNext != NULL)
        {
            pNode->pNext->wFlags |= INPUT_LIST_NODE_FLAG_DEFAULT;
        }
        else if (pNode->pPrev != NULL)
        {
            pNode->pPrev->wFlags |= INPUT_LIST_NODE_FLAG_DEFAULT;
        }
    }

    if (bRemoveNode != FALSE)
    {
        InputList_RemoveNode(pNode);
    }
}

BOOL
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
        return FALSE;
    }

    for (iIndex = 0; iIndex < iLayoutCount; ++iIndex)
    {
        LOCALE_LIST_NODE *pLocale = LocaleList_GetByHkl(pLayoutList[iIndex]);
        LAYOUT_LIST_NODE *pLayout = LayoutList_GetByHkl(pLayoutList[iIndex]);
        if (!pLocale || !pLayout)
            continue;

        pInput = InputList_AppendNode();
        pInput->pLocale = pLocale;
        pInput->pLayout = pLayout;
        pInput->hkl     = pLayoutList[iIndex];

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
    return TRUE;
}


INPUT_LIST_NODE*
InputList_GetFirst(VOID)
{
    return _InputList;
}
