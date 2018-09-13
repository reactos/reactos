#include <wininetp.h>


#define SZ_COOKIE_EXPORT_HEADER "#  Internet Explorer cookie file, exported for Netscape browsers."
#define IE_LOCAL_PREFIX             "~~local~~"
#define IE_COOKIE_PREFIX            "Cookie:"


void FileTimeToDosTime( FILETIME ft, DWORD* pTime_t);


struct CACHE_ENTRY_INFO_BUFFER : public INTERNET_CACHE_ENTRY_INFO
{
    BYTE _ab[MAX_CACHE_ENTRY_INFO_SIZE];
};


//****************************************************
//
//  FileOutputStream - utility
//

class FileOutputStream
{
public:
    FileOutputStream()
    : m_hFile(INVALID_HANDLE_VALUE), m_fError(FALSE), m_dwLastError(0)
    {
    }

    ~FileOutputStream()
    {
        if( m_hFile != INVALID_HANDLE_VALUE)
            CloseHandle( m_hFile);
    }

    BOOL Load( LPCTSTR szFilename, BOOL fAppend)
    {
        m_hFile = CreateFile( szFilename, GENERIC_WRITE | GENERIC_READ, 0, NULL, 
                              fAppend ? OPEN_ALWAYS : CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL, NULL);

        if( m_hFile == INVALID_HANDLE_VALUE)
        {
            m_fError = TRUE;
            m_dwLastError = GetLastError();
            return FALSE;
        }

        if( fAppend
            && SetFilePointer( m_hFile, 0, NULL, FILE_END) == 0xFFFFFFFF)
        {
            m_fError = TRUE;
            m_dwLastError = GetLastError();
            return FALSE;
        }

        return TRUE;
    }

    BOOL DumpStr( LPCSTR szString, DWORD cbSize)
    {
        DWORD dwTemp;

        if( m_fError == TRUE)
            return FALSE;
        
        if( WriteFile( m_hFile, szString, cbSize, &dwTemp, NULL) == TRUE)
        {
            return TRUE;
        }
        else
        {
            m_fError = TRUE;
            m_dwLastError = GetLastError();
            return FALSE;
        }
    }

    BOOL WriteBool( BOOL fBool)
    {
        if( fBool == TRUE)
            return DumpStr( "TRUE", sizeof("TRUE") - 1);
        else
            return DumpStr( "FALSE", sizeof("FALSE") - 1);
    }

    BOOL WriteTab()
    {
        return DumpStr( "\t", sizeof('\t'));
    }

    BOOL WriteNSNewline()
    {
        return DumpStr( "\r\n\r\n", sizeof("\r\n\r\n") - 1);
    }

    BOOL IsError()
    {
        return m_fError;
    }

private:
    HANDLE m_hFile;
    BOOL m_fError;
    DWORD m_dwLastError;
};


//****************************************************
//
//  ExportCookieFile
//

// per-line format of Netscape cookie file
//DOMAIN \t is_given_domain(TRUE|FALSE) \t PATH \t secure(TRUE|FALSE) \t time_t(EXPIRES) \t NAME \t VALUE

BOOL ExportCookieFileW( IN LPCWSTR szFilename, IN BOOL fAppend)
{
    MEMORYPACKET mpFilename;
    ALLOC_MB(szFilename,0,mpFilename);
    if (!mpFilename.psStr)
    {
        return FALSE;
    }
    UNICODE_TO_ANSI(szFilename,mpFilename);

    return ExportCookieFileA( mpFilename.psStr, fAppend);
};



BOOL ExportCookieFileA( IN LPCSTR szFilename, IN BOOL fAppend)
{
    BOOL retVal = FALSE;

    CHAR* cursor;
    DWORD dwTemp;

    FileOutputStream outFile;

    CACHE_ENTRY_INFO_BUFFER cei;
    DWORD cbCeiSize = sizeof(cei);
    HANDLE hEnumeration = FindFirstUrlCacheEntry( IE_COOKIE_PREFIX, &cei, &cbCeiSize);
    if( hEnumeration == NULL)
        goto doneExportCookieFile;

    if( outFile.Load( szFilename, fAppend) != TRUE)
        goto doneExportCookieFile;

    if( !fAppend)
    {
        outFile.DumpStr( SZ_COOKIE_EXPORT_HEADER, sizeof(SZ_COOKIE_EXPORT_HEADER) - 1);
        outFile.WriteNSNewline();
    }
    
    if( outFile.IsError() == TRUE)
        goto doneExportCookieFile;

    //  enumerate over the cookie cache entries.
    //Each cache entry will have a file which contains multiple cookies,
    //so inside this loop we will be enumerating over individual cookies.
    do
    {
        CHAR* pFileBuf = NULL;
        DWORD cbFileBufSize;
        CHAR* pDomain;

        //Generate the DOMAIN for all the cookies in this entry
        // example Urls in pEntry:
        //  "Cookie:t-franks@amazon.com/"
        //  "Cookie:t-franks@~~local~~/c:\local\path\"
        // The correspondingDomain entry in NS cookie.txt:
        //   "amazon.com"
        //   ""  ->  local paths are empty strings to NS
        cursor = cei.lpszSourceUrlName; 

        while( *cursor != '@' && *cursor != '\0')
            cursor++;
            
        if( *cursor != '@')
            goto doneWithEntryOnExportCookie;

        pDomain = ++cursor;

        if( StrCmpN( pDomain, IE_LOCAL_PREFIX, sizeof(IE_LOCAL_PREFIX) - 1) != 0) 
        {
            //  if we have a non-local domain, clip off the path.
            while( *cursor != '/' && *cursor != '\0')
                cursor++;

            if( *cursor != '/')
                goto doneWithEntryOnExportCookie;

            *cursor = '\0';
        }
        else
        {
            //  Else local cookies have a blank domain in the Netscape cookie files.
            pDomain[0] = '\0';
        }

        // load the attached file and enumerate through the contained cookies.
        if( (cei.lpszLocalFileName == NULL)
            || (ReadFileToBuffer( cei.lpszLocalFileName, (BYTE**)&pFileBuf, &cbFileBufSize) 
                != TRUE))
        {
            goto doneWithEntryOnExportCookie;           
        }
        
        CHAR* pFileCursor;
        pFileCursor = pFileBuf;
        while( pFileCursor < pFileBuf + cbFileBufSize)
        {
            CHAR *pszName, *pszValue, *pszHash, *pszFlags,
                 *pszExpireTimeLow, *pszExpireTimeHigh,
                 *pszLastTimeHigh,  *pszLastTimeLow,
                 *pszDelimiter;
            FILETIME ftExpire;
            DWORD dwFlags;
       
            // Get the first token (cookie name).
            pszName           = StrTokEx(&pFileCursor, "\n");
            if (!pszName)                               // Cookie name.
            {
                // Normal termination of the parse.
                goto doneWithEntryOnExportCookie;
            }

            // Parse the rest of the cookie
            if( *pFileCursor == '\n')
            {
                pszValue = pFileCursor;
                *pszValue = '\0';
                pFileCursor++;
            }
            else
                pszValue          = StrTokEx(&pFileCursor, "\n");      // Cookie value.

            pszHash           = StrTokEx(&pFileCursor, "\n");      // Combo of domain and path.
            pszFlags          = StrTokEx(&pFileCursor, "\n");      // Cookie flags.
            pszExpireTimeLow  = StrTokEx(&pFileCursor, "\n");      // Expire time.
            pszExpireTimeHigh = StrTokEx(&pFileCursor, "\n");             
            pszLastTimeLow    = StrTokEx(&pFileCursor, "\n");      // Last Modified time.
            pszLastTimeHigh   = StrTokEx(&pFileCursor, "\n");
            pszDelimiter      = StrTokEx(&pFileCursor, "\n");      // Delimiter should be "*"

            // Abnormal termination of parse.
            if (!pszDelimiter || pszDelimiter[0] != '*')
            {
                INET_ASSERT(FALSE);
                goto doneWithEntryOnExportCookie;
            }

            // Get the expire time.
            ftExpire.dwLowDateTime  = atoi(pszExpireTimeLow);
            ftExpire.dwHighDateTime = atoi(pszExpireTimeHigh);

            // Get the flags
            dwFlags = atoi(pszFlags);

            //  If this is a session cookie, its non-persistent
            //and shouldn't be saved to file.
            //  (session cookies shouldn't be in the index file, anyhow)
            INET_ASSERT( (dwFlags & COOKIE_SESSION) == 0 ? TRUE : FALSE);
            if( (dwFlags & COOKIE_SESSION) != 0)
                continue;
                
            //  process parsed contents.
            CHAR* pszNSPath;
            DWORD timetExpire;

            // Example IE cookie file:
            // pszHash = "amazon.com/main/"
            // pszHash = "~~local~~/c:\local\path\"
            // corresponding Netscape path format:
            // pszNSPath = "/main/"
            // pszNSPath = "/c|/local/path/"
            cursor = pszHash;

            // move to the first '/'
            while( *cursor != '/' && *cursor != '\0')
                cursor++;
            if( *cursor != '/')
                continue;

            pszNSPath = cursor;
                
            //  If this is a path on the Hard Disk, turn the '\\'s to '/'s
            //and ':'s to '|'s.  (to conform with funky NS format)
            if( StrCmpN( pszHash, IE_LOCAL_PREFIX, sizeof(IE_LOCAL_PREFIX) - 1) == 0)
            {
                while( *cursor != '\0')
                {
                    if( *cursor == '\\')
                        *cursor = '/';

                    if( *cursor == ':')
                        *cursor = '|';

                    cursor++;
                }
            }

            //  convert to Netscape time format
            FileTimeToDosTime( ftExpire, &timetExpire);

            outFile.DumpStr( pDomain, lstrlen( pDomain));
            outFile.WriteTab();
            outFile.WriteBool( TRUE);
            outFile.WriteTab();
            outFile.DumpStr( pszNSPath, lstrlen( pszNSPath));
            outFile.WriteTab();
            outFile.WriteBool( (dwFlags & COOKIE_SECURE) != 0 ? TRUE : FALSE);
            outFile.WriteTab();

            CHAR expireBuffer[16];
            wsprintf( expireBuffer, "%lu", timetExpire);
            outFile.DumpStr( expireBuffer, lstrlen( expireBuffer));
            outFile.WriteTab();

            outFile.DumpStr( pszName, lstrlen( pszName));
            outFile.WriteTab();
            outFile.DumpStr( pszValue, lstrlen( pszValue));
            outFile.WriteNSNewline();
        }
        
    doneWithEntryOnExportCookie:
        if( pFileBuf != NULL)
            delete [] pFileBuf;

        if( outFile.IsError() == TRUE)
            goto doneExportCookieFile;
    }
    while( FindNextUrlCacheEntry( hEnumeration, &cei, &(cbCeiSize = sizeof(cei))));

    retVal = TRUE;
    
doneExportCookieFile:

    if( hEnumeration != NULL)
        FindCloseUrlCache( hEnumeration);

    return retVal;
}

// reverse of part of transformation in InternetTimeFromTime_t
//  A filetime is the numbers of 100 ns since Jan 1, 1601, while
//a dostime is the number of seconds since Jan 1, 1970.
void FileTimeToDosTime( FILETIME ft, DWORD* pTime_t)
{
    //dwl1970Offset is the number of ns from 1601 to 1970
    const DWORDLONG dwl1970Offset = 0x019dae9064bafa80;

    DWORDLONG lVal;
    
    lVal = ft.dwLowDateTime;
    lVal |= Int64ShllMod32( Int64ShllMod32( ft.dwHighDateTime, 16), 16 );

    *pTime_t = (DWORD)((lVal - dwl1970Offset) / 10000000);

    return;
}



