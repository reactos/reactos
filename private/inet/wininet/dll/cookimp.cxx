#include <wininetp.h>
#include <wininet.h>

//#include "..\inc\cookimp.h"

#define NS_COOKIE_IMPORT_KEY        TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\5.0\\NSCookieUpgrade")
#define NS_COOKIE_IMPORT_VERSION    TEXT("Version")
#define NS_COOKIE_IMPORT_FLAG       TEXT("DisableUpgrade")

#define HTML_FILE_EXTENSION         TEXT(".htm")
#define NS_EXE_FILENAME             TEXT("Netscape.exe")
#define NS_COOKIE_FILENAME          TEXT("cookies.txt")

#define OPEN_COMMAND_OF_APP         TEXT("\\shell\\open\\command")
#define NS_APP_PATHS_REG            TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\Netscape.exe")

#define NS3_COOKIE_REG_PATH         TEXT("Software\\Netscape\\Netscape Navigator\\Cookies")
#define NS3_COOKIE_REG_KEY          TEXT("Cookie File")
#define NS4_USERS_REG_PATH          TEXT("Software\\Netscape\\Netscape Navigator\\Users")
#define NS4_USERPATH_REG_KEY        TEXT("DirRoot")

//  note:  LENGTHOFSTR()'s result includes terminating character
#define LENGTHOFTSTR( x ) (sizeof(x) / sizeof(TCHAR))

bool EndOfLine( CHAR chr );
BOOL InternetTimeFromTime_tA( IN DWORD dwTime, OUT LPSTR lpszTime, /* in-out */ DWORD cbBufferSize );


//--------------------------------------------------------------------
//
//  ImportCookieFile
//

//  parses a Netscape 'cookies.txt' file and adds each cookie using InternetSetCookie
BOOL ImportCookieFileW( IN LPCWSTR szFilename)
{
    MEMORYPACKET mpFilename;
    ALLOC_MB(szFilename,0,mpFilename);
    if (!mpFilename.psStr)
    {
        return FALSE;
    }
    UNICODE_TO_ANSI(szFilename,mpFilename);

    return ImportCookieFileA( mpFilename.psStr);
};


BOOL ImportCookieFileA( IN LPCSTR szFilename )
{
    BOOL retVal = FALSE;

    HANDLE hFile = (HKEY) INVALID_HANDLE_VALUE;
    LPSTR szFile = NULL;
    DWORD cbFileSize;

    if( ReadFileToBuffer( szFilename, (BYTE**)&szFile, &cbFileSize) != TRUE)
        goto doneImportFile;

    // parse file line by line
    DWORD iPosition;
    iPosition = 0;
    while( iPosition < cbFileSize )
    {
        LPSTR szValue = NULL;
        LPSTR szURL = NULL;

        //   blank lines are ignored
        if( EndOfLine( szFile[iPosition]))
        {
            iPosition++;
            continue;
        }

        //   lines beginning with # are comments
        if( szFile[iPosition] == '#')
        {
            while ( iPosition < cbFileSize && !EndOfLine( szFile[iPosition]))
                iPosition++;

            iPosition++;
            continue;
        }

        //   see CookieFormat text file for the spec of a Netscape 'cookies.txt'
        const DWORD iArgCount = 7;
        enum eArgTypes { DOMAIN=0,UNUSED=1,PATH=2,SECURE=3,EXPIRES=4,NAME=5,VALUE=6 };

        //  parse cookie into 7 fields (szArg[0..6]) with lengths stored in cbArgLength[0..6]
        LPSTR szArg[iArgCount];
        DWORD cbArgLength[iArgCount];

        // Continue parsing fields only if end of line/file hasn't been hit.
        // 'for' is re-entered when the end of each cookie is found:
        //      On re-entry, null terminate the cookie found.
        for( DWORD iArgIndex = 0;
                iPosition < cbFileSize && !EndOfLine( szFile[iPosition]);
                iArgIndex++, szFile[iPosition++] = '\0')
        {
            //  sanity check: are there really only 7 fields?
            if( iArgIndex >= iArgCount )
            {
                INET_ASSERT(FALSE);

                // if there are more than 7 fields, move to where a new cookie should start
                while ( iPosition < cbFileSize && !EndOfLine( szFile[iPosition]))
                    iPosition++;

                // end of invalid cookie found, now increment to next cookie.
                iPosition++;

                goto doneImportCookie;
            }

            //  Mark the beginning of the field.
            szArg[iArgIndex] = &szFile[iPosition];

            //  Find the end of the field.
            while( iPosition < cbFileSize && szFile[iPosition] != '\t' && !EndOfLine( szFile[iPosition]) )
                iPosition++;

            // Calculate the length of the cookie.
            cbArgLength[iArgIndex] = (unsigned long) (&szFile[iPosition] - szArg[iArgIndex]);
        }

        if( iArgIndex < iArgCount )
        {
            //sanity check: Are there enough fields?
            INET_ASSERT(FALSE);
            goto doneImportCookie;
        }

        // build HTTP cookie string in szValue
        DWORD cbValueSize;
        cbValueSize =  cbArgLength[NAME]
                    + cbArgLength[VALUE]
                    + cbArgLength[DOMAIN]
                    + sizeof("=;secure;expires=;domain=")
                    + INTERNET_RFC1123_BUFSIZE
                    + 1;
        szValue = new CHAR[ cbValueSize];

        if( szValue == NULL )
            goto doneImportCookie;

        LPSTR pValueIterator;
        pValueIterator = szValue;

        // add 'name=value' pair
        memcpy( pValueIterator, szArg[NAME], cbArgLength[NAME] );
        pValueIterator += cbArgLength[NAME];
        pValueIterator[0] = '=';
        pValueIterator++;
        memcpy( pValueIterator, szArg[VALUE], cbArgLength[VALUE] );
        pValueIterator += cbArgLength[VALUE];

        //  set security information
        if( szArg[SECURE][0] == 'T' || szArg[SECURE][0] == 't' )
        {
            memcpy( pValueIterator, ";secure", sizeof(";secure") - 1);
            pValueIterator += sizeof(";secure") - 1;
        }

        //  In a path that is on the HD, Netscape's path is like "/c|/directory/filename.ext"
        //  I need the path to read as "/c:\directory\filename.ext".
        if( cbArgLength[DOMAIN] == 0 )
            for( LPSTR pchr = szArg[PATH]; *pchr != '\0'; pchr++ )
            {
                if( *pchr == '|' )
                    *pchr = ':';
                if( *pchr == '/' && pchr != szArg[PATH] )
                    *pchr = '\\';
            }

        //  InternetSetCookie accepts files with a local domain if it isn't
        // explicitly declared.. which related to a bug in InternetSetCookie.
        if( cbArgLength[DOMAIN] != 0 )
        {
            //  add the domain=DOMAIN pair
            memcpy( pValueIterator, ";domain=", sizeof(";domain=") - 1);
            pValueIterator += sizeof(";domain=") - 1;
            memcpy( pValueIterator, szArg[DOMAIN], cbArgLength[DOMAIN]);
            pValueIterator += cbArgLength[DOMAIN];
        }

        //  InternetSetCookie doesn't work right if the path ends in a filename.
        //so I'll stick the path in the szURL parameter (which accepts filenames)
        ////  add the path=PATH pair
        //memcpy( pValueIterator, ";path=", sizeof(";path=") - 1);
        //pValueIterator += sizeof(";path=") - 1;
        //memcpy( pValueIterator, szArg[PATH], cbArgLength[PATH]);
        //pValueIterator += cbArgLength[PATH];

        //  set expiration info
        memcpy( pValueIterator,";expires=", sizeof(";expires=") - 1);
        pValueIterator += sizeof(";expires=") - 1;

        if( InternetTimeFromTime_tA( StrToLong( szArg[EXPIRES]), pValueIterator, INTERNET_RFC1123_BUFSIZE ) != TRUE )
            goto doneImportCookie;

        //  InternetSetCookie requires an URL.. There isn't a complete url in the netscape cookie, but it can
        //be fudged.  The path in the URL will become the path stored for the cookie, the rest is checked for
        //legality then ignored.
        DWORD cbURLSize;
        cbURLSize = sizeof("nnnn://")
            + cbArgLength[DOMAIN]
            + cbArgLength[PATH]
            + 1;
        szURL = new CHAR[ cbURLSize];

        if( szURL == NULL )
            goto doneImportCookie;

        LPSTR pURLIterator;
        pURLIterator = szURL;

        if( szArg[DOMAIN][0] == '\0')
        {
            memcpy( pURLIterator, "file:/", sizeof("file:/") - 1);
            pURLIterator += sizeof("file:/") - 1;
            memcpy( pURLIterator, szArg[PATH], cbArgLength[PATH]);
            pURLIterator += cbArgLength[PATH];
        }
        else
        {
            memcpy( pURLIterator, "http://", sizeof("http://") - 1);
            pURLIterator += sizeof("http://") - 1;
            memcpy( pURLIterator, szArg[DOMAIN], cbArgLength[DOMAIN]);
            pURLIterator += cbArgLength[DOMAIN];
            memcpy( pURLIterator, szArg[PATH], cbArgLength[PATH]);
            pURLIterator += cbArgLength[PATH];
        }
        pURLIterator[0] = '\0';

        //  don't overwrite exisiting cookies.
        DWORD dwTemp;
        CHAR szNullTerminatedName[ MAX_PATH];
        memcpy( szNullTerminatedName, szArg[NAME], cbArgLength[NAME]);
        szNullTerminatedName[ cbArgLength[NAME]] = '\0';
        if( InternetGetCookieEx( szURL, szNullTerminatedName,
            NULL, &dwTemp, 0, NULL) == TRUE)
        {
            // this cookie exists!  don't import.
            goto doneImportCookie;
        }

        DEBUG_PUT(( "\n\nCalled InternetSetCookie with\nUrl:%s\nValue:%s", szURL,  szValue));

#ifdef UNICODE
#error "ImportCookieFile(): InternetSetCookieA needs to be called directly from here."
#endif
        if( InternetSetCookieEx( szURL, NULL, szValue, COOKIE_NOUI, NULL ) == FALSE )
        {
            DWORD temp = GetLastError();
            DEBUG_PUT(("\n Cookie Rejected!  Error code: %x", temp));
        }

	// If we got this far, we're done.
	retVal = TRUE;

    doneImportCookie:
        if( szValue != NULL)
            delete [] szValue;
        if( szURL != NULL)
            delete [] szURL;
    }



doneImportFile:
    if( hFile != (HKEY) INVALID_HANDLE_VALUE)
        CloseHandle( hFile);

    if( szFile != NULL)
        delete [] szFile;


    return retVal;
}


//--------------------------------------------------------------------
//
//  FindNetscapeCookieFile
//

//  Gets the cookie file and stores it in buffer szFilename.
//lpnBufSize contains the buffer's size on the way in, and the amount used on the way out (in characters)
//If FindNetscapeCookieFile succeeds, it returns TRUE, else it returns FALSE.
//If it fails, information may be written to the buffer with error information,
//  look at RegQueryValueEx for details.
BOOL FindNetscapeCookieFile( IN DWORD dwNSVer, OUT LPTSTR szFilename, /* in-out */ LPDWORD lpnBufSize)
{
    BOOL retVal = FALSE;

    //  for Version<MS_NAVI4
    HKEY hCookieKey, hUserRootKey, hProfileKey;

    //  for MS_NAVI4 >= Version < MS_NAVI5
    hCookieKey = (HKEY) INVALID_HANDLE_VALUE;
    hUserRootKey = (HKEY) INVALID_HANDLE_VALUE;
    hProfileKey = (HKEY) INVALID_HANDLE_VALUE;

    INET_ASSERT( dwNSVer != 0x0000);

    if( dwNSVer < NS_NAVI4)
    {
        if( REGOPENKEYEX( HKEY_CURRENT_USER, NS3_COOKIE_REG_PATH, 0, KEY_READ, &hCookieKey) != ERROR_SUCCESS)
        {
            hCookieKey = (HKEY) INVALID_HANDLE_VALUE;
            goto doneFindNetscapeCookieFile;
        }

        DWORD dwType;  //  should be REG_SZ when returned from QueryValue
        DWORD cbBufSize;
        cbBufSize = *lpnBufSize * sizeof(TCHAR);
        if( RegQueryValueEx( hCookieKey, NS3_COOKIE_REG_KEY, NULL,  &dwType, (LPBYTE)szFilename, &cbBufSize) != ERROR_SUCCESS
            || dwType != REG_SZ)
        {
            *lpnBufSize = cbBufSize / sizeof(TCHAR);
            goto doneFindNetscapeCookieFile;
        }
        *lpnBufSize = cbBufSize / sizeof(TCHAR);

        retVal = TRUE;
    }
    else if ( dwNSVer < NS_NAVI5)
    {
        if( REGOPENKEYEX( HKEY_LOCAL_MACHINE, NS4_USERS_REG_PATH, 0, KEY_READ, &hUserRootKey) != ERROR_SUCCESS)
        {
            hUserRootKey = (HKEY) INVALID_HANDLE_VALUE;
            goto doneFindNetscapeCookieFile;
        }

        DWORD dwNumberOfProfiles;
        if( RegQueryInfoKey( hUserRootKey, NULL, NULL, NULL, &dwNumberOfProfiles,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL) != ERROR_SUCCESS
            || dwNumberOfProfiles != 1)
        {
            goto doneFindNetscapeCookieFile;
        }

        TCHAR szProfileName[MAX_PATH];
        DWORD nProfileNameBufferSize;
        nProfileNameBufferSize = sizeof(szProfileName);
        if( RegEnumKeyEx( hUserRootKey, 0, szProfileName, &nProfileNameBufferSize, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
        {
            goto doneFindNetscapeCookieFile;
        }

        if( REGOPENKEYEX( hUserRootKey, szProfileName, 0, KEY_READ, &hProfileKey) != ERROR_SUCCESS)
        {
            hProfileKey = (HKEY) INVALID_HANDLE_VALUE;
            goto doneFindNetscapeCookieFile;
        }

        DWORD dwType;  //  should be REG_SZ when returned from QueryValue
        DWORD cbBufSize;
        cbBufSize = *lpnBufSize * sizeof(TCHAR);
        if( RegQueryValueEx( hProfileKey, NS4_USERPATH_REG_KEY, NULL, &dwType, (LPBYTE)szFilename, &cbBufSize) != ERROR_SUCCESS
            || dwType != REG_SZ)
        {
            *lpnBufSize = (cbBufSize / sizeof(TCHAR) - 1) + LENGTHOFTSTR( NS_COOKIE_FILENAME);
            goto doneFindNetscapeCookieFile;
        }

        if( (*lpnBufSize - (cbBufSize / sizeof(TCHAR) - 1)) < LENGTHOFTSTR( NS_COOKIE_FILENAME))
        {
            *lpnBufSize = (cbBufSize / sizeof(TCHAR) - 1) + LENGTHOFTSTR( NS_COOKIE_FILENAME);
            goto doneFindNetscapeCookieFile;
        }

        *lpnBufSize = cbBufSize / sizeof(TCHAR) - 1;
        szFilename[(*lpnBufSize)++] = TCHAR('\\');
        memcpy( szFilename + *lpnBufSize, NS_COOKIE_FILENAME, sizeof(NS_COOKIE_FILENAME));
        *lpnBufSize += LENGTHOFTSTR( NS_COOKIE_FILENAME);

        retVal = TRUE;
    }

doneFindNetscapeCookieFile:

    if( hCookieKey != (HKEY) INVALID_HANDLE_VALUE )
        REGCLOSEKEY( hCookieKey);

    if( hUserRootKey != (HKEY) INVALID_HANDLE_VALUE)
        REGCLOSEKEY( hUserRootKey);

    if( hProfileKey != (HKEY) INVALID_HANDLE_VALUE)
        REGCLOSEKEY( hProfileKey);

    return retVal;
}


//--------------------------------------------------------------------
//
//  GetActiveNetscapeVersion
//

BOOL GetActiveNetscapeVersion( LPDWORD lpVersion)
{
    BOOL retVal = FALSE;

    TCHAR szFilename[MAX_PATH];
    DWORD cFilenameSize = MAX_PATH;
    BYTE* pVerBuffer = NULL;

    //  If the application currently associated with .htm files is NS,
    //thats the active version.  If it isn't, we grab the last installed
    //version of NS and say thats the active version.

    LPTSTR pFilenameSubstring;
    if( GetExecuteableFromExtension( HTML_FILE_EXTENSION, szFilename, &cFilenameSize, &pFilenameSubstring) == FALSE
        || StrCmpI( pFilenameSubstring, NS_EXE_FILENAME) != 0 )
    {
        goto doneGetActiveNetscapeVersion;
    }

    DWORD cbFileVersionBufSize;
    DWORD dwTemp;
    if( (cbFileVersionBufSize = GetFileVersionInfoSize( szFilename, &dwTemp)) == 0 )
        goto doneGetActiveNetscapeVersion;

    pVerBuffer = new BYTE[cbFileVersionBufSize];
    if( pVerBuffer == NULL)
        goto doneGetActiveNetscapeVersion;

    if( GetFileVersionInfo( szFilename, 0, cbFileVersionBufSize, pVerBuffer) == 0 )
        goto doneGetActiveNetscapeVersion;

    VS_FIXEDFILEINFO *lpVSFixedFileInfo;

    unsigned uiLength;
    if( VerQueryValue( pVerBuffer, TEXT("\\"),(LPVOID*)&lpVSFixedFileInfo, &uiLength) == 0
        || uiLength == 0)
        goto doneGetActiveNetscapeVersion;

    *lpVersion = lpVSFixedFileInfo->dwFileVersionMS;
    retVal = TRUE;

doneGetActiveNetscapeVersion:
    if( pVerBuffer != NULL)
        delete [] pVerBuffer;

    return retVal;
}


//--------------------------------------------------------------------
//
//  ImportCookieFile
//

//  Retrieves the executable file associated with the open command for
//a particular file extension.
//
//  Parsing the open command for the path\file can be ambiguous.
//If there is a '\' in a prefixed file argument of the open command
//the returned string will contain everying up to and including
//that argument
//
//example:
//""c:\program files\argy.exe \load file" -postfix"
//will give you
//"c:\program files\ argv.exe \load"
//
//typical case:
//""c:\program files\exec.exe -prearg" -postarg"
//will give you
//"c:\program files\exec.exe"
//
//pcFilenameSubstring will point one char past the last '\' to allow
//immediate checking of that last word.
//
BOOL GetExecuteableFromExtension(
    IN LPCTSTR szExtension,
    OUT LPTSTR szFilepath,
    /* in-out */ LPDWORD pcFilenameSize,
    OUT LPTSTR* pFilenameSubstring)
{
    DWORD dwType;   // rewritten often, temporary values
    TCHAR szBuffer[MAX_PATH];   // reused as szCommandKeyPath and szOpenCommand
    DWORD cBufSize;

    BOOL retVal = FALSE;

    HKEY hHtmlEntry = (HKEY) INVALID_HANDLE_VALUE;
    HKEY hOpenCmd = (HKEY) INVALID_HANDLE_VALUE;

    if( REGOPENKEYEX( HKEY_CLASSES_ROOT, szExtension, 0, KEY_READ, &hHtmlEntry) != ERROR_SUCCESS)
    {
        hHtmlEntry = (HKEY) INVALID_HANDLE_VALUE;
        goto doneGetExecuteableFromExtension;
    }

    //  to build a registry key in szCommandKeyPath, we first retrieve
    //the filetype (e.g. htmlfile) and then attach the registry path
    //for the 'open' command in HKEY_CLASSES_ROOT

    LPTSTR szCommandKeyPath;
    szCommandKeyPath = szBuffer;

    //  retrieve filetype
    cBufSize = MAX_PATH * sizeof(TCHAR);
    if( RegQueryValueEx( hHtmlEntry, NULL, NULL, &dwType, (BYTE*)szCommandKeyPath, &cBufSize) != ERROR_SUCCESS
        || dwType != REG_SZ )
    {
        goto doneGetExecuteableFromExtension;
    }

    //  append path ('htmlfile' -> 'htmlfile\\shell\\open\\command')
    memcpy( szCommandKeyPath + (cBufSize / sizeof(TCHAR)) - 1, OPEN_COMMAND_OF_APP, sizeof(OPEN_COMMAND_OF_APP));

    //  get the open command from the registry
    if( REGOPENKEYEX( HKEY_CLASSES_ROOT, szCommandKeyPath, 0, KEY_READ, &hOpenCmd) != ERROR_SUCCESS)
    {
        hOpenCmd = (HKEY) INVALID_HANDLE_VALUE;
        goto doneGetExecuteableFromExtension;
    }

    LPTSTR szOpenCommand;
    szOpenCommand = szBuffer;
    cBufSize = MAX_PATH * sizeof(TCHAR);
    if( RegQueryValueEx( hOpenCmd, "", NULL, &dwType, (BYTE*)szOpenCommand, &cBufSize) != ERROR_SUCCESS
        || dwType != REG_SZ)
    {
        goto doneGetExecuteableFromExtension;
    }

    //  we now have a command line entry with the netscape filename.
    //  typical format:  ""path\filename -prefixed args" -postfixed arguments"

    LPTSTR pSubstring;
    DWORD cLength;
    //  Now, too build a return value, point to the path\filename within the
    //command line entry and null-terminate it.  cLength will eventually
    //contain the string's length
    pSubstring = szOpenCommand;
    cLength = cBufSize / sizeof(TCHAR) - 1;

    // move beginning past any prefixed quotes or spaces
    while( *pSubstring == TCHAR('\"') || *pSubstring == TCHAR(' '))
    {
        pSubstring++;
        cLength--;
    }
    // move right end to the rightmost '\'
    while( cLength > 0 && pSubstring[cLength] != TCHAR('\\'))
    {
        cLength--;
    }
    //  mark the beginning of the last token (a filename, we assume)
    *pFilenameSubstring = &pSubstring[++cLength];

    // move right end left past the last token
    while( pSubstring[cLength] != TCHAR('\"')
           && pSubstring[cLength] != TCHAR(' ')
           && pSubstring[cLength] != TCHAR('\0') )
    {
        cLength++;
    }

    // null terminate the substring
    pSubstring[cLength] = TCHAR('\0');

    if( *pcFilenameSize >= cLength)
    {
        memcpy( szFilepath, pSubstring, (cLength + 1) * sizeof(TCHAR));
        retVal = TRUE;
    }

    *pcFilenameSize = (cLength + 1);

doneGetExecuteableFromExtension:
    if( hHtmlEntry != (HKEY) INVALID_HANDLE_VALUE)
        REGCLOSEKEY( hHtmlEntry);

    if( hOpenCmd != (HKEY) INVALID_HANDLE_VALUE)
        REGCLOSEKEY( hOpenCmd);

    return retVal;
}






//***********************************************************************************
//    registry functions to store which version of Netscape is to be imported from


//--------------------------------------------------------------------
//
//  SetNetscapeImportVersion
//

//  Store version of Netscape to import cookies from in the registry
BOOL SetNetscapeImportVersion( IN DWORD dwNSVersion)
{
    BOOL retVal = FALSE;

    HKEY hNSVersionKey = (HKEY) INVALID_HANDLE_VALUE;

    DWORD dwDisposition;  // ignored output parameter
    if( REGCREATEKEYEX( HKEY_LOCAL_MACHINE, NS_COOKIE_IMPORT_KEY, 0, TEXT("REG_SZ"),
                        REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hNSVersionKey, &dwDisposition) != ERROR_SUCCESS)
    {
        hNSVersionKey = (HKEY) INVALID_HANDLE_VALUE;
        goto doneSetNetscapeImportVersion;
    }

    if( RegSetValueEx( hNSVersionKey, NS_COOKIE_IMPORT_VERSION, 0, REG_DWORD, (BYTE*)&dwNSVersion, sizeof(DWORD)) != ERROR_SUCCESS )
        goto doneSetNetscapeImportVersion;

    retVal = TRUE;

doneSetNetscapeImportVersion:
    if( hNSVersionKey != (HKEY) INVALID_HANDLE_VALUE )
        REGCLOSEKEY( hNSVersionKey);

    return retVal;
}


//--------------------------------------------------------------------
//
//  GetNetscapeImportVersion
//

//  Retrieves version of Netscape to import coookies from in the registry
BOOL GetNetscapeImportVersion( OUT DWORD* pNSVersion)
{
    BOOL retVal = FALSE;

    HKEY hNSVersionKey = (HKEY) INVALID_HANDLE_VALUE;

    TCHAR szNSFilename[MAX_PATH];
    DWORD cNSFilenameSize = MAX_PATH;

    if( REGOPENKEYEX( HKEY_LOCAL_MACHINE, NS_COOKIE_IMPORT_KEY, 0, KEY_READ, &hNSVersionKey) != ERROR_SUCCESS)
    {
        hNSVersionKey = (HKEY) INVALID_HANDLE_VALUE;
        goto doneGetNetscapeImportVersion;
    }

    DWORD dwType;
    DWORD dwBufSize;
    dwBufSize = sizeof(DWORD);
    if( RegQueryValueEx( hNSVersionKey, NS_COOKIE_IMPORT_VERSION, 0, &dwType, (BYTE*)pNSVersion, &dwBufSize) != ERROR_SUCCESS
        || dwType != REG_DWORD)
    {
        goto doneGetNetscapeImportVersion;
    }

    //  if the "DisableImport" key has been set to a non-FALSE value,
    //fail to find a version to import from.
    DWORD fDontImport;
    if( (RegQueryValueEx( hNSVersionKey, NS_COOKIE_IMPORT_FLAG, 0, &dwType,
                        (BYTE*)&fDontImport, &(dwBufSize = sizeof(fDontImport)))
           == ERROR_SUCCESS)
        && dwType == REG_DWORD
        && fDontImport != FALSE)
    {
        goto doneGetNetscapeImportVersion;
    }

    retVal = TRUE;

doneGetNetscapeImportVersion:
    if( hNSVersionKey != (HKEY) INVALID_HANDLE_VALUE )
        REGCLOSEKEY( hNSVersionKey);

    return retVal;

}


//***************************************************************************
//  utility functions internal to cookimp.cxx


//  indicates if a character is an endofline character in a Netscape cookie file.
bool EndOfLine( CHAR chr )
{
    return chr == '\n' || chr == '\r';
}


//--------------------------------------------------------------------
//
//  InternetTimeFromTime_tA
//

//  InternetTimeFromTime_T takes a time value formatted to the run-time library's time_t
//and gives a formatted time string formatted to the RFC format in HTTP spec 1.0
//
//  dwTime is the number of seconds since Jan 1, 1970 00:00:00.
BOOL InternetTimeFromTime_tA( IN DWORD dwTime, OUT LPSTR lpszTime, DWORD cbBufferSize )
{
    if( cbBufferSize < INTERNET_RFC1123_BUFSIZE )
        return false;

//    FILETIME ft2;
//    DosTime2FileTime( dwTime, &ft2);

    //  First create a FILETIME value, to convert to SYSTEMTIME.

    //  i19700Offset is the FILETIME offset of Jan 1, 1970 at 00:00:00...
    const DWORDLONG dwl1970Offset = 0x019dae9064bafa80;

    DWORDLONG dwlTime = dwl1970Offset + (DWORDLONG)dwTime * (DWORDLONG)10000000;
    FILETIME ft;
    ft.dwLowDateTime  = (DWORD)( dwlTime & 0x00000000FFFFFFFF);
    ft.dwHighDateTime = (DWORD)Int64ShrlMod32( Int64ShrlMod32( dwlTime & 0xFFFFFFFF00000000, 16), 16 );

    //  Second I convert the FILETIME to a SYSTEMTIME   
    SYSTEMTIME st;

    if( FileTimeToSystemTime( &ft, &st ) == 0)
        return FALSE;

#ifdef UNICODE
#error "InternetTimeFromTime_tA(): InternetTimeFromSystemTimeA needs to be called directly from here."
#endif
    //  Finally I use SYSTEMTIME to produce the formatted string.
    return InternetTimeFromSystemTime( &st, INTERNET_RFC1123_FORMAT, lpszTime, cbBufferSize );
}


//--------------------------------------------------------------------
//
//  ReadFileToBuffer
//

//  opens a file and spits it out to memory.
//  if *ppBuf originally points to NULL, memory is allocated for the file
//      and cbBufSize contains the number of bytes allocated.
//      (use delete [] *ppBuf when done)
//  if *ppBuf is not NULL, it is assumed to point to a target buffer of
//      size *lpcbBufSize, and size used is then recorded in *lpcbBufSize.
//  returns TRUE if successful.
//  on FALSE return, *lpcbBufSize contains the size needed to be allocated
//      or 0xFFFFFFFF on total failure
BOOL ReadFileToBuffer( IN LPCTSTR szFilename, LPBYTE* ppBuf, LPDWORD lpcbBufSize)
{
    BOOL retVal = FALSE;

    HANDLE hFile = INVALID_HANDLE_VALUE;

    hFile = CreateFile( szFilename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);

    if( hFile == INVALID_HANDLE_VALUE )
    {
        //  notify failure and exit
        *lpcbBufSize = 0xFFFFFFFF;
        goto doneReadFileToBuffer;
    }

    DWORD dwHighOrderedBitsOfFileSize;
    DWORD cbFileSize;
    cbFileSize = GetFileSize( hFile, &dwHighOrderedBitsOfFileSize );
    if(  cbFileSize == 0xFFFFFFFF || dwHighOrderedBitsOfFileSize != 0 )
    {
        //  notify file was too big and exit
        *lpcbBufSize = 0xFFFFFFFF;
        goto doneReadFileToBuffer;
    }

    if( *ppBuf != NULL )
    {
        //  verify passed buffer is large enough
        if( *lpcbBufSize < cbFileSize)
        {
            //  set needed filesize and exit
            *lpcbBufSize = cbFileSize;
            goto doneReadFileToBuffer;
        }
    }
    else
    {
        *ppBuf = new BYTE[cbFileSize];

        if( *ppBuf == NULL)
        {
            //  notify failure and exit
            *lpcbBufSize = 0xFFFFFFFF;
            goto doneReadFileToBuffer;
        }
    }

    //  set output result
    *lpcbBufSize = cbFileSize;

    DWORD cbRead;
    if( ReadFile( hFile, *ppBuf, cbFileSize, &cbRead, NULL) != TRUE
        || cbRead != cbFileSize)
    {
        //  notify failure and exit
        *lpcbBufSize = 0xFFFFFFFF;
        goto doneReadFileToBuffer;
    }

    CloseHandle( hFile );
    hFile = INVALID_HANDLE_VALUE;

    retVal = TRUE;

doneReadFileToBuffer:

    if( hFile != INVALID_HANDLE_VALUE)
        CloseHandle( hFile);

    return retVal;
}

