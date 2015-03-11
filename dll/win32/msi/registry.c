/*
 * Implementation of the Microsoft Installer (msi.dll)
 *
 * Copyright 2005 Mike McCormack for CodeWeavers
 * Copyright 2005 Aric Stewart for CodeWeavers
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

#include "msipriv.h"

WINE_DEFAULT_DEBUG_CHANNEL(msi);

/* 
 * This module will be all the helper functions for registry access by the
 * installer bits. 
 */

static const WCHAR szUserDataFeatures_fmt[] = {
    'S','o','f','t','w','a','r','e','\\','M','i','c','r','o','s','o','f','t','\\',
    'W','i','n','d','o','w','s','\\','C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
    'I','n','s','t','a','l','l','e','r','\\','U','s','e','r','D','a','t','a','\\',
    '%','s','\\','P','r','o','d','u','c','t','s','\\','%','s','\\','F','e','a','t','u','r','e','s',0};

static const WCHAR szUserDataComp_fmt[] = {
    'S','o','f','t','w','a','r','e','\\','M','i','c','r','o','s','o','f','t','\\',
    'W','i','n','d','o','w','s','\\','C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
    'I','n','s','t','a','l','l','e','r','\\','U','s','e','r','D','a','t','a','\\',
    '%','s','\\','C','o','m','p','o','n','e','n','t','s','\\','%','s',0};

static const WCHAR szUserDataComponents_fmt[] = {
    'S','o','f','t','w','a','r','e','\\','M','i','c','r','o','s','o','f','t','\\',
    'W','i','n','d','o','w','s','\\','C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
    'I','n','s','t','a','l','l','e','r','\\','U','s','e','r','D','a','t','a','\\',
    '%','s','\\','C','o','m','p','o','n','e','n','t','s',0};

static const WCHAR szUserDataProd_fmt[] = {
    'S','o','f','t','w','a','r','e','\\','M','i','c','r','o','s','o','f','t','\\',
    'W','i','n','d','o','w','s','\\','C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
    'I','n','s','t','a','l','l','e','r','\\','U','s','e','r','D','a','t','a','\\',
    '%','s','\\','P','r','o','d','u','c','t','s','\\','%','s',0};

static const WCHAR szUserDataProducts_fmt[] = {
    'S','o','f','t','w','a','r','e','\\','M','i','c','r','o','s','o','f','t','\\',
    'W','i','n','d','o','w','s','\\','C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
    'I','n','s','t','a','l','l','e','r','\\','U','s','e','r','D','a','t','a','\\',
    '%','s','\\','P','r','o','d','u','c','t','s',0};

static const WCHAR szUserDataPatch_fmt[] = {
    'S','o','f','t','w','a','r','e','\\','M','i','c','r','o','s','o','f','t','\\',
    'W','i','n','d','o','w','s','\\','C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
    'I','n','s','t','a','l','l','e','r','\\','U','s','e','r','D','a','t','a','\\',
    '%','s','\\','P','a','t','c','h','e','s','\\','%','s',0};

static const WCHAR szUserDataPatches_fmt[] = {
    'S','o','f','t','w','a','r','e','\\','M','i','c','r','o','s','o','f','t','\\',
    'W','i','n','d','o','w','s','\\','C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
    'I','n','s','t','a','l','l','e','r','\\','U','s','e','r','D','a','t','a','\\',
    '%','s','\\','P','a','t','c','h','e','s',0};

static const WCHAR szUserDataProductPatches_fmt[] = {
    'S','o','f','t','w','a','r','e','\\','M','i','c','r','o','s','o','f','t','\\',
    'W','i','n','d','o','w','s','\\','C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
    'I','n','s','t','a','l','l','e','r','\\','U','s','e','r','D','a','t','a','\\',
    '%','s','\\','P','r','o','d','u','c','t','s','\\','%','s','\\','P','a','t','c','h','e','s',0};

static const WCHAR szInstallProperties_fmt[] = {
    'S','o','f','t','w','a','r','e','\\','M','i','c','r','o','s','o','f','t','\\',
    'W','i','n','d','o','w','s','\\','C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
    'I','n','s','t','a','l','l','e','r','\\','U','s','e','r','D','a','t','a','\\',
    '%','s','\\','P','r','o','d','u','c','t','s','\\','%','s','\\',
    'I','n','s','t','a','l','l','P','r','o','p','e','r','t','i','e','s',0};

static const WCHAR szInstaller_LocalManagedProd_fmt[] = {
    'S','o','f','t','w','a','r','e','\\','M','i','c','r','o','s','o','f','t','\\',
    'W','i','n','d','o','w','s','\\','C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
    'I','n','s','t','a','l','l','e','r','\\','M','a','n','a','g','e','d','\\','%','s','\\',
    'I','n','s','t','a','l','l','e','r','\\','P','r','o','d','u','c','t','s','\\','%','s',0};

static const WCHAR szInstaller_LocalManagedFeat_fmt[] = {
    'S','o','f','t','w','a','r','e','\\','M','i','c','r','o','s','o','f','t','\\',
    'W','i','n','d','o','w','s','\\','C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
    'I','n','s','t','a','l','l','e','r','\\','M','a','n','a','g','e','d','\\','%','s','\\',
    'I','n','s','t','a','l','l','e','r','\\','F','e','a','t','u','r','e','s','\\','%','s',0};

static const WCHAR szInstaller_Products[] = {
    'S','o','f','t','w','a','r','e','\\','M','i','c','r','o','s','o','f','t','\\',
    'W','i','n','d','o','w','s','\\','C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
    'I','n','s','t','a','l','l','e','r','\\','P','r','o','d','u','c','t','s',0};

static const WCHAR szInstaller_Patches[] = {
    'S','o','f','t','w','a','r','e','\\','M','i','c','r','o','s','o','f','t','\\',
    'W','i','n','d','o','w','s','\\','C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
    'I','n','s','t','a','l','l','e','r','\\','P','a','t','c','h','e','s',0};

static const WCHAR szInstaller_LocalClassesProducts[] = {
    'S','o','f','t','w','a','r','e','\\','C','l','a','s','s','e','s','\\',
    'I','n','s','t','a','l','l','e','r','\\','P','r','o','d','u','c','t','s',0};

static const WCHAR szInstaller_LocalClassesFeatures[] = {
    'S','o','f','t','w','a','r','e','\\','C','l','a','s','s','e','s','\\',
    'I','n','s','t','a','l','l','e','r','\\','F','e','a','t','u','r','e','s',0};

static const WCHAR szInstaller_LocalClassesProd[] = {
    'S','o','f','t','w','a','r','e','\\','C','l','a','s','s','e','s','\\',
    'I','n','s','t','a','l','l','e','r','\\','P','r','o','d','u','c','t','s','\\',0};

static const WCHAR szInstaller_LocalClassesFeat[] = {
    'S','o','f','t','w','a','r','e','\\','C','l','a','s','s','e','s','\\',
    'I','n','s','t','a','l','l','e','r','\\','F','e','a','t','u','r','e','s','\\',0};

static const WCHAR szInstaller_ClassesUpgradeCode[] = {
    'S','o','f','t','w','a','r','e','\\','C','l','a','s','s','e','s','\\',
    'I','n','s','t','a','l','l','e','r','\\','U','p','g','r','a','d','e','C','o','d','e','s','\\',0};

static const WCHAR szInstaller_ClassesUpgradeCodes[] = {
    'S','o','f','t','w','a','r','e','\\','C','l','a','s','s','e','s','\\',
    'I','n','s','t','a','l','l','e','r','\\','U','p','g','r','a','d','e','C','o','d','e','s',0};

static const WCHAR szInstaller_Features[] = {
    'S','o','f','t','w','a','r','e','\\','M','i','c','r','o','s','o','f','t','\\',
    'W','i','n','d','o','w','s','\\','C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
    'I','n','s','t','a','l','l','e','r','\\','F','e','a','t','u','r','e','s','\\',0};

static const WCHAR szInstaller_UpgradeCodes[] = {
    'S','o','f','t','w','a','r','e','\\','M','i','c','r','o','s','o','f','t','\\',
    'W','i','n','d','o','w','s','\\','C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
    'I','n','s','t','a','l','l','e','r','\\','U','p','g','r','a','d','e','C','o','d','e','s','\\',0};

static const WCHAR szInstaller_UserUpgradeCodes[] = {
    'S','o','f','t','w','a','r','e','\\','M','i','c','r','o','s','o','f','t','\\',
    'I','n','s','t','a','l','l','e','r','\\','U','p','g','r','a','d','e','C','o','d','e','s','\\',0};

static const WCHAR szUninstall[] = {
    'S','o','f','t','w','a','r','e','\\','M','i','c','r','o','s','o','f','t','\\',
    'W','i','n','d','o','w','s','\\','C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
    'U','n','i','n','s','t','a','l','l','\\',0};

static const WCHAR szUninstall_32node[] = {
    'S','o','f','t','w','a','r','e','\\','W','o','w','6','4','3','2','N','o','d','e','\\',
    'M','i','c','r','o','s','o','f','t','\\','W','i','n','d','o','w','s','\\',
    'C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\','U','n','i','n','s','t','a','l','l','\\',0};

static const WCHAR szUserComponents[] = {
    'S','o','f','t','w','a','r','e','\\','M','i','c','r','o','s','o','f','t','\\',
    'I','n','s','t','a','l','l','e','r','\\','C','o','m','p','o','n','e','n','t','s','\\',0};

static const WCHAR szUserFeatures[] = {
    'S','o','f','t','w','a','r','e','\\','M','i','c','r','o','s','o','f','t','\\',
    'I','n','s','t','a','l','l','e','r','\\','F','e','a','t','u','r','e','s','\\',0};

static const WCHAR szUserProducts[] = {
    'S','o','f','t','w','a','r','e','\\','M','i','c','r','o','s','o','f','t','\\',
    'I','n','s','t','a','l','l','e','r','\\','P','r','o','d','u','c','t','s','\\',0};

static const WCHAR szUserPatches[] = {
    'S','o','f','t','w','a','r','e','\\','M','i','c','r','o','s','o','f','t','\\',
    'I','n','s','t','a','l','l','e','r','\\','P','a','t','c','h','e','s','\\',0};

BOOL unsquash_guid(LPCWSTR in, LPWSTR out)
{
    DWORD i,n=0;

    if (lstrlenW(in) != 32)
        return FALSE;

    out[n++]='{';
    for(i=0; i<8; i++)
        out[n++] = in[7-i];
    out[n++]='-';
    for(i=0; i<4; i++)
        out[n++] = in[11-i];
    out[n++]='-';
    for(i=0; i<4; i++)
        out[n++] = in[15-i];
    out[n++]='-';
    for(i=0; i<2; i++)
    {
        out[n++] = in[17+i*2];
        out[n++] = in[16+i*2];
    }
    out[n++]='-';
    for( ; i<8; i++)
    {
        out[n++] = in[17+i*2];
        out[n++] = in[16+i*2];
    }
    out[n++]='}';
    out[n]=0;
    return TRUE;
}

BOOL squash_guid(LPCWSTR in, LPWSTR out)
{
    DWORD i,n=1;
    GUID guid;

    out[0] = 0;

    if (FAILED(CLSIDFromString((LPCOLESTR)in, &guid)))
        return FALSE;

    for(i=0; i<8; i++)
        out[7-i] = in[n++];
    n++;
    for(i=0; i<4; i++)
        out[11-i] = in[n++];
    n++;
    for(i=0; i<4; i++)
        out[15-i] = in[n++];
    n++;
    for(i=0; i<2; i++)
    {
        out[17+i*2] = in[n++];
        out[16+i*2] = in[n++];
    }
    n++;
    for( ; i<8; i++)
    {
        out[17+i*2] = in[n++];
        out[16+i*2] = in[n++];
    }
    out[32]=0;
    return TRUE;
}


/* tables for encoding and decoding base85 */
static const unsigned char table_dec85[0x80] = {
0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
0xff,0x00,0xff,0xff,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0xff,
0x0c,0x0d,0x0e,0x0f,0x10,0x11,0x12,0x13,0x14,0x15,0xff,0xff,0xff,0x16,0xff,0x17,
0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,
0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,0x30,0x31,0x32,0x33,0xff,0x34,0x35,0x36,
0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,0x40,0x41,0x42,0x43,0x44,0x45,0x46,
0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,0x50,0x51,0x52,0xff,0x53,0x54,0xff,
};

static const char table_enc85[] =
"!$%&'()*+,-.0123456789=?@ABCDEFGHIJKLMNO"
"PQRSTUVWXYZ[]^_`abcdefghijklmnopqrstuvwx"
"yz{}~";

/*
 *  Converts a base85 encoded guid into a GUID pointer
 *  Base85 encoded GUIDs should be 20 characters long.
 *
 *  returns TRUE if successful, FALSE if not
 */
BOOL decode_base85_guid( LPCWSTR str, GUID *guid )
{
    DWORD i, val = 0, base = 1, *p;

    if (!str)
        return FALSE;

    p = (DWORD*) guid;
    for( i=0; i<20; i++ )
    {
        if( (i%5) == 0 )
        {
            val = 0;
            base = 1;
        }
        val += table_dec85[str[i]] * base;
        if( str[i] >= 0x80 )
            return FALSE;
        if( table_dec85[str[i]] == 0xff )
            return FALSE;
        if( (i%5) == 4 )
            p[i/5] = val;
        base *= 85;
    }
    return TRUE;
}

/*
 *  Encodes a base85 guid given a GUID pointer
 *  Caller should provide a 21 character buffer for the encoded string.
 *
 *  returns TRUE if successful, FALSE if not
 */
BOOL encode_base85_guid( GUID *guid, LPWSTR str )
{
    unsigned int x, *p, i;

    p = (unsigned int*) guid;
    for( i=0; i<4; i++ )
    {
        x = p[i];
        *str++ = table_enc85[x%85];
        x = x/85;
        *str++ = table_enc85[x%85];
        x = x/85;
        *str++ = table_enc85[x%85];
        x = x/85;
        *str++ = table_enc85[x%85];
        x = x/85;
        *str++ = table_enc85[x%85];
    }
    *str = 0;

    return TRUE;
}

DWORD msi_version_str_to_dword(LPCWSTR p)
{
    DWORD major, minor = 0, build = 0, version = 0;

    if (!p)
        return version;

    major = atoiW(p);

    p = strchrW(p, '.');
    if (p)
    {
        minor = atoiW(p+1);
        p = strchrW(p+1, '.');
        if (p)
            build = atoiW(p+1);
    }

    return MAKELONG(build, MAKEWORD(minor, major));
}

LONG msi_reg_set_val_str( HKEY hkey, LPCWSTR name, LPCWSTR value )
{
    DWORD len;
    if (!value) value = szEmpty;
    len = (lstrlenW(value) + 1) * sizeof (WCHAR);
    return RegSetValueExW( hkey, name, 0, REG_SZ, (const BYTE *)value, len );
}

LONG msi_reg_set_val_multi_str( HKEY hkey, LPCWSTR name, LPCWSTR value )
{
    LPCWSTR p = value;
    while (*p) p += lstrlenW(p) + 1;
    return RegSetValueExW( hkey, name, 0, REG_MULTI_SZ,
                           (const BYTE *)value, (p + 1 - value) * sizeof(WCHAR) );
}

LONG msi_reg_set_val_dword( HKEY hkey, LPCWSTR name, DWORD val )
{
    return RegSetValueExW( hkey, name, 0, REG_DWORD, (LPBYTE)&val, sizeof (DWORD) );
}

LONG msi_reg_set_subkey_val( HKEY hkey, LPCWSTR path, LPCWSTR name, LPCWSTR val )
{
    HKEY hsubkey = 0;
    LONG r;

    r = RegCreateKeyW( hkey, path, &hsubkey );
    if (r != ERROR_SUCCESS)
        return r;
    r = msi_reg_set_val_str( hsubkey, name, val );
    RegCloseKey( hsubkey );
    return r;
}

LPWSTR msi_reg_get_val_str( HKEY hkey, LPCWSTR name )
{
    DWORD len = 0;
    LPWSTR val;
    LONG r;

    r = RegQueryValueExW(hkey, name, NULL, NULL, NULL, &len);
    if (r != ERROR_SUCCESS)
        return NULL;

    len += sizeof (WCHAR);
    val = msi_alloc( len );
    if (!val)
        return NULL;
    val[0] = 0;
    RegQueryValueExW(hkey, name, NULL, NULL, (LPBYTE) val, &len);
    return val;
}

BOOL msi_reg_get_val_dword( HKEY hkey, LPCWSTR name, DWORD *val)
{
    DWORD type, len = sizeof (DWORD);
    LONG r = RegQueryValueExW(hkey, name, NULL, &type, (LPBYTE) val, &len);
    return r == ERROR_SUCCESS && type == REG_DWORD;
}

static WCHAR *get_user_sid(void)
{
    HANDLE token;
    DWORD size = 256;
    TOKEN_USER *user;
    WCHAR *ret;

    if (!OpenProcessToken( GetCurrentProcess(), TOKEN_QUERY, &token )) return NULL;
    if (!(user = msi_alloc( size )))
    {
        CloseHandle( token );
        return NULL;
    }
    if (!GetTokenInformation( token, TokenUser, user, size, &size ))
    {
        msi_free( user );
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER || !(user = msi_alloc( size )))
        {
            CloseHandle( token );
            return NULL;
        }
        GetTokenInformation( token, TokenUser, user, size, &size );
    }
    CloseHandle( token );
    if (!ConvertSidToStringSidW( user->User.Sid, &ret ))
    {
        msi_free( user );
        return NULL;
    }
    msi_free( user );
    return ret;
}

UINT MSIREG_OpenUninstallKey(const WCHAR *product, enum platform platform, HKEY *key, BOOL create)
{
    WCHAR keypath[0x200];

    TRACE("%s\n", debugstr_w(product));

    if (is_64bit && platform == PLATFORM_INTEL)
    {
        strcpyW(keypath, szUninstall_32node);
        strcatW(keypath, product);
    }
    else
    {
        strcpyW(keypath, szUninstall);
        strcatW(keypath, product);
    }
    if (create) return RegCreateKeyExW(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, KEY_ALL_ACCESS, NULL, key, NULL);
    return RegOpenKeyExW(HKEY_LOCAL_MACHINE, keypath, 0, KEY_ALL_ACCESS, key);
}

UINT MSIREG_DeleteUninstallKey(const WCHAR *product, enum platform platform)
{
    WCHAR keypath[0x200];

    TRACE("%s\n", debugstr_w(product));

    if (is_64bit && platform == PLATFORM_INTEL)
    {
        strcpyW(keypath, szUninstall_32node);
        strcatW(keypath, product);
    }
    else
    {
        strcpyW(keypath, szUninstall);
        strcatW(keypath, product);
    }
    return SHDeleteKeyW(HKEY_LOCAL_MACHINE, keypath);
}

UINT MSIREG_OpenProductKey(LPCWSTR szProduct, LPCWSTR szUserSid, MSIINSTALLCONTEXT context, HKEY *key, BOOL create)
{
    LPWSTR usersid = NULL;
    HKEY root = HKEY_LOCAL_MACHINE;
    REGSAM access = KEY_WOW64_64KEY | KEY_ALL_ACCESS;
    WCHAR squished_pc[GUID_SIZE], keypath[MAX_PATH];

    if (!squash_guid(szProduct, squished_pc)) return ERROR_FUNCTION_FAILED;
    TRACE("%s squished %s\n", debugstr_w(szProduct), debugstr_w(squished_pc));

    if (context == MSIINSTALLCONTEXT_MACHINE)
    {
        strcpyW(keypath, szInstaller_LocalClassesProd);
        strcatW(keypath, squished_pc);
    }
    else if (context == MSIINSTALLCONTEXT_USERUNMANAGED)
    {
        root = HKEY_CURRENT_USER;
        strcpyW(keypath, szUserProducts);
        strcatW(keypath, squished_pc);
    }
    else
    {
        if (!szUserSid)
        {
            if (!(usersid = get_user_sid()))
            {
                ERR("Failed to retrieve user SID\n");
                return ERROR_FUNCTION_FAILED;
            }
            szUserSid = usersid;
        }
        sprintfW(keypath, szInstaller_LocalManagedProd_fmt, szUserSid, squished_pc);
        LocalFree(usersid);
    }
    if (create) return RegCreateKeyExW(root, keypath, 0, NULL, 0, access, NULL, key, NULL);
    return RegOpenKeyExW(root, keypath, 0, access, key);
}

UINT MSIREG_DeleteUserProductKey(LPCWSTR szProduct)
{
    WCHAR squished_pc[GUID_SIZE], keypath[0x200];

    if (!squash_guid(szProduct, squished_pc)) return ERROR_FUNCTION_FAILED;
    TRACE("%s squished %s\n", debugstr_w(szProduct), debugstr_w(squished_pc));

    strcpyW(keypath, szUserProducts);
    strcatW(keypath, squished_pc);
    return SHDeleteKeyW(HKEY_CURRENT_USER, keypath);
}

UINT MSIREG_OpenUserPatchesKey(LPCWSTR szPatch, HKEY *key, BOOL create)
{
    WCHAR squished_pc[GUID_SIZE], keypath[0x200];

    if (!squash_guid(szPatch, squished_pc)) return ERROR_FUNCTION_FAILED;
    TRACE("%s squished %s\n", debugstr_w(szPatch), debugstr_w(squished_pc));

    strcpyW(keypath, szUserPatches);
    strcatW(keypath, squished_pc);

    if (create) return RegCreateKeyW(HKEY_CURRENT_USER, keypath, key);
    return RegOpenKeyW(HKEY_CURRENT_USER, keypath, key);
}

UINT MSIREG_OpenFeaturesKey(LPCWSTR szProduct, LPCWSTR szUserSid, MSIINSTALLCONTEXT context,
                            HKEY *key, BOOL create)
{
    HKEY root = HKEY_LOCAL_MACHINE;
    REGSAM access = KEY_WOW64_64KEY | KEY_ALL_ACCESS;
    WCHAR squished_pc[GUID_SIZE], keypath[MAX_PATH], *usersid = NULL;

    if (!squash_guid(szProduct, squished_pc)) return ERROR_FUNCTION_FAILED;
    TRACE("%s squished %s\n", debugstr_w(szProduct), debugstr_w(squished_pc));

    if (context == MSIINSTALLCONTEXT_MACHINE)
    {
        strcpyW(keypath, szInstaller_LocalClassesFeat);
        strcatW(keypath, squished_pc);
    }
    else if (context == MSIINSTALLCONTEXT_USERUNMANAGED)
    {
        root = HKEY_CURRENT_USER;
        strcpyW(keypath, szUserFeatures);
        strcatW(keypath, squished_pc);
    }
    else
    {
        if (!szUserSid)
        {
            if (!(usersid = get_user_sid()))
            {
                ERR("Failed to retrieve user SID\n");
                return ERROR_FUNCTION_FAILED;
            }
            szUserSid = usersid;
        }
        sprintfW(keypath, szInstaller_LocalManagedFeat_fmt, szUserSid, squished_pc);
        LocalFree(usersid);
    }
    if (create) return RegCreateKeyExW(root, keypath, 0, NULL, 0, access, NULL, key, NULL);
    return RegOpenKeyExW(root, keypath, 0, access, key);
}

UINT MSIREG_DeleteUserFeaturesKey(LPCWSTR szProduct)
{
    WCHAR squished_pc[GUID_SIZE], keypath[0x200];

    if (!squash_guid(szProduct, squished_pc)) return ERROR_FUNCTION_FAILED;
    TRACE("%s squished %s\n", debugstr_w(szProduct), debugstr_w(squished_pc));

    strcpyW(keypath, szUserFeatures);
    strcatW(keypath, squished_pc);
    return SHDeleteKeyW(HKEY_CURRENT_USER, keypath);
}

static UINT MSIREG_OpenInstallerFeaturesKey(LPCWSTR szProduct, HKEY *key, BOOL create)
{
    REGSAM access = KEY_WOW64_64KEY | KEY_ALL_ACCESS;
    WCHAR squished_pc[GUID_SIZE], keypath[0x200];

    if (!squash_guid(szProduct, squished_pc)) return ERROR_FUNCTION_FAILED;
    TRACE("%s squished %s\n", debugstr_w(szProduct), debugstr_w(squished_pc));

    strcpyW(keypath, szInstaller_Features);
    strcatW(keypath, squished_pc);

    if (create) return RegCreateKeyExW(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, key, NULL);
    return RegOpenKeyExW(HKEY_LOCAL_MACHINE, keypath, 0, access, key);
}

UINT MSIREG_OpenUserDataFeaturesKey(LPCWSTR szProduct, LPCWSTR szUserSid, MSIINSTALLCONTEXT context,
                                    HKEY *key, BOOL create)
{
    REGSAM access = KEY_WOW64_64KEY | KEY_ALL_ACCESS;
    WCHAR squished_pc[GUID_SIZE], keypath[0x200], *usersid = NULL;

    if (!squash_guid(szProduct, squished_pc)) return ERROR_FUNCTION_FAILED;
    TRACE("%s squished %s\n", debugstr_w(szProduct), debugstr_w(squished_pc));

    if (context == MSIINSTALLCONTEXT_MACHINE)
    {
        sprintfW(keypath, szUserDataFeatures_fmt, szLocalSid, squished_pc);
    }
    else
    {
        if (!szUserSid)
        {
            if (!(usersid = get_user_sid()))
            {
                ERR("Failed to retrieve user SID\n");
                return ERROR_FUNCTION_FAILED;
            }
            szUserSid = usersid;
        }
        sprintfW(keypath, szUserDataFeatures_fmt, szUserSid, squished_pc);
        LocalFree(usersid);
    }
    if (create) return RegCreateKeyExW(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, key, NULL);
    return RegOpenKeyExW(HKEY_LOCAL_MACHINE, keypath, 0, access, key);
}

UINT MSIREG_OpenUserComponentsKey(LPCWSTR szComponent, HKEY *key, BOOL create)
{
    WCHAR squished_cc[GUID_SIZE], keypath[0x200];

    if (!squash_guid(szComponent, squished_cc)) return ERROR_FUNCTION_FAILED;
    TRACE("%s squished %s\n", debugstr_w(szComponent), debugstr_w(squished_cc));

    strcpyW(keypath, szUserComponents);
    strcatW(keypath, squished_cc);

    if (create) return RegCreateKeyW(HKEY_CURRENT_USER, keypath, key);
    return RegOpenKeyW(HKEY_CURRENT_USER, keypath, key);
}

UINT MSIREG_OpenUserDataComponentKey(LPCWSTR szComponent, LPCWSTR szUserSid, HKEY *key, BOOL create)
{
    REGSAM access = KEY_WOW64_64KEY | KEY_ALL_ACCESS;
    WCHAR comp[GUID_SIZE], keypath[0x200];
    LPWSTR usersid;

    if (!squash_guid(szComponent, comp)) return ERROR_FUNCTION_FAILED;
    TRACE("%s squished %s\n", debugstr_w(szComponent), debugstr_w(comp));

    if (!szUserSid)
    {
        if (!(usersid = get_user_sid()))
        {
            ERR("Failed to retrieve user SID\n");
            return ERROR_FUNCTION_FAILED;
        }
        sprintfW(keypath, szUserDataComp_fmt, usersid, comp);
        LocalFree(usersid);
    }
    else
        sprintfW(keypath, szUserDataComp_fmt, szUserSid, comp);

    if (create) return RegCreateKeyExW(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, key, NULL);
    return RegOpenKeyExW(HKEY_LOCAL_MACHINE, keypath, 0, access, key);
}

UINT MSIREG_DeleteUserDataComponentKey(LPCWSTR szComponent, LPCWSTR szUserSid)
{
    REGSAM access = KEY_WOW64_64KEY | KEY_ALL_ACCESS;
    WCHAR comp[GUID_SIZE], keypath[0x200];
    LPWSTR usersid;
    HKEY hkey;
    LONG r;

    if (!squash_guid(szComponent, comp)) return ERROR_FUNCTION_FAILED;
    TRACE("%s squished %s\n", debugstr_w(szComponent), debugstr_w(comp));

    if (!szUserSid)
    {
        if (!(usersid = get_user_sid()))
        {
            ERR("Failed to retrieve user SID\n");
            return ERROR_FUNCTION_FAILED;
        }
        sprintfW(keypath, szUserDataComponents_fmt, usersid);
        LocalFree(usersid);
    }
    else
        sprintfW(keypath, szUserDataComponents_fmt, szUserSid);

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, keypath, 0, access, &hkey)) return ERROR_SUCCESS;
    r = SHDeleteKeyW(hkey, comp);
    RegCloseKey(hkey);
    return r;
}

UINT MSIREG_OpenUserDataProductKey(LPCWSTR szProduct, MSIINSTALLCONTEXT dwContext, LPCWSTR szUserSid, HKEY *key, BOOL create)
{
    REGSAM access = KEY_WOW64_64KEY | KEY_ALL_ACCESS;
    WCHAR squished_pc[GUID_SIZE], keypath[0x200];
    LPWSTR usersid;

    if (!squash_guid(szProduct, squished_pc)) return ERROR_FUNCTION_FAILED;
    TRACE("%s squished %s\n", debugstr_w(szProduct), debugstr_w(squished_pc));

    if (dwContext == MSIINSTALLCONTEXT_MACHINE)
        sprintfW(keypath, szUserDataProd_fmt, szLocalSid, squished_pc);
    else if (szUserSid)
        sprintfW(keypath, szUserDataProd_fmt, szUserSid, squished_pc);
    else
    {
        if (!(usersid = get_user_sid()))
        {
            ERR("Failed to retrieve user SID\n");
            return ERROR_FUNCTION_FAILED;
        }
        sprintfW(keypath, szUserDataProd_fmt, usersid, squished_pc);
        LocalFree(usersid);
    }
    if (create) return RegCreateKeyExW(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, key, NULL);
    return RegOpenKeyExW(HKEY_LOCAL_MACHINE, keypath, 0, access, key);
}

UINT MSIREG_OpenUserDataPatchKey(LPCWSTR szPatch, MSIINSTALLCONTEXT dwContext, HKEY *key, BOOL create)
{
    REGSAM access = KEY_WOW64_64KEY | KEY_ALL_ACCESS;
    WCHAR squished_patch[GUID_SIZE], keypath[0x200];
    LPWSTR usersid;

    if (!squash_guid(szPatch, squished_patch)) return ERROR_FUNCTION_FAILED;
    TRACE("%s squished %s\n", debugstr_w(szPatch), debugstr_w(squished_patch));

    if (dwContext == MSIINSTALLCONTEXT_MACHINE)
        sprintfW(keypath, szUserDataPatch_fmt, szLocalSid, squished_patch);
    else
    {
        if (!(usersid = get_user_sid()))
        {
            ERR("Failed to retrieve user SID\n");
            return ERROR_FUNCTION_FAILED;
        }
        sprintfW(keypath, szUserDataPatch_fmt, usersid, squished_patch);
        LocalFree(usersid);
    }
    if (create) return RegCreateKeyExW(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, key, NULL);
    return RegOpenKeyExW(HKEY_LOCAL_MACHINE, keypath, 0, access, key);
}

UINT MSIREG_DeleteUserDataPatchKey(LPCWSTR patch, MSIINSTALLCONTEXT context)
{
    REGSAM access = KEY_WOW64_64KEY | KEY_ALL_ACCESS;
    WCHAR squished_patch[GUID_SIZE], keypath[0x200];
    LPWSTR usersid;
    HKEY hkey;
    LONG r;

    if (!squash_guid(patch, squished_patch)) return ERROR_FUNCTION_FAILED;
    TRACE("%s squished %s\n", debugstr_w(patch), debugstr_w(squished_patch));

    if (context == MSIINSTALLCONTEXT_MACHINE)
        sprintfW(keypath, szUserDataPatches_fmt, szLocalSid);
    else
    {
        if (!(usersid = get_user_sid()))
        {
            ERR("Failed to retrieve user SID\n");
            return ERROR_FUNCTION_FAILED;
        }
        sprintfW(keypath, szUserDataPatches_fmt, usersid);
        LocalFree(usersid);
    }
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, keypath, 0, access, &hkey)) return ERROR_SUCCESS;
    r = SHDeleteKeyW(hkey, squished_patch);
    RegCloseKey(hkey);
    return r;
}

UINT MSIREG_OpenUserDataProductPatchesKey(LPCWSTR product, MSIINSTALLCONTEXT context, HKEY *key, BOOL create)
{
    REGSAM access = KEY_WOW64_64KEY | KEY_ALL_ACCESS;
    WCHAR squished_product[GUID_SIZE], keypath[0x200];
    LPWSTR usersid;

    if (!squash_guid(product, squished_product)) return ERROR_FUNCTION_FAILED;
    TRACE("%s squished %s\n", debugstr_w(product), debugstr_w(squished_product));

    if (context == MSIINSTALLCONTEXT_MACHINE)
        sprintfW(keypath, szUserDataProductPatches_fmt, szLocalSid, squished_product);
    else
    {
        if (!(usersid = get_user_sid()))
        {
            ERR("Failed to retrieve user SID\n");
            return ERROR_FUNCTION_FAILED;
        }
        sprintfW(keypath, szUserDataProductPatches_fmt, usersid, squished_product);
        LocalFree(usersid);
    }
    if (create) return RegCreateKeyExW(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, key, NULL);
    return RegOpenKeyExW(HKEY_LOCAL_MACHINE, keypath, 0, access, key);
}

UINT MSIREG_OpenInstallProps(LPCWSTR szProduct, MSIINSTALLCONTEXT dwContext, LPCWSTR szUserSid, HKEY *key, BOOL create)
{
    LPWSTR usersid;
    REGSAM access = KEY_WOW64_64KEY | KEY_ALL_ACCESS;
    WCHAR squished_pc[GUID_SIZE], keypath[0x200];

    if (!squash_guid(szProduct, squished_pc)) return ERROR_FUNCTION_FAILED;
    TRACE("%s squished %s\n", debugstr_w(szProduct), debugstr_w(squished_pc));

    if (dwContext == MSIINSTALLCONTEXT_MACHINE)
        sprintfW(keypath, szInstallProperties_fmt, szLocalSid, squished_pc);
    else if (szUserSid)
        sprintfW(keypath, szInstallProperties_fmt, szUserSid, squished_pc);
    else
    {
        if (!(usersid = get_user_sid()))
        {
            ERR("Failed to retrieve user SID\n");
            return ERROR_FUNCTION_FAILED;
        }
        sprintfW(keypath, szInstallProperties_fmt, usersid, squished_pc);
        LocalFree(usersid);
    }
    if (create) return RegCreateKeyExW(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, key, NULL);
    return RegOpenKeyExW(HKEY_LOCAL_MACHINE, keypath, 0, access, key);
}

UINT MSIREG_DeleteUserDataProductKey(LPCWSTR szProduct)
{
    REGSAM access = KEY_WOW64_64KEY | KEY_ALL_ACCESS;
    WCHAR squished_pc[GUID_SIZE], keypath[0x200];
    LPWSTR usersid;
    HKEY hkey;
    LONG r;

    if (!squash_guid(szProduct, squished_pc)) return ERROR_FUNCTION_FAILED;
    TRACE("%s squished %s\n", debugstr_w(szProduct), debugstr_w(squished_pc));

    if (!(usersid = get_user_sid()))
    {
        ERR("Failed to retrieve user SID\n");
        return ERROR_FUNCTION_FAILED;
    }
    sprintfW(keypath, szUserDataProducts_fmt, usersid);
    LocalFree(usersid);

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, keypath, 0, access, &hkey)) return ERROR_SUCCESS;
    r = SHDeleteKeyW(hkey, squished_pc);
    RegCloseKey(hkey);
    return r;
}

UINT MSIREG_DeleteProductKey(LPCWSTR szProduct)
{
    REGSAM access = KEY_WOW64_64KEY | KEY_ALL_ACCESS;
    WCHAR squished_pc[GUID_SIZE];
    HKEY hkey;
    LONG r;

    if (!squash_guid(szProduct, squished_pc)) return ERROR_FUNCTION_FAILED;
    TRACE("%s squished %s\n", debugstr_w(szProduct), debugstr_w(squished_pc));

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, szInstaller_Products, 0, access, &hkey)) return ERROR_SUCCESS;
    r = SHDeleteKeyW(hkey, squished_pc);
    RegCloseKey(hkey);
    return r;
}

UINT MSIREG_OpenPatchesKey(LPCWSTR szPatch, HKEY *key, BOOL create)
{
    REGSAM access = KEY_WOW64_64KEY | KEY_ALL_ACCESS;
    WCHAR squished_pc[GUID_SIZE], keypath[0x200];

    if (!squash_guid(szPatch, squished_pc)) return ERROR_FUNCTION_FAILED;
    TRACE("%s squished %s\n", debugstr_w(szPatch), debugstr_w(squished_pc));

    sprintfW(keypath, szInstaller_Patches, squished_pc);

    if (create) return RegCreateKeyExW(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, key, NULL);
    return RegOpenKeyExW(HKEY_LOCAL_MACHINE, keypath, 0, access, key);
}

UINT MSIREG_OpenUpgradeCodesKey(LPCWSTR szUpgradeCode, HKEY *key, BOOL create)
{
    REGSAM access = KEY_WOW64_64KEY | KEY_ALL_ACCESS;
    WCHAR squished_pc[GUID_SIZE], keypath[0x200];

    if (!squash_guid(szUpgradeCode, squished_pc)) return ERROR_FUNCTION_FAILED;
    TRACE("%s squished %s\n", debugstr_w(szUpgradeCode), debugstr_w(squished_pc));

    strcpyW(keypath, szInstaller_UpgradeCodes);
    strcatW(keypath, squished_pc);

    if (create) return RegCreateKeyExW(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, key, NULL);
    return RegOpenKeyExW(HKEY_LOCAL_MACHINE, keypath, 0, access, key);
}

UINT MSIREG_OpenUserUpgradeCodesKey(LPCWSTR szUpgradeCode, HKEY* key, BOOL create)
{
    WCHAR squished_pc[GUID_SIZE], keypath[0x200];

    if (!squash_guid(szUpgradeCode, squished_pc)) return ERROR_FUNCTION_FAILED;
    TRACE("%s squished %s\n", debugstr_w(szUpgradeCode), debugstr_w(squished_pc));

    strcpyW(keypath, szInstaller_UserUpgradeCodes);
    strcatW(keypath, squished_pc);

    if (create) return RegCreateKeyW(HKEY_CURRENT_USER, keypath, key);
    return RegOpenKeyW(HKEY_CURRENT_USER, keypath, key);
}

UINT MSIREG_DeleteUserUpgradeCodesKey(LPCWSTR szUpgradeCode)
{
    WCHAR squished_pc[GUID_SIZE], keypath[0x200];

    if (!squash_guid(szUpgradeCode, squished_pc)) return ERROR_FUNCTION_FAILED;
    TRACE("%s squished %s\n", debugstr_w(szUpgradeCode), debugstr_w(squished_pc));

    strcpyW(keypath, szInstaller_UserUpgradeCodes);
    strcatW(keypath, squished_pc);
    return SHDeleteKeyW(HKEY_CURRENT_USER, keypath);
}

UINT MSIREG_DeleteLocalClassesProductKey(LPCWSTR szProductCode)
{
    REGSAM access = KEY_WOW64_64KEY | KEY_ALL_ACCESS;
    WCHAR squished_pc[GUID_SIZE];
    HKEY hkey;
    LONG r;

    if (!squash_guid(szProductCode, squished_pc)) return ERROR_FUNCTION_FAILED;
    TRACE("%s squished %s\n", debugstr_w(szProductCode), debugstr_w(squished_pc));

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, szInstaller_LocalClassesProducts, 0, access, &hkey)) return ERROR_SUCCESS;
    r = SHDeleteKeyW(hkey, squished_pc);
    RegCloseKey(hkey);
    return r;
}

UINT MSIREG_DeleteLocalClassesFeaturesKey(LPCWSTR szProductCode)
{
    REGSAM access = KEY_WOW64_64KEY | KEY_ALL_ACCESS;
    WCHAR squished_pc[GUID_SIZE];
    HKEY hkey;
    LONG r;

    if (!squash_guid(szProductCode, squished_pc)) return ERROR_FUNCTION_FAILED;
    TRACE("%s squished %s\n", debugstr_w(szProductCode), debugstr_w(squished_pc));

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, szInstaller_LocalClassesFeatures, 0, access, &hkey)) return ERROR_SUCCESS;
    r = SHDeleteKeyW(hkey, squished_pc);
    RegCloseKey(hkey);
    return r;
}

UINT MSIREG_OpenClassesUpgradeCodesKey(LPCWSTR szUpgradeCode, HKEY *key, BOOL create)
{
    REGSAM access = KEY_WOW64_64KEY | KEY_ALL_ACCESS;
    WCHAR squished_pc[GUID_SIZE], keypath[0x200];

    if (!squash_guid(szUpgradeCode, squished_pc)) return ERROR_FUNCTION_FAILED;
    TRACE("%s squished %s\n", debugstr_w(szUpgradeCode), debugstr_w(squished_pc));

    strcpyW(keypath, szInstaller_ClassesUpgradeCode);
    strcatW(keypath, squished_pc);

    if (create) return RegCreateKeyExW(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, key, NULL);
    return RegOpenKeyExW(HKEY_LOCAL_MACHINE, keypath, 0, access, key);
}

UINT MSIREG_DeleteClassesUpgradeCodesKey(LPCWSTR szUpgradeCode)
{
    REGSAM access = KEY_WOW64_64KEY | KEY_ALL_ACCESS;
    WCHAR squished_pc[GUID_SIZE];
    HKEY hkey;
    LONG r;

    if (!squash_guid(szUpgradeCode, squished_pc)) return ERROR_FUNCTION_FAILED;
    TRACE("%s squished %s\n", debugstr_w(szUpgradeCode), debugstr_w(squished_pc));

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, szInstaller_ClassesUpgradeCodes, 0, access, &hkey)) return ERROR_SUCCESS;
    r = SHDeleteKeyW(hkey, squished_pc);
    RegCloseKey(hkey);
    return r;
}

/*************************************************************************
 *  MsiDecomposeDescriptorW   [MSI.@]
 *
 * Decomposes an MSI descriptor into product, feature and component parts.
 * An MSI descriptor is a string of the form:
 *   [base 85 guid] [feature code] '>' [base 85 guid]
 *
 * PARAMS
 *   szDescriptor  [I]  the descriptor to decompose
 *   szProduct     [O]  buffer of MAX_FEATURE_CHARS+1 for the product guid
 *   szFeature     [O]  buffer of MAX_FEATURE_CHARS+1 for the feature code
 *   szComponent   [O]  buffer of MAX_FEATURE_CHARS+1 for the component guid
 *   pUsed         [O]  the length of the descriptor
 *
 * RETURNS
 *   ERROR_SUCCESS             if everything worked correctly
 *   ERROR_INVALID_PARAMETER   if the descriptor was invalid
 *
 */
UINT WINAPI MsiDecomposeDescriptorW( LPCWSTR szDescriptor, LPWSTR szProduct,
                LPWSTR szFeature, LPWSTR szComponent, LPDWORD pUsed )
{
    UINT r, len;
    LPWSTR p;
    GUID product, component;

    TRACE("%s %p %p %p %p\n", debugstr_w(szDescriptor), szProduct,
          szFeature, szComponent, pUsed);

    r = decode_base85_guid( szDescriptor, &product );
    if( !r )
        return ERROR_INVALID_PARAMETER;

    TRACE("product %s\n", debugstr_guid( &product ));

    p = strchrW(&szDescriptor[20],'>');
    if( !p )
        return ERROR_INVALID_PARAMETER;

    len = (p - &szDescriptor[20]);
    if( len > MAX_FEATURE_CHARS )
        return ERROR_INVALID_PARAMETER;

    TRACE("feature %s\n", debugstr_wn( &szDescriptor[20], len ));

    r = decode_base85_guid( p+1, &component );
    if( !r )
        return ERROR_INVALID_PARAMETER;

    TRACE("component %s\n", debugstr_guid( &component ));

    if (szProduct)
        StringFromGUID2( &product, szProduct, MAX_FEATURE_CHARS+1 );
    if (szComponent)
        StringFromGUID2( &component, szComponent, MAX_FEATURE_CHARS+1 );
    if (szFeature)
    {
        memcpy( szFeature, &szDescriptor[20], len*sizeof(WCHAR) );
        szFeature[len] = 0;
    }
    len = ( &p[21] - szDescriptor );

    TRACE("length = %d\n", len);
    if (pUsed) *pUsed = len;

    return ERROR_SUCCESS;
}

UINT WINAPI MsiDecomposeDescriptorA( LPCSTR szDescriptor, LPSTR szProduct,
                LPSTR szFeature, LPSTR szComponent, LPDWORD pUsed )
{
    WCHAR product[MAX_FEATURE_CHARS+1];
    WCHAR feature[MAX_FEATURE_CHARS+1];
    WCHAR component[MAX_FEATURE_CHARS+1];
    LPWSTR str = NULL, p = NULL, f = NULL, c = NULL;
    UINT r;

    TRACE("%s %p %p %p %p\n", debugstr_a(szDescriptor), szProduct,
          szFeature, szComponent, pUsed);

    str = strdupAtoW( szDescriptor );
    if( szDescriptor && !str )
        return ERROR_OUTOFMEMORY;

    if (szProduct)
        p = product;
    if (szFeature)
        f = feature;
    if (szComponent)
        c = component;

    r = MsiDecomposeDescriptorW( str, p, f, c, pUsed );

    if (r == ERROR_SUCCESS)
    {
        WideCharToMultiByte( CP_ACP, 0, p, -1,
                             szProduct, MAX_FEATURE_CHARS+1, NULL, NULL );
        WideCharToMultiByte( CP_ACP, 0, f, -1,
                             szFeature, MAX_FEATURE_CHARS+1, NULL, NULL );
        WideCharToMultiByte( CP_ACP, 0, c, -1,
                             szComponent, MAX_FEATURE_CHARS+1, NULL, NULL );
    }

    msi_free( str );

    return r;
}

UINT WINAPI MsiEnumProductsA(DWORD index, LPSTR lpguid)
{
    DWORD r;
    WCHAR szwGuid[GUID_SIZE];

    TRACE("%d %p\n", index, lpguid);

    if (NULL == lpguid)
        return ERROR_INVALID_PARAMETER;
    r = MsiEnumProductsW(index, szwGuid);
    if( r == ERROR_SUCCESS )
        WideCharToMultiByte(CP_ACP, 0, szwGuid, -1, lpguid, GUID_SIZE, NULL, NULL);

    return r;
}

UINT WINAPI MsiEnumProductsW(DWORD index, LPWSTR lpguid)
{
    TRACE("%d %p\n", index, lpguid);

    if (NULL == lpguid)
        return ERROR_INVALID_PARAMETER;

    return MsiEnumProductsExW( NULL, szAllSid, MSIINSTALLCONTEXT_ALL, index, lpguid,
                               NULL, NULL, NULL );
}

UINT WINAPI MsiEnumFeaturesA(LPCSTR szProduct, DWORD index, 
      LPSTR szFeature, LPSTR szParent)
{
    DWORD r;
    WCHAR szwFeature[GUID_SIZE], szwParent[GUID_SIZE];
    LPWSTR szwProduct = NULL;

    TRACE("%s %d %p %p\n", debugstr_a(szProduct), index, szFeature, szParent);

    if( szProduct )
    {
        szwProduct = strdupAtoW( szProduct );
        if( !szwProduct )
            return ERROR_OUTOFMEMORY;
    }

    r = MsiEnumFeaturesW(szwProduct, index, szwFeature, szwParent);
    if( r == ERROR_SUCCESS )
    {
        WideCharToMultiByte(CP_ACP, 0, szwFeature, -1,
                            szFeature, GUID_SIZE, NULL, NULL);
        WideCharToMultiByte(CP_ACP, 0, szwParent, -1,
                            szParent, GUID_SIZE, NULL, NULL);
    }

    msi_free( szwProduct);

    return r;
}

UINT WINAPI MsiEnumFeaturesW(LPCWSTR szProduct, DWORD index, 
      LPWSTR szFeature, LPWSTR szParent)
{
    HKEY hkeyProduct = 0;
    DWORD r, sz;

    TRACE("%s %d %p %p\n", debugstr_w(szProduct), index, szFeature, szParent);

    if( !szProduct )
        return ERROR_INVALID_PARAMETER;

    r = MSIREG_OpenInstallerFeaturesKey(szProduct,&hkeyProduct,FALSE);
    if( r != ERROR_SUCCESS )
        return ERROR_NO_MORE_ITEMS;

    sz = GUID_SIZE;
    r = RegEnumValueW(hkeyProduct, index, szFeature, &sz, NULL, NULL, NULL, NULL);
    RegCloseKey(hkeyProduct);

    return r;
}

UINT WINAPI MsiEnumComponentsA(DWORD index, LPSTR lpguid)
{
    DWORD r;
    WCHAR szwGuid[GUID_SIZE];

    TRACE("%u, %p\n", index, lpguid);

    if (!lpguid) return ERROR_INVALID_PARAMETER;

    r = MsiEnumComponentsW(index, szwGuid);
    if( r == ERROR_SUCCESS )
        WideCharToMultiByte(CP_ACP, 0, szwGuid, -1, lpguid, GUID_SIZE, NULL, NULL);

    return r;
}

UINT WINAPI MsiEnumComponentsW(DWORD index, LPWSTR lpguid)
{
    TRACE("%u, %p\n", index, lpguid);

    if (!lpguid) return ERROR_INVALID_PARAMETER;

    return MsiEnumComponentsExW( szAllSid, MSIINSTALLCONTEXT_ALL, index, lpguid, NULL, NULL, NULL );
}

UINT WINAPI MsiEnumComponentsExA( LPCSTR user_sid, DWORD ctx, DWORD index, CHAR guid[39],
                                  MSIINSTALLCONTEXT *installed_ctx, LPSTR sid, LPDWORD sid_len )
{
    UINT r;
    WCHAR *user_sidW = NULL, *sidW = NULL, guidW[GUID_SIZE];

    TRACE("%s, %u, %u, %p, %p, %p, %p\n", debugstr_a(user_sid), ctx, index, guid, installed_ctx,
          sid, sid_len);

    if (sid && !sid_len) return ERROR_INVALID_PARAMETER;
    if (user_sid && !(user_sidW = strdupAtoW( user_sid ))) return ERROR_OUTOFMEMORY;
    if (sid && !(sidW = msi_alloc( *sid_len * sizeof(WCHAR) )))
    {
        msi_free( user_sidW );
        return ERROR_OUTOFMEMORY;
    }
    r = MsiEnumComponentsExW( user_sidW, ctx, index, guidW, installed_ctx, sidW, sid_len );
    if (r == ERROR_SUCCESS)
    {
        if (guid) WideCharToMultiByte( CP_ACP, 0, guidW, GUID_SIZE, guid, GUID_SIZE, NULL, NULL );
        if (sid) WideCharToMultiByte( CP_ACP, 0, sidW, *sid_len + 1, sid, *sid_len + 1, NULL, NULL );
    }
    msi_free( user_sidW );
    msi_free( sidW );
    return r;
}

static UINT fetch_machine_component( DWORD ctx, DWORD index, DWORD *idx, WCHAR guid[39],
                                     MSIINSTALLCONTEXT *installed_ctx, LPWSTR sid, LPDWORD sid_len )
{
    static const WCHAR componentsW[] =
        {'S','o','f','t','w','a','r','e','\\','M','i','c','r','o','s','o','f','t','\\',
         'W','i','n','d','o','w','s','\\','C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
         'I','n','s','t','a','l','l','e','r','\\','U','s','e','r','D','a','t','a','\\',
         'S','-','1','-','5','-','1','8','\\','C','o','m','p','o','n','e','n','t','s',0};
    UINT r = ERROR_SUCCESS;
    WCHAR component[GUID_SIZE];
    DWORD i = 0, len_component;
    REGSAM access = KEY_ENUMERATE_SUB_KEYS | KEY_WOW64_64KEY;
    HKEY key_components;

    if (RegOpenKeyExW( HKEY_LOCAL_MACHINE, componentsW, 0, access, &key_components ))
        return ERROR_NO_MORE_ITEMS;

    len_component = sizeof(component)/sizeof(component[0]);
    while (!RegEnumKeyExW( key_components, i, component, &len_component, NULL, NULL, NULL, NULL ))
    {
        if (*idx == index) goto found;
        (*idx)++;
        len_component = sizeof(component)/sizeof(component[0]);
        i++;
    }
    RegCloseKey( key_components );
    return ERROR_NO_MORE_ITEMS;

found:
    if (sid_len)
    {
        if (*sid_len < 1)
        {
            *sid_len = 1;
            r = ERROR_MORE_DATA;
        }
        else if (sid)
        {
            *sid_len = 0;
            sid[0] = 0;
        }
    }
    if (guid) unsquash_guid( component, guid );
    if (installed_ctx) *installed_ctx = MSIINSTALLCONTEXT_MACHINE;
    RegCloseKey( key_components );
    return r;
}

static UINT fetch_user_component( const WCHAR *usersid, DWORD ctx, DWORD index, DWORD *idx,
                                  WCHAR guid[39], MSIINSTALLCONTEXT *installed_ctx, LPWSTR sid,
                                  LPDWORD sid_len )
{
    static const WCHAR userdataW[] =
        {'S','o','f','t','w','a','r','e','\\','M','i','c','r','o','s','o','f','t','\\',
         'W','i','n','d','o','w','s','\\','C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
         'I','n','s','t','a','l','l','e','r','\\','U','s','e','r','D','a','t','a',0};
    static const WCHAR componentsW[] = {'\\','C','o','m','p','o','n','e','n','t','s',0};
    UINT r = ERROR_SUCCESS;
    WCHAR path[MAX_PATH], component[GUID_SIZE], user[128];
    DWORD i = 0, j = 0, len_component, len_user;
    REGSAM access = KEY_ENUMERATE_SUB_KEYS | KEY_WOW64_64KEY;
    HKEY key_users, key_components;

    if (ctx == MSIINSTALLCONTEXT_USERMANAGED) /* FIXME: were to find these? */
        return ERROR_NO_MORE_ITEMS;

    if (RegOpenKeyExW( HKEY_LOCAL_MACHINE, userdataW, 0, access, &key_users ))
        return ERROR_NO_MORE_ITEMS;

    len_user = sizeof(user)/sizeof(user[0]);
    while (!RegEnumKeyExW( key_users, i, user, &len_user, NULL, NULL, NULL, NULL ))
    {
        if ((strcmpW( usersid, szAllSid ) && strcmpW( usersid, user )) ||
            !strcmpW( szLocalSid, user ))
        {
            i++;
            len_user = sizeof(user)/sizeof(user[0]);
            continue;
        }
        strcpyW( path, user );
        strcatW( path, componentsW );
        if (RegOpenKeyExW( key_users, path, 0, access, &key_components ))
        {
            i++;
            len_user = sizeof(user)/sizeof(user[0]);
            continue;
        }
        len_component = sizeof(component)/sizeof(component[0]);
        while (!RegEnumKeyExW( key_components, j, component, &len_component, NULL, NULL, NULL, NULL ))
        {
            if (*idx == index) goto found;
            (*idx)++;
            len_component = sizeof(component)/sizeof(component[0]);
            j++;
        }
        RegCloseKey( key_components );
        len_user = sizeof(user)/sizeof(user[0]);
        i++;
    }
    RegCloseKey( key_users );
    return ERROR_NO_MORE_ITEMS;

found:
    if (sid_len)
    {
        if (*sid_len < len_user + 1)
        {
            *sid_len = len_user + 1;
            r = ERROR_MORE_DATA;
        }
        else if (sid)
        {
            *sid_len = len_user;
            strcpyW( sid, user );
        }
    }
    if (guid) unsquash_guid( component, guid );
    if (installed_ctx) *installed_ctx = ctx;
    RegCloseKey( key_components );
    RegCloseKey( key_users );
    return r;
}

static UINT enum_components( const WCHAR *usersid, DWORD ctx, DWORD index, DWORD *idx, WCHAR guid[39],
                             MSIINSTALLCONTEXT *installed_ctx, LPWSTR sid, LPDWORD sid_len )
{
    UINT r = ERROR_NO_MORE_ITEMS;
    WCHAR *user = NULL;

    if (!usersid)
    {
        usersid = user = get_user_sid();
        if (!user) return ERROR_FUNCTION_FAILED;
    }
    if (ctx & MSIINSTALLCONTEXT_USERMANAGED)
    {
        r = fetch_user_component( usersid, MSIINSTALLCONTEXT_USERMANAGED, index, idx, guid,
                                  installed_ctx, sid, sid_len );
        if (r != ERROR_NO_MORE_ITEMS) goto done;
    }
    if (ctx & MSIINSTALLCONTEXT_USERUNMANAGED)
    {
        r = fetch_user_component( usersid, MSIINSTALLCONTEXT_USERUNMANAGED, index, idx, guid,
                                  installed_ctx, sid, sid_len );
        if (r != ERROR_NO_MORE_ITEMS) goto done;
    }
    if (ctx & MSIINSTALLCONTEXT_MACHINE)
    {
        r = fetch_machine_component( MSIINSTALLCONTEXT_MACHINE, index, idx, guid, installed_ctx,
                                     sid, sid_len );
        if (r != ERROR_NO_MORE_ITEMS) goto done;
    }

done:
    LocalFree( user );
    return r;
}

UINT WINAPI MsiEnumComponentsExW( LPCWSTR user_sid, DWORD ctx, DWORD index, WCHAR guid[39],
                                  MSIINSTALLCONTEXT *installed_ctx, LPWSTR sid, LPDWORD sid_len )
{
    UINT r;
    DWORD idx = 0;
    static DWORD last_index;

    TRACE("%s, %u, %u, %p, %p, %p, %p\n", debugstr_w(user_sid), ctx, index, guid, installed_ctx,
          sid, sid_len);

    if ((sid && !sid_len) || !ctx || (user_sid && ctx == MSIINSTALLCONTEXT_MACHINE))
        return ERROR_INVALID_PARAMETER;

    if (index && index - last_index != 1)
        return ERROR_INVALID_PARAMETER;

    if (!index) last_index = 0;

    r = enum_components( user_sid, ctx, index, &idx, guid, installed_ctx, sid, sid_len );
    if (r == ERROR_SUCCESS)
        last_index = index;
    else
        last_index = 0;

    return r;
}

UINT WINAPI MsiEnumClientsA(LPCSTR szComponent, DWORD index, LPSTR szProduct)
{
    DWORD r;
    WCHAR szwProduct[GUID_SIZE];
    LPWSTR szwComponent = NULL;

    TRACE("%s %d %p\n", debugstr_a(szComponent), index, szProduct);

    if ( !szProduct )
        return ERROR_INVALID_PARAMETER;

    if( szComponent )
    {
        szwComponent = strdupAtoW( szComponent );
        if( !szwComponent )
            return ERROR_OUTOFMEMORY;
    }

    r = MsiEnumClientsW(szComponent?szwComponent:NULL, index, szwProduct);
    if( r == ERROR_SUCCESS )
    {
        WideCharToMultiByte(CP_ACP, 0, szwProduct, -1,
                            szProduct, GUID_SIZE, NULL, NULL);
    }

    msi_free( szwComponent);

    return r;
}

UINT WINAPI MsiEnumClientsW(LPCWSTR szComponent, DWORD index, LPWSTR szProduct)
{
    HKEY hkeyComp = 0;
    DWORD r, sz;
    WCHAR szValName[SQUISH_GUID_SIZE];

    TRACE("%s %d %p\n", debugstr_w(szComponent), index, szProduct);

    if (!szComponent || !*szComponent || !szProduct)
        return ERROR_INVALID_PARAMETER;

    if (MSIREG_OpenUserDataComponentKey(szComponent, NULL, &hkeyComp, FALSE) != ERROR_SUCCESS &&
        MSIREG_OpenUserDataComponentKey(szComponent, szLocalSid, &hkeyComp, FALSE) != ERROR_SUCCESS)
        return ERROR_UNKNOWN_COMPONENT;

    /* see if there are any products at all */
    sz = SQUISH_GUID_SIZE;
    r = RegEnumValueW(hkeyComp, 0, szValName, &sz, NULL, NULL, NULL, NULL);
    if (r != ERROR_SUCCESS)
    {
        RegCloseKey(hkeyComp);

        if (index != 0)
            return ERROR_INVALID_PARAMETER;

        return ERROR_UNKNOWN_COMPONENT;
    }

    sz = SQUISH_GUID_SIZE;
    r = RegEnumValueW(hkeyComp, index, szValName, &sz, NULL, NULL, NULL, NULL);
    if( r == ERROR_SUCCESS )
    {
        unsquash_guid(szValName, szProduct);
        TRACE("-> %s\n", debugstr_w(szProduct));
    }
    RegCloseKey(hkeyComp);
    return r;
}

UINT WINAPI MsiEnumClientsExA(LPCSTR component, LPCSTR usersid, DWORD ctx, DWORD index,
                              CHAR installed_product[GUID_SIZE],
                              MSIINSTALLCONTEXT *installed_ctx, LPSTR sid, LPDWORD sid_len)
{
    FIXME("%s, %s, %u, %u, %p, %p, %p, %p\n", debugstr_a(component), debugstr_a(usersid),
          ctx, index, installed_product, installed_ctx, sid, sid_len);
    return ERROR_ACCESS_DENIED;
}

UINT WINAPI MsiEnumClientsExW(LPCWSTR component, LPCWSTR usersid, DWORD ctx, DWORD index,
                              WCHAR installed_product[GUID_SIZE],
                              MSIINSTALLCONTEXT *installed_ctx, LPWSTR sid, LPDWORD sid_len)
{
    FIXME("%s, %s, %u, %u, %p, %p, %p, %p\n", debugstr_w(component), debugstr_w(usersid),
          ctx, index, installed_product, installed_ctx, sid, sid_len);
    return ERROR_ACCESS_DENIED;
}

static UINT MSI_EnumComponentQualifiers( LPCWSTR szComponent, DWORD iIndex,
                awstring *lpQualBuf, LPDWORD pcchQual,
                awstring *lpAppBuf, LPDWORD pcchAppBuf )
{
    DWORD name_sz, val_sz, name_max, val_max, type, ofs;
    LPWSTR name = NULL, val = NULL;
    UINT r, r2;
    HKEY key;

    TRACE("%s %08x %p %p %p %p\n", debugstr_w(szComponent), iIndex,
          lpQualBuf, pcchQual, lpAppBuf, pcchAppBuf);

    if (!szComponent)
        return ERROR_INVALID_PARAMETER;

    r = MSIREG_OpenUserComponentsKey( szComponent, &key, FALSE );
    if (r != ERROR_SUCCESS)
        return ERROR_UNKNOWN_COMPONENT;

    /* figure out how big the name is we want to return */
    name_max = 0x10;
    r = ERROR_OUTOFMEMORY;
    name = msi_alloc( name_max * sizeof(WCHAR) );
    if (!name)
        goto end;

    val_max = 0x10;
    r = ERROR_OUTOFMEMORY;
    val = msi_alloc( val_max );
    if (!val)
        goto end;

    /* loop until we allocate enough memory */
    while (1)
    {
        name_sz = name_max;
        val_sz = val_max;
        r = RegEnumValueW( key, iIndex, name, &name_sz,
                           NULL, &type, (LPBYTE)val, &val_sz );
        if (r == ERROR_SUCCESS)
            break;
        if (r != ERROR_MORE_DATA)
            goto end;
 
        if (type != REG_MULTI_SZ)
        {
            ERR("component data has wrong type (%d)\n", type);
            goto end;
        }

        r = ERROR_OUTOFMEMORY;
        if (name_sz + 1 >= name_max)
        {
            name_max *= 2;
            msi_free( name );
            name = msi_alloc( name_max * sizeof (WCHAR) );
            if (!name)
                goto end;
            continue;
        }
        if (val_sz > val_max)
        {
            val_max = val_sz + sizeof (WCHAR);
            msi_free( val );
            val = msi_alloc( val_max * sizeof (WCHAR) );
            if (!val)
                goto end;
            continue;
        }
        ERR("should be enough data, but isn't %d %d\n", name_sz, val_sz );
        goto end;
    }

    ofs = 0;
    r = MsiDecomposeDescriptorW( val, NULL, NULL, NULL, &ofs );
    if (r != ERROR_SUCCESS)
        goto end;

    TRACE("Providing %s and %s\n", debugstr_w(name), debugstr_w(val+ofs));

    r = msi_strcpy_to_awstring( name, -1, lpQualBuf, pcchQual );
    r2 = msi_strcpy_to_awstring( val+ofs, -1, lpAppBuf, pcchAppBuf );

    if (r2 != ERROR_SUCCESS)
        r = r2;

end:
    msi_free(val);
    msi_free(name);
    RegCloseKey(key);
    return r;
}

/*************************************************************************
 *  MsiEnumComponentQualifiersA [MSI.@]
 */
UINT WINAPI MsiEnumComponentQualifiersA( LPCSTR szComponent, DWORD iIndex,
                LPSTR lpQualifierBuf, LPDWORD pcchQualifierBuf,
                LPSTR lpApplicationDataBuf, LPDWORD pcchApplicationDataBuf )
{
    awstring qual, appdata;
    LPWSTR comp;
    UINT r;

    TRACE("%s %08x %p %p %p %p\n", debugstr_a(szComponent), iIndex,
          lpQualifierBuf, pcchQualifierBuf, lpApplicationDataBuf,
          pcchApplicationDataBuf);

    comp = strdupAtoW( szComponent );
    if (szComponent && !comp)
        return ERROR_OUTOFMEMORY;

    qual.unicode = FALSE;
    qual.str.a = lpQualifierBuf;

    appdata.unicode = FALSE;
    appdata.str.a = lpApplicationDataBuf;

    r = MSI_EnumComponentQualifiers( comp, iIndex,
              &qual, pcchQualifierBuf, &appdata, pcchApplicationDataBuf );
    msi_free( comp );
    return r;
}

/*************************************************************************
 *  MsiEnumComponentQualifiersW [MSI.@]
 */
UINT WINAPI MsiEnumComponentQualifiersW( LPCWSTR szComponent, DWORD iIndex,
                LPWSTR lpQualifierBuf, LPDWORD pcchQualifierBuf,
                LPWSTR lpApplicationDataBuf, LPDWORD pcchApplicationDataBuf )
{
    awstring qual, appdata;

    TRACE("%s %08x %p %p %p %p\n", debugstr_w(szComponent), iIndex,
          lpQualifierBuf, pcchQualifierBuf, lpApplicationDataBuf,
          pcchApplicationDataBuf);

    qual.unicode = TRUE;
    qual.str.w = lpQualifierBuf;

    appdata.unicode = TRUE;
    appdata.str.w = lpApplicationDataBuf;

    return MSI_EnumComponentQualifiers( szComponent, iIndex,
                 &qual, pcchQualifierBuf, &appdata, pcchApplicationDataBuf );
}

/*************************************************************************
 *  MsiEnumRelatedProductsW   [MSI.@]
 *
 */
UINT WINAPI MsiEnumRelatedProductsW(LPCWSTR szUpgradeCode, DWORD dwReserved,
                                    DWORD iProductIndex, LPWSTR lpProductBuf)
{
    UINT r;
    HKEY hkey;
    DWORD dwSize = SQUISH_GUID_SIZE;
    WCHAR szKeyName[SQUISH_GUID_SIZE];

    TRACE("%s %u %u %p\n", debugstr_w(szUpgradeCode), dwReserved,
          iProductIndex, lpProductBuf);

    if (NULL == szUpgradeCode)
        return ERROR_INVALID_PARAMETER;
    if (NULL == lpProductBuf)
        return ERROR_INVALID_PARAMETER;

    r = MSIREG_OpenUpgradeCodesKey(szUpgradeCode, &hkey, FALSE);
    if (r != ERROR_SUCCESS)
        return ERROR_NO_MORE_ITEMS;

    r = RegEnumValueW(hkey, iProductIndex, szKeyName, &dwSize, NULL, NULL, NULL, NULL);
    if( r == ERROR_SUCCESS )
        unsquash_guid(szKeyName, lpProductBuf);
    RegCloseKey(hkey);

    return r;
}

/*************************************************************************
 *  MsiEnumRelatedProductsA   [MSI.@]
 *
 */
UINT WINAPI MsiEnumRelatedProductsA(LPCSTR szUpgradeCode, DWORD dwReserved,
                                    DWORD iProductIndex, LPSTR lpProductBuf)
{
    LPWSTR szwUpgradeCode = NULL;
    WCHAR productW[GUID_SIZE];
    UINT r;

    TRACE("%s %u %u %p\n", debugstr_a(szUpgradeCode), dwReserved,
          iProductIndex, lpProductBuf);

    if (szUpgradeCode)
    {
        szwUpgradeCode = strdupAtoW( szUpgradeCode );
        if( !szwUpgradeCode )
            return ERROR_OUTOFMEMORY;
    }

    r = MsiEnumRelatedProductsW( szwUpgradeCode, dwReserved,
                                 iProductIndex, productW );
    if (r == ERROR_SUCCESS)
    {
        WideCharToMultiByte( CP_ACP, 0, productW, GUID_SIZE,
                             lpProductBuf, GUID_SIZE, NULL, NULL );
    }
    msi_free( szwUpgradeCode);
    return r;
}

/***********************************************************************
 * MsiEnumPatchesExA            [MSI.@]
 */
UINT WINAPI MsiEnumPatchesExA(LPCSTR szProductCode, LPCSTR szUserSid,
        DWORD dwContext, DWORD dwFilter, DWORD dwIndex, LPSTR szPatchCode,
        LPSTR szTargetProductCode, MSIINSTALLCONTEXT *pdwTargetProductContext,
        LPSTR szTargetUserSid, LPDWORD pcchTargetUserSid)
{
    LPWSTR prodcode = NULL;
    LPWSTR usersid = NULL;
    LPWSTR targsid = NULL;
    WCHAR patch[GUID_SIZE];
    WCHAR targprod[GUID_SIZE];
    DWORD len;
    UINT r;

    TRACE("(%s, %s, %d, %d, %d, %p, %p, %p, %p, %p)\n",
          debugstr_a(szProductCode), debugstr_a(szUserSid), dwContext, dwFilter,
          dwIndex, szPatchCode, szTargetProductCode, pdwTargetProductContext,
          szTargetUserSid, pcchTargetUserSid);

    if (szTargetUserSid && !pcchTargetUserSid)
        return ERROR_INVALID_PARAMETER;

    if (szProductCode) prodcode = strdupAtoW(szProductCode);
    if (szUserSid) usersid = strdupAtoW(szUserSid);

    r = MsiEnumPatchesExW(prodcode, usersid, dwContext, dwFilter, dwIndex,
                          patch, targprod, pdwTargetProductContext,
                          NULL, &len);
    if (r != ERROR_SUCCESS)
        goto done;

    WideCharToMultiByte(CP_ACP, 0, patch, -1, szPatchCode,
                        GUID_SIZE, NULL, NULL);
    WideCharToMultiByte(CP_ACP, 0, targprod, -1, szTargetProductCode,
                        GUID_SIZE, NULL, NULL);

    if (!szTargetUserSid)
    {
        if (pcchTargetUserSid)
            *pcchTargetUserSid = len;

        goto done;
    }

    targsid = msi_alloc(++len * sizeof(WCHAR));
    if (!targsid)
    {
        r = ERROR_OUTOFMEMORY;
        goto done;
    }

    r = MsiEnumPatchesExW(prodcode, usersid, dwContext, dwFilter, dwIndex,
                          patch, targprod, pdwTargetProductContext,
                          targsid, &len);
    if (r != ERROR_SUCCESS || !szTargetUserSid)
        goto done;

    WideCharToMultiByte(CP_ACP, 0, targsid, -1, szTargetUserSid,
                        *pcchTargetUserSid, NULL, NULL);

    len = lstrlenW(targsid);
    if (*pcchTargetUserSid < len + 1)
    {
        r = ERROR_MORE_DATA;
        *pcchTargetUserSid = len * sizeof(WCHAR);
    }
    else
        *pcchTargetUserSid = len;

done:
    msi_free(prodcode);
    msi_free(usersid);
    msi_free(targsid);

    return r;
}

static UINT msi_get_patch_state(LPCWSTR prodcode, LPCWSTR usersid,
                                MSIINSTALLCONTEXT context,
                                LPWSTR patch, MSIPATCHSTATE *state)
{
    DWORD type, val, size;
    HKEY prod, hkey = 0;
    HKEY udpatch = 0;
    LONG res;
    UINT r = ERROR_NO_MORE_ITEMS;

    *state = MSIPATCHSTATE_INVALID;

    r = MSIREG_OpenUserDataProductKey(prodcode, context,
                                      usersid, &prod, FALSE);
    if (r != ERROR_SUCCESS)
        return ERROR_NO_MORE_ITEMS;

    res = RegOpenKeyExW(prod, szPatches, 0, KEY_READ, &hkey);
    if (res != ERROR_SUCCESS)
        goto done;

    res = RegOpenKeyExW(hkey, patch, 0, KEY_READ, &udpatch);
    if (res != ERROR_SUCCESS)
        goto done;

    size = sizeof(DWORD);
    res = RegGetValueW(udpatch, NULL, szState, RRF_RT_DWORD, &type, &val, &size);
    if (res != ERROR_SUCCESS ||
        val < MSIPATCHSTATE_APPLIED || val > MSIPATCHSTATE_REGISTERED)
    {
        r = ERROR_BAD_CONFIGURATION;
        goto done;
    }

    *state = val;
    r = ERROR_SUCCESS;

done:
    RegCloseKey(udpatch);
    RegCloseKey(hkey);
    RegCloseKey(prod);

    return r;
}

static UINT msi_check_product_patches(LPCWSTR prodcode, LPCWSTR usersid,
        MSIINSTALLCONTEXT context, DWORD filter, DWORD index, DWORD *idx,
        LPWSTR patch, LPWSTR targetprod, MSIINSTALLCONTEXT *targetctx,
        LPWSTR targetsid, DWORD *sidsize, LPWSTR *transforms)
{
    MSIPATCHSTATE state = MSIPATCHSTATE_INVALID;
    LPWSTR ptr, patches = NULL;
    HKEY prod, patchkey = 0;
    HKEY localprod = 0, localpatch = 0;
    DWORD type, size;
    LONG res;
    UINT temp, r = ERROR_NO_MORE_ITEMS;

    if (MSIREG_OpenProductKey(prodcode, usersid, context,
                              &prod, FALSE) != ERROR_SUCCESS)
        return ERROR_NO_MORE_ITEMS;

    size = 0;
    res = RegGetValueW(prod, szPatches, szPatches, RRF_RT_ANY, &type, NULL,
                       &size);
    if (res != ERROR_SUCCESS)
        goto done;

    if (type != REG_MULTI_SZ)
    {
        r = ERROR_BAD_CONFIGURATION;
        goto done;
    }

    patches = msi_alloc(size);
    if (!patches)
    {
        r = ERROR_OUTOFMEMORY;
        goto done;
    }

    res = RegGetValueW(prod, szPatches, szPatches, RRF_RT_ANY, &type,
                       patches, &size);
    if (res != ERROR_SUCCESS)
        goto done;

    for (ptr = patches; *ptr && r == ERROR_NO_MORE_ITEMS; ptr += lstrlenW(ptr) + 1)
    {
        if (!unsquash_guid(ptr, patch))
        {
            r = ERROR_BAD_CONFIGURATION;
            goto done;
        }

        size = 0;
        res = RegGetValueW(prod, szPatches, ptr, RRF_RT_REG_SZ,
                           &type, NULL, &size);
        if (res != ERROR_SUCCESS)
            continue;

        if (transforms)
        {
            *transforms = msi_alloc(size);
            if (!*transforms)
            {
                r = ERROR_OUTOFMEMORY;
                goto done;
            }

            res = RegGetValueW(prod, szPatches, ptr, RRF_RT_REG_SZ,
                               &type, *transforms, &size);
            if (res != ERROR_SUCCESS)
                continue;
        }

        if (context == MSIINSTALLCONTEXT_USERMANAGED)
        {
            if (!(filter & MSIPATCHSTATE_APPLIED))
            {
                temp = msi_get_patch_state(prodcode, usersid, context,
                                           ptr, &state);
                if (temp == ERROR_BAD_CONFIGURATION)
                {
                    r = ERROR_BAD_CONFIGURATION;
                    goto done;
                }

                if (temp != ERROR_SUCCESS || !(filter & state))
                    continue;
            }
        }
        else if (context == MSIINSTALLCONTEXT_USERUNMANAGED)
        {
            if (!(filter & MSIPATCHSTATE_APPLIED))
            {
                temp = msi_get_patch_state(prodcode, usersid, context,
                                           ptr, &state);
                if (temp == ERROR_BAD_CONFIGURATION)
                {
                    r = ERROR_BAD_CONFIGURATION;
                    goto done;
                }

                if (temp != ERROR_SUCCESS || !(filter & state))
                    continue;
            }
            else
            {
                temp = MSIREG_OpenUserDataPatchKey(patch, context,
                                                   &patchkey, FALSE);
                RegCloseKey(patchkey);
                if (temp != ERROR_SUCCESS)
                    continue;
            }
        }
        else if (context == MSIINSTALLCONTEXT_MACHINE)
        {
            usersid = szEmpty;

            if (MSIREG_OpenUserDataProductKey(prodcode, context, NULL, &localprod, FALSE) == ERROR_SUCCESS &&
                RegOpenKeyExW(localprod, szPatches, 0, KEY_READ, &localpatch) == ERROR_SUCCESS &&
                RegOpenKeyExW(localpatch, ptr, 0, KEY_READ, &patchkey) == ERROR_SUCCESS)
            {
                res = RegGetValueW(patchkey, NULL, szState, RRF_RT_REG_DWORD,
                                   &type, &state, &size);

                if (!(filter & state))
                    res = ERROR_NO_MORE_ITEMS;

                RegCloseKey(patchkey);
            }

            RegCloseKey(localpatch);
            RegCloseKey(localprod);

            if (res != ERROR_SUCCESS)
                continue;
        }

        if (*idx < index)
        {
            (*idx)++;
            continue;
        }

        r = ERROR_SUCCESS;
        if (targetprod)
            lstrcpyW(targetprod, prodcode);

        if (targetctx)
            *targetctx = context;

        if (targetsid)
        {
            lstrcpynW(targetsid, usersid, *sidsize);
            if (lstrlenW(usersid) >= *sidsize)
                r = ERROR_MORE_DATA;
        }

        if (sidsize)
        {
            *sidsize = lstrlenW(usersid);
            if (!targetsid)
                *sidsize *= sizeof(WCHAR);
        }
    }

done:
    RegCloseKey(prod);
    msi_free(patches);

    return r;
}

static UINT msi_enum_patches(LPCWSTR szProductCode, LPCWSTR szUserSid,
        DWORD dwContext, DWORD dwFilter, DWORD dwIndex, DWORD *idx,
        LPWSTR szPatchCode, LPWSTR szTargetProductCode,
        MSIINSTALLCONTEXT *pdwTargetProductContext, LPWSTR szTargetUserSid,
        LPDWORD pcchTargetUserSid, LPWSTR *szTransforms)
{
    LPWSTR usersid = NULL;
    UINT r = ERROR_INVALID_PARAMETER;

    if (!szUserSid)
    {
        szUserSid = usersid = get_user_sid();
        if (!usersid) return ERROR_FUNCTION_FAILED;
    }

    if (dwContext & MSIINSTALLCONTEXT_USERMANAGED)
    {
        r = msi_check_product_patches(szProductCode, szUserSid,
                                      MSIINSTALLCONTEXT_USERMANAGED, dwFilter,
                                      dwIndex, idx, szPatchCode,
                                      szTargetProductCode,
                                      pdwTargetProductContext, szTargetUserSid,
                                      pcchTargetUserSid, szTransforms);
        if (r != ERROR_NO_MORE_ITEMS)
            goto done;
    }

    if (dwContext & MSIINSTALLCONTEXT_USERUNMANAGED)
    {
        r = msi_check_product_patches(szProductCode, szUserSid,
                                      MSIINSTALLCONTEXT_USERUNMANAGED, dwFilter,
                                      dwIndex, idx, szPatchCode,
                                      szTargetProductCode,
                                      pdwTargetProductContext, szTargetUserSid,
                                      pcchTargetUserSid, szTransforms);
        if (r != ERROR_NO_MORE_ITEMS)
            goto done;
    }

    if (dwContext & MSIINSTALLCONTEXT_MACHINE)
    {
        r = msi_check_product_patches(szProductCode, szUserSid,
                                      MSIINSTALLCONTEXT_MACHINE, dwFilter,
                                      dwIndex, idx, szPatchCode,
                                      szTargetProductCode,
                                      pdwTargetProductContext, szTargetUserSid,
                                      pcchTargetUserSid, szTransforms);
        if (r != ERROR_NO_MORE_ITEMS)
            goto done;
    }

done:
    LocalFree(usersid);
    return r;
}

/***********************************************************************
 * MsiEnumPatchesExW            [MSI.@]
 */
UINT WINAPI MsiEnumPatchesExW(LPCWSTR szProductCode, LPCWSTR szUserSid,
        DWORD dwContext, DWORD dwFilter, DWORD dwIndex, LPWSTR szPatchCode,
        LPWSTR szTargetProductCode, MSIINSTALLCONTEXT *pdwTargetProductContext,
        LPWSTR szTargetUserSid, LPDWORD pcchTargetUserSid)
{
    WCHAR squished_pc[GUID_SIZE];
    DWORD idx = 0;
    UINT r;

    static DWORD last_index;

    TRACE("(%s, %s, %d, %d, %d, %p, %p, %p, %p, %p)\n",
          debugstr_w(szProductCode), debugstr_w(szUserSid), dwContext, dwFilter,
          dwIndex, szPatchCode, szTargetProductCode, pdwTargetProductContext,
          szTargetUserSid, pcchTargetUserSid);

    if (!szProductCode || !squash_guid(szProductCode, squished_pc))
        return ERROR_INVALID_PARAMETER;

    if (szUserSid && !strcmpW( szUserSid, szLocalSid ))
        return ERROR_INVALID_PARAMETER;

    if (dwContext & MSIINSTALLCONTEXT_MACHINE && szUserSid)
        return ERROR_INVALID_PARAMETER;

    if (dwContext <= MSIINSTALLCONTEXT_NONE ||
        dwContext > MSIINSTALLCONTEXT_ALL)
        return ERROR_INVALID_PARAMETER;

    if (dwFilter <= MSIPATCHSTATE_INVALID || dwFilter > MSIPATCHSTATE_ALL)
        return ERROR_INVALID_PARAMETER;

    if (dwIndex && dwIndex - last_index != 1)
        return ERROR_INVALID_PARAMETER;

    if (dwIndex == 0)
        last_index = 0;

    r = msi_enum_patches(szProductCode, szUserSid, dwContext, dwFilter,
                         dwIndex, &idx, szPatchCode, szTargetProductCode,
                         pdwTargetProductContext, szTargetUserSid,
                         pcchTargetUserSid, NULL);

    if (r == ERROR_SUCCESS)
        last_index = dwIndex;
    else
        last_index = 0;

    return r;
}

/***********************************************************************
 * MsiEnumPatchesA            [MSI.@]
 */
UINT WINAPI MsiEnumPatchesA(LPCSTR szProduct, DWORD iPatchIndex,
        LPSTR lpPatchBuf, LPSTR lpTransformsBuf, LPDWORD pcchTransformsBuf)
{
    LPWSTR product, transforms;
    WCHAR patch[GUID_SIZE];
    DWORD len;
    UINT r;

    TRACE("(%s %d %p %p %p)\n", debugstr_a(szProduct), iPatchIndex,
          lpPatchBuf, lpTransformsBuf, pcchTransformsBuf);

    if (!szProduct || !lpPatchBuf || !lpTransformsBuf || !pcchTransformsBuf)
        return ERROR_INVALID_PARAMETER;

    product = strdupAtoW(szProduct);
    if (!product)
        return ERROR_OUTOFMEMORY;

    len = *pcchTransformsBuf;
    transforms = msi_alloc( len * sizeof(WCHAR) );
    if (!transforms)
    {
        r = ERROR_OUTOFMEMORY;
        goto done;
    }

    r = MsiEnumPatchesW(product, iPatchIndex, patch, transforms, &len);
    if (r != ERROR_SUCCESS && r != ERROR_MORE_DATA)
        goto done;

    WideCharToMultiByte(CP_ACP, 0, patch, -1, lpPatchBuf,
                        GUID_SIZE, NULL, NULL);

    if (!WideCharToMultiByte(CP_ACP, 0, transforms, -1, lpTransformsBuf,
                             *pcchTransformsBuf, NULL, NULL))
        r = ERROR_MORE_DATA;

    if (r == ERROR_MORE_DATA)
    {
        lpTransformsBuf[*pcchTransformsBuf - 1] = '\0';
        *pcchTransformsBuf = len * 2;
    }
    else
        *pcchTransformsBuf = strlen( lpTransformsBuf );

done:
    msi_free(transforms);
    msi_free(product);

    return r;
}

/***********************************************************************
 * MsiEnumPatchesW            [MSI.@]
 */
UINT WINAPI MsiEnumPatchesW(LPCWSTR szProduct, DWORD iPatchIndex,
        LPWSTR lpPatchBuf, LPWSTR lpTransformsBuf, LPDWORD pcchTransformsBuf)
{
    WCHAR squished_pc[GUID_SIZE];
    LPWSTR transforms = NULL;
    HKEY prod;
    DWORD idx = 0;
    UINT r;

    TRACE("(%s %d %p %p %p)\n", debugstr_w(szProduct), iPatchIndex,
          lpPatchBuf, lpTransformsBuf, pcchTransformsBuf);

    if (!szProduct || !squash_guid(szProduct, squished_pc))
        return ERROR_INVALID_PARAMETER;

    if (!lpPatchBuf || !lpTransformsBuf || !pcchTransformsBuf)
        return ERROR_INVALID_PARAMETER;

    if (MSIREG_OpenProductKey(szProduct, NULL, MSIINSTALLCONTEXT_USERMANAGED,
                              &prod, FALSE) != ERROR_SUCCESS &&
        MSIREG_OpenProductKey(szProduct, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                              &prod, FALSE) != ERROR_SUCCESS &&
        MSIREG_OpenProductKey(szProduct, NULL, MSIINSTALLCONTEXT_MACHINE,
                              &prod, FALSE) != ERROR_SUCCESS)
        return ERROR_UNKNOWN_PRODUCT;

    RegCloseKey(prod);

    r = msi_enum_patches(szProduct, NULL, MSIINSTALLCONTEXT_ALL,
                         MSIPATCHSTATE_ALL, iPatchIndex, &idx, lpPatchBuf,
                         NULL, NULL, NULL, NULL, &transforms);
    if (r != ERROR_SUCCESS)
        goto done;

    lstrcpynW(lpTransformsBuf, transforms, *pcchTransformsBuf);
    if (*pcchTransformsBuf <= lstrlenW(transforms))
    {
        r = ERROR_MORE_DATA;
        *pcchTransformsBuf = lstrlenW(transforms);
    }
    else
        *pcchTransformsBuf = lstrlenW(transforms);

done:
    msi_free(transforms);
    return r;
}

UINT WINAPI MsiEnumProductsExA( LPCSTR product, LPCSTR usersid, DWORD ctx, DWORD index,
                                CHAR installed_product[GUID_SIZE],
                                MSIINSTALLCONTEXT *installed_ctx, LPSTR sid, LPDWORD sid_len )
{
    UINT r;
    WCHAR installed_productW[GUID_SIZE], *productW = NULL, *usersidW = NULL, *sidW = NULL;

    TRACE("%s, %s, %u, %u, %p, %p, %p, %p\n", debugstr_a(product), debugstr_a(usersid),
          ctx, index, installed_product, installed_ctx, sid, sid_len);

    if (sid && !sid_len) return ERROR_INVALID_PARAMETER;
    if (product && !(productW = strdupAtoW( product ))) return ERROR_OUTOFMEMORY;
    if (usersid && !(usersidW = strdupAtoW( usersid )))
    {
        msi_free( productW );
        return ERROR_OUTOFMEMORY;
    }
    if (sid && !(sidW = msi_alloc( *sid_len * sizeof(WCHAR) )))
    {
        msi_free( usersidW );
        msi_free( productW );
        return ERROR_OUTOFMEMORY;
    }
    r = MsiEnumProductsExW( productW, usersidW, ctx, index, installed_productW,
                            installed_ctx, sidW, sid_len );
    if (r == ERROR_SUCCESS)
    {
        if (installed_product) WideCharToMultiByte( CP_ACP, 0, installed_productW, GUID_SIZE,
                                                    installed_product, GUID_SIZE, NULL, NULL );
        if (sid) WideCharToMultiByte( CP_ACP, 0, sidW, *sid_len + 1, sid, *sid_len + 1, NULL, NULL );
    }
    msi_free( productW );
    msi_free( usersidW );
    msi_free( sidW );
    return r;
}

static UINT fetch_machine_product( const WCHAR *match, DWORD index, DWORD *idx,
                                   WCHAR installed_product[GUID_SIZE],
                                   MSIINSTALLCONTEXT *installed_ctx, WCHAR *sid, DWORD *sid_len )
{
    static const WCHAR productsW[] =
        {'S','o','f','t','w','a','r','e','\\','C','l','a','s','s','e','s','\\',
         'I','n','s','t','a','l','l','e','r','\\','P','r','o','d','u','c','t','s',0};
    UINT r;
    WCHAR product[GUID_SIZE];
    DWORD i = 0, len;
    REGSAM access = KEY_ENUMERATE_SUB_KEYS | KEY_WOW64_64KEY;
    HKEY key;

    if (RegOpenKeyExW( HKEY_LOCAL_MACHINE, productsW, 0, access, &key ))
        return ERROR_NO_MORE_ITEMS;

    len = sizeof(product)/sizeof(product[0]);
    while (!RegEnumKeyExW( key, i, product, &len, NULL, NULL, NULL, NULL ))
    {
        if (match && strcmpW( match, product ))
        {
            i++;
            len = sizeof(product)/sizeof(product[0]);
            continue;
        }
        if (*idx == index) goto found;
        (*idx)++;
        len = sizeof(product)/sizeof(product[0]);
        i++;
    }
    RegCloseKey( key );
    return ERROR_NO_MORE_ITEMS;

found:
    if (sid_len && *sid_len < 1)
    {
        *sid_len = 1;
        r = ERROR_MORE_DATA;
    }
    else
    {
        if (installed_product) unsquash_guid( product, installed_product );
        if (installed_ctx) *installed_ctx = MSIINSTALLCONTEXT_MACHINE;
        if (sid)
        {
            sid[0] = 0;
            *sid_len = 0;
        }
        r = ERROR_SUCCESS;
    }
    RegCloseKey( key );
    return r;
}

static UINT fetch_user_product( const WCHAR *match, const WCHAR *usersid, DWORD ctx, DWORD index,
                                DWORD *idx, WCHAR installed_product[GUID_SIZE],
                                MSIINSTALLCONTEXT *installed_ctx, WCHAR *sid, DWORD *sid_len )
{
    static const WCHAR managedW[] =
        {'S','o','f','t','w','a','r','e','\\','M','i','c','r','o','s','o','f','t','\\',
         'W','i','n','d','o','w','s','\\','C','u','r','r','e','n','t','V','e','r','s',
         'i','o','n','\\','I','n','s','t','a','l','l','e','r','\\','M','a','n','a','g','e','d',0};
    static const WCHAR managed_productsW[] =
        {'\\','I','n','s','t','a','l','l','e','r','\\','P','r','o','d','u','c','t','s',0};
    static const WCHAR unmanaged_productsW[] =
        {'\\','S','o','f','t','w','a','r','e','\\','M','i','c','r','o','s','o','f','t','\\',
         'I','n','s','t','a','l','l','e','r','\\','P','r','o','d','u','c','t','s',0};
    UINT r;
    const WCHAR *subkey;
    WCHAR path[MAX_PATH], product[GUID_SIZE], user[128];
    DWORD i = 0, j = 0, len_product, len_user;
    REGSAM access = KEY_ENUMERATE_SUB_KEYS | KEY_WOW64_64KEY;
    HKEY key_users, key_products;

    if (ctx == MSIINSTALLCONTEXT_USERMANAGED)
    {
        subkey = managed_productsW;
        if (RegOpenKeyExW( HKEY_LOCAL_MACHINE, managedW, 0, access, &key_users ))
            return ERROR_NO_MORE_ITEMS;
    }
    else if (ctx == MSIINSTALLCONTEXT_USERUNMANAGED)
    {
        subkey = unmanaged_productsW;
        if (RegOpenKeyExW( HKEY_USERS, NULL, 0, access, &key_users ))
            return ERROR_NO_MORE_ITEMS;
    }
    else return ERROR_INVALID_PARAMETER;

    len_user = sizeof(user)/sizeof(user[0]);
    while (!RegEnumKeyExW( key_users, i, user, &len_user, NULL, NULL, NULL, NULL ))
    {
        if (strcmpW( usersid, user ) && strcmpW( usersid, szAllSid ))
        {
            i++;
            len_user = sizeof(user)/sizeof(user[0]);
            continue;
        }
        strcpyW( path, user );
        strcatW( path, subkey );
        if (RegOpenKeyExW( key_users, path, 0, access, &key_products ))
        {
            i++;
            len_user = sizeof(user)/sizeof(user[0]);
            continue;
        }
        len_product = sizeof(product)/sizeof(product[0]);
        while (!RegEnumKeyExW( key_products, j, product, &len_product, NULL, NULL, NULL, NULL ))
        {
            if (match && strcmpW( match, product ))
            {
                j++;
                len_product = sizeof(product)/sizeof(product[0]);
                continue;
            }
            if (*idx == index) goto found;
            (*idx)++;
            len_product = sizeof(product)/sizeof(product[0]);
            j++;
        }
        RegCloseKey( key_products );
        len_user = sizeof(user)/sizeof(user[0]);
        i++;
    }
    RegCloseKey( key_users );
    return ERROR_NO_MORE_ITEMS;

found:
    if (sid_len && *sid_len <= len_user)
    {
        *sid_len = len_user;
        r = ERROR_MORE_DATA;
    }
    else
    {
        if (installed_product) unsquash_guid( product, installed_product );
        if (installed_ctx) *installed_ctx = ctx;
        if (sid)
        {
            strcpyW( sid, user );
            *sid_len = len_user;
        }
        r = ERROR_SUCCESS;
    }
    RegCloseKey( key_products );
    RegCloseKey( key_users );
    return r;
}

static UINT enum_products( const WCHAR *product, const WCHAR *usersid, DWORD ctx, DWORD index,
                           DWORD *idx, WCHAR installed_product[GUID_SIZE],
                           MSIINSTALLCONTEXT *installed_ctx, WCHAR *sid, DWORD *sid_len )
{
    UINT r = ERROR_NO_MORE_ITEMS;
    WCHAR *user = NULL;

    if (!usersid)
    {
        usersid = user = get_user_sid();
        if (!user) return ERROR_FUNCTION_FAILED;
    }
    if (ctx & MSIINSTALLCONTEXT_MACHINE)
    {
        r = fetch_machine_product( product, index, idx, installed_product, installed_ctx,
                                   sid, sid_len );
        if (r != ERROR_NO_MORE_ITEMS) goto done;
    }
    if (ctx & MSIINSTALLCONTEXT_USERUNMANAGED)
    {
        r = fetch_user_product( product, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, index,
                                idx, installed_product, installed_ctx, sid, sid_len );
        if (r != ERROR_NO_MORE_ITEMS) goto done;
    }
    if (ctx & MSIINSTALLCONTEXT_USERMANAGED)
    {
        r = fetch_user_product( product, usersid, MSIINSTALLCONTEXT_USERMANAGED, index,
                                idx, installed_product, installed_ctx, sid, sid_len );
        if (r != ERROR_NO_MORE_ITEMS) goto done;
    }

done:
    LocalFree( user );
    return r;
}

UINT WINAPI MsiEnumProductsExW( LPCWSTR product, LPCWSTR usersid, DWORD ctx, DWORD index,
                                WCHAR installed_product[GUID_SIZE],
                                MSIINSTALLCONTEXT *installed_ctx, LPWSTR sid, LPDWORD sid_len )
{
    UINT r;
    DWORD idx = 0;
    static DWORD last_index;

    TRACE("%s, %s, %u, %u, %p, %p, %p, %p\n", debugstr_w(product), debugstr_w(usersid),
          ctx, index, installed_product, installed_ctx, sid, sid_len);

    if ((sid && !sid_len) || !ctx || (usersid && ctx == MSIINSTALLCONTEXT_MACHINE))
        return ERROR_INVALID_PARAMETER;

    if (index && index - last_index != 1)
        return ERROR_INVALID_PARAMETER;

    if (!index) last_index = 0;

    r = enum_products( product, usersid, ctx, index, &idx, installed_product, installed_ctx,
                       sid, sid_len );
    if (r == ERROR_SUCCESS)
        last_index = index;
    else
        last_index = 0;

    return r;
}
