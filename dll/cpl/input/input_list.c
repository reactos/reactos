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
    BOOL bResult = FALSE;
    HKEY hTempKey = NULL;
    HKEY hKey = NULL;

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

    if (RegCreateKeyW(HKEY_CURRENT_USER, L"Keyboard Layout", &hKey) != ERROR_SUCCESS)
    {
        goto Cleanup;
    }

    if (RegCreateKeyW(hKey, L"Preload", &hTempKey) != ERROR_SUCCESS)
    {
        goto Cleanup;
    }

    RegCloseKey(hTempKey);

    if (RegCreateKeyW(hKey, L"Substitutes", &hTempKey) != ERROR_SUCCESS)
    {
        goto Cleanup;
    }

    RegCloseKey(hTempKey);

    bResult = TRUE;

Cleanup:
    if (hTempKey != NULL)
        RegCloseKey(hTempKey);
    if (hKey != NULL)
        RegCloseKey(hKey);

    return bResult;
}


static VOID
InputList_AddInputMethodToUserRegistry(DWORD dwIndex, INPUT_LIST_NODE *pNode)
{
    WCHAR szMethodIndex[MAX_PATH];
    WCHAR szPreload[MAX_PATH];
    BOOL bIsImeMethod = FALSE;
    HKEY hKey;

    StringCchPrintfW(szMethodIndex, ARRAYSIZE(szMethodIndex), L"%lu", dwIndex);

    /* Check is IME method */
    if ((HIWORD(pNode->pLayout->dwId) & 0xF000) == 0xE000)
    {
        StringCchPrintfW(szPreload, ARRAYSIZE(szPreload), L"%08X", pNode->pLayout->dwId);
        bIsImeMethod = TRUE;
    }
    else
    {
        StringCchPrintfW(szPreload, ARRAYSIZE(szPreload), L"%08X", pNode->pLocale->dwId);
    }

    if (RegOpenKeyExW(HKEY_CURRENT_USER,
                      L"Keyboard Layout\\Preload",
                      0,
                      KEY_SET_VALUE,
                      &hKey) == ERROR_SUCCESS)
    {
        RegSetValueExW(hKey,
                       szMethodIndex,
                       0,
                       REG_SZ,
                       (LPBYTE)szPreload,
                       (wcslen(szPreload) + 1) * sizeof(WCHAR));

        RegCloseKey(hKey);
    }

    if (pNode->pLocale->dwId != pNode->pLayout->dwId && bIsImeMethod == FALSE)
    {
        if (RegOpenKeyExW(HKEY_CURRENT_USER,
                          L"Keyboard Layout\\Substitutes",
                          0,
                          KEY_SET_VALUE,
                          &hKey) == ERROR_SUCCESS)
        {
            WCHAR szSubstitutes[MAX_PATH];

            StringCchPrintfW(szSubstitutes, ARRAYSIZE(szSubstitutes), L"%08X", pNode->pLayout->dwId);

            RegSetValueExW(hKey,
                           szPreload,
                           0,
                           REG_SZ,
                           (LPBYTE)szSubstitutes,
                           (wcslen(szSubstitutes) + 1) * sizeof(WCHAR));

            RegCloseKey(hKey);
        }
    }

    if ((pNode->wFlags & INPUT_LIST_NODE_FLAG_ADDED) ||
        (pNode->wFlags & INPUT_LIST_NODE_FLAG_EDITED))
    {
        pNode->hkl = LoadKeyboardLayoutW(szPreload, KLF_SUBSTITUTE_OK | KLF_NOTELLSHELL);
    }
}


/*
 * Writes any changes in input methods to the registry
 */
BOOL
InputList_Process(VOID)
{
    INPUT_LIST_NODE *pCurrent;
    DWORD dwIndex;
    BOOL bRet = FALSE;

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
            InputList_AddInputMethodToUserRegistry(1, pCurrent);
            break;
        }
    }

    if (SystemParametersInfoW(SPI_SETDEFAULTINPUTLANG,
                              0,
                              (LPVOID)((LPDWORD)&pCurrent->hkl),
                              0))
    {
        DWORD dwRecipients;

        dwRecipients = BSM_ALLCOMPONENTS;

        BroadcastSystemMessageW(BSF_POSTMESSAGE,
                                &dwRecipients,
                                WM_INPUTLANGCHANGEREQUEST,
                                0,
                                (LPARAM)pCurrent->hkl);
    }

    /* Add methods to registry */
    dwIndex = 2;

    for (pCurrent = _InputList; pCurrent != NULL; pCurrent = pCurrent->pNext)
    {
        if (pCurrent->wFlags & INPUT_LIST_NODE_FLAG_DEFAULT)
            continue;

        InputList_AddInputMethodToUserRegistry(dwIndex, pCurrent);

        dwIndex++;
    }

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


VOID
InputList_Create(VOID)
{
    INT iLayoutCount;
    HKL *pLayoutList;

    iLayoutCount = GetKeyboardLayoutList(0, NULL);
    pLayoutList = (HKL*) malloc(iLayoutCount * sizeof(HKL));

    if (pLayoutList != NULL)
    {
        if (GetKeyboardLayoutList(iLayoutCount, pLayoutList) > 0)
        {
            INT iIndex;

            for (iIndex = 0; iIndex < iLayoutCount; iIndex++)
            {
                LOCALE_LIST_NODE *pLocale = LocaleList_GetByHkl(pLayoutList[iIndex]);
                LAYOUT_LIST_NODE *pLayout = LayoutList_GetByHkl(pLayoutList[iIndex]);

                if (pLocale != NULL && pLayout != NULL)
                {
                    WCHAR szIndicator[MAX_STR_LEN] = { 0 };
                    INPUT_LIST_NODE *pInput;
                    HKL hklDefault;

                    pInput = InputList_AppendNode();

                    pInput->pLocale = pLocale;
                    pInput->pLayout = pLayout;
                    pInput->hkl     = pLayoutList[iIndex];

                    if (SystemParametersInfoW(SPI_GETDEFAULTINPUTLANG,
                                              0,
                                              (LPVOID)((LPDWORD)&hklDefault),
                                              0) == FALSE)
                    {
                        hklDefault = GetKeyboardLayout(0);
                    }

                    if (pInput->hkl == hklDefault)
                    {
                        pInput->wFlags |= INPUT_LIST_NODE_FLAG_DEFAULT;
                    }

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
            }
        }

        free(pLayoutList);
    }
}


INPUT_LIST_NODE*
InputList_GetFirst(VOID)
{
    return _InputList;
}
