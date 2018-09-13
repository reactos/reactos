//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 1997.
//
//  File:       src\site\base\unixbase.cxx
//
//  Contents:   Implementation of Unix specific/different operations
//
//  Classes:    
//
//  Functions:
//
//  History:    03-Sep-97   davidd    Created
//
//----------------------------------------------------------------------------

#include "headers.hxx"
#include "formkrnl.hxx"

EXTERN_C char *MwGetXDisplayString( void );

HRESULT CDoc::InvokeEditor( LPCSTR lpszPath )
{
    HKEY  hkeyUnix;
    CHAR  szCommand [2*MAX_PATH];
    CHAR  szCmdTempl[MAX_PATH+1];
    CHAR  szName    [MAX_PATH+1];
    CHAR  hKeyName  [MAX_PATH+1];

    STARTUPINFOA st;
    PROCESS_INFORMATION pi;

    BOOL  bIsKnownEdit  = FALSE;
    char  displayString [2*MAX_PATH];
    DWORD editors       = 0;
    DWORD type          = REG_SZ;
    DWORD dwLength      = sizeof(szCmdTempl);

    if( MwGetXDisplayString() )
        sprintf( displayString, "-display %s", MwGetXDisplayString() );
    else
        sprintf( displayString, " ");
        

    // Get user preferred editor.

    if( getenv("EDITOR" ) )
       strcpy(szName, getenv("EDITOR") );
    else
       strcpy(szName, "vi");

    // Check editor against the list of known editors in 
    // registry.

    sprintf( hKeyName, 
             "Software\\Microsoft\\Internet Explorer\\Unix\\Editors\\%s",
             szName );

    LONG lResult = RegOpenKeyExA(
       HKEY_CURRENT_USER,
       hKeyName,
       0,
       KEY_QUERY_VALUE,
       &hkeyUnix);

    if (lResult == ERROR_SUCCESS) 
    {
        // Read command template for the registered editor

        lResult = RegQueryValueExA(
           hkeyUnix,
           "command",
           NULL,
           (LPDWORD) &type,
           (LPBYTE)  &szCmdTempl,
           (LPDWORD) &dwLength);

        if( lResult == ERROR_SUCCESS )
            bIsKnownEdit = TRUE;
            
        RegCloseKey(hkeyUnix);

    }

    // Create proper command and append dissplay string to make the
    // editor appear on the same XServer as the Iexplorer.
    if( !bIsKnownEdit )
    {
        // Default use vi
        sprintf( szCommand, "xterm %s -e vi %s ", displayString, lpszPath  );
    }
    else
    {
        // Use template command from registry to create actual command.
        sprintf( szCommand, szCmdTempl, displayString, lpszPath );
    }

    // Initialize startup info struct.
    st.cb = sizeof(STARTUPINFO);
    st.lpReserved = NULL;
    st.lpDesktop  = NULL;
    st.lpTitle    = NULL;
    st.dwFlags    = 0;
    st.wShowWindow= SW_SHOWNORMAL;
    st.cbReserved2= 0;
    st.lpReserved2= NULL;

    // Launch the command
    if ( CreateProcessA( NULL, szCommand, NULL, NULL, TRUE, 
                         CREATE_NEW_CONSOLE, NULL, NULL, &st, &pi )) 
    {
        return S_OK;
    }

    RRETURN( GetLastError());
}

