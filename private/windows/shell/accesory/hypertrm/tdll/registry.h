#if !defined(INCL_REGISTRY)
#define INCL_REGISTRY

/*	File: D:\WACKER\tdll\registry.h (Created: 27-Nov-1996 by cab)
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
 *	$Date: 10/05/98 12:35p $
 */

// regQueryValue
//
// A generic function to get a value from the registry.
// Returns 0 if successful, -1 if error.
//
int regQueryValue(HKEY hKey, LPCTSTR pszSubKey, LPCTSTR pszValue,
    LPBYTE pData, LPDWORD pdwDataSize);

// regSetStringValue
//
// A generic function to set the value of a registry entry. This value
// is a null-terminated string. Returns 0 if successful, -1 if error.
//
int regSetStringValue(HKEY hKey, LPCTSTR pszSubKey, LPCTSTR pszValue,
    LPCTSTR pszData);

// regSetDwordValue
//
// A generic function to set the value of a registry entry. This value
// is a doubleword (32 bits). Returns 0 if successful, -1 if error.
//
int regSetDwordValue(HKEY hKey, LPCTSTR pszSubKey, LPCTSTR pszValue,
    DWORD dwData);

#endif
