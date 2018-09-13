/****************************************************************************
 *
 *  Microsoft Confidential
 *  Copyright (c) Microsoft Corporation 1994
 *  All rights reserved
 *
 * This module handles the File Association UI
 *
 ***************************************************************************/

#include "inetcplp.h"
#include "mluisupp.h"
#include <unistd.h>

// For definition of FTA_OpenIsSafe
// the file class's open verb may be safely invoked for downloaded files
#include "../inc/filetype.h"

// Macros required for default button processing
#define REMOVE_DEF_BORDER(hDlg, cntrl )  \
    SendMessage( hDlg,  DM_SETDEFID, -1, 0 ); \
    SendDlgItemMessage( hDlg, cntrl, BM_SETSTYLE, BS_PUSHBUTTON, TRUE );  \

#define SET_DEF_BORDER(hDlg, cntrl )  \
    SendMessage( hDlg, DM_SETDEFID, cntrl, 0 );   \
    SendDlgItemMessage( hDlg, cntrl, BM_SETSTYLE, BS_DEFPUSHBUTTON, TRUE );  \

// Editing modes
#define NORMAL   0x00
#define ADDING   0x01
#define UPDATING 0x02

// Association status
#define NEW      0x01
#define UPD      0x02
#define UNM      0x03

#define LocalRealloc(a, b) LocalReAlloc(a, b, LMEM_MOVEABLE)

static TCHAR g_szDefaultIcon[]  = TEXT("shell32.dll,3");
static TCHAR g_szIEUnix[]       = TEXT("IEUNIX");
static TCHAR g_szIEUnixEntry[]   = TEXT("IEUNIX Specific entry");
static TCHAR g_szEditFlags[]    = TEXT("EditFlags");
static TCHAR g_szDocClass[]     = TEXT("DocClass");
static TCHAR g_szMimeKey[]      = TEXT("MIME\\Database\\Content Type");
static TCHAR g_szCmndSubKey[]   = TEXT("Shell\\Open\\Command");
static TCHAR g_szPolicySubKey[] = REGSTR_PATH_INETCPL_RESTRICTIONS;
static TCHAR g_szPolicyName[]   = TEXT("Mappings");

int SwitchToAddMode( HWND hDlg );
int SwitchToNrmlMode( HWND hDlg );
BOOL CALLBACK EnterAssocDlgProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);


BOOL IsAssocEnabled()
{
    HKEY  hKey = NULL;

    LONG lResult = RegOpenKey(HKEY_CURRENT_USER, g_szPolicySubKey, &hKey);

    if( lResult == ERROR_SUCCESS )
    {
	// Get then size first
	DWORD dwPolicy, dwType, dwSize = sizeof(DWORD);
	if (RegQueryValueEx( hKey, g_szPolicyName, NULL, &dwType, (LPBYTE)&dwPolicy, &dwSize ) == ERROR_SUCCESS )
	{
	    if( dwPolicy )
	    {
	        RegCloseKey( hKey );
	        return FALSE;
	    }
	}
	RegCloseKey( hKey );
    }
    return TRUE;
}

/*
** AddStringToComboBox()
**
** Adds a string to a combo box.  Does not check to see if the string has
** already been added.
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
BOOL AddStringToComboBox(HWND hwndComboBox, LPCTSTR pcsz)
{
   BOOL bResult;
   LONG lAddStringResult;

   lAddStringResult = SendMessage(hwndComboBox, CB_ADDSTRING, 0, (LPARAM)pcsz);

   bResult = (lAddStringResult != CB_ERR &&
              lAddStringResult != CB_ERRSPACE);

   return(bResult);
}


/*
** SafeAddStringToComboBox()
**
** Adds a string to a combo box.  Checks to see if the string has already been
** added.
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
BOOL SafeAddStringToComboBox(HWND hwndComboBox, LPCTSTR pcsz)
{
   BOOL bResult;

   if (SendMessage(hwndComboBox, CB_FINDSTRINGEXACT, 0, (LPARAM)pcsz) == CB_ERR)
      bResult = AddStringToComboBox(hwndComboBox, pcsz);
   else
   {
      bResult = TRUE;
   }

   return(bResult);
}

typedef struct ASSOCIATION
{
    HWND hDlg;
    int  mode;
    BOOL fInternalChange;
    BOOL fChanged;
} ASSOCINFO, * LPASSOCTABINFO;

typedef struct ENTERASSOC
{
    LPASSOCTABINFO pgti;
    TCHAR *pszAssoc;
} ENTERASSOC, * LPENTERASSOC;

#define PGTI_FROM_HDLG( hDlg ) \
    ((LPASSOCTABINFO)GetWindowLong(hDlg, DWL_USER)) \

#define PGTI_FROM_PARENT_HDLG( hDlg ) \
    ((LPASSOCTABINFO)GetWindowLong(GetParent(hDlg), DWL_USER)) \

class CMime
{
public:
    TCHAR * m_mime;
    TCHAR * m_exts;

    CMime( TCHAR * name );
   ~CMime();

    // Operations defined for Asscociation

};

CMime::CMime( TCHAR * name )
{
    m_mime = (TCHAR *) LocalAlloc( LPTR, (lstrlen(name) + 1)*sizeof(TCHAR) );
    StrCpy( m_mime, name );
    m_exts = NULL;
}

CMime::~CMime()
{
    if( m_mime ) LocalFree( m_mime );
    if( m_exts ) LocalFree( m_exts );
}


HDPA mimeList = NULL;

BOOL FreeExtensions( HDPA dpa )
{
    if( dpa == (HDPA)NULL ) return FALSE;

    int count = DPA_GetPtrCount( dpa );

    for(int i=0; i<count; i++ )
    {
        LPTSTR ptr = (LPTSTR)DPA_FastGetPtr( dpa, i );
        if(ptr) LocalFree(ptr);
    }

    DPA_Destroy( dpa );
    return TRUE;
}

class CAssoc
{
public:
    TCHAR * m_type;  // File Type class
    TCHAR * m_desc;  // File TYpe Description
    TCHAR * m_mime;  // File Type Mime
    TCHAR * m_cmnd;  // Shell/open/command
    BOOL    m_safe;  // protected or not
    DWORD   m_edit;  // EditFlags value
    UINT    m_stat;  // Status

    HDPA    m_exts;  // Dynamic array for extensions

    CAssoc( TCHAR * name );
   ~CAssoc();
    
    // Operations defined for Asscociation

    Load();
    Save(); 
    Print();
    Delete();

    // Some helper functions

    HDPA   GetExtsOfAssoc( );
    LPTSTR GetDescOfAssoc( );
    LPTSTR GetMimeOfAssoc( );
    LPTSTR GetCmndOfAssoc( );
    DWORD  GetEditOfAssoc( );

};

// Some Helper Function Prototypes

BOOL     FAR PASCAL InitAssocDialog(HWND hDlg, CAssoc * current = NULL);
void     HandleSelChange( LPASSOCTABINFO pgti , BOOL bChangeAppl = TRUE);
TCHAR *  EatSpaces( TCHAR * str );
TCHAR *  ChopSpaces( TCHAR * str );
TCHAR *  DuplicateString( TCHAR * str );
CAssoc * GetCurrentAssoc( HWND hDlg );

// Member function definitions for CAssoc.

CAssoc::CAssoc( TCHAR * name )
{
    m_type = (TCHAR *) LocalAlloc( LPTR, (lstrlen(name) + 1)*sizeof(TCHAR) );
    StrCpy( m_type, name );
    m_desc = NULL;
    m_mime = NULL;
    m_cmnd = NULL;
    m_stat = NEW ;
    m_safe = TRUE; // Internal Assoc, Dont mess with this
    m_exts = NULL;
    m_edit = 0;
}

CAssoc::~CAssoc()
{
    if( m_type ) LocalFree( m_type );
    if( m_desc ) LocalFree( m_desc );
    if( m_mime ) LocalFree( m_mime );
    if( m_cmnd ) LocalFree( m_cmnd );

    if( m_exts ) FreeExtensions( m_exts );
}

CAssoc::Load()
{
    if(m_type)
    {
       TCHAR * ptr = NULL;

       m_exts = GetExtsOfAssoc();
       m_edit = GetEditOfAssoc();

       if ((ptr = GetDescOfAssoc( )) != NULL)
       { 
          m_desc = (TCHAR *)LocalAlloc( LPTR, (lstrlen(ptr) + 1)*sizeof(TCHAR));
          StrCpy( m_desc, ptr );
          ptr = NULL;
       }
       else 
           // Each type must have a description. (Required)
           return FALSE;

       if ((ptr = GetMimeOfAssoc()) != NULL)
       {
          m_mime = (TCHAR *)LocalAlloc( LPTR, (lstrlen(ptr) + 1)*sizeof(TCHAR));
          StrCpy( m_mime, ptr );
          ptr = NULL;
       }

       if ((ptr = GetCmndOfAssoc()) != NULL)
       {
          m_cmnd = (TCHAR *)LocalAlloc( LPTR, (lstrlen(ptr) + 1)*sizeof(TCHAR));
          StrCpy( m_cmnd, ptr );
          ptr = NULL;
       }

       m_stat = UNM;
       m_safe = FALSE;
    }

    return TRUE;
}

CAssoc::Save()
{

    if( m_safe ) return TRUE;

    if( m_stat != UPD ) return TRUE;

    // Create a Key for DocType in HKEY_CLASSES_ROOT
    // [doctype
    //   (--reg-val-- "Description")
    //   [defaulticon
    //      (reg-val "shell32.dll,3")
    //   ]
    //   [shell
    //     [open
    //       [command
    //         (--reg-val-- "Command" )
    //       ]
    //     ]
    //   ]
    // ]

    HKEY hKey1, hKey2, hKey3, hKey4;

    LONG lResult = RegOpenKeyEx(
            HKEY_CLASSES_ROOT,
            m_type,
            0,
            KEY_QUERY_VALUE|KEY_WRITE,
            &hKey1);

    if (lResult != ERROR_SUCCESS)
    {
        lResult = RegCreateKey(
        HKEY_CLASSES_ROOT,
        m_type,
        &hKey1);
    }

    if (lResult == ERROR_SUCCESS)
    {
        DWORD dwType = REG_SZ;
        DWORD dwLen  = (lstrlen(m_desc)+1)*sizeof(TCHAR);
        RegSetValue( hKey1, NULL, dwType, m_desc, dwLen );

        // Add IEUNIX tag to this entry
        dwLen  = (lstrlen(g_szIEUnixEntry)+1)*sizeof(TCHAR);
        RegSetValueEx( hKey1, g_szIEUnix, 0,
            dwType, (LPBYTE)g_szIEUnixEntry, dwLen );
        
        // Add Edit flags to this entry
        DWORD value = m_edit;
        RegSetValueEx( hKey1, g_szEditFlags, 0,
            REG_DWORD, (LPBYTE)(&value), sizeof(DWORD) ); 

        HKEY hKey2;

        RegDeleteKey( hKey1, TEXT("defaulticon" ) );
       
        lResult = RegCreateKey(
            hKey1,
            TEXT("defaulticon"),
            &hKey2);

        if(lResult == ERROR_SUCCESS)
        {
            DWORD dwType = REG_SZ;
            DWORD dwLen  = (lstrlen(g_szDefaultIcon)+1)*sizeof(TCHAR);
            RegSetValue( hKey2, NULL, dwType, g_szDefaultIcon, dwLen );
            RegCloseKey( hKey2);
        }

        RegDeleteKey( hKey1, g_szCmndSubKey );

        lResult = RegOpenKeyEx(
                hKey1,
                TEXT("shell"),
                0,
                KEY_QUERY_VALUE| KEY_WRITE,
                &hKey2);
       
        if(lResult != ERROR_SUCCESS)
        {
            lResult = RegCreateKey(
                hKey1,
                TEXT("shell"),
                &hKey2);      
        }

        if(lResult == ERROR_SUCCESS)
        {
            lResult = RegOpenKeyEx(
                hKey2,
                TEXT("open"),
                0,
                KEY_QUERY_VALUE| KEY_WRITE,
                &hKey3);

            if( lResult != ERROR_SUCCESS )
            {
                lResult = RegCreateKey(
                    hKey2,
                    TEXT("open"),
                    &hKey3);      
            }

            if( lResult == ERROR_SUCCESS )
            {
                lResult = RegOpenKeyEx(
                    hKey3,
                    TEXT("command"),
                    0,
                    KEY_QUERY_VALUE| KEY_WRITE,
                    &hKey4);

                if( lResult != ERROR_SUCCESS )
                {
                    lResult = RegCreateKey(
                        hKey3,
                        TEXT("command"),
                        &hKey4);      
                }

                if( lResult == ERROR_SUCCESS )
                {
                    DWORD dwType = REG_SZ;
                    DWORD dwLen  = (lstrlen(m_cmnd)+1)*sizeof(TCHAR);

                    RegSetValue( hKey4, NULL, dwType, m_cmnd, dwLen );
                    RegCloseKey( hKey4);
                }
               
                RegCloseKey(hKey3);
            }

            RegCloseKey(hKey2);
        }   

        RegCloseKey(hKey1);
    }    
  
    // Add mime type to the mimetype data base if it doesn't exist
    LPTSTR mimeKey = (LPTSTR)LocalAlloc( LPTR, (lstrlen(m_mime)+lstrlen(g_szMimeKey) + 3)*sizeof(TCHAR));
    
    if(mimeKey && m_mime)
    {
        StrCpy( mimeKey, g_szMimeKey );
        StrCat( mimeKey, TEXT("\\")  );
        StrCat( mimeKey, m_mime      );
        
        lResult = RegOpenKeyEx(
                HKEY_CLASSES_ROOT,
                mimeKey,
                0,
                KEY_QUERY_VALUE| KEY_WRITE,
                &hKey1);
    
        if(lResult != ERROR_SUCCESS)
        {
            lResult = RegCreateKey(
                HKEY_CLASSES_ROOT,
                mimeKey,
                &hKey1);

            if(lResult == ERROR_SUCCESS && m_exts)
            {
                int count    = DPA_GetPtrCount( m_exts );
                if(count > 0 )
                {
                    LPTSTR firstExt = (LPTSTR)DPA_FastGetPtr( m_exts, 0 );
                    RegSetValueEx( hKey1, TEXT("extension"), NULL,
                        REG_SZ, (LPBYTE)firstExt, (lstrlen(firstExt)+1)*sizeof(TCHAR) );
                }

                RegCloseKey( hKey1 );
            }
        }
        else
        {
            RegCloseKey( hKey1 );
        }

    }

    if(mimeKey) 
         LocalFree(mimeKey);

    // Add extention/document type association
    // [.ext
    //   (--reg-val-- "Application")
    //   (content.type "mimetype"  )
    // ]

    // First remove all the extensions for the current assoc from the
    // registry.

    HDPA prevExts = GetExtsOfAssoc();
    if( prevExts )
    {
        int extCount = DPA_GetPtrCount( prevExts );
        for( int i=0; i< extCount; i++ )
           RegDeleteKey( HKEY_CLASSES_ROOT, (LPTSTR)DPA_FastGetPtr( prevExts, i ) ); 

        FreeExtensions( prevExts );
    }


    if( m_exts )
    {
        int count = DPA_GetPtrCount( m_exts );

        for( int i=0; i<count; i++ )
        { 
            LPTSTR ptr = (LPTSTR)DPA_FastGetPtr( m_exts, i );
            if( ptr && *ptr == TEXT('.') )
            {
                lResult = RegOpenKeyEx(
                    HKEY_CLASSES_ROOT,
                    ptr,
                    0,
                    KEY_QUERY_VALUE| KEY_WRITE,
                    &hKey1);

                if( lResult != ERROR_SUCCESS )
                {
                    lResult = RegCreateKey(
                        HKEY_CLASSES_ROOT,
                        ptr, 
                        &hKey1);      
                }

                if( lResult == ERROR_SUCCESS )
                {
                    DWORD dwType = REG_SZ;
                    DWORD dwLen  = (lstrlen(m_type)+1)*sizeof(TCHAR);
                    RegSetValue( hKey1, NULL, dwType, m_type, dwLen );
                       
                    EatSpaces(m_mime ); 
                    dwLen  = lstrlen(m_mime);
                    if(m_mime && (dwLen=lstrlen(m_mime))>0)
                        RegSetValueEx( hKey1, TEXT("Content Type"), 0,
                            dwType, (LPBYTE)m_mime, (dwLen+1)*sizeof(TCHAR) );

                    // Add IEUNIX tag to this entry
                    dwLen  = (lstrlen(g_szIEUnixEntry)+1)*sizeof(TCHAR);
                    RegSetValueEx( hKey1, g_szIEUnix, 0,
                        dwType, (LPBYTE)g_szIEUnixEntry, dwLen );

                    RegCloseKey( hKey1);
                    hKey1 = NULL;
                }
            }
        }
    } 
    else
        return FALSE; 
    
    return TRUE;
}


CAssoc::Delete()
{
    HKEY hKey;
    
    // Don't touch the safe keys
    if( m_safe ) return FALSE;

    // Delete Application from HKEY_CLASSES_ROOT
    EatSpaces(m_type);
    if(m_type && *m_type)
    {
        // NT restrictions
        TCHAR * key = (TCHAR *)LocalAlloc(LPTR, (lstrlen(m_type) + 200)*sizeof(TCHAR) ) ;

        if(!key) return FALSE;
        
        StrCpy( key, m_type );        
        StrCat( key, TEXT("\\defaulticon") );        
        RegDeleteKey(HKEY_CLASSES_ROOT, key);

        StrCpy( key, m_type );        
        StrCat( key, TEXT("\\") );        
        StrCat( key, g_szCmndSubKey );
        RegDeleteKey(HKEY_CLASSES_ROOT, key);

        StrCpy( key, m_type );        
        StrCat( key, TEXT("\\shell\\open") );        
        RegDeleteKey(HKEY_CLASSES_ROOT, key);

        StrCpy( key, m_type );        
        StrCat( key, TEXT("\\shell") );        
        RegDeleteKey(HKEY_CLASSES_ROOT, key);

        RegDeleteKey(HKEY_CLASSES_ROOT, m_type);

        LocalFree( key );
    }
    else
        return FALSE; 

    // Delete Extensions  from HKEY_CLASSES_ROOT
    if( m_exts )
    {
        int count = DPA_GetPtrCount( m_exts );

        for( int i=0; i<count; i++ )
        { 
            LPTSTR ptr = (LPTSTR)DPA_FastGetPtr( m_exts, i );
            if( ptr && *ptr == TEXT('.') )
            {
                RegDeleteKey(HKEY_CLASSES_ROOT, ptr);
            }
        }
    } 
    else
        return FALSE; 

    return TRUE;
}

CAssoc::Print()
{
#ifndef UNICODE
    if( m_type ) printf( m_type );
    printf(",");
    if( m_desc ) printf( m_desc );
    printf(",");
    if( m_mime ) printf( m_mime );
    printf(",");
    if( m_cmnd ) printf( m_cmnd );

    if( m_exts )
    {
        int count = DPA_GetPtrCount( m_exts );
        for( int i=0; i<count; i++ )
        { 
            LPTSTR ptr = (LPTSTR)DPA_FastGetPtr( m_exts, i );
            if( ptr && *ptr == TEXT('.') )
            {
                printf("%s;", ptr );
            }
        }
    }

    printf("\n");
#endif
    return TRUE;
}

HDPA CAssoc::GetExtsOfAssoc()
{
    TCHAR buffer[MAX_PATH];

    DWORD index  = 0;
    DWORD type   = REG_SZ;
    long  dwLen  = MAX_PATH;
    DWORD dwLen2 = MAX_PATH;

    HDPA  exts = DPA_Create(4);
    
    *buffer = TEXT('\0');

    if( !m_type ) return NULL;

    TCHAR * key   = (TCHAR *)LocalAlloc( LPTR, (MAX_PATH+1)*sizeof(TCHAR) );
    TCHAR * value = (TCHAR *)LocalAlloc( LPTR, (MAX_PATH+1)*sizeof(TCHAR) );

    if( !key || !value  ) goto EXIT;

    while( RegEnumKey( HKEY_CLASSES_ROOT, index, key, MAX_PATH ) != ERROR_NO_MORE_ITEMS ) 
    {    
        if( *key == TEXT('.') )
        {
            HKEY  hKey = NULL;

            LONG lResult = RegOpenKeyEx(
                HKEY_CLASSES_ROOT,
                key,
                0,
                KEY_QUERY_VALUE,
                &hKey);

            if( lResult == ERROR_SUCCESS )
            {
                // Get then size first
	        dwLen = (MAX_PATH+1)*sizeof(TCHAR);
		if (RegQueryValue( hKey, NULL, value,  &dwLen )
		    == ERROR_SUCCESS )
		{
		    if( !StrCmpIC( value, m_type ) )
		    {
			DPA_InsertPtr( exts, 0x7FFF, (LPVOID)DuplicateString(key) );
		    }
		}

                RegCloseKey( hKey );
            }
        }
        index++;
    }

EXIT:
    if( key )
        LocalFree( key );
    if( value)
        LocalFree( value );

    return exts;
}


LPTSTR CAssoc::GetDescOfAssoc()
{
    HKEY hKey;

    static TCHAR buffer[MAX_PATH];
    DWORD type  = REG_SZ;
    DWORD dwLen = sizeof(buffer);

    *buffer = TEXT('\0');

    if( !m_type ) return NULL;

    LONG lResult = RegOpenKeyEx(
        HKEY_CLASSES_ROOT,
        m_type,
        0,
        KEY_QUERY_VALUE | KEY_READ,
        &hKey);

    if (lResult == ERROR_SUCCESS)
    {
        lResult = RegQueryValueEx(
           hKey,
           NULL,
           NULL,
           (LPDWORD) &type,
           (LPBYTE)  buffer,
           (LPDWORD) &dwLen);

        RegCloseKey( hKey );

        if( lResult == ERROR_SUCCESS ) 
        {
            return buffer;
        }
        
    }
    return NULL;
}

DWORD CAssoc::GetEditOfAssoc()
{
    HKEY hKey;

    DWORD buffer = 0;
    DWORD type  = REG_DWORD;
    DWORD dwLen = sizeof(buffer);

    if( !m_type ) return NULL;

    LONG lResult = RegOpenKeyEx(
        HKEY_CLASSES_ROOT,
        m_type,
        0,
        KEY_QUERY_VALUE | KEY_READ,
        &hKey);

    if (lResult == ERROR_SUCCESS)
    {
        lResult = RegQueryValueEx(
           hKey,
           g_szEditFlags, 
           NULL,
           (LPDWORD) &type,
           (LPBYTE)  &buffer,
           (LPDWORD) &dwLen);

        RegCloseKey( hKey );

        if( lResult == ERROR_SUCCESS )
        {
            return buffer;
        }

    }

    return 0;
}

LPTSTR CAssoc::GetMimeOfAssoc()
{
    HKEY hKey;

    static TCHAR buffer[MAX_PATH];
    DWORD type  = REG_SZ;
    DWORD dwLen = sizeof(buffer);

    *buffer = TEXT('\0');

    if( !m_type || !m_exts ) return NULL;

    int count = DPA_GetPtrCount( m_exts );

    for( int i=0; i< count; i++ )
    {
        LPTSTR str = (LPTSTR)DPA_FastGetPtr( m_exts, i );

        if( !str ) continue;

        LONG lResult = RegOpenKeyEx(
            HKEY_CLASSES_ROOT,
            str,
            0,
            KEY_QUERY_VALUE | KEY_READ,
            &hKey);

        if (lResult == ERROR_SUCCESS)
        {
            lResult = RegQueryValueEx(
               hKey,
               TEXT("Content Type"),
               NULL,
               (LPDWORD) &type,
               (LPBYTE)  buffer,
               (LPDWORD) &dwLen);

            RegCloseKey( hKey );

            if( lResult == ERROR_SUCCESS ) 
            {
                return buffer;
            }
        }
    }

    return NULL;
}

LPTSTR CAssoc::GetCmndOfAssoc()
{
    HKEY hKey;

    static TCHAR buffer [MAX_PATH];
    TCHAR keyName[MAX_PATH+40];
    DWORD type  = REG_SZ;
    DWORD dwLen = sizeof(buffer);

    *buffer = TEXT('\0');

    if( !m_type ) return NULL;

    StrCpy( keyName, m_type );
    StrCat( keyName, TEXT("\\") );
    StrCat( keyName, g_szCmndSubKey );

    LONG lResult = RegOpenKeyEx(
        HKEY_CLASSES_ROOT,
        keyName,
        0,
        KEY_QUERY_VALUE | KEY_READ,
        &hKey);

    if (lResult == ERROR_SUCCESS)
    {
        lResult = RegQueryValueEx(
           hKey,
           NULL,
           NULL,
           (LPDWORD) &type,
           (LPBYTE)  buffer,
           (LPDWORD) &dwLen);

        RegCloseKey( hKey );

        if( lResult == ERROR_SUCCESS ) 
        {
            return buffer;
        }
    }
    return NULL;
}

HDPA assocList    = (HDPA)NULL;
HDPA assocDelList = (HDPA)NULL;

SetEditLimits(HWND hDlg )
{
    SendMessage(GetDlgItem( hDlg, IDC_DOC_TYPE ), EM_LIMITTEXT, (WPARAM) MAX_PATH-1, 0);
    SendMessage(GetDlgItem( hDlg, IDC_DOC_MIME ), EM_LIMITTEXT, (WPARAM) MAX_PATH-1, 0);
    SendMessage(GetDlgItem( hDlg, IDC_DOC_EXTS ), EM_LIMITTEXT, (WPARAM) MAX_PATH-1, 0);
    SendMessage(GetDlgItem( hDlg, IDC_DOC_DESC ), EM_LIMITTEXT, (WPARAM) MAX_PATH-1, 0);
    SendMessage(GetDlgItem( hDlg, IDC_DOC_CMND ), EM_LIMITTEXT, (WPARAM) MAX_PATH-1, 0);

    return TRUE;
}

TCHAR *DuplicateString( TCHAR *orig )
{
    TCHAR * newStr;
    if( !orig ) return NULL;

    newStr  = (TCHAR *)LocalAlloc( LPTR, (lstrlen(orig) + 1)*sizeof(TCHAR));
    if(newStr) StrCpy( newStr, orig );

    return newStr;
}

TCHAR * EatSpaces( TCHAR * str )
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

TCHAR * ChopSpaces( TCHAR * str )
{
    if( !str ) return NULL;

    TCHAR *ptr = str;

    while( *ptr && 
         (*ptr == TEXT(' ')  || *ptr == TEXT('\t')) ||
         (*ptr == TEXT('\n') || *ptr == TEXT('\r')) 
         ) ptr++;

    TCHAR *tmpStr = DuplicateString( ptr );
    TCHAR *tmpPtr = tmpStr + lstrlen(tmpStr);

    tmpPtr--;

    while( tmpPtr>= tmpStr && 
         (*tmpPtr == TEXT(' ')  || *tmpPtr == TEXT('\t')) ||
         (*tmpPtr == TEXT('\n') || *tmpPtr == TEXT('\r'))
         ) tmpPtr--;

    tmpPtr++;

    *tmpPtr = TEXT('\0');

    StrCpy( str, tmpStr );
    
    LocalFree( tmpStr );

    return str;
}


BOOL FreeAssociations( )
{
    if( assocList == (HDPA)NULL ) return FALSE;

    int assocCount = DPA_GetPtrCount( assocList );

    for(int i=0; i<assocCount; i++ )
    {
        CAssoc * ptr = (CAssoc *)DPA_FastGetPtr( assocList, i );
        if(ptr) delete ptr;
    }

    DPA_Destroy( assocList );
    assocList = (HDPA)NULL;
    return TRUE;
}

BOOL LoadAssociations( )
{
    HKEY hKey;
    int index = 0;
    DWORD dwLen  = MAX_PATH;

    if(assocList) 
        FreeAssociations();

    if((assocList = DPA_Create(4)) == (HDPA)NULL ) 
        return FALSE;

    TCHAR * buffer = (TCHAR *)LocalAlloc( LPTR, (MAX_PATH+1)*sizeof(TCHAR) );
    TCHAR * tmpKey = (TCHAR *)LocalAlloc( LPTR, (MAX_PATH+MAX_PATH+2)*sizeof(TCHAR) );

    while( buffer && tmpKey )
    {
        dwLen  = (MAX_PATH+sizeof(g_szCmndSubKey)+2)*sizeof(TCHAR); 
        if( RegEnumKeyEx( HKEY_CLASSES_ROOT, index, buffer, &dwLen,
                              NULL, NULL, NULL, NULL )
                == ERROR_NO_MORE_ITEMS ) break;
        {
            // Skip Extensions and *.
            if( *buffer == TEXT('.') || *buffer == TEXT('*')) 
            {
                index++;
                continue;
            }

            CAssoc * ptr = NULL;

            StrCpy( tmpKey, buffer );
            StrCat( tmpKey, TEXT("\\"));
            StrCat( tmpKey, g_szCmndSubKey);

            LONG lResult = RegOpenKeyEx(
                    HKEY_CLASSES_ROOT,
                    tmpKey,
                    0,       
                    KEY_QUERY_VALUE | KEY_READ,
                    &hKey);
            if( lResult == ERROR_SUCCESS )
            {
                ptr=new CAssoc(buffer);
                if( ptr->Load() == TRUE ) 
                    DPA_InsertPtr( assocList, 0x7FFF, (LPVOID)ptr);
                else 
                {
                    delete ptr;
                    ptr = NULL;
                }
                RegCloseKey( hKey ); 
            }
   
            // Check if this association needs to be protected.
            //  - uses DDE
            //  - has clsid
            //  - has protected key.
            if(ptr)
            {
                StrCpy(tmpKey, buffer);
                StrCat(tmpKey, TEXT("\\shell\\open\\ddeexec") );
                // wnsprintf(tmpKey, ARRAYSIZE(tmpKey), TEXT("%s\\shell\\open\\ddeexec"), buffer);

                lResult = RegOpenKeyEx(
                        HKEY_CLASSES_ROOT,
                        tmpKey,
                        0,
                        KEY_QUERY_VALUE | KEY_READ,
                        &hKey);
                if( lResult == ERROR_SUCCESS )
                {
                    ptr->m_safe = TRUE;
                    RegCloseKey( hKey );
                    goto Cont;
                }

                StrCpy(tmpKey, buffer);
                StrCat(tmpKey, TEXT("\\clsid") );
                //wnsprintf(tmpKey, ARRAYSIZE(tmpKey), TEXT("%s\\clsid"), buffer);

                lResult = RegOpenKeyEx(
                        HKEY_CLASSES_ROOT,
                        tmpKey,
                        0,
                        KEY_QUERY_VALUE | KEY_READ,
                        &hKey);
                if( lResult == ERROR_SUCCESS )
                {
                    ptr->m_safe = TRUE;
                    RegCloseKey( hKey );
                    goto Cont;
                }

                StrCpy(tmpKey, buffer);
                StrCat(tmpKey, TEXT("\\protected") );
                // wnsprintf(tmpKey, ARRAYSIZE(tmpKey), TEXT("%s\\protected"), buffer);

                lResult = RegOpenKeyEx(
                        HKEY_CLASSES_ROOT,
                        tmpKey,
                        0,
                        KEY_QUERY_VALUE | KEY_READ,
                        &hKey);
                if( lResult == ERROR_SUCCESS )
                {
                    ptr->m_safe = TRUE;
                    RegCloseKey( hKey );
                }
            }

Cont:

            index++;
        }
    }

    if( tmpKey ) LocalFree( tmpKey );
    if( buffer ) LocalFree( buffer );

    return TRUE;
}


BOOL FreeMimeTypes( )
{
    if( mimeList == NULL ) return FALSE;

    int mimeCount = DPA_GetPtrCount( mimeList );

    for(int i=0; i<mimeCount; i++ )
    {
        CMime * ptr = (CMime *)DPA_FastGetPtr( mimeList, i );
        if(ptr) delete ptr;
    }
    
    DPA_Destroy( mimeList );
    mimeList = (HDPA)NULL;
    return TRUE;
}


BOOL LoadMimeTypes( )
{
    HKEY hKeyMime, hKey;
    int index = 0;
    DWORD dwLen  = MAX_PATH;

    if(mimeList) FreeMimeTypes();

    if((mimeList = DPA_Create(4)) == (HDPA)NULL) return FALSE;

    // TODO : Get Max length of the key from registry and use it
    // instead of MAX_PATH.
    TCHAR * buffer = (TCHAR *)LocalAlloc( LPTR, (MAX_PATH+1)*sizeof(TCHAR) );

    LONG lResult = RegOpenKeyEx(
                HKEY_CLASSES_ROOT,
                g_szMimeKey,
                0,
                KEY_QUERY_VALUE | KEY_READ,
                &hKeyMime);
    
    if( lResult == ERROR_SUCCESS )
    {

    while( buffer )
    {
        dwLen  = MAXPATH;
        if( RegEnumKeyEx( hKeyMime, index, buffer, &dwLen,
                              NULL, NULL, NULL, NULL )
                == ERROR_NO_MORE_ITEMS ) break;
        {
            CMime * ptr = new CMime( buffer );

            lResult = RegOpenKeyEx(
                    hKeyMime,
                    buffer,
                    0,
                    KEY_QUERY_VALUE | KEY_READ,
                    &hKey);
            if( lResult == ERROR_SUCCESS )
            {
                dwLen = MAX_PATH;
                if (RegQueryValue( hKey, TEXT("extension"), buffer,  (long *)&dwLen ) 
                    == ERROR_SUCCESS )
                {
                    ptr->m_exts = (TCHAR *)LocalAlloc( LPTR, (dwLen+1)*sizeof(TCHAR));
                    StrCpy(ptr->m_exts, buffer);
                }
                
                RegCloseKey( hKey );
            }

            DPA_InsertPtr(mimeList, 0x7FFF, (LPVOID)ptr);

            index++;
        }
    }

    RegCloseKey( hKeyMime );

    }

    if( buffer ) LocalFree( buffer );

    return TRUE;
}


BOOL PrintAssociations()
{
    printf("Listing Associations:\n");

    if( !assocList ) return FALSE;

    int assocCount = DPA_GetPtrCount( assocList );
    for(int i=0; i<assocCount; i++ )
    {
       CAssoc * ptr = (CAssoc *)DPA_FastGetPtr( assocList, i );
       ptr->Print();
    }

    return TRUE;
}

BOOL FindCommand(HWND hDlg)
{
    TCHAR szFile[MAX_PATH] = TEXT("");
    TCHAR szFilter[5];
    OPENFILENAME ofn;

    HWND hCmnd = GetDlgItem(hDlg, IDC_DOC_CMND );

    memset((void*)&szFilter, 0, 5*sizeof(TCHAR));
    szFilter[0] = TCHAR('*');
    szFilter[2] = TCHAR('*');

    memset((void*)&ofn, 0, sizeof(ofn));
    ofn.lpstrFilter = szFilter;
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hDlg;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_HIDEREADONLY|OFN_CREATEPROMPT;

    if (GetOpenFileName(&ofn))
    {
        SendMessage(hCmnd, EM_SETSEL, 0, -1);
        SendMessage(hCmnd, EM_REPLACESEL, 0, (LPARAM)szFile);
    }

    return TRUE;
}

int FindIndex(LPTSTR doc)
{
    if( ! assocList ) return -1;

    int assocCount = DPA_GetPtrCount( assocList );
    for(int i = 0; i< assocCount;i++ )
    {
        CAssoc * ptr = (CAssoc *)DPA_FastGetPtr( assocList, i );
        if( !StrCmp(doc, ptr->m_type ) )
            return i;
    }

    return -1;
}

HDPA CreateDPAForExts( TCHAR * strPassed, int * error, CAssoc * existing)
{
    HKEY hKey;
    int  bFound = 0;
    HDPA hdpaExts = NULL;

    // Check for existing associations not created by IEUNIX
    // [.ext
    //   (--reg-val--  "Application")
    //   (content.type "mimetype"   )
    //   (g_szIEUnix   "g_szIEUnixTag" )
    // ]

    EatSpaces(strPassed);
    if(strPassed && *strPassed)
    {
        TCHAR *strExt = DuplicateString( strPassed );
        TCHAR *pos, *ptr = strExt;
        BOOL  bCont = TRUE;
         
        while(bCont && *ptr)
        {   
            pos = StrChr(ptr, TEXT(';'));

            if(pos)
            {
                bCont = ( *pos == TEXT(';') )? TRUE : FALSE;
                *pos = TEXT('\0');
            }
            else
            {
               bCont = FALSE;
            }
             
            if( !hdpaExts ) hdpaExts = DPA_Create(4);
            DPA_InsertPtr( hdpaExts, 0x7FFF, (LPVOID)DuplicateString( ptr ) );

            if(*ptr == TEXT('.') && *(ptr+1) )
            {
                int  assocCount, extCount;
                assocCount = DPA_GetPtrCount( assocList );
                for( int i = 0; i< assocCount; i++ )
                {
                    CAssoc * pAssoc = (CAssoc*)DPA_FastGetPtr( assocList, i );
                    
                    if( pAssoc->m_exts == NULL || pAssoc == existing ) continue;

                    extCount = DPA_GetPtrCount(pAssoc->m_exts) ;
                    for(int j=0;j<extCount;j++ )
                    {
                       if( !StrCmpI(ptr, (LPTSTR)DPA_FastGetPtr( pAssoc->m_exts, j ) ) )
                       {
                          bFound = IDS_ERROR_EXTS_ALREADY_EXISTS;
                          break;
                       }
                    }
                   
                    if( bFound ) break;
                }
            }
            else
            {
                // Extension must start with a '.'
                bFound = IDS_ERROR_NOT_AN_EXT;
                break;
            }
            ptr = pos+1;
        }

        if(strExt) LocalFree(strExt);
    } 
    else
        bFound = IDS_ERROR_MISSING_EXTS;

    *error = bFound;

    // Error occured while checking extensions
    if( bFound )
    {
        if(hdpaExts) FreeExtensions( hdpaExts );
        return NULL;
    }

    return hdpaExts;
}

// Following functions are called in response to the 
// actions performed on the associations.

AssocDel( HWND hDlg )
{
    int index    = 0;
    int lbindex  = 0;
    HWND lb      = GetDlgItem( hDlg, IDC_DOC_LIST );

    if( (lbindex = SendMessage( lb, LB_GETCURSEL, 0, 0 ) )!= LB_ERR )
    {
        LPTSTR str = (LPTSTR)SendMessage(lb, LB_GETITEMDATA, lbindex, 0 );
        if(str)
        {
            if( (index = FindIndex( str ) ) != -1 )
            {
                 CAssoc * ptr = (CAssoc *)DPA_FastGetPtr( assocList, index );
                 TCHAR question[MAX_PATH];
                 wnsprintf( question, ARRAYSIZE(question), TEXT("Are you Sure you want to delete '%s'?"), ptr->m_desc );
                 if( MessageBox( GetParent(hDlg), question, TEXT("Delete Association"), MB_YESNO ) == IDYES )
                 {
                     CAssoc *pAssoc = (CAssoc *)DPA_DeletePtr( assocList, index );

                    // Add to List of deleted entries
                    if( assocDelList == NULL ) assocDelList = DPA_Create(4);
                    if( assocDelList != NULL ) DPA_InsertPtr( assocDelList, 0x7FFF, pAssoc );

                    SendMessage( lb, LB_DELETESTRING, lbindex, 0 );
                    InitAssocDialog( hDlg ); 
                    SwitchToNrmlMode( hDlg );
                    PropSheet_Changed(GetParent(hDlg),hDlg);
                    LocalFree( str );
                } 
            }
        }
    }

    return TRUE;
}

#ifdef 0
AssocUpd( HWND hDlg )
{
    BOOL bFound = FALSE;
    TCHAR szTemp [1024];
    TCHAR szTitle [80];
    CAssoc *ptr = NULL;
    TCHAR *str;
    int    index, len;
    HDPA   hdpaExts = NULL;

    HWND lb   = GetDlgItem( hDlg, IDC_DOC_LIST );
    HWND exts = GetDlgItem( hDlg, IDC_DOC_EXTS );
    HWND desc = GetDlgItem( hDlg, IDC_DOC_DESC );
    HWND mime = GetDlgItem( hDlg, IDC_DOC_MIME );
    HWND cmnd = GetDlgItem( hDlg, IDC_DOC_CMND );

    MLLoadString(IDS_ERROR_REGISTRY_TITLE, szTitle, sizeof(szTitle));

    // Get the pointer to existing associations.
    if(PGTI_FROM_HDLG(hDlg)->mode == UPDATING)
    {
         ptr = GetCurrentAssoc( hDlg );
    }

    //Check for Description
    len = SendMessage( GetDlgItem(hDlg, IDC_DOC_DESC), WM_GETTEXTLENGTH, 0, 0 );
    if( len > 0 )
    {
        str = (TCHAR *)LocalAlloc( LPTR, (len+1)*sizeof(TCHAR) );
        SendMessage(GetDlgItem(hDlg, IDC_DOC_DESC), WM_GETTEXT, len+1, (LPARAM)str );
        ChopSpaces( str );
        if( *str == TEXT('\0') )
        {    
            LocalFree(str);
            MLLoadString(IDS_ERROR_MISSING_DESC, szTemp, sizeof(szTemp));
            MessageBox(GetParent(hDlg), szTemp, szTitle, MB_OK);
            SetFocus( GetDlgItem(hDlg, IDC_DOC_DESC) );
            return FALSE;
        }

        int assocCount = DPA_GetPtrCount( assocList );
        for( int i= 0; i<assocCount; i++ )
        {
            CAssoc * pAssoc ;
            if((pAssoc= (CAssoc *)DPA_FastGetPtr( assocList, i )) == ptr ) continue;
            
            if( StrCmpI( str, pAssoc->m_cmnd ) == NULL )
            {
                LocalFree(str);
                MLLoadString(IDS_ERROR_DESC_ALREADY_EXISTS, szTemp, sizeof(szTemp));
                MessageBox(GetParent(hDlg), szTemp, szTitle, MB_OK);
                SetFocus( GetDlgItem(hDlg, IDC_DOC_DESC) );
                return FALSE;
            }
        }

        LocalFree(str);
    }
    else
    {
        MLLoadString(IDS_ERROR_MISSING_DESC, szTemp, sizeof(szTemp));
        MessageBox(GetParent(hDlg), szTemp, szTitle, MB_OK);
        SetFocus( GetDlgItem(hDlg, IDC_DOC_DESC) );
        return FALSE;
    }

    //Check for MIME
    len = SendMessage( GetDlgItem(hDlg, IDC_DOC_MIME), WM_GETTEXTLENGTH, 0, 0 );
    if( len > 0 )
    {
        str = (TCHAR *)LocalAlloc( LPTR, (len+1)*sizeof(TCHAR) );
        SendMessage(GetDlgItem(hDlg, IDC_DOC_MIME), WM_GETTEXT, len+1, (LPARAM)str );
        ChopSpaces( str );
        if( *str != TEXT('\0') )
        {
            if( !StrChr( str, TEXT('/') ) )
            {
                LocalFree(str);
                MLLoadString(IDS_ERROR_INVALID_MIME, szTemp, sizeof(szTemp));
                MessageBox(GetParent(hDlg), szTemp, szTitle, MB_OK);
                SetFocus( GetDlgItem(hDlg, IDC_DOC_MIME) );
                return FALSE;
            }
        }
        LocalFree(str);
    }
    

    //Check for command line
    len = SendMessage( GetDlgItem(hDlg, IDC_DOC_CMND), WM_GETTEXTLENGTH, 0, 0 );
    if( len > 0 )
    {
        str = (TCHAR *)LocalAlloc( LPTR, (len+1)*sizeof(TCHAR) );
        SendMessage(GetDlgItem(hDlg, IDC_DOC_CMND), WM_GETTEXT, len+1, (LPARAM)str );
        ChopSpaces( str );
        if( *str == TEXT('\0') )
        {
            LocalFree(str);
            MLLoadString(IDS_ERROR_MISSING_CMND, szTemp, sizeof(szTemp));
            MessageBox(GetParent(hDlg), szTemp, szTitle, MB_OK);
            SetFocus( GetDlgItem(hDlg, IDC_DOC_CMND) );
            return FALSE;
        }    

        LocalFree(str);
    }
    else
    {
        MLLoadString(IDS_ERROR_MISSING_CMND, szTemp, sizeof(szTemp));
        MessageBox(GetParent(hDlg), szTemp, szTitle, MB_OK);
        SetFocus( GetDlgItem(hDlg, IDC_DOC_CMND) );
        return FALSE;
    }

    // Check for extensions.
    // User entered may already have associations in the
    // registry.
    len = SendMessage( GetDlgItem(hDlg, IDC_DOC_EXTS), WM_GETTEXTLENGTH, 0, 0 );
    str = (TCHAR *)LocalAlloc( LPTR, (len+1)*sizeof(TCHAR) );
    SendMessage(GetDlgItem(hDlg, IDC_DOC_EXTS), WM_GETTEXT, len+1, (LPARAM)str );

    int error;
    if(!(hdpaExts = CreateDPAForExts(EatSpaces(str), &error, ptr)))
    {
        LocalFree( str );
        MLLoadString(error, szTemp, sizeof(szTemp));
        MessageBox(GetParent(hDlg), szTemp, szTitle, MB_OK);
        SetFocus( GetDlgItem(hDlg, IDC_DOC_EXTS) );
        return FALSE;
    }
    LocalFree( str );

  
    // Check and Add CAssoc if we are inserting a new entry.

    if(PGTI_FROM_HDLG(hDlg)->mode == ADDING)
    {
         LPTSTR firstExt = (LPTSTR)DPA_FastGetPtr( hdpaExts, 0 );
         int len = lstrlen( firstExt ) + lstrlen(g_szDocClass) + 1;
         str = (TCHAR *)LocalAlloc( LPTR, len*sizeof(TCHAR) );

         StrCpy( str, firstExt+1   );
         StrCat( str, g_szDocClass );

         ptr = new CAssoc( str );
         ptr->m_safe = FALSE; // Since we are adding this entry.
         DPA_InsertPtr( assocList, 0x7FFF, ptr );
         LocalFree( str );
    }

    // We are not able to add or retrieve Assoc from the List
    if( ptr == NULL || ptr->m_safe == TRUE )
    {
        FreeExtensions( hdpaExts );
        return FALSE;
    }

    // Start replacing components of the Associations.

    // Replace Extensions
    if(ptr->m_exts) FreeExtensions( ptr->m_exts );
    ptr->m_exts = hdpaExts;
 
    // Replace mime type
    len = SendMessage( GetDlgItem(hDlg, IDC_DOC_MIME), WM_GETTEXTLENGTH, 0, 0 );
    str = (TCHAR *)LocalAlloc( LPTR, (len+1)*sizeof(TCHAR) );
    SendMessage(GetDlgItem(hDlg, IDC_DOC_MIME), WM_GETTEXT, len+1, (LPARAM)str );
    if( ptr->m_mime ) LocalFree( ptr->m_mime );
    ptr->m_mime = EatSpaces(str);

    // Replace Description
    len = SendMessage( GetDlgItem(hDlg, IDC_DOC_DESC), WM_GETTEXTLENGTH, 0, 0 );
    str = (TCHAR *)LocalAlloc( LPTR, (len+1)*sizeof(TCHAR) );
    SendMessage(GetDlgItem(hDlg, IDC_DOC_DESC), WM_GETTEXT, len+1, (LPARAM)str );
    if( ptr->m_desc ) LocalFree( ptr->m_desc);
    ptr->m_desc = ChopSpaces(str);

    // Replace Command Line
    len = SendMessage( GetDlgItem(hDlg, IDC_DOC_CMND), WM_GETTEXTLENGTH, 0, 0 );
    str = (TCHAR *)LocalAlloc( LPTR, (len+4)*sizeof(TCHAR) );
    SendMessage(GetDlgItem(hDlg, IDC_DOC_CMND), WM_GETTEXT, len+1, (LPARAM)str );
    if( ptr->m_cmnd ) LocalFree( ptr->m_cmnd );
    ptr->m_cmnd = ChopSpaces(str);
    if (ptr->m_stat == NEW)
	if (!StrStr(ptr->m_cmnd, TEXT("%1")))
	    lstrcat(ptr->m_cmnd, TEXT(" %1"));
    {
        DWORD dwCurChar;
        BOOL  bPath = FALSE;
	    
        for (dwCurChar = 0; dwCurChar < lstrlen(ptr->m_cmnd); dwCurChar++)
        {
	    if (ptr->m_cmnd[dwCurChar] == TEXT('/'))
            {
		bPath = TRUE;
	    }
	    if (ptr->m_cmnd[dwCurChar] == TEXT(' '))
            {
		break;
	    }
	}
	if (bPath)  // if it's file name with no path we assume it's in the user's PATH
    {    
        CHAR szExeFile[MAX_PATH];
        TCHAR szWarning[MAX_PATH + 128];

#ifndef UNICODE
        lstrcpyn(szExeFile, ptr->m_cmnd, dwCurChar + 1);
#else
        WCHAR wszExeFile[MAX_PATH];
        lstrcpyn(wszExeFile, ptr->m_cmnd, dwCurChar + 1);
        SHUnicodeToAnsi(wszExeFile, szExeFile, ARRAYSIZE(szExeFile));
#endif
        if (access(szExeFile, X_OK) != 0)
	    {
#ifndef UNICODE
            wsprintf(szWarning, TEXT("File %s is not an executable file."), szExeFile);
#else
            wsprintf(szWarning, TEXT("File %s is not an executable file."), wszExeFile);
#endif
		    MessageBox( GetParent(hDlg), szWarning, TEXT("Warning"), MB_OK);
	    }
	}
    }

    if (Button_GetCheck(GetDlgItem(hDlg, IDC_ASSOC_EDIT)) != BST_UNCHECKED)
       ptr->m_edit &= (~FTA_OpenIsSafe );
    else
       ptr->m_edit |= FTA_OpenIsSafe;

    ptr->m_stat = UPD;

    InitAssocDialog( hDlg, ptr );
    SwitchToNrmlMode( hDlg);
    
    // SetFocus to Ok button again
    SetFocus( GetDlgItem(GetParent( hDlg ), IDOK ) );

    PropSheet_Changed(GetParent(hDlg),hDlg);
    return TRUE;
}


AssocAdd( HWND hDlg)
{
    SwitchToAddMode( hDlg );
    PropSheet_Changed(GetParent(hDlg),hDlg);
    return TRUE;
}
#endif // 0

CAssoc * GetCurrentAssoc( HWND hDlg )
{
    int index = 0;
    HWND lb   = GetDlgItem( hDlg, IDC_DOC_LIST );

    if( (index = SendMessage( lb, LB_GETCURSEL, 0, 0 ) )!= LB_ERR )
    {
        TCHAR * str = (LPTSTR)SendMessage( lb, LB_GETITEMDATA, index, 0 );
        if(!str) return NULL;

        if(str && *str)
        {

            if( (index = FindIndex( str ) ) != -1 )
            {
                CAssoc * ptr = (CAssoc *)DPA_FastGetPtr( assocList, index );
                return ptr;
            }
        }
    }
    return NULL;
}

SwitchToNrmlMode( HWND hDlg )
{
    PGTI_FROM_HDLG(hDlg)->mode = NORMAL;

    EnableWindow(GetDlgItem(hDlg,IDC_DOC_DESC), FALSE );
    EnableWindow(GetDlgItem(hDlg,IDC_DOC_EXTS), FALSE );
    EnableWindow(GetDlgItem(hDlg,IDC_DOC_MIME), FALSE );
    EnableWindow(GetDlgItem(hDlg,IDC_DOC_CMND), FALSE );
    EnableWindow(GetDlgItem(hDlg,IDC_ASSOC_EDIT), FALSE );

    if (IsAssocEnabled())
    {
	CAssoc *pAssoc = GetCurrentAssoc( hDlg );
        EnableWindow(GetDlgItem(hDlg,IDC_ASSOC_ADD), TRUE );
	if( pAssoc ) 
        {
	    if( pAssoc->m_safe )
	    {
		EnableWindow(GetDlgItem(hDlg,IDC_ASSOC_DEL), FALSE);
		EnableWindow(GetDlgItem(hDlg,IDC_ASSOC_UPD), FALSE );
	    }
	    else
	    {
		EnableWindow(GetDlgItem(hDlg,IDC_ASSOC_DEL), TRUE );
		EnableWindow(GetDlgItem(hDlg,IDC_ASSOC_UPD), TRUE );
	    }
	}
    }
    else
    {
        EnableWindow(GetDlgItem(hDlg,IDC_ASSOC_DEL), FALSE);
        EnableWindow(GetDlgItem(hDlg,IDC_ASSOC_UPD), FALSE );
        EnableWindow(GetDlgItem(hDlg,IDC_ASSOC_ADD), FALSE );
    }


    REMOVE_DEF_BORDER(hDlg, IDC_ASSOC_ADD );
    REMOVE_DEF_BORDER(hDlg, IDC_ASSOC_UPD );
    REMOVE_DEF_BORDER(hDlg, IDC_ASSOC_DEL );
    SET_DEF_BORDER( GetParent(hDlg), IDOK );

    return TRUE;
}


SwitchToAddMode( HWND hDlg )
{
    PGTI_FROM_HDLG(hDlg)->mode = ADDING;

    LPASSOCTABINFO pgti = (LPASSOCTABINFO)GetWindowLong(hDlg, DWL_USER);

    // Remove Selection from Listbox.
    SendMessage( GetDlgItem(hDlg, IDC_DOC_LIST), LB_SETCURSEL,
           (WPARAM)-1, 0);

    pgti->fInternalChange = TRUE;
    
    // Clear all the fields
    SendMessage( GetDlgItem(hDlg, IDC_DOC_DESC), WM_SETTEXT,
           0, LPARAM( TEXT("")) );
    SendMessage( GetDlgItem(hDlg, IDC_DOC_MIME), WM_SETTEXT,
           0, LPARAM( TEXT("")) );
    SendMessage( GetDlgItem(hDlg, IDC_DOC_EXTS), WM_SETTEXT,
           0, LPARAM( TEXT("")) );
    SendMessage( GetDlgItem(hDlg, IDC_DOC_CMND), WM_SETTEXT,
           0, LPARAM( TEXT("")) );
    
    // Default value
    Button_SetCheck( GetDlgItem( hDlg, IDC_ASSOC_EDIT),  TRUE ); 

    pgti->fInternalChange = FALSE;

    // Enable all edit windows
    EnableWindow(GetDlgItem(hDlg,IDC_DOC_DESC), TRUE);
    EnableWindow(GetDlgItem(hDlg,IDC_DOC_MIME), TRUE);
    EnableWindow(GetDlgItem(hDlg,IDC_DOC_EXTS), TRUE);
    EnableWindow(GetDlgItem(hDlg,IDC_DOC_CMND), TRUE);
    EnableWindow(GetDlgItem(hDlg,IDC_BROWSE  ), TRUE);
    EnableWindow(GetDlgItem(hDlg,IDC_ASSOC_EDIT  ), TRUE);

    // Enable Add option
    EnableWindow(GetDlgItem(hDlg,IDC_ASSOC_ADD), FALSE);
    EnableWindow(GetDlgItem(hDlg,IDC_ASSOC_UPD), TRUE );
    EnableWindow(GetDlgItem(hDlg,IDC_ASSOC_DEL), FALSE);

    // Remove Default Border
    REMOVE_DEF_BORDER( GetParent(hDlg), IDOK );
    REMOVE_DEF_BORDER( hDlg, IDC_ASSOC_ADD);
    REMOVE_DEF_BORDER( hDlg, IDC_ASSOC_DEL);
    SET_DEF_BORDER( hDlg, IDC_ASSOC_UPD );
    SetFocus( GetDlgItem( hDlg, IDC_ASSOC_UPD ) );

    return TRUE;
}

SwitchToUpdMode( HWND hDlg )
{
    if(PGTI_FROM_HDLG(hDlg)->mode != NORMAL )
       return FALSE;

    PGTI_FROM_HDLG(hDlg)->mode = UPDATING;

    // Enable Upd option
    EnableWindow(GetDlgItem(hDlg,IDC_ASSOC_ADD), TRUE );
    EnableWindow(GetDlgItem(hDlg,IDC_ASSOC_UPD), TRUE );
    EnableWindow(GetDlgItem(hDlg,IDC_ASSOC_DEL), TRUE );

    // Remove Default Border
    REMOVE_DEF_BORDER( GetParent(hDlg), IDOK );
    REMOVE_DEF_BORDER( hDlg, IDC_ASSOC_ADD);
    REMOVE_DEF_BORDER( hDlg, IDC_ASSOC_DEL);
    SET_DEF_BORDER( hDlg, IDC_ASSOC_UPD);

    return TRUE;
}

void ListBoxResetContents(HWND listBox)
{
    int count = SendMessage( listBox, LB_GETCOUNT, 0, 0 );

    for( int i= 0; i< count; i++ )
    {
        LPSTR data = (LPSTR)SendMessage( listBox, LB_GETITEMDATA, i, 0 );
        if( data ) LocalFree( data );
    }
    SendMessage( listBox, LB_RESETCONTENT, 0, 0 );
}

BOOL FAR PASCAL InitAssocDialog(HWND hDlg, CAssoc * current)
{
    HRESULT  hr = E_FAIL;
    HKEY     hKey;
    HWND     listBox = GetDlgItem( hDlg, IDC_DOC_LIST );
    TCHAR *  displayString;

    // Allocate memory for a structure which will hold all the info
    // gathered from this page
    //
    LPASSOCTABINFO pgti = (LPASSOCTABINFO)GetWindowLong(hDlg, DWL_USER);
    pgti->fInternalChange = FALSE;

    ListBoxResetContents( listBox );

    if( assocList == NULL ) return FALSE;

    int assocCount = DPA_GetPtrCount( assocList );

    for(int i = 0; i< assocCount; i++ )
    {
        CAssoc * ptr = (CAssoc *)DPA_FastGetPtr( assocList, i );
        int index = SendMessage( listBox, LB_ADDSTRING, 0, (LPARAM)ptr->m_desc );
        SendMessage( listBox, LB_SETITEMDATA, index, (LPARAM)DuplicateString( ptr->m_type ) );
    }

    if( i>0 )
        SendMessage( listBox, LB_SETCURSEL, 0, 0 );
    else
    {
        SendMessage( GetDlgItem(hDlg, IDC_DOC_DESC), WM_SETTEXT,
           0, LPARAM( TEXT("")) );
        SendMessage( GetDlgItem(hDlg, IDC_DOC_MIME), WM_SETTEXT,
           0, LPARAM( TEXT("")) );
        SendMessage( GetDlgItem(hDlg, IDC_DOC_EXTS), WM_SETTEXT,
           0, LPARAM( TEXT("")) );
        SendMessage( GetDlgItem(hDlg, IDC_DOC_CMND), WM_SETTEXT,
           0, LPARAM( TEXT("")) );

        // Default value
        Button_SetCheck( GetDlgItem( hDlg, IDC_ASSOC_EDIT),  TRUE ); 
    }

    // Add Strings to Mimetype dialog
    int mimeCount = 0;

    if( mimeList && (mimeCount = DPA_GetPtrCount(mimeList)))
    {
        for(i=0; i< mimeCount; i++)
        {
            CMime * ptr = (CMime *)DPA_FastGetPtr( mimeList , i );
		    SafeAddStringToComboBox( GetDlgItem(hDlg, IDC_DOC_MIME) , ptr->m_mime) ;
        }   
    }

    if( current )
        SendMessage( listBox, LB_SELECTSTRING, -1, (LPARAM)current->m_desc );

    HandleSelChange( pgti );

    SwitchToNrmlMode( hDlg );

    if( IsAssocEnabled() && i > 0 )
        EnableWindow(GetDlgItem(hDlg,IDC_ASSOC_DEL), TRUE);
    else
        EnableWindow(GetDlgItem(hDlg,IDC_ASSOC_DEL), FALSE);

    return TRUE;
}

void HandleSelChange( LPASSOCTABINFO pgti, BOOL bApplChange )
{
    int index = 0;
    HWND hDlg = pgti->hDlg;
    TCHAR str[MAX_PATH] = TEXT("");

    if( hDlg == NULL ) return;

    HWND exts = GetDlgItem( hDlg, IDC_DOC_EXTS );
    HWND desc = GetDlgItem( hDlg, IDC_DOC_DESC );
    HWND mime = GetDlgItem( hDlg, IDC_DOC_MIME );
    HWND cmnd = GetDlgItem( hDlg, IDC_DOC_CMND );
    HWND brws = GetDlgItem( hDlg, IDC_BROWSE   );
    HWND edit = GetDlgItem( hDlg, IDC_ASSOC_EDIT );

    CAssoc * ptr = GetCurrentAssoc( hDlg );
 
    if(ptr)
    {
        SendMessage( desc, WM_SETTEXT, 0, (LPARAM)ptr->m_desc );
        SendMessage( mime, WM_SETTEXT, 0, (LPARAM)ptr->m_mime );
        SendMessage( cmnd, WM_SETTEXT, 0, (LPARAM)ptr->m_cmnd );

        // Create Extension List
        if( ptr->m_exts )
        {
            int i, count = DPA_GetPtrCount( ptr->m_exts );
            for(i=0;i<count;i++)
            {
                StrCat( str, (LPTSTR)DPA_FastGetPtr( ptr->m_exts, i ) );
                StrCat( str, TEXT(";") );
            }
            SendMessage( exts, WM_SETTEXT, 0, (LPARAM)str );
        }
                
        Button_SetCheck( GetDlgItem( hDlg, IDC_ASSOC_EDIT), !(ptr->m_edit & FTA_OpenIsSafe) ); 

        EnableWindow( desc, !(ptr->m_safe) );
        EnableWindow( exts, !(ptr->m_safe) );
        EnableWindow( mime, !(ptr->m_safe) );
        EnableWindow( cmnd, !(ptr->m_safe) );
        EnableWindow( brws, !(ptr->m_safe) );
        EnableWindow( edit, !(ptr->m_safe) );

        pgti->fInternalChange = FALSE;

        // Change back to the NORMAL mode if not coming from 
        // edit.
        SwitchToNrmlMode( hDlg );
    }
}

BOOL AssocOnCommand(LPASSOCTABINFO pgti, UINT id, UINT nCmd)
{
    switch (id)
    { 
        case IDC_BROWSE:
            switch (nCmd )
            {
               case BN_CLICKED:
               {
                    FindCommand( pgti->hDlg );
                    break;
               }
            }
            break;

        case IDC_ASSOC_ADD:
            switch (nCmd )
            {
                case BN_CLICKED:
                {
		    ENTERASSOC enter;
		    enter.pgti = pgti;
		    enter.pszAssoc = NULL;
		    DialogBoxParam(MLGetHinst(), MAKEINTRESOURCE(IDD_ENTER_ASSOC), 
					 pgti->hDlg, EnterAssocDlgProc, (LPARAM) &enter);
		    InitAssocDialog( pgti->hDlg, GetCurrentAssoc( pgti->hDlg ) );
		    SwitchToNrmlMode( pgti->hDlg);
		    // SetFocus to Ok button again
		    SetFocus( GetDlgItem(GetParent( pgti->hDlg ), IDOK ) );
		    if (pgti->fChanged)
		    {
		        pgti->fChanged = FALSE;
			PropSheet_Changed(GetParent(pgti->hDlg),pgti->hDlg);
		    }
                    //AssocAdd( pgti->hDlg );
                    break;
               }
            }
            break;

        case IDC_ASSOC_UPD:
            switch (nCmd )
            {
               case BN_CLICKED:
               {
		    HWND lb   = GetDlgItem( pgti->hDlg, IDC_DOC_LIST );
		    int  index;

		    if( (index = SendMessage( lb, LB_GETCURSEL, 0, 0 ) )!= LB_ERR )
		    {
		        TCHAR * str = (LPTSTR)SendMessage( lb, LB_GETITEMDATA, index, 0 );
			ENTERASSOC enter;
			if(!str) return FALSE;
			enter.pgti = pgti;
			enter.pszAssoc = str;
			DialogBoxParam(MLGetHinst(), MAKEINTRESOURCE(IDD_ENTER_ASSOC), 
					     pgti->hDlg, EnterAssocDlgProc, (LPARAM) &enter);
			InitAssocDialog( pgti->hDlg, GetCurrentAssoc( pgti->hDlg ) );
			SwitchToNrmlMode( pgti->hDlg);
			// SetFocus to Ok button again
			SetFocus( GetDlgItem(GetParent( pgti->hDlg ), IDOK ) );
			if (pgti->fChanged)
			{
			    pgti->fChanged = FALSE;
			    PropSheet_Changed(GetParent(pgti->hDlg),pgti->hDlg);
			}
		    }
                    //AssocUpd( pgti->hDlg );
                    break;
               }
            }
            break;

        case IDC_ASSOC_DEL:
            switch (nCmd )
            {
               case BN_CLICKED:
               {
                    AssocDel( pgti->hDlg ); 
                    break;
               }
            }
            break;

        case IDC_DOC_LIST:
            switch (nCmd )
            {
                case LBN_SELCHANGE:
                     HandleSelChange( pgti );
                     break;
	        case LBN_DBLCLK:
               {
		    HWND lb   = GetDlgItem( pgti->hDlg, IDC_DOC_LIST );
		    int  index;

		    if( (index = SendMessage( lb, LB_GETCURSEL, 0, 0 ) )!= LB_ERR )
		    {
		        TCHAR * str = (LPTSTR)SendMessage( lb, LB_GETITEMDATA, index, 0 );
			ENTERASSOC enter;
			CAssoc *pAssoc = GetCurrentAssoc( pgti->hDlg );
			if(!IsAssocEnabled() || pAssoc->m_safe || !str) return FALSE;
			enter.pgti = pgti;
			enter.pszAssoc = str;
			DialogBoxParam(MLGetHinst(), MAKEINTRESOURCE(IDD_ENTER_ASSOC), 
					     pgti->hDlg, EnterAssocDlgProc, (LPARAM) &enter);
			InitAssocDialog( pgti->hDlg, GetCurrentAssoc( pgti->hDlg ) );
			SwitchToNrmlMode( pgti->hDlg);
			// SetFocus to Ok button again
			SetFocus( GetDlgItem(GetParent( pgti->hDlg ), IDOK ) );
			if (pgti->fChanged)
			{
			    pgti->fChanged = FALSE;
			    PropSheet_Changed(GetParent(pgti->hDlg),pgti->hDlg);
			}
		    }
                    //AssocUpd( pgti->hDlg );
                    break;
               }
            }
            break;
#ifdef 0
        case IDC_DOC_MIME:
            switch (nCmd)
            {
                case CBN_SELCHANGE:
                case CBN_EDITCHANGE:
                    if (!pgti->fInternalChange)
                    {
                        pgti->fChanged = TRUE;
                        SwitchToUpdMode( pgti->hDlg );
                    }
                    break;
            }
            break;

        case IDC_ASSOC_EDIT:
            switch (nCmd )
            {
               case BN_CLICKED:
               {
                    if (!pgti->fInternalChange)
                    {
                        pgti->fChanged = TRUE;
                        SwitchToUpdMode( pgti->hDlg );
                    }
                    break;
               }
            }
            break;

        case IDC_DOC_TYPE:
        case IDC_DOC_EXTS:
        case IDC_DOC_DESC:
        case IDC_DOC_CMND:
            switch (nCmd)
            {
                case EN_CHANGE:
                    if (!pgti->fInternalChange)
                    {
                        pgti->fChanged = TRUE;
                        SwitchToUpdMode( pgti->hDlg );
                    }
                    break;
                case EN_SETFOCUS:
                    if ( pgti->mode == ADDING)
                    {
                        SET_DEF_BORDER( pgti->hDlg, IDC_ASSOC_UPD );
                        break; 
                    }
            }
            break;
#endif //0
    }

    return FALSE;
}

void AssocApply(HWND hDlg)
{
    // Delete the associations removed by the user.
    if( assocDelList )
    {
        int count = DPA_GetPtrCount( assocDelList );
        
        for(int i=0;i<count;i++)
        {
            CAssoc * pAssoc = (CAssoc *)DPA_FastGetPtr( assocDelList, i );
            if(pAssoc) 
            {
                pAssoc->Delete();
                delete pAssoc;
            }
        }

        DPA_Destroy( assocDelList );
        assocDelList = NULL;
    }

    // Save the currently changed associations.
    if( assocList )
    {
        int count = DPA_GetPtrCount( assocList );
        
        for(int i=0;i<count;i++)
        {
            CAssoc * pAssoc = (CAssoc *)DPA_FastGetPtr( assocList, i );
            if(pAssoc) 
            {  
                pAssoc->Save();
            }
        }
    }
}

/****************************************************************************
 *
 * AssocDlgProc
 *
 *
 ***************************************************************************/
 
BOOL CALLBACK AssocDlgProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // get our tab info structure
    LPASSOCTABINFO pgti;

    if (uMsg == WM_INITDIALOG)
    {
        // Set Limits for edit fields
        SetEditLimits( hDlg );
  
        // Allocate memory for a structure which will hold all the info
        // gathered from this page
        //
        LPASSOCTABINFO pgti = (LPASSOCTABINFO)LocalAlloc(LPTR, sizeof(ASSOCINFO));
        if (!pgti)
        {
            EndDialog(hDlg, 0);
            return FALSE;
        }

        pgti->hDlg = hDlg;
        pgti->mode = NORMAL;

        pgti->fInternalChange = FALSE;
        SetWindowLong(hDlg, DWL_USER, (LPARAM)pgti);
        
        // Create an association array from registry
        LoadAssociations();
        LoadMimeTypes();

        // Initailize dialog 
        if( InitAssocDialog(hDlg) ) 
        {
            HandleSelChange(pgti);
            return TRUE;
        }
        else
        {
            TCHAR szTitle[MAX_PATH];
            MLLoadString(IDS_ERROR_REGISTRY_TITLE, szTitle, sizeof(szTitle));
            MessageBox( GetParent(hDlg), TEXT("Cannot read associations from registry."), szTitle, MB_OK ); 
            return FALSE;
        }
    }
    else
        pgti = (LPASSOCTABINFO)GetWindowLong(hDlg, DWL_USER);

    if (!pgti)
        return FALSE;

    switch (uMsg)
    {
        case WM_NOTIFY:
        {
            NMHDR *lpnm = (NMHDR *) lParam;

            switch (lpnm->code)
            {
                case PSN_QUERYCANCEL:
                case PSN_KILLACTIVE:
                case PSN_RESET:
                    SetWindowLong( hDlg, DWL_MSGRESULT, FALSE );
                    return TRUE;

                case PSN_APPLY:
                    AssocApply(hDlg);
                    break;
            }
            break;
        }

        case WM_COMMAND:
            AssocOnCommand(pgti, LOWORD(wParam), HIWORD(wParam));
            break;

        case WM_HELP:           // F1
            //ResWinHelp( (HWND)((LPHELPINFO)lParam)->hItemHandle, IDS_HELPFILE,
            //            HELP_WM_HELP, (DWORD_PTR)(LPSTR)mapIDCsToIDHs);
            break;

        case WM_CONTEXTMENU:    // right mouse click
            //ResWinHelp( (HWND) wParam, IDS_HELPFILE,
            //            HELP_CONTEXTMENU, (DWORD_PTR)(LPSTR)mapIDCsToIDHs);
            break;

        case WM_DESTROY:
#if 0
            RemoveDefaultDialogFont(hDlg);
#endif

            if (pgti)
                LocalFree(pgti);

            // Remove Associated data items for the listbos items.
            ListBoxResetContents( GetDlgItem( hDlg, IDC_DOC_LIST ) );
  
            // Delete registry information
            FreeAssociations();
            FreeMimeTypes();

            SetWindowLong(hDlg, DWL_USER, (LONG)NULL);  // make sure we don't re-enter
            break;
    }
    return FALSE;
}

BOOL AssocEnter( HWND hDlg )
{
    LPASSOCTABINFO pgti = ((LPENTERASSOC)GetWindowLong(hDlg, DWL_USER))->pgti;
    BOOL bFound = FALSE;
    TCHAR szTemp [1024];
    TCHAR szTitle [80];
    CAssoc *ptr = NULL;
    TCHAR *str;
    int    index, len;
    HDPA   hdpaExts = NULL;

    HWND exts = GetDlgItem( hDlg, IDC_DOC_EXTS );
    HWND desc = GetDlgItem( hDlg, IDC_DOC_DESC );
    HWND mime = GetDlgItem( hDlg, IDC_DOC_MIME );
    HWND cmnd = GetDlgItem( hDlg, IDC_DOC_CMND );

    MLLoadString(IDS_ERROR_REGISTRY_TITLE, szTitle, sizeof(szTitle));

    // Get the pointer to existing associations.
    if(GetWindowLong(hDlg, DWL_USER))
    {
        TCHAR *pszAssoc = ((LPENTERASSOC)GetWindowLong(hDlg, DWL_USER))->pszAssoc;
	if(pszAssoc && *pszAssoc)
	    if( (index = FindIndex( pszAssoc ) ) != -1 )
	        ptr = (CAssoc *)DPA_FastGetPtr( assocList, index );
    }

    //Check for Description
    len = SendMessage( GetDlgItem(hDlg, IDC_DOC_DESC), WM_GETTEXTLENGTH, 0, 0 );
    if( len > 0 )
    {
        str = (TCHAR *)LocalAlloc( LPTR, (len+1)*sizeof(TCHAR) );
        SendMessage(GetDlgItem(hDlg, IDC_DOC_DESC), WM_GETTEXT, len+1, (LPARAM)str );
        ChopSpaces( str );
        if( *str == TEXT('\0') )
        {    
            LocalFree(str);
            MLLoadString(IDS_ERROR_MISSING_DESC, szTemp, sizeof(szTemp));
            MessageBox(GetParent(hDlg), szTemp, szTitle, MB_OK);
            SetFocus( GetDlgItem(hDlg, IDC_DOC_DESC) );
            return FALSE;
        }

        int assocCount = DPA_GetPtrCount( assocList );
        for( int i= 0; i<assocCount; i++ )
        {
            CAssoc * pAssoc ;
            if((pAssoc= (CAssoc *)DPA_FastGetPtr( assocList, i )) == ptr ) continue;
            
            if( StrCmpI( str, pAssoc->m_cmnd ) == NULL )
            {
                LocalFree(str);
                MLLoadString(IDS_ERROR_DESC_ALREADY_EXISTS, szTemp, sizeof(szTemp));
                MessageBox(GetParent(hDlg), szTemp, szTitle, MB_OK);
                SetFocus( GetDlgItem(hDlg, IDC_DOC_DESC) );
                return FALSE;
            }
        }

        LocalFree(str);
    }
    else
    {
        MLLoadString(IDS_ERROR_MISSING_DESC, szTemp, sizeof(szTemp));
        MessageBox(GetParent(hDlg), szTemp, szTitle, MB_OK);
        SetFocus( GetDlgItem(hDlg, IDC_DOC_DESC) );
        return FALSE;
    }

    //Check for MIME
    len = SendMessage( GetDlgItem(hDlg, IDC_DOC_MIME), WM_GETTEXTLENGTH, 0, 0 );
    if( len > 0 )
    {
        str = (TCHAR *)LocalAlloc( LPTR, (len+1)*sizeof(TCHAR) );
        SendMessage(GetDlgItem(hDlg, IDC_DOC_MIME), WM_GETTEXT, len+1, (LPARAM)str );
        ChopSpaces( str );
        if( *str != TEXT('\0') )
        {
            if( !StrChr( str, TEXT('/') ) )
            {
                LocalFree(str);
                MLLoadString(IDS_ERROR_INVALID_MIME, szTemp, sizeof(szTemp));
                MessageBox(GetParent(hDlg), szTemp, szTitle, MB_OK);
                SetFocus( GetDlgItem(hDlg, IDC_DOC_MIME) );
                return FALSE;
            }
        }
        LocalFree(str);
    }
    

    //Check for command line
    len = SendMessage( GetDlgItem(hDlg, IDC_DOC_CMND), WM_GETTEXTLENGTH, 0, 0 );
    if( len > 0 )
    {
        str = (TCHAR *)LocalAlloc( LPTR, (len+1)*sizeof(TCHAR) );
        SendMessage(GetDlgItem(hDlg, IDC_DOC_CMND), WM_GETTEXT, len+1, (LPARAM)str );
        ChopSpaces( str );
        if( *str == TEXT('\0') )
        {
            LocalFree(str);
            MLLoadString(IDS_ERROR_MISSING_CMND, szTemp, sizeof(szTemp));
            MessageBox(GetParent(hDlg), szTemp, szTitle, MB_OK);
            SetFocus( GetDlgItem(hDlg, IDC_DOC_CMND) );
            return FALSE;
        }    

        LocalFree(str);
    }
    else
    {
        MLLoadString(IDS_ERROR_MISSING_CMND, szTemp, sizeof(szTemp));
        MessageBox(GetParent(hDlg), szTemp, szTitle, MB_OK);
        SetFocus( GetDlgItem(hDlg, IDC_DOC_CMND) );
        return FALSE;
    }

    // Check for extensions.
    // User entered may already have associations in the
    // registry.
    len = SendMessage( GetDlgItem(hDlg, IDC_DOC_EXTS), WM_GETTEXTLENGTH, 0, 0 );
    str = (TCHAR *)LocalAlloc( LPTR, (len+1)*sizeof(TCHAR) );
    SendMessage(GetDlgItem(hDlg, IDC_DOC_EXTS), WM_GETTEXT, len+1, (LPARAM)str );

    int error;
    if(!(hdpaExts = CreateDPAForExts(EatSpaces(str), &error, ptr)))
    {
        LocalFree( str );
        MLLoadString(error, szTemp, sizeof(szTemp));
        MessageBox(GetParent(hDlg), szTemp, szTitle, MB_OK);
        SetFocus( GetDlgItem(hDlg, IDC_DOC_EXTS) );
        return FALSE;
    }
    LocalFree( str );

  
    // Check and Add CAssoc if we are inserting a new entry.

    if(!((LPENTERASSOC)GetWindowLong(hDlg, DWL_USER))->pszAssoc)
    {
         LPTSTR firstExt = (LPTSTR)DPA_FastGetPtr( hdpaExts, 0 );
         int len = lstrlen( firstExt ) + lstrlen(g_szDocClass) + 1;
         str = (TCHAR *)LocalAlloc( LPTR, len*sizeof(TCHAR) );

         StrCpy( str, firstExt+1   );
         StrCat( str, g_szDocClass );

         ptr = new CAssoc( str );
         ptr->m_safe = FALSE; // Since we are adding this entry.
         DPA_InsertPtr( assocList, 0x7FFF, ptr );
         LocalFree( str );
    }

    // We are not able to add or retrieve Assoc from the List
    if( ptr == NULL || ptr->m_safe == TRUE )
    {
        FreeExtensions( hdpaExts );
        return FALSE;
    }

    // Start replacing components of the Associations.

    // Replace Extensions
    if(ptr->m_exts) FreeExtensions( ptr->m_exts );
    ptr->m_exts = hdpaExts;
 
    // Replace mime type
    len = SendMessage( GetDlgItem(hDlg, IDC_DOC_MIME), WM_GETTEXTLENGTH, 0, 0 );
    str = (TCHAR *)LocalAlloc( LPTR, (len+1)*sizeof(TCHAR) );
    SendMessage(GetDlgItem(hDlg, IDC_DOC_MIME), WM_GETTEXT, len+1, (LPARAM)str );
    if( ptr->m_mime ) LocalFree( ptr->m_mime );
    ptr->m_mime = EatSpaces(str);

    // Replace Description
    len = SendMessage( GetDlgItem(hDlg, IDC_DOC_DESC), WM_GETTEXTLENGTH, 0, 0 );
    str = (TCHAR *)LocalAlloc( LPTR, (len+1)*sizeof(TCHAR) );
    SendMessage(GetDlgItem(hDlg, IDC_DOC_DESC), WM_GETTEXT, len+1, (LPARAM)str );
    if( ptr->m_desc ) LocalFree( ptr->m_desc);
    ptr->m_desc = ChopSpaces(str);

    // Replace Command Line
    len = SendMessage( GetDlgItem(hDlg, IDC_DOC_CMND), WM_GETTEXTLENGTH, 0, 0 );
    str = (TCHAR *)LocalAlloc( LPTR, (len+4)*sizeof(TCHAR) );
    SendMessage(GetDlgItem(hDlg, IDC_DOC_CMND), WM_GETTEXT, len+1, (LPARAM)str );
    if( ptr->m_cmnd ) LocalFree( ptr->m_cmnd );
    ptr->m_cmnd = ChopSpaces(str);
    if (ptr->m_stat == NEW)
	if (!StrStr(ptr->m_cmnd, TEXT("%1")))
	    lstrcat(ptr->m_cmnd, TEXT(" %1"));
    {
        DWORD dwCurChar;
        BOOL  bPath = FALSE;
	    
        for (dwCurChar = 0; dwCurChar < lstrlen(ptr->m_cmnd); dwCurChar++)
        {
	    if (ptr->m_cmnd[dwCurChar] == TEXT('/'))
            {
		bPath = TRUE;
	    }
	    if (ptr->m_cmnd[dwCurChar] == TEXT(' '))
            {
		break;
	    }
	}
	if (bPath)  // if it's file name with no path we assume it's in the user's PATH
    {    
        CHAR szExeFile[MAX_PATH];
        TCHAR szWarning[MAX_PATH + 128];

#ifndef UNICODE
        lstrcpyn(szExeFile, ptr->m_cmnd, dwCurChar + 1);
#else
        WCHAR wszExeFile[MAX_PATH];
        lstrcpyn(wszExeFile, ptr->m_cmnd, dwCurChar + 1);
        SHUnicodeToAnsi(wszExeFile, szExeFile, ARRAYSIZE(szExeFile));
#endif
        if (access(szExeFile, X_OK) != 0)
	    {
#ifndef UNICODE
            wsprintf(szWarning, TEXT("File %s is not an executable file."), szExeFile);
#else
            wsprintf(szWarning, TEXT("File %s is not an executable file."), wszExeFile);
#endif
		    MessageBox( GetParent(hDlg), szWarning, TEXT("Warning"), MB_OK);
	    }
	}
    }

    if (Button_GetCheck(GetDlgItem(hDlg, IDC_ASSOC_EDIT)) != BST_UNCHECKED)
       ptr->m_edit &= (~FTA_OpenIsSafe );
    else
       ptr->m_edit |= FTA_OpenIsSafe;

    ptr->m_stat = UPD;

    pgti->fChanged = TRUE;

    return TRUE;
}

BOOL FAR PASCAL InitEnterAssocDialog(HWND hDlg, LPENTERASSOC penter)
{
    LPASSOCTABINFO pgti = penter->pgti;
    int index = 0;
    HWND hDlgParent = pgti->hDlg;
    TCHAR str[MAX_PATH] = TEXT("");

    if( hDlg == NULL ) return;

    HWND exts = GetDlgItem( hDlg, IDC_DOC_EXTS );
    HWND desc = GetDlgItem( hDlg, IDC_DOC_DESC );
    HWND mime = GetDlgItem( hDlg, IDC_DOC_MIME );
    HWND cmnd = GetDlgItem( hDlg, IDC_DOC_CMND );
    HWND brws = GetDlgItem( hDlg, IDC_BROWSE   );
    HWND edit = GetDlgItem( hDlg, IDC_ASSOC_EDIT );

    CAssoc * ptr = NULL;

    if(penter->pszAssoc && *(penter->pszAssoc))
	if( (index = FindIndex( penter->pszAssoc ) ) != -1 )
	    ptr = (CAssoc *)DPA_FastGetPtr( assocList, index );
 
    if(ptr)
    {
        SendMessage( desc, WM_SETTEXT, 0, (LPARAM)ptr->m_desc );
        SendMessage( mime, WM_SETTEXT, 0, (LPARAM)ptr->m_mime );
        SendMessage( cmnd, WM_SETTEXT, 0, (LPARAM)ptr->m_cmnd );

        // Create Extension List
        if( ptr->m_exts )
        {
            int i, count = DPA_GetPtrCount( ptr->m_exts );
            for(i=0;i<count;i++)
            {
                StrCat( str, (LPTSTR)DPA_FastGetPtr( ptr->m_exts, i ) );
                StrCat( str, TEXT(";") );
            }
            SendMessage( exts, WM_SETTEXT, 0, (LPARAM)str );
        }
                
        Button_SetCheck( GetDlgItem( hDlg, IDC_ASSOC_EDIT), !(ptr->m_edit & FTA_OpenIsSafe) ); 

        EnableWindow( desc, !(ptr->m_safe) );
        EnableWindow( exts, !(ptr->m_safe) );
        EnableWindow( mime, !(ptr->m_safe) );
        EnableWindow( cmnd, !(ptr->m_safe) );
        EnableWindow( brws, !(ptr->m_safe) );
        EnableWindow( edit, !(ptr->m_safe) );

        pgti->fInternalChange = FALSE;
    }

    SetWindowLong(hDlg, DWL_USER, (LPARAM)penter);

    int mimeCount = 0;

    if( mimeList && (mimeCount = DPA_GetPtrCount(mimeList)))
    {
        for(int i=0; i< mimeCount; i++)
        {
            CMime * ptr = (CMime *)DPA_FastGetPtr( mimeList , i );
	    SafeAddStringToComboBox( GetDlgItem(hDlg, IDC_DOC_MIME) , ptr->m_mime) ;
        }   
    }


}

BOOL EnterAssocOnCommand(HWND hDlg, UINT id, UINT nCmd, BOOL *pbChanged)
{
    switch (id)
    { 
        case IDC_BROWSE:
            switch (nCmd )
            {
               case BN_CLICKED:
               {
                    FindCommand( hDlg );
                    break;
               }
            }
            break;
        case IDOK:
            switch (nCmd )
            {
                case BN_CLICKED:
                {
		    SwitchToUpdMode(GetParent(hDlg));
		    if (!*pbChanged || AssocEnter(hDlg))
		        return EndDialog(hDlg, 0);
		    else
		        break;
                }
            }
            break;
        case IDCANCEL:
            switch (nCmd )
            {
                case BN_CLICKED:
                {
		    return EndDialog(hDlg, 0);
                }
            }
            break;
        case IDC_DOC_MIME:
            switch (nCmd)
            {
                case CBN_SELCHANGE:
                case CBN_EDITCHANGE:
		    if (!*pbChanged)
		    {
		        *pbChanged = TRUE;
		    }
                    break;
            }
            break;

        case IDC_ASSOC_EDIT:
            switch (nCmd )
            {
               case BN_CLICKED:
               {
		    if (!*pbChanged)
		    {
		        *pbChanged = TRUE;
		    }
                    break;
               }
            }
            break;

        case IDC_DOC_TYPE:
        case IDC_DOC_EXTS:
        case IDC_DOC_DESC:
        case IDC_DOC_CMND:
            switch (nCmd)
            {
                case EN_CHANGE:
		    if (!*pbChanged)
		    {
		        *pbChanged = TRUE;
		    }
                    break;
            }
            break;
    }

    return FALSE;
}

BOOL CALLBACK EnterAssocDlgProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static BOOL bChanged;
    if (uMsg == WM_INITDIALOG)
    {
        SendMessage(GetDlgItem( hDlg, IDC_DOC_MIME ), EM_LIMITTEXT, (WPARAM) MAX_PATH-1, 0);
	SendMessage(GetDlgItem( hDlg, IDC_DOC_EXTS ), EM_LIMITTEXT, (WPARAM) MAX_PATH-1, 0);
	SendMessage(GetDlgItem( hDlg, IDC_DOC_DESC ), EM_LIMITTEXT, (WPARAM) MAX_PATH-1, 0);
	SendMessage(GetDlgItem( hDlg, IDC_DOC_CMND ), EM_LIMITTEXT, (WPARAM) MAX_PATH-1, 0);
	InitEnterAssocDialog(hDlg, (LPENTERASSOC)lParam);
	bChanged = FALSE;
    }

    switch (uMsg)
    {
        case WM_COMMAND:
            return EnterAssocOnCommand(hDlg, LOWORD(wParam), HIWORD(wParam), &bChanged);
            break;
    }
    return FALSE;
}
