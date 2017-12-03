/*
 *  Assoc.C - assoc internal command.
 *
 *
 *  History:
 *
 * 14-Mar-2009 Lee C. Baker
 * - initial implementation
 *
 * 15-Mar-2009 Lee C. Baker
 * - Don't write to (or use) HKEY_CLASSES_ROOT directly
 * - Externalize strings
 *
 * TODO:
 * - PrintAllAssociations might could be optimized to not fetch all registry subkeys under 'Classes', just the ones that start with '.'
 * - Make sure that non-administrator users can list associations, and get appropriate error messages when they don't have sufficient
 *   privileges to perform an operation
 */

#include "precomp.h"

#ifdef INCLUDE_CMD_ASSOC

static INT
PrintAssociation(LPTSTR extension)
{
    DWORD return_val;
    HKEY hKey = NULL, hInsideKey = NULL;

    DWORD fileTypeLength = 0;
    LPTSTR fileType = NULL;

    return_val = RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Classes"), 0, KEY_READ, &hKey);

    if (return_val != ERROR_SUCCESS)
        return -1;

    return_val = RegOpenKeyEx(hKey, extension, 0, KEY_READ, &hInsideKey);
    RegCloseKey(hKey);

    if (return_val != ERROR_SUCCESS)
        return 0;

    /* obtain string length */
    return_val = RegQueryValueEx(hInsideKey, NULL, NULL, NULL, NULL, &fileTypeLength);

    if (return_val == ERROR_FILE_NOT_FOUND) /* no default value, don't display */
    {
        RegCloseKey(hInsideKey);
        return 0;
    }

    if (return_val != ERROR_SUCCESS)
    {
        RegCloseKey(hInsideKey);
        return -2;
    }

    fileType = cmd_alloc(fileTypeLength * sizeof(TCHAR));
    if (!fileType)
    {
        WARN("Cannot allocate memory for fileType!\n");
        RegCloseKey(hInsideKey);
        return -2;
    }

    /* obtain actual file type */
    return_val = RegQueryValueEx(hInsideKey, NULL, NULL, NULL, (LPBYTE)fileType, &fileTypeLength);
    RegCloseKey(hInsideKey);

    if (return_val != ERROR_SUCCESS)
    {
        cmd_free(fileType);
        return -2;
    }

    if (fileTypeLength != 0)    /* if there is a default key, display relevant information */
    {
        ConOutPrintf(_T("%s=%s\n"), extension, fileType);
    }

    cmd_free(fileType);
    return 1;
}

static INT
PrintAllAssociations(VOID)
{
    DWORD return_val = 0;
    HKEY hKey = NULL;
    DWORD numKeys = 0;

    DWORD extLength = 0;
    LPTSTR extName = NULL;
    DWORD keyCtr = 0;

    return_val = RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Classes"), 0, KEY_READ, &hKey);

    if (return_val != ERROR_SUCCESS)
        return -1;

    return_val = RegQueryInfoKey(hKey, NULL, NULL, NULL, &numKeys, &extLength, NULL, NULL, NULL, NULL, NULL, NULL);

    if (return_val != ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        return -2;
    }

    extLength++;
    extName = cmd_alloc(extLength * sizeof(TCHAR));
    if (!extName)
    {
        WARN("Cannot allocate memory for extName!\n");
        RegCloseKey(hKey);
        return -2;
    }

    for (keyCtr = 0; keyCtr < numKeys; keyCtr++)
    {
        DWORD buffer_size = extLength;
        return_val = RegEnumKeyEx(hKey, keyCtr, extName, &buffer_size, NULL, NULL, NULL, NULL);

        if (return_val == ERROR_SUCCESS || return_val == ERROR_MORE_DATA)
        {
            if (*extName == _T('.'))
                PrintAssociation(extName);
        }
        else
        {
            cmd_free(extName);
            RegCloseKey(hKey);
            return -1;
        }
    }

    RegCloseKey(hKey);

    cmd_free(extName);
    return numKeys;
}

static INT
AddAssociation(LPTSTR extension, LPTSTR type)
{
    DWORD return_val;
    HKEY hKey = NULL, insideKey = NULL;

    return_val = RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Classes"), 0, KEY_ALL_ACCESS, &hKey);

    if (return_val != ERROR_SUCCESS)
        return -1;

    return_val = RegCreateKeyEx(hKey, extension, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &insideKey, NULL);
    RegCloseKey(hKey);

    if (return_val != ERROR_SUCCESS)
        return -1;

    return_val = RegSetValueEx(insideKey, NULL, 0, REG_SZ, (LPBYTE)type, (_tcslen(type) + 1) * sizeof(TCHAR));
    RegCloseKey(insideKey);

    if (return_val != ERROR_SUCCESS)
        return -2;

    return 0;
}

static int
RemoveAssociation(LPTSTR extension)
{
    DWORD return_val;
    HKEY hKey;

    return_val = RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Classes"), 0, KEY_ALL_ACCESS, &hKey);

    if (return_val != ERROR_SUCCESS)
        return -1;

    return_val = RegDeleteKey(hKey, extension);
    RegCloseKey(hKey);

    if (return_val != ERROR_SUCCESS)
        return -2;

    return 0;
}


INT CommandAssoc (LPTSTR param)
{
    /* print help */
    if (!_tcsncmp (param, _T("/?"), 2))
    {
        ConOutResPaging(TRUE,STRING_ASSOC_HELP);
        return 0;
    }

    nErrorLevel = 0;

    if (_tcslen(param) == 0)
    {
        PrintAllAssociations();
    }
    else
    {
        LPTSTR lpEqualSign = _tcschr(param, _T('='));
        if (lpEqualSign != NULL)
        {
            LPTSTR fileType = lpEqualSign + 1;
            LPTSTR extension = cmd_alloc((lpEqualSign - param + 1) * sizeof(TCHAR));
            if (!extension)
            {
                WARN("Cannot allocate memory for extension!\n");
                error_out_of_memory();
                return 1;
            }

            _tcsncpy(extension, param, lpEqualSign - param);
            extension[lpEqualSign - param] = _T('\0');

            if (_tcslen(fileType) == 0)
            /* if the equal sign is the last character
            in the string, then delete the key */
            {
                RemoveAssociation(extension);
            }
            else
            /* otherwise, add the key and print out the association*/
            {
                AddAssociation(extension, fileType);
                PrintAssociation(extension);
            }

            cmd_free(extension);
        }
        else
        {
            /* no equal sign, print all associations */
            INT retval = PrintAssociation(param);

            if (retval == 0)    /* if nothing printed out */
                ConOutResPrintf(STRING_ASSOC_ERROR, param);
        }
    }

    return 0;
}

#endif /* INCLUDE_CMD_ASSOC */
