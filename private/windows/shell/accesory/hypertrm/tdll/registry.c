/*	File: D:\WACKER\tdll\registry.c (Created: 26-Nov-1996 by cab)
 *
 *	Copyright 1996 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *  Description:
 *      Declares the functions used for manipulating the Windows 95
 *      system registry.
 *
 *      An explanation of registry terms:
 *
 *          The Windows 95 registry uses the terms "keys", "values",
 *          and "data". The way the registry stores information can
 *          best be described as a folder analogy.
 *
 *          Keys are the equivalent of folders. The can contain other
 *          keys (subkeys) or values.
 *
 *          Values are the equivalent of documents. They contain data.
 *
 *          The data is the actual contents of the document, i.e. it
 *          is the information we are interested in.
 *
 *      An example:
 *
 *          HyperTerminal uses the registry to store the value of the
 *          "Don't ask me this question" check box of the "Default Telnet
 *          App" dialog.
 *
 *          The key for this is "HKEY_LOCAL_MACHINE\SOFTWARE\Hilgraeve\
 *          HyperTerminal PE\3.0".
 *
 *          The value for this is "Telnet Check".
 *
 *          The data for this will be either 0 or 1 depending on if the
 *          user wants HT to check if it is the default telnet app.
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:38p $
 */

#include <windows.h>
#pragma hdrstop

#include "assert.h"
#include "stdtyp.h"
#include "tchar.h"


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	regOpenKey
 *
 * DESCRIPTION:
 *	Opens the specified key. If the key does not exist, it will be
 *  created.
 *
 * PARAMETERS:
 *	hKey      - A pointer to an opened key.
 *  pszSubKey - The name of the subkey to open.
 *  samAccess - The type of access desired for the key.
 *  phOpenKey - Pointer to the opened key.
 *
 * RETURNS:
 *	ERROR_SUCCESS if successful, otherwise an error value.
 *
 * AUTHOR:  C. Baumgartner, 12/06/96
 */
long regOpenKey(HKEY hKey, LPCTSTR pszSubKey, REGSAM samAccess, HKEY* phOpenKey)
    {
    DWORD dwDisposition = 0;

    // Instead of calling RegOpenKeyEx, call RegCreateKeyEx which will return
    // an open in key, but will also create a key that does not exist.
    //
    return RegCreateKeyEx(hKey, pszSubKey, 0, 0, REG_OPTION_NON_VOLATILE,
        samAccess, NULL, phOpenKey, &dwDisposition);
    }

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	regQueryValue
 *
 * DESCRIPTION:
 *	A generic function to get a value from the registry.
 *
 * PARAMETERS:
 *	hKey        - A pointer to an opened key.
 *  pszSubKey   - The name of the subkey to open.
 *  pszValue    - The name of the value to query.
 *  pData       - The value's data.
 *  pdwDataSize - Upon input this must be the size of pData,
 *                on exit this will be the size of the data
 *                read.
 *
 * RETURNS:
 *	0 if successful, -1 if error.
 *
 * AUTHOR:  C. Baumgartner, 11/26/96
 */
 int regQueryValue(HKEY hKey, LPCTSTR pszSubKey, LPCTSTR pszValue,
        LPBYTE pData, LPDWORD pdwDataSize)
    {
    long lResult = ERROR_SUCCESS;
    HKEY hSubKey = 0;
    
    // Open the sub key with the given name.
    //
    lResult = regOpenKey(hKey, pszSubKey, KEY_READ, &hSubKey);

    if ( lResult == ERROR_SUCCESS )
        {
        // Get value of that subkey.
        //
        lResult = RegQueryValueEx(hSubKey, pszValue, NULL, NULL,
                pData, pdwDataSize);
        }

    if (hSubKey != 0)
        {
        RegCloseKey(hSubKey);
        }

    return lResult == ERROR_SUCCESS ? 0 : -1;
    }

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	regSetStringValue
 *
 * DESCRIPTION:
 *	A generic function to set the value of a registry entry. This value
 *  is a null-terminated string.
 *
 * PARAMETERS:
 *	hKey        - A pointer to an opened key.
 *  pszSubKey   - The name of the subkey to open.
 *  pszValue    - The name of the value to query.
 *  pszData     - The value's *string* data.
 *
 * RETURNS:
 *	0 if successful, -1 if error.
 *
 * AUTHOR:  C. Baumgartner, 11/27/96
 */
int regSetStringValue(HKEY hKey, LPCTSTR pszSubKey, LPCTSTR pszValue,
        LPCTSTR pszData)
    {
    long  lResult = ERROR_SUCCESS;
    DWORD dwSize = 0;
    HKEY  hSubKey = 0;
    
    // Open the sub key with the given name.
    //
    lResult = regOpenKey(hKey, pszSubKey, KEY_WRITE, &hSubKey);

    if ( lResult == ERROR_SUCCESS )
        {
        // The size of the string must include the null terminator.
        //
        dwSize = StrCharGetByteCount(pszData) + sizeof(TCHAR);

        // Set value of that subkey.
        //
        lResult = RegSetValueEx(hSubKey, pszValue, 0, REG_SZ,
                pszData, dwSize);
        }

    if (hSubKey != 0)
        {
        RegCloseKey(hSubKey);
        }

    return lResult == ERROR_SUCCESS ? 0 : -1;
    }

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	regSetDwordValue
 *
 * DESCRIPTION:
 *	A generic function to set the value of a registry entry. This value
 *  is a doubleword (32 bits).
 *
 * PARAMETERS:
 *	hKey        - A pointer to an opened key.
 *  pszSubKey   - The name of the subkey to open.
 *  pszValue    - The name of the value to query.
 *  dwData      - The value's *doubleword* data.
 *
 * RETURNS:
 *	0 if successful, -1 if error.
 *
 * AUTHOR:  C. Baumgartner, 11/27/96
 */
int regSetDwordValue(HKEY hKey, LPCTSTR pszSubKey, LPCTSTR pszValue,
        DWORD dwData)
    {
    long  lResult = ERROR_SUCCESS;
    HKEY  hSubKey = 0;
    
    // Open the sub key with the given name.
    //
    lResult = regOpenKey(hKey, pszSubKey, KEY_WRITE, &hSubKey);

    if ( lResult == ERROR_SUCCESS )
        {
        // Set value of that subkey.
        //
        lResult = RegSetValueEx(hSubKey, pszValue, 0, REG_DWORD,
                (LPBYTE)&dwData, sizeof(dwData));
        }

    if (hSubKey != 0)
        {
        RegCloseKey(hSubKey);
        }

    return lResult == ERROR_SUCCESS ? 0 : -1;
    }

