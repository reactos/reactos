/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    mapembed.c

Abstract:

    This module contains the functions that perform the mapping
    between the "embedding" section of win.ini, and the subkeys
    of HKEY_CLASSES_ROOT.

    This mapping is a hack implemented on Win3.1, that must also
    exist on NT.
    It is implemnted in the WOW layer, since only some win16 apps
    that read or write to the "embedding" section ( WinWord and
    MsMail) depend on it.



Author:


    Jaime F. Sasson (jaimes) 25-Nov-1992



--*/

#include "precomp.h"
#pragma hdrstop

MODNAME(mapembed.c);


#define WININITIMEOUT   2000
#define BUFFER_SIZE     128

#define EMPTY_STRING        ""

DWORD   _LastTimeUpdated = 0;



BOOL
IsWinIniHelper(
    IN LPSTR    FileName
    )


/*++

Routine Description:

    Determine if the name passed as argument refers to the file win.ini.
    Used by IS_WIN_INI macro, which assures the argument is non-null and
    deals with exact match of "win.ini".

Arguments:

    FileName -  File name to be examined.


Return Value:

    BOOL - Returns TRUE if 'Name' refers to win.ini.
              Otherwise, returns FALSE.

--*/

{
    CHAR    BufferForFullPath[MAX_PATH];
    PSTR    PointerToName;
    DWORD   SizeOfFullPath;

    BOOL    Result;

#ifdef DEBUG
    //
    // Filename argument must already be lowercase.  Be sure.
    //

    {
        char Lowercase[MAX_PATH];

        WOW32ASSERT(strlen(FileName) < MAX_PATH-1);
        strcpy(Lowercase, FileName);
        WOW32_strlwr(Lowercase);
        WOW32ASSERT(!WOW32_strcmp(FileName, Lowercase));
    }
#endif

    if (!WOW32_strcmp(FileName, szWinDotIni)) {
        Result = TRUE;
        goto Done;
    }

    SizeOfFullPath = GetFullPathName( FileName,
                                      sizeof BufferForFullPath,
                                      BufferForFullPath,
                                      &PointerToName );

    if (SizeOfFullPath == 0) {
        Result = FALSE;
        goto Done;
    }

    WOW32ASSERT( (SizeOfFullPath + 1) <= sizeof BufferForFullPath );

    WOW32ASSERTMSG(pszWinIniFullPath && pszWinIniFullPath[0],
                   "WOW32 ERROR pszWinIniFullPath not initialized.\n");

    Result = !WOW32_stricmp( pszWinIniFullPath, BufferForFullPath );

Done:
    return Result;
}



VOID
UpdateEmbeddingAllKeys(
        )

/*++

Routine Description:

    Update the "embedding" section of win.ini based on the information
    stored on the subkeys of HKEY_CLASSES_ROOT.

Arguments:

    None.


Return Value:

    None.

--*/

{
    LONG iClass;
    CHAR szClass[MAX_PATH + 1];
    LONG Status;

    for (iClass = 0;
        (Status = RegEnumKey(HKEY_CLASSES_ROOT,iClass,szClass,sizeof( szClass ))) != ERROR_NO_MORE_ITEMS;
        iClass++)
      {
        if( Status == ERROR_SUCCESS ) {
            UpdateEmbeddingKey( szClass );
        }
      }
}




VOID
UpdateEmbeddingKey(
    IN  LPSTR   KeyName
    )


/*++

Routine Description:

    Update one key of the "embedding" section of win.ini based on the
    information stored on the correspondent subkey of HKEY_CLASSES_ROOT.

    The code below is an improved version of the function
    "UpdateWinIni" extracted from Win 3.1 (shell\library\dbf.c).

Arguments:

    KeyName - Name of the key to be updated.


Return Value:

    None.

--*/

{
    LONG    Status;
    HKEY    Key;
    PSTR    szClass;

    LPSTR   szClassName;
    CHAR    BufferForClassName[BUFFER_SIZE];
//    char szClassName[60];

    LPSTR   szServer;
    CHAR    BufferForServer[BUFFER_SIZE];
//    char szServer[64];

    LPSTR   szLine;
    CHAR    BufferForLine[2*BUFFER_SIZE];
//    char szLine[128];

    char szOldLine[2*BUFFER_SIZE];
//    char szOldLine[128];
    LPSTR lpDesc, lpForms;
    int nCommas;

    LONG cchClassNameSize;
    LONG cchServerSize;
    LONG cchLineSize;


    if( KeyName == NULL ) {
        return;
    }

    szClass = KeyName;
    Key = NULL;

    szClassName = BufferForClassName;
    cchClassNameSize = sizeof( BufferForClassName );

    szServer = BufferForServer;
    cchServerSize = sizeof( BufferForServer );

    szLine = BufferForLine;


    if( RegOpenKey( HKEY_CLASSES_ROOT, szClass, &Key ) != ERROR_SUCCESS )
        goto NukeClass;

    Status = RegQueryValue(Key,NULL,szClassName,&cchClassNameSize);
    if( ( Status != ERROR_SUCCESS ) &&
        ( Status != ERROR_MORE_DATA ) )
        goto NukeClass;

    if( Status == ERROR_MORE_DATA ) {
        cchClassNameSize++;
        szClassName = ( PSTR )malloc_w( cchClassNameSize );
        if( szClassName == NULL )
            goto NukeClass;

        Status = RegQueryValue(Key,NULL,szClassName,&cchClassNameSize);
        if( Status != ERROR_SUCCESS )
            goto NukeClass;
    }

    if (!*szClassName)
        goto NukeClass;


    Status = RegQueryValue(Key,szServerKey,szServer,&cchServerSize);
    if( ( Status != ERROR_SUCCESS ) &&
        ( Status != ERROR_MORE_DATA ) )
        goto NukeClass;

    if( Status == ERROR_MORE_DATA ) {
        cchServerSize++;
        szServer = malloc_w( cchServerSize );
        if( szServer == NULL )
            goto NukeClass;

        Status = RegQueryValue(Key,szServerKey,szServer,&cchServerSize);
        if( Status != ERROR_SUCCESS )
            goto NukeClass;
    }

    if (!*szServer)
        goto NukeClass;


    if (GetProfileString(szEmbedding, szClass, EMPTY_STRING,
          szOldLine, sizeof(szOldLine)))
      {
        for (lpForms=szOldLine, nCommas=0; ; lpForms=AnsiNext(lpForms))
          {
            while (*lpForms == ',')
              {
                *lpForms++ = '\0';
                if (++nCommas == 3)
                    goto FoundForms;
              }
            if (!*lpForms)
                goto DoDefaults;
          }
FoundForms:
        lpDesc = szOldLine;
      }
    else
      {
DoDefaults:
        lpDesc = szClassName;
        lpForms = szPicture;
      }

    // we have a class, a classname, and a server, so its an le class

    cchLineSize = strlen( lpDesc ) +
                  strlen( szClassName ) +
                  strlen( szServer ) +
                  strlen( lpForms ) +
                  3 +
                  1;

    if( cchLineSize > sizeof( BufferForLine ) ) {
        szLine = malloc_w( cchLineSize );
        if( szLine == NULL )
            goto NukeClass;
    }
    wsprintf(szLine, "%s,%s,%s,%s",
             lpDesc, (LPSTR)szClassName, (LPSTR)szServer, lpForms);

    WriteProfileString(szEmbedding, szClass, szLine);
    if( Key != NULL ) {
        RegCloseKey( Key );
    }
    if( szClassName != BufferForClassName ) {
        free_w( szClassName );
    }
    if( szServer != BufferForServer ) {
        free_w( szServer );
    }
    if( szLine != BufferForLine ) {
        free_w( szLine );
    }
    return;

NukeClass:
/*
    Don't nuke the class because someone else may use it!

*/
    if( Key != NULL ) {
        RegCloseKey( Key );
    }
    if( szClassName != BufferForClassName ) {
        free_w( szClassName );
    }
    if( szServer != BufferForServer ) {
        free_w( szServer );
    }
    if( szLine != BufferForLine ) {
        free_w( szLine );
    }
    WriteProfileString(szEmbedding,szClass,NULL);
}



VOID
UpdateClassesRootSubKey(
    IN  LPSTR   KeyName,
    IN  LPSTR   Value
    )

/*++

Routine Description:

    Update a subkeys of HKEY_CLASSES_ROOT, based on the corresponding
    key in the "embedding" section of win.ini.

    The code below is an improved version of the function
    "UpdateFromWinIni" extracted from Win 3.1 (shell\library\dbf.c).

Arguments:

    KeyName - Name of the subkey to be updated

    Value - The value associated to the key, that was already written
            to the "embedding" section of win.ini.


Return Value:

    None.

--*/

{
    LPSTR   szLine;
    LPSTR lpClass,lpServer,lpClassName;
    LPSTR lpT;
    HKEY key = NULL;
    HKEY key1 = NULL;

    if( ( KeyName == NULL ) || ( Value == NULL ) ) {
        return;
    }

    lpClass = KeyName;
    szLine = Value;

    if (!(lpClassName=WOW32_strchr(szLine, ',')))
        return;
    // get the server name and null terminate the class name
    if (!(lpServer=WOW32_strchr(++lpClassName, ','))) {
        return;
    }
    *lpServer++ = '\0';

    // null terminate the server
    if (!(lpT=WOW32_strchr(lpServer, ','))) {
        return;
    }
    *lpT++ = '\0';

    // make sure the classname is nonblank
    while (*lpClassName == ' ')
            lpClassName++;
    if (!*lpClassName)
        return;

    // make sure the server name is nonblank
    while (*lpServer == ' ')
        lpServer++;
    if (!*lpServer)
        return;

    // we now have a valid entry
    key = NULL;
    if( ( RegCreateKey( HKEY_CLASSES_ROOT, lpClass, &key ) != ERROR_SUCCESS ) ||
        ( RegSetValue( key, NULL, REG_SZ, lpClassName, strlen( lpClassName ) ) != ERROR_SUCCESS ) ) {
        if( key != NULL ) {
            RegCloseKey( key );
        }
        return;
    }
    if( ( RegCreateKey( key, szServerKey, &key1 ) != ERROR_SUCCESS ) ||
        ( RegSetValue( key1, NULL, REG_SZ, lpServer, strlen( lpServer ) ) != ERROR_SUCCESS ) ) {
        if( key != NULL ) {
            RegCloseKey( key );
        }
        if( key1 != NULL ) {
            RegCloseKey( key1 );
        }
        return;
    }
    RegCloseKey( key );
    RegCloseKey( key1 );
}



VOID
SetLastTimeUpdated(
    )

/*++

Routine Description:

    Set the variable that contains the information of when the "embedding"
    section of win.ini was last updated.

Arguments:

    None.

Return Value:

    None.

--*/

{
    _LastTimeUpdated = GetTickCount();
}



BOOL
WasSectionRecentlyUpdated(
    )

/*++

Routine Description:

    Inform the caller whether the "embedding" section of win.ini
    was recently updated ( less than 2 seconds ).

Arguments:

    None.

Return Value:

    BOOLEAN - Returns TRUE if the "embedding" section was updated less than
              2 seconds ago.

--*/

{
    DWORD   Now;

    Now = GetTickCount();
    return( ( ( Now - _LastTimeUpdated ) < WININITIMEOUT ) ? TRUE : FALSE );
}
