#include <lnkdump.h>
#pragma hdrstop



void ReadString( HANDLE hFile, LPVOID * lpVoid, BOOL bUnicode )
{

    USHORT cch;
    DWORD  dwBytesRead;

    if (bUnicode)
    {
        LPWSTR lpWStr = NULL;

        ReadFile( hFile, (LPVOID)&cch, sizeof(cch), &dwBytesRead, NULL );
        lpWStr = (LPWSTR)HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, (cch+1)*sizeof(WCHAR) );
        if (lpWStr) {
            ReadFile( hFile, (LPVOID)lpWStr, cch*sizeof(WCHAR), &dwBytesRead, NULL );
            lpWStr[cch] = L'\0';
        }
        *(PDWORD_PTR)lpVoid = (DWORD_PTR)lpWStr;
    }
    else
    {
        LPSTR lpStr = NULL;

        ReadFile( hFile, (LPVOID)&cch, sizeof(cch), &dwBytesRead, NULL );
        lpStr = (LPSTR)HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, (cch+1) );
        if (lpStr) {
            ReadFile( hFile, (LPVOID)lpStr, cch, &dwBytesRead, NULL );
            lpStr[cch] = '\0';
        }
        *(PDWORD_PTR)lpVoid = (DWORD_PTR)lpStr;
    }

}


int __cdecl main( int argc, char *argv[])
{
    HANDLE hFile;
    CShellLink csl;
    CShellLink * this = &csl;
    DWORD cbSize, cbTotal, cbToRead, dwBytesRead;
    SYSTEMTIME st;
    LPSTR pTemp = NULL;

    this->pidl = 0;
    this->pli = NULL;
    memset( this, 0, sizeof(CShellLink) );
    if (argc!=2)
    {
        printf("usage: lnkdump filename.lnk\n" );
        return(1);
    }


    // Try to open the file
    hFile = CreateFile( argv[1],
                        GENERIC_READ,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL
                       );

    if (hFile == INVALID_HANDLE_VALUE)
    {
        printf( "lnkdump: unable to open file %s, err %d\n",
                argv[1],
                GetLastError()
               );
        return(1);
    }

    // Now, read out data...

    ReadFile( hFile, (LPVOID)&this->sld, sizeof(this->sld), &dwBytesRead, NULL );

    // read all of the members

    if (this->sld.dwFlags & SLDF_HAS_ID_LIST)
    {
        // Read the size of the IDLIST
        cbSize = 0; // need to zero out to get HIWORD 0 'cause USHORT is only 2 bytes
        ReadFile( hFile, (LPVOID)&cbSize, sizeof(USHORT), &dwBytesRead, NULL );
        if (cbSize)
        {
            SetFilePointer(hFile,cbSize,NULL,FILE_CURRENT);
        }
        else
        {
            printf( "Error readling PIDL out of link!\n" );
            return( 1 );
        }
    }

    // BUGBUG: this part is not unicode ready, talk to daviddi

    if (this->sld.dwFlags & (SLDF_HAS_LINK_INFO))
    {
        LPVOID pli;

        ReadFile( hFile, (LPVOID)&cbSize, sizeof(cbSize), &dwBytesRead, NULL );
        if (cbSize >= sizeof(cbSize))
        {
            cbSize -= sizeof(cbSize);
            SetFilePointer(hFile,cbSize,NULL,FILE_CURRENT);
        }

    }

    if (this->sld.dwFlags & SLDF_HAS_NAME)
        ReadString( hFile, &this->pszName, this->sld.dwFlags & SLDF_UNICODE);
    if (this->sld.dwFlags & SLDF_HAS_RELPATH)
        ReadString( hFile, &this->pszRelPath, this->sld.dwFlags & SLDF_UNICODE);
    if (this->sld.dwFlags & SLDF_HAS_WORKINGDIR)
        ReadString( hFile, &this->pszWorkingDir, this->sld.dwFlags & SLDF_UNICODE);
    if (this->sld.dwFlags & SLDF_HAS_ARGS)
        ReadString( hFile, &this->pszArgs, this->sld.dwFlags & SLDF_UNICODE);
    if (this->sld.dwFlags & SLDF_HAS_ICONLOCATION)
        ReadString( hFile, &this->pszIconLocation, this->sld.dwFlags & SLDF_UNICODE);

    // Read in extra data sections
    this->pExtraData = NULL;
    cbTotal = 0;
    while (TRUE)
    {

        LPSTR pReadData = NULL;

        cbSize = 0;
        ReadFile( hFile, (LPVOID)&cbSize, sizeof(cbSize), &dwBytesRead, NULL );

        if (cbSize < sizeof(cbSize))
            break;

        if (pTemp)
        {
            pTemp = (void *)HeapReAlloc( GetProcessHeap(),
                                         HEAP_ZERO_MEMORY,
                                         this->pExtraData,
                                         cbTotal + cbSize + sizeof(DWORD)
                                        );
            if (pTemp)
            {
                this->pExtraData = (LPDBLIST)pTemp;
            }
        }
        else
        {
            (LPVOID)this->pExtraData = pTemp = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, cbTotal + cbSize + sizeof(DWORD) );

        }

        if (!pTemp)
            break;

        cbToRead = cbSize - sizeof(cbSize);
        pReadData = pTemp + cbTotal;

        ReadFile( hFile, (LPVOID)(pReadData + sizeof(cbSize)), cbToRead, &dwBytesRead, NULL );
        if (dwBytesRead==cbToRead)
        {
            // got all of the extra data, comit it
            *((UNALIGNED DWORD *)pReadData) = cbSize;
            cbTotal += cbSize;
        }
        else
            break;

    }

    printf( "\n===== Dump of link file (%s) =====\n\n", argv[1] );

    printf( "[Shell Link Data (sld)]\n" );
    printf( "    cbSize           = 0x%08X\n", this->sld.cbSize );
    printf( "    GUID             = {%08lX-%04X-%04X-%02X%02X-%02X%02X%02x%02x%02x%02x}\n",
            this->sld.clsid.Data1, this->sld.clsid.Data2, this->sld.clsid.Data3,
            this->sld.clsid.Data4[0], this->sld.clsid.Data4[1],
            this->sld.clsid.Data4[2], this->sld.clsid.Data4[3],
            this->sld.clsid.Data4[4], this->sld.clsid.Data4[5],
            this->sld.clsid.Data4[6], this->sld.clsid.Data4[7]
           );
    printf( "    dwFlags          = 0x%08X\n", this->sld.dwFlags );
    if (this->sld.dwFlags & SLDF_HAS_ID_LIST)
        printf( "                           SLDF_HAS_ID_LIST          is set.\n" );
    if (this->sld.dwFlags & SLDF_HAS_LINK_INFO)
        printf( "                           SLDF_HAS_LINK_INFO        is set.\n" );
    if (this->sld.dwFlags & SLDF_HAS_NAME)
        printf( "                           SLDF_HAS_NAME             is set.\n" );
    if (this->sld.dwFlags & SLDF_HAS_RELPATH)
        printf( "                           SLDF_HAS_RELPATH          is set.\n" );
    if (this->sld.dwFlags & SLDF_HAS_WORKINGDIR)
        printf( "                           SLDF_HAS_WORKINGDIR       is set.\n" );
    if (this->sld.dwFlags & SLDF_HAS_ARGS)
        printf( "                           SLDF_HAS_ARGS             is set.\n" );
    if (this->sld.dwFlags & SLDF_HAS_ICONLOCATION)
        printf( "                           SLDF_HAS_ICONLOCATION     is set.\n" );
    if (this->sld.dwFlags & SLDF_HAS_EXP_SZ)
        printf( "                           SLDF_HAS_EXP_SZ           is set.\n" );
    if (this->sld.dwFlags & SLDF_RUN_IN_SEPARATE)
        printf( "                           SLDF_RUN_IN_SEPARATE      is set.\n" );
    if (this->sld.dwFlags & SLDF_UNICODE)
        printf( "                           SLDF_HAS_UNICODE          is set.\n" );
    if (this->sld.dwFlags & SLDF_FORCE_NO_LINKINFO)
        printf( "                           SLDF_FORCE_NO_LINKINFO    is set.\n" );
    printf( "    dwFileAttributes = 0x%08X\n", this->sld.dwFileAttributes );
    if (this->sld.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
        printf( "                           FILE_ATTRIBUTE_READONLY   is set.\n" );
    if (this->sld.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
        printf( "                           FILE_ATTRIBUTE_HIDDEN     is set.\n" );
    if (this->sld.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM)
        printf( "                           FILE_ATTRIBUTE_SYSTEM     is set.\n" );
    if (this->sld.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        printf( "                           FILE_ATTRIBUTE_DIRECTORY  is set.\n" );
    if (this->sld.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE)
        printf( "                           FILE_ATTRIBUTE_ARCHIVE    is set.\n" );
    if (this->sld.dwFileAttributes & FILE_ATTRIBUTE_NORMAL)
        printf( "                           FILE_ATTRIBUTE_NORMAL     is set.\n" );
    if (this->sld.dwFileAttributes & FILE_ATTRIBUTE_TEMPORARY)
        printf( "                           FILE_ATTRIBUTE_TEMPORARY  is set.\n" );
    if (this->sld.dwFileAttributes & FILE_ATTRIBUTE_COMPRESSED)
        printf( "                           FILE_ATTRIBUTE_COMPRESSED is set.\n" );
    FileTimeToSystemTime( &this->sld.ftCreationTime, &st );
    printf("    ftCreationTime   = %02d:%02d:%02d, %02d/%02d/%04d\n", st.wHour, st.wMinute, st.wSecond, st.wMonth, st.wDay, st.wYear );
    FileTimeToSystemTime( &this->sld.ftLastAccessTime, &st );
    printf("    ftLastAccessTime = %02d:%02d:%02d, %02d/%02d/%04d\n", st.wHour, st.wMinute, st.wSecond, st.wMonth, st.wDay, st.wYear );
    FileTimeToSystemTime( &this->sld.ftLastWriteTime, &st );
    printf("    ftLastWriteTime  = %02d:%02d:%02d, %02d/%02d/%04d\n", st.wHour, st.wMinute, st.wSecond, st.wMonth, st.wDay, st.wYear );
    printf("    nFileSizeLow     = %d bytes\n", this->sld.nFileSizeLow );
    printf("    iIcon            = %d\n", this->sld.iIcon );
    printf("    wHotkey          = 0x08%X\n", this->sld.wHotkey );
    printf("    dwRes1           = 0x08%X\n", this->sld.dwRes1 );
    printf("    dwRes2           = 0x08%X\n", this->sld.dwRes2 );

    printf("\n[Strings]\n");
    if (this->pszName)
    {
        printf( "    pszName          = " );
        if (this->sld.dwFlags & SLDF_UNICODE)
        {
            CHAR szTemp[ 256 ];

            WideCharToMultiByte( CP_ACP, 0,
                                 (LPWSTR)this->pszName,
                                 -1,
                                 szTemp,
                                 256,
                                 NULL,
                                 NULL
                                );
            printf( "(UNICODE) %s\n", szTemp );

        }
        else
            printf( "%s\n", (LPSTR)this->pszName );
    }

    if (this->pszRelPath)
    {
        printf( "    pszRelPath       = " );
        if (this->sld.dwFlags & SLDF_UNICODE)
        {
            CHAR szTemp[ 256 ];

            WideCharToMultiByte( CP_ACP, 0,
                                 (LPWSTR)this->pszRelPath,
                                 -1,
                                 szTemp,
                                 256,
                                 NULL,
                                 NULL
                                );
            printf( "(UNICODE) %s\n", szTemp );

        }
        else
            printf( "%s\n", (LPSTR)this->pszRelPath );
    }

    if (this->pszWorkingDir)
    {
        printf( "    pszWorkingDir    = " );
        if (this->sld.dwFlags & SLDF_UNICODE)
        {
            CHAR szTemp[ 256 ];

            WideCharToMultiByte( CP_ACP, 0,
                                 (LPWSTR)this->pszWorkingDir,
                                 -1,
                                 szTemp,
                                 256,
                                 NULL,
                                 NULL
                                );
            printf( "(UNICODE) %s\n", szTemp );

        }
        else
            printf( "%s\n", (LPSTR)this->pszWorkingDir );
    }

    if (this->pszArgs)
    {
        printf( "    pszArgs          = " );
        if (this->sld.dwFlags & SLDF_UNICODE)
        {
            CHAR szTemp[ 256 ];

            WideCharToMultiByte( CP_ACP, 0,
                                 (LPWSTR)this->pszArgs,
                                 -1,
                                 szTemp,
                                 256,
                                 NULL,
                                 NULL
                                );
            printf( "(UNICODE) %s\n", szTemp );

        }
        else
            printf( "%s\n", (LPSTR)this->pszArgs );
    }

    if (this->pszIconLocation)
    {
        printf( "    pszIconLocation  = " );
        if (this->sld.dwFlags & SLDF_UNICODE)
        {
            CHAR szTemp[ 256 ];

            WideCharToMultiByte( CP_ACP, 0,
                                 (LPWSTR)this->pszIconLocation,
                                 -1,
                                 szTemp,
                                 256,
                                 NULL,
                                 NULL
                                );
            printf( "(UNICODE) %s\n", szTemp );

        }
        else
            printf( "%s\n", (LPSTR)this->pszIconLocation );
    }


    if (this->pExtraData)
    {
        LPEXP_SZ_LINK lpData = (LPEXP_SZ_LINK)this->pExtraData;

        while( lpData && lpData->cbSize!=0 )
        {
            switch( lpData->dwSignature )
            {

            case EXP_SZ_LINK_SIG:
                {
                    CHAR szTemp[ 256 ];


                    printf("\n[Extra Data -- EXP_SZ_LINK_SIG info]\n");
                    printf("    cbSize      = 0x%X bytes\n", lpData->cbSize );
                    printf("    dwSignature = 0x%X\n", lpData->dwSignature );
                    printf("    szTarget    = %s\n", lpData->szTarget );
                    WideCharToMultiByte( CP_ACP, 0, lpData->swzTarget, -1, szTemp, 256, NULL, NULL );
                    printf("    swzTarget   = (UNICODE) %s\n\n", szTemp );
                }
                break;

            case NT_CONSOLE_PROPS_SIG:
                {
                    CHAR szTemp[ 256 ];
                    INT i;
                    LPNT_CONSOLE_PROPS lpConData = (LPNT_CONSOLE_PROPS)lpData;

                    printf("\n[Extra Data -- NT_CONSOLE_PROPS_SIG info]\n");
                    printf("    cbSize              = 0x%X bytes\n", lpConData->cbSize );
                    printf("    dwSignature         = 0x%X\n", lpConData->dwSignature );
                    printf("    wFillAttribute      = 0x%04X\n", lpConData->wFillAttribute );
                    printf("    wPopupFillAttribute = 0x%04X\n", lpConData->wPopupFillAttribute );
                    printf("    dwScreenBufferSize  = (%d , %d)\n", lpConData->dwScreenBufferSize.X, lpConData->dwScreenBufferSize.Y );
                    printf("    dwWindowSize        = (%d , %d)\n", lpConData->dwWindowSize.X, lpConData->dwWindowSize.Y );
                    printf("    dwWindowOrigin      = (%d , %d)\n", lpConData->dwWindowOrigin.X, lpConData->dwWindowOrigin.Y );
                    printf("    nFont               = %d\n", lpConData->nFont );
                    printf("    nInputBufferSize    = %d\n", lpConData->nInputBufferSize );
                    printf("    dwFontSize          = (%d , %d)\n", lpConData->dwFontSize.X, lpConData->dwFontSize.Y );
                    printf("    uFontFamily         = 0x%08X\n", lpConData->uFontFamily );
                    printf("    uFontWeight         = 0x%08X\n", lpConData->uFontWeight );
                    WideCharToMultiByte( CP_ACP, 0, lpConData->FaceName, LF_FACESIZE, szTemp, LF_FACESIZE, NULL, NULL );
                    szTemp[ LF_FACESIZE ] = (CHAR)0;
                    printf("    FaceName            = %s\n", szTemp );
                    printf("    uCursorSize         = %d\n", lpConData->uCursorSize );
                    printf("    bFullScreen         = %s\n", lpConData->bFullScreen ? "TRUE" : "FALSE" );
                    printf("    bQuickEdit          = %s\n", lpConData->bQuickEdit  ? "TRUE" : "FALSE" );
                    printf("    bInsertMode         = %s\n", lpConData->bInsertMode ? "TRUE" : "FALSE" );
                    printf("    bAutoPosition       = %s\n", lpConData->bAutoPosition ? "TRUE" : "FALSE" );
                    printf("    uHistoryBufferSize  = %d\n", lpConData->uHistoryBufferSize );
                    printf("    uNumHistoryBuffers  = %d\n", lpConData->uNumberOfHistoryBuffers );
                    printf("    bHistoryNoDup       = %s\n", lpConData->bHistoryNoDup ? "TRUE" : "FALSE" );
                    printf("    ColorTable = [" );
                    i=0;
                    while( i < 16 )
                    {
                        printf("\n         ");
                        printf("0x%08X ", lpConData->ColorTable[i++]);
                        printf("0x%08X ", lpConData->ColorTable[i++]);
                        printf("0x%08X ", lpConData->ColorTable[i++]);
                        printf("0x%08X ", lpConData->ColorTable[i++]);
                    }
                    printf( "]\n\n" );
                }
                break;

            case NT_FE_CONSOLE_PROPS_SIG:
                {
                    LPNT_FE_CONSOLE_PROPS lpFEConData = (LPNT_FE_CONSOLE_PROPS)lpData;

                    printf("\n[Extra Data -- NT_FE_CONSOLE_PROPS_SIG info]\n");
                    printf("    cbSize              = 0x%X bytes\n", lpFEConData->cbSize );
                    printf("    dwSignature         = 0x%X\n", lpFEConData->dwSignature );
                    printf("    CodePage            = %d\n\n", lpFEConData->uCodePage );
                }
                break;

            case EXP_DARWIN_ID_SIG:
                {
                    CHAR szTemp[ 256 ];
                    LPEXP_DARWIN_LINK lpDarwin = (LPEXP_DARWIN_LINK)lpData;

                    printf("\n[Extra Data -- EXP_DARWIN_ID_SIG info]\n");
                    printf("    szDarwinID           = %s\n", lpDarwin->szDarwinID );
                    WideCharToMultiByte( CP_ACP, 0, lpDarwin->szwDarwinID, -1, szTemp, 256, NULL, NULL );
                    printf("    szwDarwinID          = (UNICODE) %s\n\n", szTemp );
                }
                break;

            case EXP_SPECIAL_FOLDER_SIG:
                {
                    LPEXP_SPECIAL_FOLDER lpFolder = (LPEXP_SPECIAL_FOLDER)lpData;

                    printf("\n[Extra Data -- EXP_SPECIAL_FOLDER_SIG info]\n");
                    printf("    idSpecialFolder      = 0x%X\n", lpFolder->idSpecialFolder );
                    printf("    cbOffset             = 0x%X\n\n", lpFolder->cbOffset );
                }
                break;

#ifdef WINNT
            case EXP_TRACKER_SIG:
                {
                    LPEXP_TRACKER lpTracker = (LPEXP_TRACKER)lpData;
                    WCHAR wszGUID[ MAX_PATH ];

                    printf("\n[Extra Data -- EXP_TRACKER_SIG info]\n");
#if 0
                    // BUGBUG (reinerf) - traker info looks like a byte-array, how do we dump it?
                    printf("    abTracker           = 0x%X\n", lpTracker->abTracker);
#endif
                }
#endif



            } // end swtich

            (LPBYTE)lpData += lpData->cbSize;
        }

        LocalFree( this->pExtraData );
    }

    if (this->pidl)
        LocalFree( (HLOCAL)this->pidl );

    if (this->pli)
        LocalFree( this->pli );

    if (this->pszName)
        HeapFree( GetProcessHeap(), 0, this->pszName );
    if (this->pszRelPath)
        HeapFree( GetProcessHeap(), 0, this->pszRelPath );
    if (this->pszWorkingDir)
        HeapFree( GetProcessHeap(), 0, this->pszWorkingDir );
    if (this->pszArgs)
        HeapFree( GetProcessHeap(), 0, this->pszArgs );
    if (this->pszIconLocation)
        HeapFree( GetProcessHeap(), 0, this->pszIconLocation );

    return(0);
}

