/*
 * Tests for color profile functions
 *
 * Copyright 2004, 2005, 2006 Hans Leidekker
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winnls.h"
#include "wingdi.h"
#include "winuser.h"
#include "icm.h"

#include "wine/test.h"

static HMODULE hmscms;
static HMODULE huser32;

static BOOL     (WINAPI *pAssociateColorProfileWithDeviceA)(PCSTR,PCSTR,PCSTR);
static BOOL     (WINAPI *pCloseColorProfile)(HPROFILE);
static HTRANSFORM (WINAPI *pCreateMultiProfileTransform)(PHPROFILE,DWORD,PDWORD,DWORD,DWORD,DWORD);
static BOOL     (WINAPI *pDeleteColorTransform)(HTRANSFORM);
static BOOL     (WINAPI *pDisassociateColorProfileFromDeviceA)(PCSTR,PCSTR,PCSTR);
static BOOL     (WINAPI *pGetColorDirectoryA)(PCHAR,PCHAR,PDWORD);
static BOOL     (WINAPI *pGetColorDirectoryW)(PWCHAR,PWCHAR,PDWORD);
static BOOL     (WINAPI *pGetColorProfileElement)(HPROFILE,TAGTYPE,DWORD,PDWORD,PVOID,PBOOL);
static BOOL     (WINAPI *pGetColorProfileElementTag)(HPROFILE,DWORD,PTAGTYPE);
static BOOL     (WINAPI *pGetColorProfileFromHandle)(HPROFILE,PBYTE,PDWORD);
static BOOL     (WINAPI *pGetColorProfileHeader)(HPROFILE,PPROFILEHEADER);
static BOOL     (WINAPI *pGetCountColorProfileElements)(HPROFILE,PDWORD);
static BOOL     (WINAPI *pGetStandardColorSpaceProfileA)(PCSTR,DWORD,PSTR,PDWORD);
static BOOL     (WINAPI *pGetStandardColorSpaceProfileW)(PCWSTR,DWORD,PWSTR,PDWORD);
static BOOL     (WINAPI *pEnumColorProfilesA)(PCSTR,PENUMTYPEA,PBYTE,PDWORD,PDWORD);
static BOOL     (WINAPI *pEnumColorProfilesW)(PCWSTR,PENUMTYPEW,PBYTE,PDWORD,PDWORD);
static BOOL     (WINAPI *pInstallColorProfileA)(PCSTR,PCSTR);
static BOOL     (WINAPI *pInstallColorProfileW)(PCWSTR,PCWSTR);
static BOOL     (WINAPI *pIsColorProfileTagPresent)(HPROFILE,TAGTYPE,PBOOL);
static HPROFILE (WINAPI *pOpenColorProfileA)(PPROFILE,DWORD,DWORD,DWORD);
static HPROFILE (WINAPI *pOpenColorProfileW)(PPROFILE,DWORD,DWORD,DWORD);
static BOOL     (WINAPI *pSetColorProfileElement)(HPROFILE,TAGTYPE,DWORD,PDWORD,PVOID);
static BOOL     (WINAPI *pSetColorProfileHeader)(HPROFILE,PPROFILEHEADER);
static BOOL     (WINAPI *pSetStandardColorSpaceProfileA)(PCSTR,DWORD,PSTR);
static BOOL     (WINAPI *pSetStandardColorSpaceProfileW)(PCWSTR,DWORD,PWSTR);
static BOOL     (WINAPI *pUninstallColorProfileA)(PCSTR,PCSTR,BOOL);
static BOOL     (WINAPI *pUninstallColorProfileW)(PCWSTR,PCWSTR,BOOL);

static BOOL     (WINAPI *pEnumDisplayDevicesA)(LPCSTR,DWORD,PDISPLAY_DEVICEA,DWORD);

#define GETFUNCPTR(func) p##func = (void *)GetProcAddress( hmscms, #func ); \
    if (!p##func) return FALSE;

static BOOL init_function_ptrs( void )
{
    GETFUNCPTR( AssociateColorProfileWithDeviceA )
    GETFUNCPTR( CloseColorProfile )
    GETFUNCPTR( CreateMultiProfileTransform )
    GETFUNCPTR( DeleteColorTransform )
    GETFUNCPTR( DisassociateColorProfileFromDeviceA )
    GETFUNCPTR( GetColorDirectoryA )
    GETFUNCPTR( GetColorDirectoryW )
    GETFUNCPTR( GetColorProfileElement )
    GETFUNCPTR( GetColorProfileElementTag )
    GETFUNCPTR( GetColorProfileFromHandle )
    GETFUNCPTR( GetColorProfileHeader )
    GETFUNCPTR( GetCountColorProfileElements )
    GETFUNCPTR( GetStandardColorSpaceProfileA )
    GETFUNCPTR( GetStandardColorSpaceProfileW )
    GETFUNCPTR( EnumColorProfilesA )
    GETFUNCPTR( EnumColorProfilesW )
    GETFUNCPTR( InstallColorProfileA )
    GETFUNCPTR( InstallColorProfileW )
    GETFUNCPTR( IsColorProfileTagPresent )
    GETFUNCPTR( OpenColorProfileA )
    GETFUNCPTR( OpenColorProfileW )
    GETFUNCPTR( SetColorProfileElement )
    GETFUNCPTR( SetColorProfileHeader )
    GETFUNCPTR( SetStandardColorSpaceProfileA )
    GETFUNCPTR( SetStandardColorSpaceProfileW )
    GETFUNCPTR( UninstallColorProfileA )
    GETFUNCPTR( UninstallColorProfileW )

    pEnumDisplayDevicesA = (void *)GetProcAddress( huser32, "EnumDisplayDevicesA" );

    return TRUE;
}

static const char machine[] = "dummy";
static const WCHAR machineW[] = { 'd','u','m','m','y',0 };

/*  To do any real functionality testing with this suite you need a copy of
 *  the freely distributable standard RGB color space profile. It comes
 *  standard with Windows, but on Wine you probably need to install it yourself
 *  in one of the locations mentioned below.
 */

/* Two common places to find the standard color space profile, relative
 * to the system directory.
 */
static const char profile1[] =
"\\color\\srgb color space profile.icm";
static const char profile2[] =
"\\spool\\drivers\\color\\srgb color space profile.icm";

static const WCHAR profile1W[] =
{ '\\','c','o','l','o','r','\\','s','r','g','b',' ','c','o','l','o','r',' ',
  's','p','a','c','e',' ','p','r','o','f','i','l','e','.','i','c','m',0 };
static const WCHAR profile2W[] =
{ '\\','s','p','o','o','l','\\','d','r','i','v','e','r','s','\\',
  'c','o','l','o','r','\\','s','r','g','b',' ','c','o','l','o','r',' ',
  's','p','a','c','e',' ','p','r','o','f','i','l','e','.','i','c','m',0 };

static BOOL have_color_profile;

static const unsigned char rgbheader[] =
{ 0x48, 0x0c, 0x00, 0x00, 0x6f, 0x6e, 0x69, 0x4c, 0x00, 0x00, 0x10, 0x02,
  0x72, 0x74, 0x6e, 0x6d, 0x20, 0x42, 0x47, 0x52, 0x20, 0x5a, 0x59, 0x58,
  0x02, 0x00, 0xce, 0x07, 0x06, 0x00, 0x09, 0x00, 0x00, 0x00, 0x31, 0x00,
  0x70, 0x73, 0x63, 0x61, 0x54, 0x46, 0x53, 0x4d, 0x00, 0x00, 0x00, 0x00,
  0x20, 0x43, 0x45, 0x49, 0x42, 0x47, 0x52, 0x73, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd6, 0xf6, 0x00, 0x00,
  0x00, 0x00, 0x01, 0x00, 0x2d, 0xd3, 0x00, 0x00, 0x20, 0x20, 0x50, 0x48 };

#define IS_SEPARATOR(ch)  ((ch) == '\\' || (ch) == '/')

static void MSCMS_basenameA( LPCSTR path, LPSTR name )
{
    INT i = strlen( path );

    while (i > 0 && !IS_SEPARATOR(path[i - 1])) i--;
    strcpy( name, &path[i] );
}

static void MSCMS_basenameW( LPCWSTR path, LPWSTR name )
{
    INT i = lstrlenW( path );

    while (i > 0 && !IS_SEPARATOR(path[i - 1])) i--;
    lstrcpyW( name, &path[i] );
}

static void test_GetColorDirectoryA(void)
{
    BOOL ret;
    DWORD size;
    char buffer[MAX_PATH];

    /* Parameter checks */

    ret = pGetColorDirectoryA( NULL, NULL, NULL );
    ok( !ret, "GetColorDirectoryA() succeeded (%d)\n", GetLastError() );

    size = 0;

    ret = pGetColorDirectoryA( NULL, NULL, &size );
    ok( !ret && size > 0, "GetColorDirectoryA() succeeded (%d)\n", GetLastError() );

    size = 0;

    ret = pGetColorDirectoryA( NULL, buffer, &size );
    ok( !ret && size > 0, "GetColorDirectoryA() succeeded (%d)\n", GetLastError() );

    size = 1;

    ret = pGetColorDirectoryA( NULL, buffer, &size );
    ok( !ret && size > 0, "GetColorDirectoryA() succeeded (%d)\n", GetLastError() );

    /* Functional checks */

    size = sizeof(buffer);

    ret = pGetColorDirectoryA( NULL, buffer, &size );
    ok( ret && size > 0, "GetColorDirectoryA() failed (%d)\n", GetLastError() );
}

static void test_GetColorDirectoryW(void)
{
    BOOL ret;
    DWORD size;
    WCHAR buffer[MAX_PATH];

    /* Parameter checks */

    /* This one crashes win2k
    
    ret = pGetColorDirectoryW( NULL, NULL, NULL );
    ok( !ret, "GetColorDirectoryW() succeeded (%d)\n", GetLastError() );

     */

    size = 0;

    ret = pGetColorDirectoryW( NULL, NULL, &size );
    ok( !ret && size > 0, "GetColorDirectoryW() succeeded (%d)\n", GetLastError() );

    size = 0;

    ret = pGetColorDirectoryW( NULL, buffer, &size );
    ok( !ret && size > 0, "GetColorDirectoryW() succeeded (%d)\n", GetLastError() );

    size = 1;

    ret = pGetColorDirectoryW( NULL, buffer, &size );
    ok( !ret && size > 0, "GetColorDirectoryW() succeeded (%d)\n", GetLastError() );

    /* Functional checks */

    size = sizeof(buffer);

    ret = pGetColorDirectoryW( NULL, buffer, &size );
    ok( ret && size > 0, "GetColorDirectoryW() failed (%d)\n", GetLastError() );
}

static void test_GetColorProfileElement( char *standardprofile )
{
    if (standardprofile)
    {
        PROFILE profile;
        HPROFILE handle;
        BOOL ret, ref;
        DWORD size;
        TAGTYPE tag = 0x63707274;  /* 'cprt' */
        static char buffer[51];
        static const char expect[] =
            { 0x74, 0x65, 0x78, 0x74, 0x00, 0x00, 0x00, 0x00, 0x43, 0x6f, 0x70,
              0x79, 0x72, 0x69, 0x67, 0x68, 0x74, 0x20, 0x28, 0x63, 0x29, 0x20,
              0x31, 0x39, 0x39, 0x38, 0x20, 0x48, 0x65, 0x77, 0x6c, 0x65, 0x74,
              0x74, 0x2d, 0x50, 0x61, 0x63, 0x6b, 0x61, 0x72, 0x64, 0x20, 0x43,
              0x6f, 0x6d, 0x70, 0x61, 0x6e, 0x79, 0x00 };

        profile.dwType = PROFILE_FILENAME;
        profile.pProfileData = standardprofile;
        profile.cbDataSize = strlen(standardprofile);

        handle = pOpenColorProfileA( &profile, PROFILE_READ, 0, OPEN_EXISTING );
        ok( handle != NULL, "OpenColorProfileA() failed (%d)\n", GetLastError() );

        /* Parameter checks */

        ret = pGetColorProfileElement( handle, tag, 0, NULL, NULL, &ref );
        ok( !ret, "GetColorProfileElement() succeeded (%d)\n", GetLastError() );

        ret = pGetColorProfileElement( handle, tag, 0, &size, NULL, NULL );
        ok( !ret, "GetColorProfileElement() succeeded (%d)\n", GetLastError() );

        size = 0;
        ret = pGetColorProfileElement( handle, tag, 0, &size, NULL, &ref );
        ok( !ret, "GetColorProfileElement() succeeded\n" );
        ok( size > 0, "wrong size\n" );

        /* Functional checks */

        size = sizeof(buffer);
        ret = pGetColorProfileElement( handle, tag, 0, &size, buffer, &ref );
        ok( ret, "GetColorProfileElement() failed %u\n", GetLastError() );
        ok( size > 0, "wrong size\n" );
        ok( !memcmp( buffer, expect, sizeof(expect) ), "Unexpected tag data\n" );

        pCloseColorProfile( handle );
    }
}

static void test_GetColorProfileElementTag( char *standardprofile )
{
    if (standardprofile)
    {
        PROFILE profile;
        HPROFILE handle;
        BOOL ret;
        DWORD index = 1;
        TAGTYPE tag, expect = 0x63707274;  /* 'cprt' */

        profile.dwType = PROFILE_FILENAME;
        profile.pProfileData = standardprofile;
        profile.cbDataSize = strlen(standardprofile);

        handle = pOpenColorProfileA( &profile, PROFILE_READ, 0, OPEN_EXISTING );
        ok( handle != NULL, "OpenColorProfileA() failed (%d)\n", GetLastError() );

        /* Parameter checks */

        ret = pGetColorProfileElementTag( NULL, index, &tag );
        ok( !ret, "GetColorProfileElementTag() succeeded (%d)\n", GetLastError() );

        ret = pGetColorProfileElementTag( handle, 0, &tag );
        ok( !ret, "GetColorProfileElementTag() succeeded (%d)\n", GetLastError() );

        ret = pGetColorProfileElementTag( handle, index, NULL );
        ok( !ret, "GetColorProfileElementTag() succeeded (%d)\n", GetLastError() );

        ret = pGetColorProfileElementTag( handle, 18, NULL );
        ok( !ret, "GetColorProfileElementTag() succeeded (%d)\n", GetLastError() );

        /* Functional checks */

        ret = pGetColorProfileElementTag( handle, index, &tag );
        ok( ret && tag == expect, "GetColorProfileElementTag() failed (%d)\n",
            GetLastError() );

        pCloseColorProfile( handle );
    }
}

static void test_GetColorProfileFromHandle( char *testprofile )
{
    if (testprofile)
    {
        PROFILE profile;
        HPROFILE handle;
        DWORD size;
        BOOL ret;
        static const unsigned char expect[] =
            { 0x00, 0x00, 0x0c, 0x48, 0x4c, 0x69, 0x6e, 0x6f, 0x02, 0x10, 0x00,
              0x00, 0x6d, 0x6e, 0x74, 0x72, 0x52, 0x47, 0x42, 0x20, 0x58, 0x59,
              0x5a, 0x20, 0x07, 0xce, 0x00, 0x02, 0x00, 0x09, 0x00, 0x06, 0x00,
              0x31, 0x00, 0x00, 0x61, 0x63, 0x73, 0x70, 0x4d, 0x53, 0x46, 0x54,
              0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x43, 0x20, 0x73, 0x52, 0x47,
              0x42, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0xf6, 0xd6, 0x00, 0x01, 0x00, 0x00, 0x00,
              0x00, 0xd3, 0x2d, 0x48, 0x50, 0x20, 0x20 };

        unsigned char *buffer;

        profile.dwType = PROFILE_FILENAME;
        profile.pProfileData = testprofile;
        profile.cbDataSize = strlen(testprofile);

        handle = pOpenColorProfileA( &profile, PROFILE_READ, 0, OPEN_EXISTING );
        ok( handle != NULL, "OpenColorProfileA() failed (%d)\n", GetLastError() );

        /* Parameter checks */

        size = 0;

        ret = pGetColorProfileFromHandle( handle, NULL, &size );
        ok( !ret && size > 0, "GetColorProfileFromHandle() failed (%d)\n", GetLastError() );

        buffer = HeapAlloc( GetProcessHeap(), 0, size );

        if (buffer)
        {
            ret = pGetColorProfileFromHandle( NULL, buffer, &size );
            ok( !ret, "GetColorProfileFromHandle() succeeded (%d)\n", GetLastError() );

            ret = pGetColorProfileFromHandle( handle, buffer, NULL );
            ok( !ret, "GetColorProfileFromHandle() succeeded (%d)\n", GetLastError() );

            /* Functional checks */

            ret = pGetColorProfileFromHandle( handle, buffer, &size );
            ok( ret && size > 0, "GetColorProfileFromHandle() failed (%d)\n", GetLastError() );

            ok( !memcmp( buffer, expect, sizeof(expect) ), "Unexpected header data\n" );

            HeapFree( GetProcessHeap(), 0, buffer );
        }

        pCloseColorProfile( handle );
    }
}

static void test_GetColorProfileHeader( char *testprofile )
{
    if (testprofile)
    {
        PROFILE profile;
        HPROFILE handle;
        BOOL ret;
        PROFILEHEADER header;

        profile.dwType = PROFILE_FILENAME;
        profile.pProfileData = testprofile;
        profile.cbDataSize = strlen(testprofile);

        handle = pOpenColorProfileA( &profile, PROFILE_READ, 0, OPEN_EXISTING );
        ok( handle != NULL, "OpenColorProfileA() failed (%d)\n", GetLastError() );

        /* Parameter checks */

        ret = pGetColorProfileHeader( NULL, NULL );
        ok( !ret, "GetColorProfileHeader() succeeded (%d)\n", GetLastError() );

        ret = pGetColorProfileHeader( NULL, &header );
        ok( !ret, "GetColorProfileHeader() succeeded (%d)\n", GetLastError() );

        if (0) /* Crashes on Vista */
        {
            ret = pGetColorProfileHeader( handle, NULL );
            ok( !ret, "GetColorProfileHeader() succeeded (%d)\n", GetLastError() );
        }

        /* Functional checks */

        ret = pGetColorProfileHeader( handle, &header );
        ok( ret, "GetColorProfileHeader() failed (%d)\n", GetLastError() );

        ok( !memcmp( &header, rgbheader, sizeof(rgbheader) ), "Unexpected header data\n" );

        pCloseColorProfile( handle );
    }
}

static void test_GetCountColorProfileElements( char *standardprofile )
{
    if (standardprofile)
    {
        PROFILE profile;
        HPROFILE handle;
        BOOL ret;
        DWORD count, expect = 17;

        profile.dwType = PROFILE_FILENAME;
        profile.pProfileData = standardprofile;
        profile.cbDataSize = strlen(standardprofile);

        handle = pOpenColorProfileA( &profile, PROFILE_READ, 0, OPEN_EXISTING );
        ok( handle != NULL, "OpenColorProfileA() failed (%d)\n", GetLastError() );

        /* Parameter checks */

        ret = pGetCountColorProfileElements( NULL, &count );
        ok( !ret, "GetCountColorProfileElements() succeeded (%d)\n",
            GetLastError() );

        ret = pGetCountColorProfileElements( handle, NULL );
        ok( !ret, "GetCountColorProfileElements() succeeded (%d)\n",
            GetLastError() );

        /* Functional checks */

        ret = pGetCountColorProfileElements( handle, &count );
        ok( ret && count == expect,
            "GetCountColorProfileElements() failed (%d)\n", GetLastError() );

        pCloseColorProfile( handle );
    }
}

static void test_GetStandardColorSpaceProfileA( char *standardprofile )
{
    BOOL ret;
    DWORD size;
    CHAR oldprofile[MAX_PATH];
    CHAR newprofile[MAX_PATH];

    /* Parameter checks */

    /* Single invalid parameter checks: */

    size = sizeof(newprofile);
    SetLastError(0xfaceabee); /* 1st param, */
    ret = pGetStandardColorSpaceProfileA(machine, LCS_sRGB, newprofile, &size);
    ok( !ret && GetLastError() == ERROR_NOT_SUPPORTED, "GetStandardColorSpaceProfileA() returns %d (GLE=%d)\n", ret, GetLastError() );

    size = sizeof(newprofile);
    SetLastError(0xfaceabee); /* 2nd param, */
    ret = pGetStandardColorSpaceProfileA(NULL, (DWORD)-1, newprofile, &size);
    ok( !ret && GetLastError() == ERROR_FILE_NOT_FOUND, "GetStandardColorSpaceProfileA() returns %d (GLE=%d)\n", ret, GetLastError() );

    size = sizeof(newprofile);
    SetLastError(0xfaceabee); /* 4th param, */
    ret = pGetStandardColorSpaceProfileA(NULL, LCS_sRGB, newprofile, NULL);
    ok( !ret && GetLastError() == ERROR_INVALID_PARAMETER, "GetStandardColorSpaceProfileA() returns %d (GLE=%d)\n", ret, GetLastError() );

    size = sizeof(newprofile);
    SetLastError(0xfaceabee); /* 3rd param, */
    ret = pGetStandardColorSpaceProfileA(NULL, LCS_sRGB, NULL, &size);
    ok( !ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER, "GetStandardColorSpaceProfileA() returns %d (GLE=%d)\n", ret, GetLastError() );

    size = 0;
    SetLastError(0xfaceabee); /* dereferenced 4th param, */
    ret = pGetStandardColorSpaceProfileA(NULL, LCS_sRGB, newprofile, &size);
    ok( !ret && (GetLastError() == ERROR_MORE_DATA || GetLastError() == ERROR_INSUFFICIENT_BUFFER),
        "GetStandardColorSpaceProfileA() returns %d (GLE=%d)\n", ret, GetLastError() );

    /* Several invalid parameter checks: */

    size = 0;
    SetLastError(0xfaceabee); /* 1st, maybe 2nd and then dereferenced 4th param, */
    ret = pGetStandardColorSpaceProfileA(machine, 0, newprofile, &size);
    ok( !ret && (GetLastError() == ERROR_INVALID_PARAMETER || GetLastError() == ERROR_NOT_SUPPORTED),
        "GetStandardColorSpaceProfileA() returns %d (GLE=%d)\n", ret, GetLastError() );

    SetLastError(0xfaceabee); /* maybe 2nd and then 4th param, */
    ret = pGetStandardColorSpaceProfileA(NULL, 0, newprofile, NULL);
    ok( !ret && GetLastError() == ERROR_INVALID_PARAMETER, "GetStandardColorSpaceProfileA() returns %d (GLE=%d)\n", ret, GetLastError() );

    size = 0;
    SetLastError(0xfaceabee); /* maybe 2nd, then 3rd and dereferenced 4th param, */
    ret = pGetStandardColorSpaceProfileA(NULL, 0, NULL, &size);
    ok( !ret && (GetLastError() == ERROR_INSUFFICIENT_BUFFER || GetLastError() == ERROR_FILE_NOT_FOUND),
        "GetStandardColorSpaceProfileA() returns %d (GLE=%d)\n", ret, GetLastError() );

    size = sizeof(newprofile);
    SetLastError(0xfaceabee); /* maybe 2nd param. */
    ret = pGetStandardColorSpaceProfileA(NULL, 0, newprofile, &size);
    if (!ret) ok( GetLastError() == ERROR_FILE_NOT_FOUND, "GetStandardColorSpaceProfileA() returns %d (GLE=%d)\n", ret, GetLastError() );
    else ok( !lstrcmpiA( newprofile, "" ) && GetLastError() == 0xfaceabee,
             "GetStandardColorSpaceProfileA() returns %d (GLE=%d)\n", ret, GetLastError() );

    /* Functional checks */

    size = sizeof(oldprofile);
    ret = pGetStandardColorSpaceProfileA( NULL, LCS_sRGB, oldprofile, &size );
    ok( ret, "GetStandardColorSpaceProfileA() failed (%d)\n", GetLastError() );

    SetLastError(0xdeadbeef);
    ret = pSetStandardColorSpaceProfileA( NULL, LCS_sRGB, standardprofile );
    if (!ret && (GetLastError() == ERROR_ACCESS_DENIED))
    {
        skip("Not enough rights for SetStandardColorSpaceProfileA\n");
        return;
    }
    ok( ret, "SetStandardColorSpaceProfileA() failed (%d)\n", GetLastError() );

    size = sizeof(newprofile);
    ret = pGetStandardColorSpaceProfileA( NULL, LCS_sRGB, newprofile, &size );
    ok( ret, "GetStandardColorSpaceProfileA() failed (%d)\n", GetLastError() );

    ret = pSetStandardColorSpaceProfileA( NULL, LCS_sRGB, oldprofile );
    ok( ret, "SetStandardColorSpaceProfileA() failed (%d)\n", GetLastError() );
}

static void test_GetStandardColorSpaceProfileW( WCHAR *standardprofileW )
{
    BOOL ret;
    DWORD size;
    WCHAR oldprofile[MAX_PATH];
    WCHAR newprofile[MAX_PATH];
    CHAR newprofileA[MAX_PATH];

    /* Parameter checks */

    /* Single invalid parameter checks: */

    size = sizeof(newprofile);
    SetLastError(0xfaceabee); /* 1st param, */
    ret = pGetStandardColorSpaceProfileW(machineW, LCS_sRGB, newprofile, &size);
    ok( !ret && GetLastError() == ERROR_NOT_SUPPORTED, "GetStandardColorSpaceProfileW() returns %d (GLE=%d)\n", ret, GetLastError() );

    size = sizeof(newprofile);
    SetLastError(0xfaceabee); /* 2nd param, */
    ret = pGetStandardColorSpaceProfileW(NULL, (DWORD)-1, newprofile, &size);
    ok( !ret && GetLastError() == ERROR_FILE_NOT_FOUND, "GetStandardColorSpaceProfileW() returns %d (GLE=%d)\n", ret, GetLastError() );

    size = sizeof(newprofile);
    SetLastError(0xfaceabee); /* 2nd param, */
    ret = pGetStandardColorSpaceProfileW(NULL, 0, newprofile, &size);
    ok( (!ret && GetLastError() == ERROR_FILE_NOT_FOUND) ||
        broken(ret), /* Win98 and WinME */
        "GetStandardColorSpaceProfileW() returns %d (GLE=%d)\n", ret, GetLastError() );

    size = sizeof(newprofile);
    SetLastError(0xfaceabee); /* 3rd param, */
    ret = pGetStandardColorSpaceProfileW(NULL, LCS_sRGB, NULL, &size);
    ok( !ret || broken(ret) /* win98 */, "GetStandardColorSpaceProfileW succeeded\n" );
    ok( GetLastError() == ERROR_INSUFFICIENT_BUFFER ||
        broken(GetLastError() == 0xfaceabee) /* win98 */,
        "GetStandardColorSpaceProfileW() returns GLE=%u\n", GetLastError() );

    size = sizeof(newprofile);
    SetLastError(0xfaceabee); /* 4th param, */
    ret = pGetStandardColorSpaceProfileW(NULL, LCS_sRGB, newprofile, NULL);
    ok( !ret && GetLastError() == ERROR_INVALID_PARAMETER, "GetStandardColorSpaceProfileW() returns %d (GLE=%d)\n", ret, GetLastError() );

    size = 0;
    SetLastError(0xfaceabee); /* dereferenced 4th param. */
    ret = pGetStandardColorSpaceProfileW(NULL, LCS_sRGB, newprofile, &size);
    ok( !ret || broken(ret) /* win98 */, "GetStandardColorSpaceProfileW succeeded\n" );
    ok( GetLastError() == ERROR_MORE_DATA ||
        GetLastError() == ERROR_INSUFFICIENT_BUFFER ||
        broken(GetLastError() == 0xfaceabee) /* win98 */,
        "GetStandardColorSpaceProfileW() returns GLE=%u\n", GetLastError() );

    /* Several invalid parameter checks: */

    size = 0;
    SetLastError(0xfaceabee); /* 1st, maybe 2nd and then dereferenced 4th param, */
    ret = pGetStandardColorSpaceProfileW(machineW, 0, newprofile, &size);
    ok( !ret && (GetLastError() == ERROR_INVALID_PARAMETER || GetLastError() == ERROR_NOT_SUPPORTED),
        "GetStandardColorSpaceProfileW() returns %d (GLE=%d)\n", ret, GetLastError() );

    SetLastError(0xfaceabee); /* maybe 2nd and then 4th param, */
    ret = pGetStandardColorSpaceProfileW(NULL, 0, newprofile, NULL);
    ok( !ret && GetLastError() == ERROR_INVALID_PARAMETER, "GetStandardColorSpaceProfileW() returns %d (GLE=%d)\n", ret, GetLastError() );

    size = 0;
    SetLastError(0xfaceabee); /* maybe 2nd, then 3rd and dereferenced 4th param, */
    ret = pGetStandardColorSpaceProfileW(NULL, 0, NULL, &size);
    ok( !ret || broken(ret) /* win98 */, "GetStandardColorSpaceProfileW succeeded\n" );
    ok( GetLastError() == ERROR_INSUFFICIENT_BUFFER ||
        GetLastError() == ERROR_FILE_NOT_FOUND ||
        broken(GetLastError() == 0xfaceabee) /* win98 */,
        "GetStandardColorSpaceProfileW() returns GLE=%u\n", GetLastError() );

    size = sizeof(newprofile);
    SetLastError(0xfaceabee); /* maybe 2nd param. */
    ret = pGetStandardColorSpaceProfileW(NULL, 0, newprofile, &size);
    if (!ret) ok( GetLastError() == ERROR_FILE_NOT_FOUND, "GetStandardColorSpaceProfileW() returns %d (GLE=%d)\n", ret, GetLastError() );
    else
    {
        WideCharToMultiByte(CP_ACP, 0, newprofile, -1, newprofileA, sizeof(newprofileA), NULL, NULL);
        ok( !lstrcmpiA( newprofileA, "" ) && GetLastError() == 0xfaceabee,
             "GetStandardColorSpaceProfileW() returns %d (GLE=%d)\n", ret, GetLastError() );
    }

    /* Functional checks */

    size = sizeof(oldprofile);
    ret = pGetStandardColorSpaceProfileW( NULL, LCS_sRGB, oldprofile, &size );
    ok( ret, "GetStandardColorSpaceProfileW() failed (%d)\n", GetLastError() );

    SetLastError(0xdeadbeef);
    ret = pSetStandardColorSpaceProfileW( NULL, LCS_sRGB, standardprofileW );
    if (!ret && (GetLastError() == ERROR_ACCESS_DENIED))
    {
        skip("Not enough rights for SetStandardColorSpaceProfileW\n");
        return;
    }
    ok( ret, "SetStandardColorSpaceProfileW() failed (%d)\n", GetLastError() );

    size = sizeof(newprofile);
    ret = pGetStandardColorSpaceProfileW( NULL, LCS_sRGB, newprofile, &size );
    ok( ret, "GetStandardColorSpaceProfileW() failed (%d)\n", GetLastError() );

    ret = pSetStandardColorSpaceProfileW( NULL, LCS_sRGB, oldprofile );
    ok( ret, "SetStandardColorSpaceProfileW() failed (%d)\n", GetLastError() );
}

static void test_EnumColorProfilesA( char *standardprofile )
{
    BOOL ret;
    DWORD total, size, number;
    ENUMTYPEA record;
    BYTE *buffer;

    /* Parameter checks */

    memset( &record, 0, sizeof(ENUMTYPEA) );

    record.dwSize = sizeof(ENUMTYPEA);
    record.dwVersion = ENUM_TYPE_VERSION;
    record.dwFields |= ET_DATACOLORSPACE;
    record.dwDataColorSpace = SPACE_RGB;

    total = 0;
    ret = pEnumColorProfilesA( NULL, &record, NULL, &total, &number );
    ok( !ret, "EnumColorProfilesA() failed (%d)\n", GetLastError() );
    buffer = HeapAlloc( GetProcessHeap(), 0, total );

    size = total;
    ret = pEnumColorProfilesA( machine, &record, buffer, &size, &number );
    ok( !ret, "EnumColorProfilesA() succeeded (%d)\n", GetLastError() );

    ret = pEnumColorProfilesA( NULL, NULL, buffer, &size, &number );
    ok( !ret, "EnumColorProfilesA() succeeded (%d)\n", GetLastError() );

    ret = pEnumColorProfilesA( NULL, &record, buffer, NULL, &number );
    ok( !ret, "EnumColorProfilesA() succeeded (%d)\n", GetLastError() );

    ret = pEnumColorProfilesA( NULL, &record, buffer, &size, &number );
    if (have_color_profile)
        ok( ret, "EnumColorProfilesA() failed (%d)\n", GetLastError() );
    else
        todo_wine ok( ret, "EnumColorProfilesA() failed (%d)\n", GetLastError() );

    size = 0;

    ret = pEnumColorProfilesA( NULL, &record, buffer, &size, &number );
    ok( !ret, "EnumColorProfilesA() succeeded (%d)\n", GetLastError() );

    /* Functional checks */

    size = total;
    ret = pEnumColorProfilesA( NULL, &record, buffer, &size, &number );
    if (have_color_profile)
        ok( ret, "EnumColorProfilesA() failed (%d)\n", GetLastError() );
    else
        todo_wine ok( ret, "EnumColorProfilesA() failed (%d)\n", GetLastError() );

    HeapFree( GetProcessHeap(), 0, buffer );
}

static void test_EnumColorProfilesW( WCHAR *standardprofileW )
{
    BOOL ret;
    DWORD total, size, number;
    ENUMTYPEW record;
    BYTE *buffer;

    /* Parameter checks */

    memset( &record, 0, sizeof(ENUMTYPEW) );

    record.dwSize = sizeof(ENUMTYPEW);
    record.dwVersion = ENUM_TYPE_VERSION;
    record.dwFields |= ET_DATACOLORSPACE;
    record.dwDataColorSpace = SPACE_RGB;

    total = 0;
    ret = pEnumColorProfilesW( NULL, &record, NULL, &total, &number );
    ok( !ret, "EnumColorProfilesW() failed (%d)\n", GetLastError() );
    buffer = HeapAlloc( GetProcessHeap(), 0, total * sizeof(WCHAR) );

    size = total;
    ret = pEnumColorProfilesW( machineW, &record, buffer, &size, &number );
    ok( !ret, "EnumColorProfilesW() succeeded (%d)\n", GetLastError() );

    ret = pEnumColorProfilesW( NULL, NULL, buffer, &size, &number );
    ok( !ret, "EnumColorProfilesW() succeeded (%d)\n", GetLastError() );

    ret = pEnumColorProfilesW( NULL, &record, buffer, NULL, &number );
    ok( !ret, "EnumColorProfilesW() succeeded (%d)\n", GetLastError() );

    ret = pEnumColorProfilesW( NULL, &record, buffer, &size, &number );
    if (have_color_profile)
        ok( ret, "EnumColorProfilesW() failed (%d)\n", GetLastError() );
    else
        todo_wine ok( ret, "EnumColorProfilesW() failed (%d)\n", GetLastError() );

    size = 0;
    ret = pEnumColorProfilesW( NULL, &record, buffer, &size, &number );
    ok( !ret, "EnumColorProfilesW() succeeded (%d)\n", GetLastError() );

    /* Functional checks */

    size = total;
    ret = pEnumColorProfilesW( NULL, &record, buffer, &size, &number );
    if (have_color_profile)
        ok( ret, "EnumColorProfilesW() failed (%d)\n", GetLastError() );
    else
        todo_wine ok( ret, "EnumColorProfilesW() failed (%d)\n", GetLastError() );

    HeapFree( GetProcessHeap(), 0, buffer );
}

static void test_InstallColorProfileA( char *standardprofile, char *testprofile )
{
    BOOL ret;

    /* Parameter checks */

    ret = pInstallColorProfileA( NULL, NULL );
    ok( !ret, "InstallColorProfileA() succeeded (%d)\n", GetLastError() );

    ret = pInstallColorProfileA( machine, NULL );
    ok( !ret, "InstallColorProfileA() succeeded (%d)\n", GetLastError() );

    ret = pInstallColorProfileA( NULL, machine );
    ok( !ret, "InstallColorProfileA() succeeded (%d)\n", GetLastError() );

    if (standardprofile)
    {
        ret = pInstallColorProfileA( NULL, standardprofile );
        ok( ret, "InstallColorProfileA() failed (%d)\n", GetLastError() );
    }

    /* Functional checks */

    if (testprofile)
    {
        CHAR dest[MAX_PATH], base[MAX_PATH];
        DWORD size = sizeof(dest);
        CHAR slash[] = "\\";
        HANDLE handle;

        SetLastError(0xdeadbeef);
        ret = pInstallColorProfileA( NULL, testprofile );
        if (!ret && (GetLastError() == ERROR_ACCESS_DENIED))
        {
            skip("Not enough rights for InstallColorProfileA\n");
            return;
        }
        ok( ret, "InstallColorProfileA() failed (%d)\n", GetLastError() );

        ret = pGetColorDirectoryA( NULL, dest, &size );
        ok( ret, "GetColorDirectoryA() failed (%d)\n", GetLastError() );

        MSCMS_basenameA( testprofile, base );

        lstrcatA( dest, slash );
        lstrcatA( dest, base );

        /* Check if the profile is really there */ 
        handle = CreateFileA( dest, 0 , 0, NULL, OPEN_EXISTING, 0, NULL );
        ok( handle != INVALID_HANDLE_VALUE, "Couldn't find the profile (%d)\n", GetLastError() );
        CloseHandle( handle );
        
        ret = pUninstallColorProfileA( NULL, dest, TRUE );
        ok( ret, "UninstallColorProfileA() failed (%d)\n", GetLastError() );
    }
}

static void test_InstallColorProfileW( WCHAR *standardprofileW, WCHAR *testprofileW )
{
    BOOL ret;

    /* Parameter checks */

    ret = pInstallColorProfileW( NULL, NULL );
    ok( !ret, "InstallColorProfileW() succeeded (%d)\n", GetLastError() );

    ret = pInstallColorProfileW( machineW, NULL );
    ok( !ret, "InstallColorProfileW() succeeded (%d)\n", GetLastError() );

    ret = pInstallColorProfileW( NULL, machineW );
    ok( !ret, "InstallColorProfileW() failed (%d)\n", GetLastError() );

    if (standardprofileW)
    {
        ret = pInstallColorProfileW( NULL, standardprofileW );
        ok( ret, "InstallColorProfileW() failed (%d)\n", GetLastError() );
    }

    /* Functional checks */

    if (testprofileW)
    {
        WCHAR dest[MAX_PATH], base[MAX_PATH];
        DWORD size = sizeof(dest);
        WCHAR slash[] = { '\\', 0 };
        HANDLE handle;

        SetLastError(0xdeadbeef);
        ret = pInstallColorProfileW( NULL, testprofileW );
        if (!ret && (GetLastError() == ERROR_ACCESS_DENIED))
        {
            skip("Not enough rights for InstallColorProfileW\n");
            return;
        }
        ok( ret, "InstallColorProfileW() failed (%d)\n", GetLastError() );

        ret = pGetColorDirectoryW( NULL, dest, &size );
        ok( ret, "GetColorDirectoryW() failed (%d)\n", GetLastError() );

        MSCMS_basenameW( testprofileW, base );

        lstrcatW( dest, slash );
        lstrcatW( dest, base );

        /* Check if the profile is really there */
        handle = CreateFileW( dest, 0 , 0, NULL, OPEN_EXISTING, 0, NULL );
        ok( handle != INVALID_HANDLE_VALUE, "Couldn't find the profile (%d)\n", GetLastError() );
        CloseHandle( handle );

        ret = pUninstallColorProfileW( NULL, dest, TRUE );
        ok( ret, "UninstallColorProfileW() failed (%d)\n", GetLastError() );
    }
}

static void test_IsColorProfileTagPresent( char *standardprofile )
{
    if (standardprofile)
    {
        PROFILE profile;
        HPROFILE handle;
        BOOL ret, present;
        TAGTYPE tag;

        profile.dwType = PROFILE_FILENAME;
        profile.pProfileData = standardprofile;
        profile.cbDataSize = strlen(standardprofile);

        handle = pOpenColorProfileA( &profile, PROFILE_READ, 0, OPEN_EXISTING );
        ok( handle != NULL, "OpenColorProfileA() failed (%d)\n", GetLastError() );

        /* Parameter checks */

        tag = 0;

        ret = pIsColorProfileTagPresent( handle, tag, &present );
        ok( !(ret && present), "IsColorProfileTagPresent() succeeded (%d)\n", GetLastError() );

        tag = 0x63707274;  /* 'cprt' */

        ret = pIsColorProfileTagPresent( NULL, tag, &present );
        ok( !ret, "IsColorProfileTagPresent() succeeded (%d)\n", GetLastError() );

        ret = pIsColorProfileTagPresent( handle, tag, NULL );
        ok( !ret, "IsColorProfileTagPresent() succeeded (%d)\n", GetLastError() );

        /* Functional checks */

        ret = pIsColorProfileTagPresent( handle, tag, &present );
        ok( ret && present, "IsColorProfileTagPresent() failed (%d)\n", GetLastError() );

        pCloseColorProfile( handle );
    }
}

static void test_OpenColorProfileA( char *standardprofile )
{
    PROFILE profile;
    HPROFILE handle;
    BOOL ret;

    profile.dwType = PROFILE_FILENAME;
    profile.pProfileData = NULL;
    profile.cbDataSize = 0;

    /* Parameter checks */

    handle = pOpenColorProfileA( NULL, 0, 0, 0 );
    ok( handle == NULL, "OpenColorProfileA() failed (%d)\n", GetLastError() );

    handle = pOpenColorProfileA( &profile, 0, 0, 0 );
    ok( handle == NULL, "OpenColorProfileA() failed (%d)\n", GetLastError() );

    handle = pOpenColorProfileA( &profile, PROFILE_READ, 0, 0 );
    ok( handle == NULL, "OpenColorProfileA() failed (%d)\n", GetLastError() );

    handle = pOpenColorProfileA( &profile, PROFILE_READWRITE, 0, 0 );
    ok( handle == NULL, "OpenColorProfileA() failed (%d)\n", GetLastError() );

    ok ( !pCloseColorProfile( NULL ), "CloseColorProfile() succeeded\n" );

    if (standardprofile)
    {
        profile.pProfileData = standardprofile;
        profile.cbDataSize = strlen(standardprofile);

        handle = pOpenColorProfileA( &profile, 0, 0, 0 );
        ok( handle == NULL, "OpenColorProfileA() failed (%d)\n", GetLastError() );

        handle = pOpenColorProfileA( &profile, PROFILE_READ, 0, 0 );
        ok( handle == NULL, "OpenColorProfileA() failed (%d)\n", GetLastError() );

        handle = pOpenColorProfileA( &profile, PROFILE_READ|PROFILE_READWRITE, 0, 0 );
        ok( handle == NULL, "OpenColorProfileA() failed (%d)\n", GetLastError() );

        /* Functional checks */

        handle = pOpenColorProfileA( &profile, PROFILE_READ, 0, OPEN_EXISTING );
        ok( handle != NULL, "OpenColorProfileA() failed (%d)\n", GetLastError() );

        ret = pCloseColorProfile( handle );
        ok( ret, "CloseColorProfile() failed (%d)\n", GetLastError() );

        profile.dwType = PROFILE_FILENAME;
        profile.pProfileData = (void *)"sRGB Color Space Profile.icm";
        profile.cbDataSize = sizeof("sRGB Color Space Profile.icm");

        handle = pOpenColorProfileA( &profile, PROFILE_READ, FILE_SHARE_READ, OPEN_EXISTING );
        ok( handle != NULL, "OpenColorProfileA() failed (%d)\n", GetLastError() );

        ret = pCloseColorProfile( handle );
        ok( ret, "CloseColorProfile() failed (%d)\n", GetLastError() );
    }
}

static void test_OpenColorProfileW( WCHAR *standardprofileW )
{
    PROFILE profile;
    HPROFILE handle;
    BOOL ret;

    profile.dwType = PROFILE_FILENAME;
    profile.pProfileData = NULL;
    profile.cbDataSize = 0;

    /* Parameter checks */

    handle = pOpenColorProfileW( NULL, 0, 0, 0 );
    ok( handle == NULL, "OpenColorProfileW() failed (%d)\n", GetLastError() );

    handle = pOpenColorProfileW( &profile, 0, 0, 0 );
    ok( handle == NULL, "OpenColorProfileW() failed (%d)\n", GetLastError() );

    handle = pOpenColorProfileW( &profile, PROFILE_READ, 0, 0 );
    ok( handle == NULL, "OpenColorProfileW() failed (%d)\n", GetLastError() );

    handle = pOpenColorProfileW( &profile, PROFILE_READWRITE, 0, 0 );
    ok( handle == NULL, "OpenColorProfileW() failed (%d)\n", GetLastError() );

    ok ( !pCloseColorProfile( NULL ), "CloseColorProfile() succeeded\n" );

    if (standardprofileW)
    {
        profile.pProfileData = standardprofileW;
        profile.cbDataSize = lstrlenW(standardprofileW) * sizeof(WCHAR);

        handle = pOpenColorProfileW( &profile, 0, 0, 0 );
        ok( handle == NULL, "OpenColorProfileW() failed (%d)\n", GetLastError() );

        handle = pOpenColorProfileW( &profile, PROFILE_READ, 0, 0 );
        ok( handle == NULL, "OpenColorProfileW() failed (%d)\n", GetLastError() );

        handle = pOpenColorProfileW( &profile, PROFILE_READ|PROFILE_READWRITE, 0, 0 );
        ok( handle == NULL, "OpenColorProfileW() failed (%d)\n", GetLastError() );

        /* Functional checks */

        handle = pOpenColorProfileW( &profile, PROFILE_READ, 0, OPEN_EXISTING );
        ok( handle != NULL, "OpenColorProfileW() failed (%d)\n", GetLastError() );

        ret = pCloseColorProfile( handle );
        ok( ret, "CloseColorProfile() failed (%d)\n", GetLastError() );
    }
}

static void test_SetColorProfileElement( char *testprofile )
{
    if (testprofile)
    {
        PROFILE profile;
        HPROFILE handle;
        DWORD size;
        BOOL ret, ref;

        TAGTYPE tag = 0x63707274;  /* 'cprt' */
        static char data[] = "(c) The Wine Project";
        static char buffer[51];

        profile.dwType = PROFILE_FILENAME;
        profile.pProfileData = testprofile;
        profile.cbDataSize = strlen(testprofile);

        /* Parameter checks */

        handle = pOpenColorProfileA( &profile, PROFILE_READ, 0, OPEN_EXISTING );
        ok( handle != NULL, "OpenColorProfileA() failed (%d)\n", GetLastError() );

        ret = pSetColorProfileElement( handle, tag, 0, &size, data );
        ok( !ret, "SetColorProfileElement() succeeded (%d)\n", GetLastError() );

        pCloseColorProfile( handle );

        handle = pOpenColorProfileA( &profile, PROFILE_READWRITE, 0, OPEN_EXISTING );
        ok( handle != NULL, "OpenColorProfileA() failed (%d)\n", GetLastError() );

        ret = pSetColorProfileElement( NULL, 0, 0, NULL, NULL );
        ok( !ret, "SetColorProfileElement() succeeded (%d)\n", GetLastError() );

        ret = pSetColorProfileElement( handle, 0, 0, NULL, NULL );
        ok( !ret, "SetColorProfileElement() succeeded (%d)\n", GetLastError() );

        ret = pSetColorProfileElement( handle, tag, 0, NULL, NULL );
        ok( !ret, "SetColorProfileElement() succeeded (%d)\n", GetLastError() );

        ret = pSetColorProfileElement( handle, tag, 0, &size, NULL );
        ok( !ret, "SetColorProfileElement() succeeded (%d)\n", GetLastError() );

        /* Functional checks */

        size = sizeof(data);
        ret = pSetColorProfileElement( handle, tag, 0, &size, data );
        ok( ret, "SetColorProfileElement() failed %u\n", GetLastError() );

        size = sizeof(buffer);
        ret = pGetColorProfileElement( handle, tag, 0, &size, buffer, &ref );
        ok( ret, "GetColorProfileElement() failed %u\n", GetLastError() );
        ok( size > 0, "wrong size\n" );

        ok( !memcmp( data, buffer, sizeof(data) ),
            "Unexpected tag data, expected %s, got %s (%u)\n", data, buffer, GetLastError() );

        pCloseColorProfile( handle );
    }
}

static void test_SetColorProfileHeader( char *testprofile )
{
    if (testprofile)
    {
        PROFILE profile;
        HPROFILE handle;
        BOOL ret;
        PROFILEHEADER header;

        profile.dwType = PROFILE_FILENAME;
        profile.pProfileData = testprofile;
        profile.cbDataSize = strlen(testprofile);

        header.phSize = 0x00000c48;
        header.phCMMType = 0x4c696e6f;
        header.phVersion = 0x02100000;
        header.phClass = 0x6d6e7472;
        header.phDataColorSpace = 0x52474220;
        header.phConnectionSpace  = 0x58595a20;
        header.phDateTime[0] = 0x07ce0002;
        header.phDateTime[1] = 0x00090006;
        header.phDateTime[2] = 0x00310000;
        header.phSignature = 0x61637370;
        header.phPlatform = 0x4d534654;
        header.phProfileFlags = 0x00000000;
        header.phManufacturer = 0x49454320;
        header.phModel = 0x73524742;
        header.phAttributes[0] = 0x00000000;
        header.phAttributes[1] = 0x00000000;
        header.phRenderingIntent = 0x00000000;
        header.phIlluminant.ciexyzX = 0x0000f6d6;
        header.phIlluminant.ciexyzY = 0x00010000;
        header.phIlluminant.ciexyzZ = 0x0000d32d;
        header.phCreator = 0x48502020;

        /* Parameter checks */

        handle = pOpenColorProfileA( &profile, PROFILE_READ, 0, OPEN_EXISTING );
        ok( handle != NULL, "OpenColorProfileA() failed (%d)\n", GetLastError() );

        ret = pSetColorProfileHeader( handle, &header );
        ok( !ret, "SetColorProfileHeader() succeeded (%d)\n", GetLastError() );

        pCloseColorProfile( handle );

        handle = pOpenColorProfileA( &profile, PROFILE_READWRITE, 0, OPEN_EXISTING );
        ok( handle != NULL, "OpenColorProfileA() failed (%d)\n", GetLastError() );

        ret = pSetColorProfileHeader( NULL, NULL );
        ok( !ret, "SetColorProfileHeader() succeeded (%d)\n", GetLastError() );

        ret = pSetColorProfileHeader( handle, NULL );
        ok( !ret, "SetColorProfileHeader() succeeded (%d)\n", GetLastError() );

        ret = pSetColorProfileHeader( NULL, &header );
        ok( !ret, "SetColorProfileHeader() succeeded (%d)\n", GetLastError() );

        /* Functional checks */

        ret = pSetColorProfileHeader( handle, &header );
        ok( ret, "SetColorProfileHeader() failed (%d)\n", GetLastError() );

        ret = pGetColorProfileHeader( handle, &header );
        ok( ret, "GetColorProfileHeader() failed (%d)\n", GetLastError() );

        ok( !memcmp( &header, rgbheader, sizeof(rgbheader) ), "Unexpected header data\n" );

        pCloseColorProfile( handle );
    }
}

static void test_UninstallColorProfileA( char *testprofile )
{
    BOOL ret;

    /* Parameter checks */

    ret = pUninstallColorProfileA( NULL, NULL, FALSE );
    ok( !ret, "UninstallColorProfileA() succeeded (%d)\n", GetLastError() );

    ret = pUninstallColorProfileA( machine, NULL, FALSE );
    ok( !ret, "UninstallColorProfileA() succeeded (%d)\n", GetLastError() );

    /* Functional checks */

    if (testprofile)
    {
        CHAR dest[MAX_PATH], base[MAX_PATH];
        DWORD size = sizeof(dest);
        CHAR slash[] = "\\";
        HANDLE handle;

        SetLastError(0xdeadbeef);
        ret = pInstallColorProfileA( NULL, testprofile );
        if (!ret && (GetLastError() == ERROR_ACCESS_DENIED))
        {
            skip("Not enough rights for InstallColorProfileA\n");
            return;
        }
        ok( ret, "InstallColorProfileA() failed (%d)\n", GetLastError() );

        ret = pGetColorDirectoryA( NULL, dest, &size );
        ok( ret, "GetColorDirectoryA() failed (%d)\n", GetLastError() );

        MSCMS_basenameA( testprofile, base );

        lstrcatA( dest, slash );
        lstrcatA( dest, base );

        ret = pUninstallColorProfileA( NULL, dest, TRUE );
        ok( ret, "UninstallColorProfileA() failed (%d)\n", GetLastError() );

        /* Check if the profile is really gone */
        handle = CreateFileA( dest, 0 , 0, NULL, OPEN_EXISTING, 0, NULL );
        ok( handle == INVALID_HANDLE_VALUE, "Found the profile (%d)\n", GetLastError() );
        CloseHandle( handle );
    }
}

static void test_UninstallColorProfileW( WCHAR *testprofileW )
{
    BOOL ret;

    /* Parameter checks */

    ret = pUninstallColorProfileW( NULL, NULL, FALSE );
    ok( !ret, "UninstallColorProfileW() succeeded (%d)\n", GetLastError() );

    ret = pUninstallColorProfileW( machineW, NULL, FALSE );
    ok( !ret, "UninstallColorProfileW() succeeded (%d)\n", GetLastError() );

    /* Functional checks */

    if (testprofileW)
    {
        WCHAR dest[MAX_PATH], base[MAX_PATH];
        char destA[MAX_PATH];
        DWORD size = sizeof(dest);
        WCHAR slash[] = { '\\', 0 };
        HANDLE handle;
        int bytes_copied;

        SetLastError(0xdeadbeef);
        ret = pInstallColorProfileW( NULL, testprofileW );
        if (!ret && (GetLastError() == ERROR_ACCESS_DENIED))
        {
            skip("Not enough rights for InstallColorProfileW\n");
            return;
        }
        ok( ret, "InstallColorProfileW() failed (%d)\n", GetLastError() );

        ret = pGetColorDirectoryW( NULL, dest, &size );
        ok( ret, "GetColorDirectoryW() failed (%d)\n", GetLastError() );

        MSCMS_basenameW( testprofileW, base );

        lstrcatW( dest, slash );
        lstrcatW( dest, base );

        ret = pUninstallColorProfileW( NULL, dest, TRUE );
        ok( ret, "UninstallColorProfileW() failed (%d)\n", GetLastError() );

        bytes_copied = WideCharToMultiByte(CP_ACP, 0, dest, -1, destA, MAX_PATH, NULL, NULL);
        ok( bytes_copied > 0 , "WideCharToMultiByte() returns %d\n", bytes_copied);
        /* Check if the profile is really gone */
        handle = CreateFileA( destA, 0 , 0, NULL, OPEN_EXISTING, 0, NULL );
        ok( handle == INVALID_HANDLE_VALUE, "Found the profile (%d)\n", GetLastError() );
        CloseHandle( handle );
    }
}

static void test_AssociateColorProfileWithDeviceA( char *testprofile )
{
    BOOL ret;
    char profile[MAX_PATH], basename[MAX_PATH];
    DWORD error, size = sizeof(profile);
    DISPLAY_DEVICEA display, monitor;
    BOOL res;

    if (testprofile && pEnumDisplayDevicesA)
    {
        display.cb = sizeof( DISPLAY_DEVICEA );
        res = pEnumDisplayDevicesA( NULL, 0, &display, 0 );
        ok( res, "Can't get display info\n" );

        monitor.cb = sizeof( DISPLAY_DEVICEA );
        res = pEnumDisplayDevicesA( display.DeviceName, 0, &monitor, 0 );
        if (res)
        {
            SetLastError(0xdeadbeef);
            ret = pAssociateColorProfileWithDeviceA( "machine", testprofile, NULL );
            error = GetLastError();
            ok( !ret, "AssociateColorProfileWithDevice() succeeded\n" );
            ok( error == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %u\n", error );

            SetLastError(0xdeadbeef);
            ret = pAssociateColorProfileWithDeviceA( "machine", NULL, monitor.DeviceID );
            error = GetLastError();
            ok( !ret, "AssociateColorProfileWithDevice() succeeded\n" );
            ok( error == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %u\n", error );

            SetLastError(0xdeadbeef);
            ret = pAssociateColorProfileWithDeviceA( "machine", testprofile, monitor.DeviceID );
            error = GetLastError();
            ok( !ret, "AssociateColorProfileWithDevice() succeeded\n" );
            ok( error == ERROR_NOT_SUPPORTED, "expected ERROR_NOT_SUPPORTED, got %u\n", error );

            ret = pInstallColorProfileA( NULL, testprofile );
            ok( ret, "InstallColorProfileA() failed (%u)\n", GetLastError() );

            ret = pGetColorDirectoryA( NULL, profile, &size );
            ok( ret, "GetColorDirectoryA() failed (%d)\n", GetLastError() );

            MSCMS_basenameA( testprofile, basename );
            lstrcatA( profile, "\\" );
            lstrcatA( profile, basename );

            ret = pAssociateColorProfileWithDeviceA( NULL, profile, monitor.DeviceID );
            ok( ret, "AssociateColorProfileWithDevice() failed (%u)\n", GetLastError() );

            SetLastError(0xdeadbeef);
            ret = pDisassociateColorProfileFromDeviceA( "machine", profile, NULL );
            error = GetLastError();
            ok( !ret, "DisassociateColorProfileFromDeviceA() succeeded\n" );
            ok( error == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %u\n", error );

            SetLastError(0xdeadbeef);
            ret = pDisassociateColorProfileFromDeviceA( "machine", NULL, monitor.DeviceID );
            error = GetLastError();
            ok( !ret, "DisassociateColorProfileFromDeviceA() succeeded\n" );
            ok( error == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %u\n", error );

            SetLastError(0xdeadbeef);
            ret = pDisassociateColorProfileFromDeviceA( "machine", profile, monitor.DeviceID );
            error = GetLastError();
            ok( !ret, "DisassociateColorProfileFromDeviceA() succeeded\n" );
            ok( error == ERROR_NOT_SUPPORTED, "expected ERROR_NOT_SUPPORTED, got %u\n", error );

            ret = pDisassociateColorProfileFromDeviceA( NULL, profile, monitor.DeviceID );
            ok( ret, "DisassociateColorProfileFromDeviceA() failed (%u)\n", GetLastError() );

            ret = pUninstallColorProfileA( NULL, profile, TRUE );
            ok( ret, "UninstallColorProfileA() failed (%d)\n", GetLastError() );
        }
        else
            skip("Unable to obtain monitor name\n");
    }
}

static BOOL have_profile(void)
{
    char glob[MAX_PATH + sizeof("\\*.icm")];
    DWORD size = MAX_PATH;
    HANDLE handle;
    WIN32_FIND_DATAA data;

    if (!pGetColorDirectoryA( NULL, glob, &size )) return FALSE;
    lstrcatA( glob, "\\*.icm" );
    handle = FindFirstFileA( glob, &data );
    if (handle == INVALID_HANDLE_VALUE) return FALSE;
    FindClose( handle );
    return TRUE;
}

static void test_CreateMultiProfileTransform( char *standardprofile, char *testprofile )
{
    PROFILE profile;
    HPROFILE handle[2];
    HTRANSFORM transform;
    DWORD intents[2] = { INTENT_PERCEPTUAL, INTENT_PERCEPTUAL };

    if (testprofile)
    {
        profile.dwType       = PROFILE_FILENAME;
        profile.pProfileData = standardprofile;
        profile.cbDataSize   = strlen(standardprofile);

        handle[0] = pOpenColorProfileA( &profile, PROFILE_READ, 0, OPEN_EXISTING );
        ok( handle[0] != NULL, "got %u\n", GetLastError() );

        profile.dwType       = PROFILE_FILENAME;
        profile.pProfileData = testprofile;
        profile.cbDataSize   = strlen(testprofile);

        handle[1] = pOpenColorProfileA( &profile, PROFILE_READ, 0, OPEN_EXISTING );
        ok( handle[1] != NULL, "got %u\n", GetLastError() );

        transform = pCreateMultiProfileTransform( handle, 2, intents, 2, 0, 0 );
        ok( transform != NULL, "got %u\n", GetLastError() );

        pDeleteColorTransform( transform );
        pCloseColorProfile( handle[0] );
        pCloseColorProfile( handle[1] );
    }
}

START_TEST(profile)
{
    UINT len;
    HANDLE handle;
    char path[MAX_PATH], file[MAX_PATH], profilefile1[MAX_PATH], profilefile2[MAX_PATH];
    WCHAR profilefile1W[MAX_PATH], profilefile2W[MAX_PATH], fileW[MAX_PATH];
    char *standardprofile = NULL, *testprofile = NULL;
    WCHAR *standardprofileW = NULL, *testprofileW = NULL;
    UINT ret;

    hmscms = LoadLibraryA( "mscms.dll" );
    if (!hmscms) return;

    huser32 = LoadLibraryA( "user32.dll" );
    if (!huser32)
    {
        FreeLibrary( hmscms );
        return;
    }

    if (!init_function_ptrs())
    {
        FreeLibrary( huser32 );
        FreeLibrary( hmscms );
        return;
    }

    /* See if we can find the standard color profile */
    ret = GetSystemDirectoryA( profilefile1, sizeof(profilefile1) );
    ok( ret > 0, "GetSystemDirectoryA() returns %d, LastError = %d\n", ret, GetLastError());
    ok(profilefile1[0] && lstrlenA(profilefile1) < MAX_PATH,
        "Expected length between 0 and MAX_PATH, got %d\n", lstrlenA(profilefile1));
    MultiByteToWideChar(CP_ACP, 0, profilefile1, -1, profilefile1W, MAX_PATH);
    ok(profilefile1W[0] && lstrlenW(profilefile1W) < MAX_PATH,
        "Expected length between 0 and MAX_PATH, got %d\n", lstrlenW(profilefile1W));
    lstrcpyA(profilefile2, profilefile1);
    lstrcpyW(profilefile2W, profilefile1W);

    lstrcatA( profilefile1, profile1 );
    lstrcatW( profilefile1W, profile1W );
    handle = CreateFileA( profilefile1, 0 , 0, NULL, OPEN_EXISTING, 0, NULL );

    if (handle != INVALID_HANDLE_VALUE)
    {
        standardprofile = profilefile1;
        standardprofileW = profilefile1W;
        CloseHandle( handle );
    }

    lstrcatA( profilefile2, profile2 );
    lstrcatW( profilefile2W, profile2W );
    handle = CreateFileA( profilefile2, 0 , 0, NULL, OPEN_EXISTING, 0, NULL );

    if (handle != INVALID_HANDLE_VALUE)
    {
        standardprofile = profilefile2;
        standardprofileW = profilefile2W;
        CloseHandle( handle );
    }

    /* If found, create a temporary copy for testing purposes */
    if (standardprofile && GetTempPathA( sizeof(path), path ))
    {
        if (GetTempFileNameA( path, "rgb", 0, file ))
        {
            if (CopyFileA( standardprofile, file, FALSE ))
            {
                testprofile = (LPSTR)&file;
                len = MultiByteToWideChar( CP_ACP, 0, testprofile, -1, NULL, 0 );
                MultiByteToWideChar( CP_ACP, 0, testprofile, -1, fileW, len );
                testprofileW = (LPWSTR)&fileW;
            }
        }
    }

    have_color_profile = have_profile();

    test_GetColorDirectoryA();
    test_GetColorDirectoryW();

    test_GetColorProfileElement( standardprofile );
    test_GetColorProfileElementTag( standardprofile );

    test_GetColorProfileFromHandle( testprofile );
    test_GetColorProfileHeader( testprofile );

    test_GetCountColorProfileElements( standardprofile );

    test_GetStandardColorSpaceProfileA( standardprofile );
    test_GetStandardColorSpaceProfileW( standardprofileW );

    test_EnumColorProfilesA( standardprofile );
    test_EnumColorProfilesW( standardprofileW );

    test_InstallColorProfileA( standardprofile, testprofile );
    test_InstallColorProfileW( standardprofileW, testprofileW );

    test_IsColorProfileTagPresent( standardprofile );

    test_OpenColorProfileA( standardprofile );
    test_OpenColorProfileW( standardprofileW );

    test_SetColorProfileElement( testprofile );
    test_SetColorProfileHeader( testprofile );

    test_UninstallColorProfileA( testprofile );
    test_UninstallColorProfileW( testprofileW );

    test_AssociateColorProfileWithDeviceA( testprofile );
    test_CreateMultiProfileTransform( standardprofile, testprofile );

    if (testprofile) DeleteFileA( testprofile );
    FreeLibrary( huser32 );
    FreeLibrary( hmscms );
}
