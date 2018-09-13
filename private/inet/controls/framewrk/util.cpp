//=--------------------------------------------------------------------------=
// Util.C
//=--------------------------------------------------------------------------=
// Copyright 1995-1996 Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
// contains routines that we will find useful.
//
#include "IPServer.H"

#include "Globals.H"
#include "Util.H"

void * _cdecl operator new(size_t size);
void  _cdecl operator delete(void *ptr);


// for ASSERT and FAIL
//
SZTHISFILE


//=---------------------------------------------------------------------------=
// overloaded new
//=---------------------------------------------------------------------------=
// for the retail case, we'll just use the win32 Local* heap management
// routines for speed and size
//
// Parameters:
//    size_t         - [in] what size do we alloc
//
// Output:
//    VOID *         - new memoery.
//
// Notes:
//
void * _cdecl operator new
(
    size_t    size
)
{
    return HeapAlloc(g_hHeap, 0, size);
}

//=---------------------------------------------------------------------------=
// overloaded delete
//=---------------------------------------------------------------------------=
// retail case just uses win32 Local* heap mgmt functions
//
// Parameters:
//    void *        - [in] free me!
//
// Notes:
//
void _cdecl operator delete ( void *ptr)
{
    HeapFree(g_hHeap, 0, ptr);
}

//=--------------------------------------------------------------------------=
// MakeWideFromAnsi
//=--------------------------------------------------------------------------=
// given a string, make a BSTR out of it.
//
// Parameters:
//    LPSTR         - [in]
//    BYTE          - [in]
//
// Output:
//    LPWSTR        - needs to be cast to final desired result
//
// Notes:
//
LPWSTR MakeWideStrFromAnsi
(
    LPSTR psz,
    BYTE  bType
)
{
    LPWSTR pwsz;
    int i;

    // arg checking.
    //
    if (!psz)
        return NULL;

    // compute the length of the required BSTR
    //
    i =  MultiByteToWideChar(CP_ACP, 0, psz, -1, NULL, 0);
    if (i <= 0) return NULL;

    // allocate the widestr
    //
    switch (bType) {
      case STR_BSTR:
        // -1 since it'll add it's own space for a NULL terminator
        //
        pwsz = (LPWSTR) SysAllocStringLen(NULL, i - 1);
        break;
      case STR_OLESTR:
        pwsz = (LPWSTR) CoTaskMemAlloc(i * sizeof(WCHAR));
        break;
      default:
        FAIL("Bogus String Type.");
    }

    if (!pwsz) return NULL;
    MultiByteToWideChar(CP_ACP, 0, psz, -1, pwsz, i);
    pwsz[i - 1] = 0;
    return pwsz;
}

//=--------------------------------------------------------------------------=
// MakeWideStrFromResId
//=--------------------------------------------------------------------------=
// given a resource ID, load it, and allocate a wide string for it.
//
// Parameters:
//    WORD            - [in] resource id.
//    BYTE            - [in] type of string desired.
//
// Output:
//    LPWSTR          - needs to be cast to desired string type.
//
// Notes:
//
LPWSTR MakeWideStrFromResourceId
(
    WORD    wId,
    BYTE    bType
)
{
    int i;

    char szTmp[512];

    // load the string from the resources.
    //
    i = LoadString(GetResourceHandle(), wId, szTmp, 512);
    if (!i) return NULL;

    return MakeWideStrFromAnsi(szTmp, bType);
}

//=--------------------------------------------------------------------------=
// MakeWideStrFromWide
//=--------------------------------------------------------------------------=
// given a wide string, make a new wide string with it of the given type.
//
// Parameters:
//    LPWSTR            - [in]  current wide str.
//    BYTE              - [in]  desired type of string.
//
// Output:
//    LPWSTR
//
// Notes:
//
LPWSTR MakeWideStrFromWide
(
    LPWSTR pwsz,
    BYTE   bType
)
{
    LPWSTR pwszTmp;
    int i;

    if (!pwsz) return NULL;

    // just copy the string, depending on what type they want.
    //
    switch (bType) {
      case STR_OLESTR:
        i = lstrlenW(pwsz);
        pwszTmp = (LPWSTR)CoTaskMemAlloc((i * sizeof(WCHAR)) + sizeof(WCHAR));
        if (!pwszTmp) return NULL;
        memcpy(pwszTmp, pwsz, (sizeof(WCHAR) * i) + sizeof(WCHAR));
        break;

      case STR_BSTR:
        pwszTmp = (LPWSTR)SysAllocString(pwsz);
        break;
    }

    return pwszTmp;
}

//=--------------------------------------------------------------------------=
// StringFromGuidA
//=--------------------------------------------------------------------------=
// returns an ANSI string from a CLSID or GUID
//
// Parameters:
//    REFIID               - [in]  clsid to make string out of.
//    LPSTR                - [in]  buffer in which to place resultant GUID.
//
// Output:
//    int                  - number of chars written out.
//
// Notes:
//
int StringFromGuidA
(
    REFIID   riid,
    LPSTR    pszBuf
)
{
    return wsprintf((char *)pszBuf, "{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}", riid.Data1,
            riid.Data2, riid.Data3, riid.Data4[0], riid.Data4[1], riid.Data4[2],
            riid.Data4[3], riid.Data4[4], riid.Data4[5], riid.Data4[6], riid.Data4[7]);

}

//=--------------------------------------------------------------------------=
// RegisterUnknownObject
//=--------------------------------------------------------------------------=
// registers a simple CoCreatable object.  nothing terribly serious.
// we add the following information to the registry:
//
// HKEY_CLASSES_ROOT\CLSID\<CLSID> = <ObjectName> Object
// HKEY_CLASSES_ROOT\CLSID\<CLSID>\InprocServer32 = <path to local server>
//
// Parameters:
//    LPCSTR       - [in] Object Name
//    REFCLSID     - [in] CLSID of the object
//
// Output:
//    BOOL         - FALSE means couldn't register it all
//
// Notes:
//
BOOL RegisterUnknownObject
(
    LPCSTR   pszObjectName,
    REFCLSID riidObject
)
{
    HKEY  hk = NULL, hkSub = NULL;
    char  szGuidStr[GUID_STR_LEN];
    DWORD dwPathLen, dwDummy;
    char  szScratch[MAX_PATH];
    long  l;

    // clean out any garbage
    //
    UnregisterUnknownObject(riidObject);

    // HKEY_CLASSES_ROOT\CLSID\<CLSID> = <ObjectName> Object
    // HKEY_CLASSES_ROOT\CLSID\<CLSID>\InprocServer32 = <path to local server>
    // HKEY_CLASSES_ROOT\CLSID\<CLSID>\InprocServer32  @ThreadingModel = Apartment
    //
    if (!StringFromGuidA(riidObject, szGuidStr)) goto CleanUp;
    wsprintf(szScratch, "CLSID\\%s", szGuidStr);
    l = RegCreateKeyEx(HKEY_CLASSES_ROOT, szScratch, 0, "", REG_OPTION_NON_VOLATILE,
                       KEY_READ | KEY_WRITE, NULL, &hk, &dwDummy);
    CLEANUP_ON_ERROR(l);

    wsprintf(szScratch, "%s Object", pszObjectName);
    l = RegSetValueEx(hk, NULL, 0, REG_SZ, (BYTE *)szScratch, lstrlen(szScratch) + 1);
    CLEANUP_ON_ERROR(l);

    l = RegCreateKeyEx(hk, "InprocServer32", 0, "", REG_OPTION_NON_VOLATILE,
                       KEY_READ | KEY_WRITE, NULL, &hkSub, &dwDummy);
    CLEANUP_ON_ERROR(l);

    dwPathLen = GetModuleFileName(g_hInstance, szScratch, sizeof(szScratch));
    if (!dwPathLen) goto CleanUp;

    l = RegSetValueEx(hkSub, NULL, 0, REG_SZ, (BYTE *)szScratch, dwPathLen + 1);
    CLEANUP_ON_ERROR(l);

    l = RegSetValueEx(hkSub, "ThreadingModel", 0, REG_SZ, (BYTE *)"Apartment", sizeof("Apartment"));
    CLEANUP_ON_ERROR(l);

    RegCloseKey(hkSub);
    RegCloseKey(hk);

    return TRUE;

    // we are not very happy!
    //
  CleanUp:
    if (hk) RegCloseKey(hk);
    if (hkSub) RegCloseKey(hkSub);
    return FALSE;

}

//=--------------------------------------------------------------------------=
// RegisterAutomationObject
//=--------------------------------------------------------------------------=
// given a little bit of information about an automation object, go and put it
// in the registry.
// we add the following information in addition to that set up in
// RegisterUnknownObject:
//
//
// HKEY_CLASSES_ROOT\<LibraryName>.<ObjectName> = <ObjectName> Object
// HKEY_CLASSES_ROOT\<LibraryName>.<ObjectName>\CLSID = <CLSID>
// HKEY_CLASSES_ROOT\<LibraryName>.<ObjectName>\CurVer = <ObjectName>.Object.<VersionNumber>
//
// HKEY_CLASSES_ROOT\<LibraryName>.<ObjectName>.<VersionNumber> = <ObjectName> Object
// HKEY_CLASSES_ROOT\<LibraryName>.<ObjectName>.<VersionNumber>\CLSID = <CLSID>
//
// HKEY_CLASSES_ROOT\CLSID\<CLSID>\TypeLib = <LibidOfTypeLibrary>
// HKEY_CLASSES_ROOT\CLSID\<CLSID>\ProgID = <LibraryName>.<ObjectName>.<VersionNumber>
// HKEY_CLASSES_ROOT\CLSID\<CLSID>\VersionIndependentProgID = <LibraryName>.<ObjectName>
//
// Parameters:
//    LPCSTR       - [in] Library Name
//    LPCSTR       - [in] Object Name
//    long         - [in] Version Number
//    REFCLSID     - [in] LIBID of type library
//    REFCLSID     - [in] CLSID of the object
//
// Output:
//    BOOL         - FALSE means not all of it was registered
//
// Notes:
//
BOOL RegisterAutomationObject
(
    LPCSTR   pszLibName,
    LPCSTR   pszObjectName,
    long     lVersion,
    REFCLSID riidLibrary,
    REFCLSID riidObject
)
{
    HKEY  hk = NULL, hkSub = NULL;
    char  szGuidStr[GUID_STR_LEN];
    char  szScratch[MAX_PATH];
    long  l;
    DWORD dwDummy;

    // first register the simple Unknown stuff.
    //
    if (!RegisterUnknownObject(pszObjectName, riidObject)) return FALSE;

    // HKEY_CLASSES_ROOT\<LibraryName>.<ObjectName> = <ObjectName> Object
    // HKEY_CLASSES_ROOT\<LibraryName>.<ObjectName>\CLSID = <CLSID>
    // HKEY_CLASSES_ROOT\<LibraryName>.<ObjectName>\CurVer = <ObjectName>.Object.<VersionNumber>
    //
    lstrcpy(szScratch, pszLibName);
    lstrcat(szScratch, ".");
    lstrcat(szScratch, pszObjectName);

    l = RegCreateKeyEx(HKEY_CLASSES_ROOT, szScratch, 0L, "",
                       REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE,
                       NULL, &hk, &dwDummy);
    CLEANUP_ON_ERROR(l);

    wsprintf(szScratch, "%s Object", pszObjectName);
    l = RegSetValueEx(hk, NULL, 0L, REG_SZ, (BYTE *)szScratch, lstrlen(szScratch)+1);
    CLEANUP_ON_ERROR(l);

    l = RegCreateKeyEx(hk, "CLSID", 0L, "", REG_OPTION_NON_VOLATILE,
                       KEY_READ | KEY_WRITE, NULL, &hkSub, &dwDummy);
    CLEANUP_ON_ERROR(l);

    if (!StringFromGuidA(riidObject, szGuidStr))
        goto CleanUp;

    l = RegSetValueEx(hkSub, NULL, 0L, REG_SZ, (BYTE *)szGuidStr, lstrlen(szGuidStr) + 1);
    CLEANUP_ON_ERROR(l);

    RegCloseKey(hkSub);
    l = RegCreateKeyEx(hk, "CurVer", 0, "", REG_OPTION_NON_VOLATILE,
                       KEY_READ | KEY_WRITE, NULL, &hkSub, &dwDummy);
    CLEANUP_ON_ERROR(l);

    wsprintf(szScratch, "%s.%s.%ld", pszLibName, pszObjectName, lVersion);
    l = RegSetValueEx(hkSub, NULL, 0, REG_SZ, (BYTE *)szScratch, lstrlen(szScratch) + 1);
    CLEANUP_ON_ERROR(l);

    RegCloseKey(hkSub);
    RegCloseKey(hk);

    // HKEY_CLASSES_ROOT\<LibraryName>.<ObjectName>.<VersionNumber> = <ObjectName> Object
    // HKEY_CLASSES_ROOT\<LibraryName>.<ObjectName>.<VersionNumber>\CLSID = <CLSID>
    //
    l = RegCreateKeyEx(HKEY_CLASSES_ROOT, szScratch, 0, "", REG_OPTION_NON_VOLATILE,
                       KEY_READ | KEY_WRITE, NULL, &hk, &dwDummy);
    CLEANUP_ON_ERROR(l);

    wsprintf(szScratch, "%s Object", pszObjectName);
    l = RegSetValueEx(hk, NULL, 0, REG_SZ, (BYTE *)szScratch, lstrlen(szScratch) + 1);
    CLEANUP_ON_ERROR(l);

    l = RegCreateKeyEx(hk, "CLSID", 0, "", REG_OPTION_NON_VOLATILE,
                       KEY_READ | KEY_WRITE, NULL, &hkSub, &dwDummy);
    CLEANUP_ON_ERROR(l);

    l = RegSetValueEx(hkSub, NULL, 0, REG_SZ, (BYTE *)szGuidStr, lstrlen(szGuidStr) + 1);
    CLEANUP_ON_ERROR(l);

    RegCloseKey(hkSub);
    RegCloseKey(hk);

    // HKEY_CLASSES_ROOT\CLSID\<CLSID>\ProgID = <LibraryName>.<ObjectName>.<VersionNumber>
    // HKEY_CLASSES_ROOT\CLSID\<CLSID>\VersionIndependentProgID = <LibraryName>.<ObjectName>
    // HKEY_CLASSES_ROOT\CLSID\<CLSID>\TypeLib = <LibidOfTypeLibrary>
    //
    if (!StringFromGuidA(riidObject, szGuidStr)) goto CleanUp;
    wsprintf(szScratch, "CLSID\\%s", szGuidStr);

    l = RegCreateKeyEx(HKEY_CLASSES_ROOT, szScratch, 0, "", REG_OPTION_NON_VOLATILE,
                       KEY_READ|KEY_WRITE, NULL, &hk, &dwDummy);
    CLEANUP_ON_ERROR(l);

    l = RegCreateKeyEx(hk, "VersionIndependentProgID", 0, "", REG_OPTION_NON_VOLATILE,
                       KEY_READ | KEY_WRITE, NULL, &hkSub, &dwDummy);
    CLEANUP_ON_ERROR(l);

    wsprintf(szScratch, "%s.%s", pszLibName, pszObjectName);
    l = RegSetValueEx(hkSub, NULL, 0, REG_SZ, (BYTE *)szScratch, lstrlen(szScratch) + 1);
    CLEANUP_ON_ERROR(l);

    RegCloseKey(hkSub);

    l = RegCreateKeyEx(hk, "ProgID", 0, "", REG_OPTION_NON_VOLATILE,
                       KEY_READ | KEY_WRITE, NULL, &hkSub, &dwDummy);
    CLEANUP_ON_ERROR(l);

    wsprintf(szScratch, "%s.%s.%ld", pszLibName, pszObjectName, lVersion);
    l = RegSetValueEx(hkSub, NULL, 0, REG_SZ, (BYTE *)szScratch, lstrlen(szScratch) + 1);
    CLEANUP_ON_ERROR(l);

    RegCloseKey(hkSub);
    l = RegCreateKeyEx(hk, "TypeLib", 0, "", REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE,
                       NULL, &hkSub, &dwDummy);

    if (!StringFromGuidA(riidLibrary, szGuidStr)) goto CleanUp;

    l = RegSetValueEx(hkSub, NULL, 0, REG_SZ, (BYTE *)szGuidStr, lstrlen(szGuidStr) + 1);
    CLEANUP_ON_ERROR(l);

    RegCloseKey(hkSub);
    RegCloseKey(hk);
    return TRUE;

  CleanUp:
    if (hk) RegCloseKey(hkSub);
    if (hk) RegCloseKey(hk);
    return FALSE;
}

//=--------------------------------------------------------------------------=
// RegisterControlObject.
//=--------------------------------------------------------------------------=
// in addition to writing out automation object information, this function
// writes out some values specific to a control.
//
// What we add here:
//
// HKEY_CLASSES_ROOT\CLSID\<CLSID>\Control
// HKEY_CLASSES_ROOT\CLSID\<CLSID>\MiscStatus\1 = <MISCSTATUSBITS>
// HKEY_CLASSES_ROOT\CLSID\<CLSID>\ToolboxBitmap32 = <PATH TO BMP>
// HKEY_CLASSES_ROOT\CLSID\<CLSID>\Version = <VERSION>
//
// Parameters:
//    LPCSTR       - [in] Library Name
//    LPCSTR       - [in] Object Name
//    long         - [in] Version Number
//    REFCLSID     - [in] LIBID of type library
//    REFCLSID     - [in] CLSID of the object
//    DWORD        - [in] misc status flags for ctl
//    WORD         - [in] toolbox id for control
//
// Output:
//    BOOL
//
// Notes:
//    - not the most terribly efficient routine.
//
BOOL RegisterControlObject
(
    LPCSTR   pszLibName,
    LPCSTR   pszObjectName,
    long     lVersion,
    REFCLSID riidLibrary,
    REFCLSID riidObject,
    DWORD    dwMiscStatus,
    WORD     wToolboxBitmapId
)
{
    HKEY    hk, hkSub = NULL, hkSub2 = NULL;
    char    szTmp[MAX_PATH];
    char    szGuidStr[GUID_STR_LEN];
    DWORD   dwDummy;
    LONG    l;

    // first register all the automation information for this sucker.
    //
    if (!RegisterAutomationObject(pszLibName, pszObjectName, lVersion, riidLibrary, riidObject)) return FALSE;

    // then go and register the control specific stuff.
    //
    StringFromGuidA(riidObject, szGuidStr);
    wsprintf(szTmp, "CLSID\\%s", szGuidStr);
    l = RegOpenKeyEx(HKEY_CLASSES_ROOT, szTmp, 0, KEY_ALL_ACCESS, &hk);
    if (l != ERROR_SUCCESS) return FALSE;

    // create the control flag.
    //
    l = RegCreateKeyEx(hk, "Control", 0, "", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkSub, &dwDummy);
    CLEANUP_ON_ERROR(l);

    // now set up the MiscStatus Bits...
    //
    RegCloseKey(hkSub);
    hkSub = NULL;
    l = RegCreateKeyEx(hk, "MiscStatus", 0, "", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkSub, &dwDummy);
    CLEANUP_ON_ERROR(l);

    szTmp[0] = '0';
    szTmp[1] = '\0';
    l = RegSetValueEx(hkSub, NULL, 0, REG_SZ, (BYTE *)szTmp, 2);
    CLEANUP_ON_ERROR(l);

    l = RegCreateKeyEx(hkSub, "1", 0, "", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkSub2, &dwDummy);
    CLEANUP_ON_ERROR(l);

    wsprintf(szTmp, "%d", dwMiscStatus);
    l = RegSetValueEx(hkSub2, NULL, 0, REG_SZ, (BYTE *)szTmp, lstrlen(szTmp) + 1);
    RegCloseKey(hkSub2);
    CLEANUP_ON_ERROR(l);

    RegCloseKey(hkSub);

    // now set up the toolbox bitmap
    //
    GetModuleFileName(g_hInstance, szTmp, MAX_PATH);
    wsprintf(szGuidStr, ", %d", wToolboxBitmapId);
    lstrcat(szTmp, szGuidStr);

    l = RegCreateKeyEx(hk, "ToolboxBitmap32", 0, "", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkSub, &dwDummy);
    CLEANUP_ON_ERROR(l);

    l = RegSetValueEx(hkSub, NULL, 0, REG_SZ, (BYTE *)szTmp, lstrlen(szTmp) + 1);
    CLEANUP_ON_ERROR(l);

    // now set up the version information
    //
    RegCloseKey(hkSub);
    l = RegCreateKeyEx(hk, "Version", 0, "", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkSub, &dwDummy);
    CLEANUP_ON_ERROR(l);

    wsprintf(szTmp, "%ld.0", lVersion);
    l = RegSetValueEx(hkSub, NULL, 0, REG_SZ, (BYTE *)szTmp, lstrlen(szTmp) + 1);

  CleanUp:
    if (hk)
        RegCloseKey(hk);
    if (hkSub)
        RegCloseKey(hkSub);

    return (l == ERROR_SUCCESS) ? TRUE : FALSE;
}

//=--------------------------------------------------------------------------=
// UnregisterUnknownObject
//=--------------------------------------------------------------------------=
// cleans up all the stuff that RegisterUnknownObject puts in the
// registry.
//
// Parameters:
//    REFCLSID     - [in] CLSID of the object
//
// Output:
//    BOOL         - FALSE means not all of it was registered
//
// Notes:
//    - WARNING: this routine will blow away all other keys under the CLSID
//      for this object.  mildly anti-social, but likely not a problem.
//
BOOL UnregisterUnknownObject
(
    REFCLSID riidObject
)
{
    char szScratch[MAX_PATH];
    HKEY hk;
    BOOL f;
    long l;

    // delete everybody of the form
    //   HKEY_CLASSES_ROOT\CLSID\<CLSID> [\] *
    //
    if (!StringFromGuidA(riidObject, szScratch))
        return FALSE;

    l = RegOpenKeyEx(HKEY_CLASSES_ROOT, "CLSID", 0, KEY_ALL_ACCESS, &hk);
    if (l != ERROR_SUCCESS) return FALSE;

    f = DeleteKeyAndSubKeys(hk, szScratch);
    RegCloseKey(hk);

    return f;
}

//=--------------------------------------------------------------------------=
// UnregisterAutomationObject
//=--------------------------------------------------------------------------=
// unregisters an automation object, including all of it's unknown object
// information.
//
// Parameters:
//    LPCSTR       - [in] Library Name
//    LPCSTR       - [in] Object Name
//    long         - [in] Version Number
//    REFCLSID     - [in] CLSID of the object
//
// Output:
//    BOOL         - FALSE means couldn't get it all unregistered.
//
// Notes:
//
BOOL UnregisterAutomationObject
(
    LPCSTR   pszLibName,
    LPCSTR   pszObjectName,
    long     lVersion,
    REFCLSID riidObject
)
{
    char szScratch[MAX_PATH];
    BOOL f;

    // first thing -- unregister Unknown information
    //
    f = UnregisterUnknownObject(riidObject);
    if (!f) return FALSE;

    // delete everybody of the form:
    //   HKEY_CLASSES_ROOT\<LibraryName>.<ObjectName> [\] *
    //
    wsprintf(szScratch, "%s.%s", pszLibName, pszObjectName);
    f = DeleteKeyAndSubKeys(HKEY_CLASSES_ROOT, szScratch);
    if (!f) return FALSE;

    // delete everybody of the form
    //   HKEY_CLASSES_ROOT\<LibraryName>.<ObjectName>.<VersionNumber> [\] *
    //
    wsprintf(szScratch, "%s.%s.%ld", pszLibName, pszObjectName, lVersion);
    f = DeleteKeyAndSubKeys(HKEY_CLASSES_ROOT, szScratch);
    if (!f) return FALSE;

    return TRUE;
}

//=--------------------------------------------------------------------------=
// UnregisterTypeLibrary
//=--------------------------------------------------------------------------=
// blows away the type library keys for a given libid.
//
// Parameters:
//    REFCLSID        - [in] libid to blow away.
//
// Output:
//    BOOL            - TRUE OK, FALSE bad.
//
// Notes:
//    - WARNING: this function just blows away the entire type library section,
//      including all localized versions of the type library.  mildly anti-
//      social, but not killer.
//
BOOL UnregisterTypeLibrary
(
    REFCLSID riidLibrary
)
{
    HKEY hk;
    char szScratch[GUID_STR_LEN];
    long l;
    BOOL f;

    // convert the libid into a string.
    //
    if (!StringFromGuidA(riidLibrary, szScratch))
        return FALSE;

    l = RegOpenKeyEx(HKEY_CLASSES_ROOT, "TypeLib", 0, KEY_ALL_ACCESS, &hk);
    if (l != ERROR_SUCCESS) return FALSE;

    f = DeleteKeyAndSubKeys(hk, szScratch);
    RegCloseKey(hk);
    return f;
}

//=--------------------------------------------------------------------------=
// DeleteKeyAndSubKeys
//=--------------------------------------------------------------------------=
// delete's a key and all of it's subkeys.
//
// Parameters:
//    HKEY                - [in] delete the descendant specified
//    LPSTR               - [in] i'm the descendant specified
//
// Output:
//    BOOL                - TRUE OK, FALSE baaaad.
//
// Notes:
//    - I don't feel too bad about implementing this recursively, since the
//      depth isn't likely to get all the great.
//    - Despite the win32 docs claiming it does, RegDeleteKey doesn't seem to
//      work with sub-keys under windows 95.
//
//    - REWRITTEN: To actually work as expected (07/30/97 -- jaym)
BOOL DeleteKeyAndSubKeys
(
    HKEY    hkIn,
    LPSTR   pszSubKey
)
{
    DWORD   dwRet;
    HKEY    hkSubKey;

    // Open the subkey so we can enumerate any children
    dwRet = RegOpenKeyEx(hkIn, pszSubKey, 0, KEY_ALL_ACCESS, &hkSubKey);
    if (dwRet == ERROR_SUCCESS)
    {
        DWORD   dwIndex;
        CHAR    szSubKeyName[MAX_PATH + 1];
        DWORD   cchSubKeyName = sizeof(szSubKeyName);
        CHAR    szClass[MAX_PATH];
        DWORD   cbClass = sizeof(szClass);

        // I can't just call RegEnumKey with an ever-increasing index, because
        // I'm deleting the subkeys as I go, which alters the indices of the
        // remaining subkeys in an implementation-dependent way.  In order to
        // be safe, I have to count backwards while deleting the subkeys.

        // Find out how many subkeys there are
        dwRet = RegQueryInfoKey(hkSubKey,
                                szClass,
                                &cbClass,
                                NULL,
                                &dwIndex, // The # of subkeys -- all we need
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL);

        if (dwRet == NO_ERROR)
        {
            // dwIndex is now the count of subkeys, but it needs to be
            // zero-based for RegEnumKey, so I'll pre-decrement, rather
            // than post-decrement.
            while (RegEnumKey(  hkSubKey,
                                --dwIndex,
                                szSubKeyName,
                                cchSubKeyName) == ERROR_SUCCESS)
            {
                DeleteKeyAndSubKeys(hkSubKey, szSubKeyName);
            }
        }

        RegCloseKey(hkSubKey);

        dwRet = RegDeleteKey(hkIn, pszSubKey);
    }

    return (dwRet == ERROR_SUCCESS);
}


//=--------------------------------------------------------------------------=
// Conversion Routines
//=--------------------------------------------------------------------------=
// the following stuff is stuff used for the various conversion routines.
//
#define HIMETRIC_PER_INCH   2540
#define MAP_PIX_TO_LOGHIM(x,ppli)   ( (HIMETRIC_PER_INCH*(x) + ((ppli)>>1)) / (ppli) )
#define MAP_LOGHIM_TO_PIX(x,ppli)   ( ((ppli)*(x) + HIMETRIC_PER_INCH/2) / HIMETRIC_PER_INCH )

static  int     s_iXppli;            // Pixels per logical inch along width
static  int     s_iYppli;            // Pixels per logical inch along height
static  BYTE    s_fGotScreenMetrics; // Are above valid?

//=--------------------------------------------------------------------------=
// GetScreenMetrics
//=--------------------------------------------------------------------------=
// private function we call to set up various metrics the conversion routines
// will use.
//
// Notes:
//
static void GetScreenMetrics
(
    void
)
{
    HDC hDCScreen;

    // we have to critical section this in case two threads are converting
    // things at the same time
    //
    EnterCriticalSection(&g_CriticalSection);
    if (s_fGotScreenMetrics)
        goto Done;

    // we want the metrics for the screen
    //
    hDCScreen = GetDC(NULL);

    ASSERT(hDCScreen, "couldn't get a DC for the screen.");
    s_iXppli = GetDeviceCaps(hDCScreen, LOGPIXELSX);
    s_iYppli = GetDeviceCaps(hDCScreen, LOGPIXELSY);

    ReleaseDC(NULL, hDCScreen);
    s_fGotScreenMetrics = TRUE;

    // we're done with our critical seciton.  clean it up
    //
  Done:
    LeaveCriticalSection(&g_CriticalSection);
}

//=--------------------------------------------------------------------------=
// HiMetricToPixel
//=--------------------------------------------------------------------------=
// converts from himetric to Pixels.
//
// Parameters:
//    const SIZEL *        - [in]  dudes in himetric
//    SIZEL *              - [out] size in pixels.
//
// Notes:
//
void HiMetricToPixel(const SIZEL * lpSizeInHiMetric, LPSIZEL lpSizeInPix)
{
    GetScreenMetrics();

    // We got logical HIMETRIC along the display, convert them to pixel units
    //
    lpSizeInPix->cx = MAP_LOGHIM_TO_PIX(lpSizeInHiMetric->cx, s_iXppli);
    lpSizeInPix->cy = MAP_LOGHIM_TO_PIX(lpSizeInHiMetric->cy, s_iYppli);
}

//=--------------------------------------------------------------------------=
// PixelToHiMetric
//=--------------------------------------------------------------------------=
// converts from pixels to himetric.
//
// Parameters:
//    const SIZEL *        - [in]  size in pixels
//    SIZEL *              - [out] size in himetric
//
// Notes:
//
void PixelToHiMetric(const SIZEL * lpSizeInPix, LPSIZEL lpSizeInHiMetric)
{
    GetScreenMetrics();

    // We got pixel units, convert them to logical HIMETRIC along the display
    //
    lpSizeInHiMetric->cx = MAP_PIX_TO_LOGHIM(lpSizeInPix->cx, s_iXppli);
    lpSizeInHiMetric->cy = MAP_PIX_TO_LOGHIM(lpSizeInPix->cy, s_iYppli);
}

//=--------------------------------------------------------------------------=
// _MakePath
//=--------------------------------------------------------------------------=
// little helper routine for RegisterLocalizedTypeLibs and GetResourceHandle.
// not terrilby efficient or smart, but it's registration code, so we don't
// really care.
//
// Notes:
//
void _MakePath
(
    LPSTR pszFull,
    const char * pszName,
    LPSTR pszOut
)
{
    LPSTR psz;
    LPSTR pszLast;

    lstrcpy(pszOut, pszFull);
    psz = pszLast = pszOut;
    while (*psz) {
        if (*psz == '\\')
            pszLast = AnsiNext(psz);
        psz = AnsiNext(psz);
    }

    // got the last \ character, so just go and replace the name.
    //
    lstrcpy(pszLast, pszName);
}

// from Globals.C
//
extern HINSTANCE    g_hInstResources;

//=--------------------------------------------------------------------------=
// GetResourceHandle
//=--------------------------------------------------------------------------=
// returns the resource handle.  we use the host's ambient Locale ID to
// determine, from a table in the DLL, which satellite DLL to load for
// localized resources.
//
// Output:
//    HINSTANCE
//
// Notes:
//
HINSTANCE GetResourceHandle
(
    void
)
{
    int i;
    char szExtension[5], szTmp[MAX_PATH];
    char szDllName[MAX_PATH], szFinalName[MAX_PATH];

    // crit sect this so that we don't screw anything up.
    //
    EnterCriticalSection(&g_CriticalSection);

    // don't do anything if we don't have to
    //
    if (g_hInstResources || !g_fSatelliteLocalization)
        goto CleanUp;

    // we're going to call GetLocaleInfo to get the abbreviated name for the
    // LCID we've got.
    //
    i = GetLocaleInfo(g_lcidLocale, LOCALE_SABBREVLANGNAME, szExtension, sizeof(szExtension));
    if (!i) goto CleanUp;

    // we've got the language extension.  go and load the DLL name from the
    // resources and then tack on the extension.
    // please note that all inproc sers -must- have the string resource 1001
    // defined to the base name of the server if they wish to support satellite
    // localization.
    //
    i = LoadString(g_hInstance, 1001, szTmp, sizeof(szTmp));
    ASSERT(i, "This server doesn't have IDS_SERVERBASENAME defined in their resources!");
    if (!i) goto CleanUp;

    // got the basename and the extention. go and combine them, and then add
    // on the .DLL for them.
    //
    wsprintf(szDllName, "%s%s.DLL", szTmp, szExtension);

    // try to load in the DLL
    //
    GetModuleFileName(g_hInstance, szTmp, MAX_PATH);
    _MakePath(szTmp, szDllName, szFinalName);

    g_hInstResources = LoadLibrary(szFinalName);

    // if we couldn't find it with the entire LCID, try it with just the primary
    // langid
    //
    if (!g_hInstResources) {
        LPSTR psz;
        LCID lcid;
        lcid = MAKELCID(MAKELANGID(PRIMARYLANGID(LANGIDFROMLCID(g_lcidLocale)), SUBLANG_DEFAULT), SORT_DEFAULT);
        i = GetLocaleInfo(lcid, LOCALE_SABBREVLANGNAME, szExtension, sizeof(szExtension));
        if (!i) goto CleanUp;

        // reconstruct the DLL name.  the -7 is the length of XXX.DLL. mildly
        // hacky, but it should be fine.  there are no DBCS lang identifiers.
        // finally, retry the load
        //
        psz = szFinalName + lstrlen(szFinalName);
        memcpy((LPBYTE)psz - 7, szExtension, 3);
        g_hInstResources = LoadLibrary(szFinalName);
    }

  CleanUp:
    // if we couldn't load the DLL for some reason, then just return the
    // current resource handle, which is good enough.
    //
    if (!g_hInstResources) g_hInstResources = g_hInstance;
    LeaveCriticalSection(&g_CriticalSection);

    return g_hInstResources;
}
