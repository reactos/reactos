//---------------------------------------------------------------------------
//
// link.c       link management routines (reading, etc.)
//
// Copyright (c) 1985 - 1999, Microsoft Corporation
//
//---------------------------------------------------------------------------



#include "precomp.h"
#pragma hdrstop


DWORD
GetTitleFromLinkName(
    IN  LPWSTR szLinkName,
    OUT LPWSTR szTitle
    )
/*++

Routine Description:

    This routine returns the title (i.e., display name of the link) in szTitle,
    and the number of bytes (not chars) in szTitle.

Arguments:

    szLinkName - fully qualified path to link file
    szTitle    - pointer to buffer to contain title (display name) of the link

    i.e.:
    "C:\nt\desktop\A Link File Name.lnk" --> "A Link File Name"

Return Value:

    number of bytes copied to szTitle

--*/
{
    DWORD dwLen;
    LPWSTR pLnk, pDot;
    LPWSTR pPath = szLinkName;

    // Error checking
    ASSERT(szLinkName);

    // find filename at end of fully qualified link name and point pLnk to it
    for (pLnk = pPath; *pPath; pPath++)
    {
        if ( (pPath[0] == L'\\' || pPath[0] == L':') &&
              pPath[1] &&
             (pPath[1] != L'\\')
            )
            pLnk = pPath + 1;
    }

    // find extension (.lnk)
    pPath = pLnk;
    for (pDot = NULL; *pPath; pPath++)
    {
        switch (*pPath) {
        case L'.':
            pDot = pPath;       // remember the last dot
            break;
        case L'\\':
        case L' ':              // extensions can't have spaces
            pDot = NULL;        // forget last dot, it was in a directory
            break;
        }
    }

    // if we found the extension, pDot points to it, if not, pDot
    // is NULL.

    if (pDot)
    {
        dwLen = (ULONG)((pDot - pLnk) * sizeof(WCHAR));
    }
    else
    {
        dwLen = lstrlenW(pLnk) * sizeof(WCHAR);
    }
    dwLen = min(dwLen, MAX_TITLE_LENGTH);

    RtlCopyMemory(szTitle, pLnk, dwLen);

    return dwLen;
}


BOOL ReadString( HANDLE hFile, LPVOID * lpVoid, BOOL bUnicode )
{

    USHORT cch;
    DWORD  dwBytesRead;
    BOOL   fResult = TRUE;

    if (bUnicode)
    {
        LPWSTR lpWStr = NULL;

        fResult &= ReadFile( hFile, (LPVOID)&cch, sizeof(cch), &dwBytesRead, NULL );
        lpWStr = (LPWSTR)ConsoleHeapAlloc( HEAP_ZERO_MEMORY, (cch+1)*sizeof(WCHAR) );
        if (lpWStr) {
            fResult &= ReadFile( hFile, (LPVOID)lpWStr, cch*sizeof(WCHAR), &dwBytesRead, NULL );
            lpWStr[cch] = L'\0';
        }
        *(LPWSTR *)lpVoid = lpWStr;
    }
    else
    {
        LPSTR lpStr = NULL;

        fResult &= ReadFile( hFile, (LPVOID)&cch, sizeof(cch), &dwBytesRead, NULL );
        lpStr = (LPSTR)ConsoleHeapAlloc( HEAP_ZERO_MEMORY, (cch+1) );
        if (lpStr) {
            fResult &= ReadFile( hFile, (LPVOID)lpStr, cch, &dwBytesRead, NULL );
            lpStr[cch] = '\0';
        }
        *(LPSTR *)lpVoid = lpStr;
    }

    return fResult;

}


BOOL LoadLink( LPWSTR pszLinkName, CShellLink * this )
{

    HANDLE hFile;
    DWORD dwBytesRead, cbSize, cbTotal, cbToRead;
    BOOL fResult = TRUE;
    LPSTR pTemp = NULL;

    // Try to open the file
    hFile = CreateFile( pszLinkName,
                        GENERIC_READ,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL
                       );

    if (hFile==INVALID_HANDLE_VALUE)
        return FALSE;


    // Now, read out data...

    fResult = ReadFile( hFile, (LPVOID)&this->sld, sizeof(this->sld), &dwBytesRead, NULL );
    fResult &= ((dwBytesRead == sizeof(this->sld)) && (this->sld.cbSize == sizeof(this->sld)));

    // read all of the members

    if (this->sld.dwFlags & SLDF_HAS_ID_LIST)
    {
        // Read the size of the IDLIST
        cbSize = 0; // need to zero out to get HIWORD 0 'cause USHORT is only 2 bytes
        fResult &= ReadFile( hFile, (LPVOID)&cbSize, sizeof(USHORT), &dwBytesRead, NULL );
        fResult &= (dwBytesRead == sizeof(USHORT));
        if (cbSize)
        {
            fResult &=
                (SetFilePointer(hFile,cbSize,NULL,FILE_CURRENT)!=0xFFFFFFFF);
        }
        else
        {
            if (hFile)
                CloseHandle( hFile );
            return FALSE;
        }
    }

    // BUGBUG: this part is not unicode ready, talk to daviddi

    if (this->sld.dwFlags & (SLDF_HAS_LINK_INFO))
    {

        fResult &= ReadFile( hFile, (LPVOID)&cbSize, sizeof(cbSize), &dwBytesRead, NULL );
        fResult &= (dwBytesRead == sizeof(cbSize));
        if (cbSize >= sizeof(cbSize))
        {
            cbSize -= sizeof(cbSize);
            fResult &=
                (SetFilePointer(hFile,cbSize,NULL,FILE_CURRENT)!=0xFFFFFFFF);
        }

    }

    if (this->sld.dwFlags & SLDF_HAS_NAME)
        fResult &= ReadString( hFile, &this->pszName, this->sld.dwFlags & SLDF_UNICODE);
    if (this->sld.dwFlags & SLDF_HAS_RELPATH)
        fResult &= ReadString( hFile, &this->pszRelPath, this->sld.dwFlags & SLDF_UNICODE);
    if (this->sld.dwFlags & SLDF_HAS_WORKINGDIR)
        fResult &= ReadString( hFile, &this->pszWorkingDir, this->sld.dwFlags & SLDF_UNICODE);
    if (this->sld.dwFlags & SLDF_HAS_ARGS)
        fResult &= ReadString( hFile, &this->pszArgs, this->sld.dwFlags & SLDF_UNICODE);
    if (this->sld.dwFlags & SLDF_HAS_ICONLOCATION)
        fResult &= ReadString( hFile, &this->pszIconLocation, this->sld.dwFlags & SLDF_UNICODE);

    // Read in extra data sections
    this->pExtraData = NULL;
    cbTotal = 0;
    while (TRUE)
    {

        LPSTR pReadData = NULL;

        cbSize = 0;
        fResult &= ReadFile( hFile, (LPVOID)&cbSize, sizeof(cbSize), &dwBytesRead, NULL );

        if (cbSize < sizeof(cbSize))
            break;

        if (pTemp)
        {
            pTemp = (void *)ConsoleHeapReAlloc(
                                         HEAP_ZERO_MEMORY,
                                         this->pExtraData,
                                         cbTotal + cbSize + sizeof(DWORD)
                                        );
            if (pTemp)
            {
                this->pExtraData = pTemp;
            }
        }
        else
        {
            this->pExtraData = pTemp = ConsoleHeapAlloc( HEAP_ZERO_MEMORY, cbTotal + cbSize + sizeof(DWORD) );

        }

        if (!pTemp)
            break;

        cbToRead = cbSize - sizeof(cbSize);
        pReadData = pTemp + cbTotal;

        fResult &= ReadFile( hFile, (LPVOID)(pReadData + sizeof(cbSize)), cbToRead, &dwBytesRead, NULL );
        if (dwBytesRead==cbToRead)
        {
            // got all of the extra data, comit it
            *((UNALIGNED DWORD *)pReadData) = cbSize;
            cbTotal += cbSize;
        }
        else
            break;

    }


    if (hFile)
        CloseHandle( hFile );

    return fResult;

}



DWORD GetLinkProperties( LPWSTR pszLinkName, LPVOID lpvBuffer, UINT cb )
{
    CShellLink mld;
    DWORD fResult;
    LPNT_CONSOLE_PROPS lpExtraData;
    DWORD dwSize = 0;

/*
    {
        TCHAR szRick[ 1024 ];
        STARTUPINFO si;
        PROCESS_INFORMATION pi;

        // HUGE, HUGE hack-o-rama to get NTSD started on this process!
        wsprintf( szRick, TEXT("ntsd -d -p %d"), GetCurrentProcessId() );
        GetStartupInfo( &si );
        CreateProcess( NULL, szRick, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi );
        CloseHandle( pi.hProcess );
        CloseHandle( pi.hThread );
        Sleep( 5*1000 );
        DebugBreak();

    }
*/



    // Zero out structure on the stack
    RtlZeroMemory( &mld, sizeof(mld) );

    // Load link data
    if (!LoadLink( pszLinkName, &mld )) {
        RIPMSG1(RIP_WARNING, "LoadLink %ws failed", pszLinkName);
        fResult = LINK_NOINFO;
        goto Cleanup;
    }

    // Check return buffer -- is it big enough?
    ASSERT(cb >= sizeof( LNKPROPNTCONSOLE));

    // Zero out callers buffer
    RtlZeroMemory( lpvBuffer, cb );

    // Copy relevant shell link data into caller's buffer
    if (mld.pszName)
        lstrcpy( ((LPLNKPROPNTCONSOLE)lpvBuffer)->pszName, mld.pszName );
    if (mld.pszIconLocation)
        lstrcpy( ((LPLNKPROPNTCONSOLE)lpvBuffer)->pszIconLocation, mld.pszIconLocation );
    ((LPLNKPROPNTCONSOLE)lpvBuffer)->uIcon = mld.sld.iIcon;
    ((LPLNKPROPNTCONSOLE)lpvBuffer)->uShowCmd = mld.sld.iShowCmd;
    ((LPLNKPROPNTCONSOLE)lpvBuffer)->uHotKey = mld.sld.wHotkey;
    fResult = LINK_SIMPLEINFO;

    // Find console properties in extra data section
    for( lpExtraData = (LPNT_CONSOLE_PROPS)mld.pExtraData;
         lpExtraData && lpExtraData->cbSize;
         (LPBYTE)lpExtraData += dwSize
        )
    {
        dwSize = lpExtraData->cbSize;
        if (dwSize)
        {
            if (lpExtraData->dwSignature == NT_CONSOLE_PROPS_SIG)
            {

                RtlCopyMemory( &((LPLNKPROPNTCONSOLE)lpvBuffer)->console_props,
                               lpExtraData,
                               sizeof( NT_CONSOLE_PROPS )
                             );
                fResult = LINK_FULLINFO;
#if !defined(FE_SB)
                break;
#endif
            }
#if defined(FE_SB)
            if (lpExtraData->dwSignature == NT_FE_CONSOLE_PROPS_SIG)
            {
                LPNT_FE_CONSOLE_PROPS lpFEExtraData = (LPNT_FE_CONSOLE_PROPS)lpExtraData;

                RtlCopyMemory( &((LPLNKPROPNTCONSOLE)lpvBuffer)->fe_console_props,
                               lpFEExtraData,
                               sizeof( NT_FE_CONSOLE_PROPS )
                             );
            }
#endif
        }
    }

#if 0
#define ConsoleInfo ((LPLNKPROPNTCONSOLE)lpvBuffer)
    {
        TCHAR szTemp[ 256 ];
        INT i;

        wsprintf( szTemp, TEXT("[GetLinkProperties -- Link Properties for %s]\n"), pszLinkName );
        OutputDebugString( szTemp );
        wsprintf( szTemp, TEXT("    wFillAttribute      = 0x%04X\n"), ConsoleInfo->console_props.wFillAttribute );
        OutputDebugString( szTemp );
        wsprintf( szTemp, TEXT("    wPopupFillAttribute = 0x%04X\n"), ConsoleInfo->console_props.wPopupFillAttribute );
        OutputDebugString( szTemp );
        wsprintf( szTemp, TEXT("    dwScreenBufferSize  = (%d , %d)\n"), ConsoleInfo->console_props.dwScreenBufferSize.X, ConsoleInfo->console_props.dwScreenBufferSize.Y );
        OutputDebugString( szTemp );
        wsprintf( szTemp, TEXT("    dwWindowSize        = (%d , %d)\n"), ConsoleInfo->console_props.dwWindowSize.X, ConsoleInfo->console_props.dwWindowSize.Y );
        OutputDebugString( szTemp );
        wsprintf( szTemp, TEXT("    dwWindowOrigin      = (%d , %d)\n"), ConsoleInfo->console_props.dwWindowOrigin.X, ConsoleInfo->console_props.dwWindowOrigin.Y );
        OutputDebugString( szTemp );
        wsprintf( szTemp, TEXT("    nFont               = 0x%X\n"), ConsoleInfo->console_props.nFont );
        OutputDebugString( szTemp );
        wsprintf( szTemp, TEXT("    nInputBufferSize    = 0x%X\n"), ConsoleInfo->console_props.nInputBufferSize );
        OutputDebugString( szTemp );
        wsprintf( szTemp, TEXT("    dwFontSize          = (%d , %d)\n"), ConsoleInfo->console_props.dwFontSize.X, ConsoleInfo->console_props.dwFontSize.Y );
        OutputDebugString( szTemp );
        wsprintf( szTemp, TEXT("    uFontFamily         = 0x%08X\n"), ConsoleInfo->console_props.uFontFamily );
        OutputDebugString( szTemp );
        wsprintf( szTemp, TEXT("    uFontWeight         = 0x%08X\n"), ConsoleInfo->console_props.uFontWeight );
        OutputDebugString( szTemp );
        wsprintf( szTemp, TEXT("    FaceName            = %ws\n"), ConsoleInfo->console_props.FaceName );
        OutputDebugString( szTemp );
        wsprintf( szTemp, TEXT("    uCursorSize         = %d\n"), ConsoleInfo->console_props.uCursorSize );
        OutputDebugString( szTemp );
        wsprintf( szTemp, TEXT("    bFullScreen         = %s\n"), ConsoleInfo->console_props.bFullScreen ? TEXT("TRUE") : TEXT("FALSE") );
        OutputDebugString( szTemp );
        wsprintf( szTemp, TEXT("    bQuickEdit          = %s\n"), ConsoleInfo->console_props.bQuickEdit  ? TEXT("TRUE") : TEXT("FALSE") );
        OutputDebugString( szTemp );
        wsprintf( szTemp, TEXT("    bInsertMode         = %s\n"), ConsoleInfo->console_props.bInsertMode ? TEXT("TRUE") : TEXT("FALSE") );
        OutputDebugString( szTemp );
        wsprintf( szTemp, TEXT("    bAutoPosition       = %s\n"), ConsoleInfo->console_props.bAutoPosition ? TEXT("TRUE") : TEXT("FALSE") );
        OutputDebugString( szTemp );
        wsprintf( szTemp, TEXT("    uHistoryBufferSize  = %d\n"), ConsoleInfo->console_props.uHistoryBufferSize );
        OutputDebugString( szTemp );
        wsprintf( szTemp, TEXT("    uNumHistoryBuffers  = %d\n"), ConsoleInfo->console_props.uNumberOfHistoryBuffers );
        OutputDebugString( szTemp );
        wsprintf( szTemp, TEXT("    bHistoryNoDup       = %s\n"), ConsoleInfo->console_props.bHistoryNoDup ? TEXT("TRUE") : TEXT("FALSE") );
        OutputDebugString( szTemp );
        OutputDebugString( TEXT("    ColorTable = [") );
        i=0;
        while( i < 16 )
        {
            OutputDebugString( TEXT("\n         ") );
            wsprintf( szTemp, TEXT("0x%08X "), ConsoleInfo->console_props.ColorTable[i++]);
            OutputDebugString( szTemp );
            wsprintf( szTemp, TEXT("0x%08X "), ConsoleInfo->console_props.ColorTable[i++]);
            OutputDebugString( szTemp );
            wsprintf( szTemp, TEXT("0x%08X "), ConsoleInfo->console_props.ColorTable[i++]);
            OutputDebugString( szTemp );
            wsprintf( szTemp, TEXT("0x%08X "), ConsoleInfo->console_props.ColorTable[i++]);
            OutputDebugString( szTemp );
        }
        OutputDebugString( TEXT("]\n\n") );
    }
#undef ConsoleInfo
#endif

Cleanup:
    if (mld.pszName)
        ConsoleHeapFree( mld.pszName );
    if (mld.pszRelPath)
        ConsoleHeapFree( mld.pszRelPath );
    if (mld.pszWorkingDir)
        ConsoleHeapFree( mld.pszWorkingDir );
    if (mld.pszArgs)
        ConsoleHeapFree( mld.pszArgs );
    if (mld.pszIconLocation)
        ConsoleHeapFree( mld.pszIconLocation );
    if (mld.pExtraData)
        ConsoleHeapFree( mld.pExtraData );

    return fResult;

}
