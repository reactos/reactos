/*
 *  ASSOC.C - assoc internal command.
 *
 *
 *  History:
 *
 * 14-Mar-2009 Lee C. Baker
 * - initial implementation.
 *
 * 15-Mar-2009 Lee C. Baker
 * - Don't write to (or use) HKEY_CLASSES_ROOT directly.
 * - Externalize strings.
 *
 * TODO:
 * - PrintAllAssociations could be optimized to not fetch all registry subkeys under 'Classes', just the ones that start with '.'
 */

#include "precomp.h"

#ifdef INCLUDE_CMD_ASSOC

static LONG
PrintAssociationEx(
    IN HKEY hKeyClasses,
    IN PCTSTR pszExtension)
{
    LONG lRet;
    HKEY hKey;
    DWORD dwFileTypeLen = 0;
    PTSTR pszFileType;

    lRet = RegOpenKeyEx(hKeyClasses, pszExtension, 0, KEY_QUERY_VALUE, &hKey);
    if (lRet != ERROR_SUCCESS)
    {
        if (lRet != ERROR_FILE_NOT_FOUND)
            ErrorMessage(lRet, NULL);
        return lRet;
    }

    /* Obtain the string length */
    lRet = RegQueryValueEx(hKey, NULL, NULL, NULL, NULL, &dwFileTypeLen);

    /* If there is no default value, don't display it */
    if (lRet == ERROR_FILE_NOT_FOUND)
    {
        RegCloseKey(hKey);
        return lRet;
    }
    if (lRet != ERROR_SUCCESS)
    {
        ErrorMessage(lRet, NULL);
        RegCloseKey(hKey);
        return lRet;
    }

    ++dwFileTypeLen;
    pszFileType = cmd_alloc(dwFileTypeLen * sizeof(TCHAR));
    if (!pszFileType)
    {
        WARN("Cannot allocate memory for pszFileType!\n");
        RegCloseKey(hKey);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    /* Obtain the actual file type */
    lRet = RegQueryValueEx(hKey, NULL, NULL, NULL, (LPBYTE)pszFileType, &dwFileTypeLen);
    RegCloseKey(hKey);

    if (lRet != ERROR_SUCCESS)
    {
        ErrorMessage(lRet, NULL);
        cmd_free(pszFileType);
        return lRet;
    }

    /* If there is a default key, display the relevant information */
    if (dwFileTypeLen != 0)
    {
        ConOutPrintf(_T("%s=%s\n"), pszExtension, pszFileType);
    }

    cmd_free(pszFileType);
    return ERROR_SUCCESS;
}

static LONG
PrintAssociation(
    IN PCTSTR pszExtension)
{
    LONG lRet;
    HKEY hKeyClasses;

    lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Classes"), 0,
                        KEY_ENUMERATE_SUB_KEYS, &hKeyClasses);
    if (lRet != ERROR_SUCCESS)
    {
        ErrorMessage(lRet, NULL);
        return lRet;
    }

    lRet = PrintAssociationEx(hKeyClasses, pszExtension);

    RegCloseKey(hKeyClasses);
    return lRet;
}

static LONG
PrintAllAssociations(VOID)
{
    LONG lRet;
    HKEY hKeyClasses;
    DWORD dwKeyCtr;
    DWORD dwNumKeys = 0;
    DWORD dwExtLen = 0;
    PTSTR pszExtName;

    lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Classes"), 0,
                        KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS, &hKeyClasses);
    if (lRet != ERROR_SUCCESS)
    {
        ErrorMessage(lRet, NULL);
        return lRet;
    }

    lRet = RegQueryInfoKey(hKeyClasses, NULL, NULL, NULL, &dwNumKeys, &dwExtLen,
                           NULL, NULL, NULL, NULL, NULL, NULL);
    if (lRet != ERROR_SUCCESS)
    {
        ErrorMessage(lRet, NULL);
        RegCloseKey(hKeyClasses);
        return lRet;
    }

    ++dwExtLen;
    pszExtName = cmd_alloc(dwExtLen * sizeof(TCHAR));
    if (!pszExtName)
    {
        WARN("Cannot allocate memory for pszExtName!\n");
        RegCloseKey(hKeyClasses);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    for (dwKeyCtr = 0; dwKeyCtr < dwNumKeys; ++dwKeyCtr)
    {
        DWORD dwBufSize = dwExtLen;
        lRet = RegEnumKeyEx(hKeyClasses, dwKeyCtr, pszExtName, &dwBufSize,
                            NULL, NULL, NULL, NULL);

        if (lRet == ERROR_SUCCESS || lRet == ERROR_MORE_DATA)
        {
            /* Name starts with '.': this is an extension */
            if (*pszExtName == _T('.'))
                PrintAssociationEx(hKeyClasses, pszExtName);
        }
        else
        {
            ErrorMessage(lRet, NULL);
            cmd_free(pszExtName);
            RegCloseKey(hKeyClasses);
            return lRet;
        }
    }

    RegCloseKey(hKeyClasses);

    cmd_free(pszExtName);
    return ERROR_SUCCESS;
}

static LONG
AddAssociation(
    IN PCTSTR pszExtension,
    IN PCTSTR pszType)
{
    LONG lRet;
    HKEY hKeyClasses, hKey;

    lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Classes"), 0,
                        KEY_CREATE_SUB_KEY, &hKeyClasses);
    if (lRet != ERROR_SUCCESS)
    {
        ErrorMessage(lRet, NULL);
        return lRet;
    }

    lRet = RegCreateKeyEx(hKeyClasses, pszExtension, 0, NULL, REG_OPTION_NON_VOLATILE,
                          KEY_SET_VALUE, NULL, &hKey, NULL);
    RegCloseKey(hKeyClasses);

    if (lRet != ERROR_SUCCESS)
    {
        ErrorMessage(lRet, NULL);
        return lRet;
    }

    lRet = RegSetValueEx(hKey, NULL, 0, REG_SZ,
                         (LPBYTE)pszType, (DWORD)(_tcslen(pszType) + 1) * sizeof(TCHAR));
    RegCloseKey(hKey);

    if (lRet != ERROR_SUCCESS)
    {
        ErrorMessage(lRet, NULL);
        return lRet;
    }

    return ERROR_SUCCESS;
}

static LONG
RemoveAssociation(
    IN PCTSTR pszExtension)
{
    LONG lRet;
    HKEY hKeyClasses;

    lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Classes"), 0,
                        KEY_QUERY_VALUE, &hKeyClasses);
    if (lRet != ERROR_SUCCESS)
    {
        ErrorMessage(lRet, NULL);
        return lRet;
    }

    lRet = RegDeleteKey(hKeyClasses, pszExtension);
    RegCloseKey(hKeyClasses);

    if (lRet != ERROR_SUCCESS)
    {
        if (lRet != ERROR_FILE_NOT_FOUND)
            ErrorMessage(lRet, NULL);
        return lRet;
    }

    return ERROR_SUCCESS;
}


INT CommandAssoc(LPTSTR param)
{
    INT retval = 0;
    PTCHAR pEqualSign;

    /* Print help */
    if (!_tcsncmp(param, _T("/?"), 2))
    {
        ConOutResPaging(TRUE, STRING_ASSOC_HELP);
        return 0;
    }

    /* Print all associations if no parameter has been specified */
    if (!*param)
    {
        PrintAllAssociations();
        goto Quit;
    }

    pEqualSign = _tcschr(param, _T('='));
    if (pEqualSign != NULL)
    {
        PTSTR pszFileType = pEqualSign + 1;

        /* NULL-terminate at the equals sign */
        *pEqualSign = 0;

        /* If the equals sign is the last character
         * in the string, delete the association. */
        if (*pszFileType == 0)
        {
            retval = RemoveAssociation(param);
        }
        else
        /* Otherwise, add the association and print it out */
        {
            retval = AddAssociation(param, pszFileType);
            PrintAssociation(param);
        }

        if (retval != ERROR_SUCCESS)
        {
            if (retval != ERROR_FILE_NOT_FOUND)
            {
                // FIXME: Localize
                ConErrPrintf(_T("Error occurred while processing: %s.\n"), param);
            }
            // retval = 1; /* Fixup the error value */
        }
    }
    else
    {
        /* No equals sign, print the association */
        retval = PrintAssociation(param);
        if (retval != ERROR_SUCCESS)
        {
            ConErrResPrintf(STRING_ASSOC_ERROR, param);
            retval = 1; /* Fixup the error value */
        }
    }

Quit:
    if (BatType != CMD_TYPE)
    {
        if (retval != 0)
            nErrorLevel = retval;
    }
    else
    {
        nErrorLevel = retval;
    }

    return retval;
}

#endif /* INCLUDE_CMD_ASSOC */
