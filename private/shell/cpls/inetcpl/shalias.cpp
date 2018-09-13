/****************************************************************************
 *
 *  Microsoft Confidential
 *  Copyright (c) Microsoft Corporation 1994
 *  All rights reserved
 *
 ***************************************************************************/

#ifdef UNIX_FEATURE_ALIAS

#undef UNICODE

#include "inetcplp.h"
#include "shalias.h"

#include "mluisupp.h"

static TCHAR g_szAliasKey[]     = TEXT("Software\\Microsoft\\Internet Explorer\\Unix\\Alias");

// Member function definitions for CAlias
CAlias::CAlias( TCHAR * name )
{
    m_alias = (TCHAR *) LocalAlloc( LPTR, (lstrlen(name) + 1)*sizeof(TCHAR) );
    StrCpy( m_alias, name );
    m_szurl = NULL;
    m_fDirty= TRUE;
}

CAlias::~CAlias()
{
    if( m_alias ) LocalFree( m_alias );
    if( m_szurl ) LocalFree( m_szurl );
}

CAlias::Load()
{
    if(m_alias)
    {
        HKEY hKey;

        TCHAR aliasKey[MAX_PATH], buffer[MAX_PATH];
        StrCpy( aliasKey, g_szAliasKey);
        StrCat( aliasKey, TEXT("\\"));
        StrCat( aliasKey, m_alias );
         
           HRESULT lResult = RegOpenKeyExA(
                  HKEY_CURRENT_USER,
                  aliasKey,
                  0,
                  KEY_QUERY_VALUE | KEY_READ,
                  &hKey);
        if( lResult == ERROR_SUCCESS )
        {
            DWORD dwLen = MAX_PATH;
            if (RegQueryValue( hKey, NULL, buffer,  (long *)&dwLen ) 
                  == ERROR_SUCCESS )
            {
                 m_szurl = (TCHAR *)LocalAlloc( LPTR, (dwLen+1)*sizeof(TCHAR));
                 StrCpy(m_szurl, buffer);
            }
                
            RegCloseKey( hKey );
        }
        else
            return FALSE;
       
    }

    return TRUE;
}

CAlias::Save()
{
    HRESULT lResult;
    HKEY    hKey;
    TCHAR aliasKey[MAX_PATH], buffer[MAX_PATH];
    StrCpy( aliasKey, g_szAliasKey);
    StrCat( aliasKey, TEXT("\\"));
    StrCat( aliasKey, m_alias );
         
    lResult = RegOpenKeyEx(
        HKEY_CURRENT_USER,
        aliasKey,
        0,
        KEY_QUERY_VALUE| KEY_WRITE,
        &hKey);

    if( lResult != ERROR_SUCCESS )
    {
        lResult = RegCreateKey(
                      HKEY_CURRENT_USER,
                      aliasKey,
                      &hKey);      
    }

    if( lResult == ERROR_SUCCESS )
    {
        DWORD dwType = REG_SZ;
        DWORD dwLen  = (lstrlen(m_szurl)+1)*sizeof(TCHAR);

        RegSetValue( hKey, NULL, dwType, m_szurl, dwLen );
        RegCloseKey( hKey);
    }
    else 
        return FALSE;
               
    m_fDirty = FALSE;
    return TRUE;
}


CAlias::Delete()
{
    TCHAR aliasKey[MAX_PATH], buffer[MAX_PATH];
    StrCpy( aliasKey, g_szAliasKey);
    StrCat( aliasKey, TEXT("\\"));
    StrCat( aliasKey, m_alias );
    RegDeleteKey( HKEY_CURRENT_USER, aliasKey ); 
    return TRUE;
}

#ifdef DEBUG
CAlias::Print()
{
    if( m_alias ) printf( m_alias );
    printf(",");
    if( m_szurl ) printf( m_szurl );
    printf("\n");
    return TRUE;
}
#endif

STDAPI_(BOOL) FreeAliases( HDPA aliasListIn )
{
    if(aliasListIn)
    {
        int aliasCount = DPA_GetPtrCount( aliasListIn );

        for(int i=0; i<aliasCount; i++ )
        {
           CAlias * ptr = (CAlias *)DPA_FastGetPtr( aliasListIn, i );
          if(ptr) delete ptr;
        }
        return TRUE;
    }

    return FALSE;
}

STDAPI_(BOOL) AddAliasToList( HDPA aliasListIn, LPTSTR aliasIn, LPTSTR szurl, HWND hwnd )
{
    int index;
    CAlias * ptr;
    TCHAR alias[ MAX_ALIAS_LENGTH + 1 ];
    TCHAR achTemp[MAX_ALIAS_LENGTH], achTemp2[MAX_ALIAS_LENGTH];

    if (aliasListIn && aliasIn && lstrlen(aliasIn)<MAX_ALIAS_LENGTH)
    {
        StrCpy( alias, aliasIn );
        EatSpaces( alias );
        if((index = FindAliasIndex( aliasListIn, alias )) != -1)
        {
            if( !hwnd ) 
                return FALSE;

            MLLoadShellLangString(IDS_ERROR_ALIAS_ALREADY_EXISTS,
                achTemp, sizeof(achTemp));
            MLLoadShellLangString(IDS_TITLE_ALIASADD,
                achTemp2, sizeof(achTemp2));

            if(hwnd && MessageBox(hwnd, achTemp, achTemp2 , MB_YESNO|MB_ICONQUESTION) != IDYES)
                 return FALSE;

            ptr = (CAlias *)DPA_FastGetPtr( aliasListIn, index );
            SetAliasInfo( ptr, NULL, szurl );
            return TRUE;
        }
        else
        {
            ptr = new CAlias( alias );
            SetAliasInfo( ptr, NULL, szurl );
            DPA_InsertPtr(aliasListIn, 0x7FFF, (LPVOID)ptr);
            return TRUE;
        }
    }

    return FALSE;
}

STDAPI_(BOOL) SaveAliases( HDPA aliasListIn )
{
    // Save the currently changed aliases
    if( aliasListIn )
    {
        int count = DPA_GetPtrCount( aliasListIn );

        for(int i=0;i<count;i++)
        {
            CAlias * pAlias = (CAlias *)DPA_FastGetPtr( aliasListIn, i );
            if(pAlias && pAlias->m_fDirty)
            {
                pAlias->Save();
            }
        }

        return TRUE;
    }

    return FALSE;
}

STDAPI_(BOOL) LoadAliases( HDPA aliasListIn )
{
    HKEY hKey, hKeyAlias;
    int index = 0;
    DWORD dwLen  = MAX_PATH;

    if(aliasListIn) 
        FreeAliases( aliasListIn );

    TCHAR * buffer = (TCHAR *)LocalAlloc( LPTR, (MAX_PATH+1)*sizeof(TCHAR) );

    LONG lResult = RegOpenKeyExA(
                HKEY_CURRENT_USER,
                g_szAliasKey,
                0,
                KEY_QUERY_VALUE | KEY_READ,
                &hKeyAlias);
    
    if( lResult == ERROR_SUCCESS )
    {
        while( buffer )
        {
            dwLen  = MAXPATH; 
            if( RegEnumKeyEx( hKeyAlias, index, buffer, &dwLen,
                              NULL, NULL, NULL, NULL )
                == ERROR_NO_MORE_ITEMS ) break;
            {
                CAlias * ptr = new CAlias( buffer );

                if(ptr->Load())
                {
                   DPA_InsertPtr(aliasListIn, 0x7FFF, (LPVOID)ptr);
                   ptr->m_fDirty = FALSE;
                }
                else
                   delete ptr;

                index++;
            }
        }

        if(buffer) LocalFree( buffer );
        RegCloseKey( hKeyAlias );
    }

    return TRUE;
}


#ifdef DEBUG
STDAPI_(BOOL) PrintAliases( HDPA aliasListIn )
{
    printf("Listing Aliases:\n");

    if( !aliasListIn ) return FALSE;

    int aliasCount = DPA_GetPtrCount( aliasListIn );
    for(int i=0; i<aliasCount; i++ )
    {
       CAlias * ptr = (CAlias *)DPA_FastGetPtr( aliasListIn, i );
       ptr->Print();
    }

    return TRUE;
}
#endif

STDAPI_(INT) FindAliasIndex(HDPA aliasListIn , LPTSTR alias)
{
    if( ! aliasListIn ) return -1;

    int aliasCount = DPA_GetPtrCount( aliasListIn );
    for(int i = 0; i< aliasCount;i++ )
    {
        CAlias * ptr = (CAlias *)DPA_FastGetPtr( aliasListIn, i );
        if( !StrCmpI(alias, ptr->m_alias) )
            return i;
    }

    return -1;
}

STDAPI_(BOOL) FindAliasByURL(HDPA aliasListIn , LPTSTR szurl, LPTSTR aliasIn, INT cchAlias)
{
    if( ! aliasListIn ) return FALSE;

    int aliasCount = DPA_GetPtrCount( aliasListIn );
    for(int i = 0; i< aliasCount;i++ )
    {
        CAlias * ptr = (CAlias *)DPA_FastGetPtr( aliasListIn, i );
        if( !StrCmp(szurl, ptr->m_szurl) )
        {
            StrCpy( aliasIn, ptr->m_alias );
            return TRUE;
        }
    }

    return FALSE;
}

STDAPI_(BOOL) GetURLForAlias(HDPA aliasListIn, LPTSTR alias, LPTSTR szUrl, int cchUrl )
{
    int index = -1;

    if(!aliasListIn || !alias || !szUrl || cchUrl <= 0 ) return FALSE;
    
    // ENTERCRITICAL;

    if( (index = FindAliasIndex(aliasListIn, alias) ) != -1 )
    {
         CAlias * ptr = (CAlias *)DPA_FastGetPtr( aliasListIn, index );
         StrCpy( szUrl, ptr->m_szurl );
    }

Done:
    // LEAVECRITICAL;
    return (index != -1);  
}


STDAPI_(LPCTSTR) GetAliasName( CAlias * ptr )
{
    if( ptr )
        return ptr->m_alias;
    return NULL;
}

STDAPI_(LPCTSTR) GetAliasUrl( CAlias * ptr )
{
    if( ptr )
        return ptr->m_szurl;
    return NULL;
}

STDAPI_(LPVOID) CreateAlias( LPTSTR str )
{
    return new CAlias(str);
}

STDAPI_(VOID) DestroyAlias( CAlias * ptr )
{
    if( ptr ) delete ptr;
}

STDAPI_(BOOL) SetAliasInfo( CAlias * ptr, TCHAR * alias, TCHAR * url )
{
    if( ptr )
    {
        if(alias)
        {
           if( ptr->m_alias ) LocalFree( ptr->m_alias );
           ptr->m_alias = DuplicateString( alias );
           ptr->m_fDirty= TRUE;
        }
        if(url)
        {
           if( ptr->m_szurl ) LocalFree( ptr->m_szurl );
           ptr->m_szurl = DuplicateString( url );
           ptr->m_fDirty= TRUE;
        }
    } 
}



#ifdef UNICODE
STDAPI_(BOOL) FindAliasByURLA(HDPA aliasListIn , LPTSTR szurl, LPTSTR aliasIn, INT cchAlias)
{
    WCHAR szwurl[MAX_URL_STRING];
    WCHAR aliasw[MAX_ALIAS_LENGTH];

    if( !szurl || !aliasIn || !aliasListIn ) 
        return FALSE;

    SHAnsiToUnicode( aliasIn, alias
}

STDAPI_(BOOL) AddAliasToListA( HDPA aliasListIn, LPTSTR aliasIn, LPTSTR szurl, HWND hwnd )
{
    return FALSE;
}


#else

TCHAR *DuplicateString( TCHAR *orig )
{
    TCHAR * newStr;
    if( !orig ) return NULL;

    newStr  = (TCHAR *)LocalAlloc( LPTR, (lstrlen(orig) + 1)*sizeof(TCHAR));
    if(newStr) StrCpy( newStr, orig );

    return newStr;
}

TCHAR *EatSpaces( TCHAR * str )
{
    if( !str ) return NULL;

    TCHAR *ptr = str, *tmpStr = DuplicateString( str );
    TCHAR *tmpPtr = tmpStr;

    while( *tmpStr )
    {
        if(*tmpStr == TEXT(' ')  || *tmpStr == TEXT('\t') || 
           *tmpStr == TEXT('\n') || *tmpStr == TEXT('\r') || 
            // Remove special characters.
            (int)(*tmpStr) >= 127)
            tmpStr++; 
        else
            *ptr++ = *tmpStr++;
    }

    *ptr = TEXT('\0');

    LocalFree( tmpPtr );

    return str;
}

#endif /* UNICODE */

#endif /* UNIX_FEATURE_ALIAS */
