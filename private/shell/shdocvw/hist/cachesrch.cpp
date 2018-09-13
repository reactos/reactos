/**********************************************************************
  Cache Search Stuff (simple strstr)

  Marc Miller (t-marcmi) - 1998
 **********************************************************************/
#include "cachesrch.h"

DWORD CacheSearchEngine::CacheStreamWrapper::s_dwPageSize = 0;

BOOL  CacheSearchEngine::CacheStreamWrapper::_ReadNextBlock() {
    if (_fEndOfFile)
        return FALSE;

    if (!s_dwPageSize) {
        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);
        s_dwPageSize = sysInfo.dwPageSize;
    }
    BOOL fNewRead = FALSE; // is this our first look at this file?
    if (!_pbBuff) {
        // Allocate a page of memory

        // Note: find out why this returned error code #87
        //_pbBuff  = (LPBYTE)(VirtualAlloc(NULL, s_dwPageSize, MEM_COMMIT, PAGE_READWRITE));
        _pbBuff = (LPBYTE)(LocalAlloc(LPTR, s_dwPageSize));
        if (!_pbBuff) {
            //DWORD dwError = GetLastError();
            return FALSE;
        }
        fNewRead          = TRUE;
        _dwCacheStreamLoc = 0;
    }

    BOOL  fSuccess;
    DWORD dwSizeRead = s_dwPageSize;
    if ((fSuccess = ReadUrlCacheEntryStream(_hCacheStream, _dwCacheStreamLoc,
                                            _pbBuff, &dwSizeRead, 0)) && dwSizeRead)
    {
        _fEndOfFile        = (dwSizeRead < s_dwPageSize);
        
        _dwCacheStreamLoc += dwSizeRead;
        _dwBuffSize        = dwSizeRead;
        _pbBuffPos         = _pbBuff;
        _pbBuffLast        = _pbBuff + dwSizeRead;

        _dataType = ASCII_DATA; // default
        if (fNewRead) {
            // deterine data type
            if (_dwBuffSize >= sizeof(USHORT)) {
                if      (*((USHORT *)_pbBuff) == UNICODE_SIGNATURE)
                    _dataType = UNICODE_DATA;
                else if (*((USHORT *)_pbBuff) == UNICODE_SIGNATURE_BACKWARDS)
                    _dataType = UNICODE_BACKWARDS_DATA;
                
                if (s_IsUnicode(_dataType))
                    _pbBuffPos += s_Charsize(_dataType);
            }
        }
    }
    else {
        fSuccess = FALSE;
        DWORD dwError = GetLastError();
        ASSERT(dwError != ERROR_INSUFFICIENT_BUFFER);
    }
    return fSuccess;
}

CacheSearchEngine::CacheStreamWrapper::CacheStreamWrapper(HANDLE hCacheStream) {
    // this class can be allocated on the stack:
    _pbBuff       = NULL;
    _pbBuffPos    = NULL;
    _pbBuffLast   = NULL;
    _dwBuffSize   = 0;
    _hCacheStream = hCacheStream;
    _fEndOfFile   = FALSE;

    // Read in preliminary block of data --
    //  Die on next read to handle failure
    _fEndOfFile   = !(_ReadNextBlock());
}

CacheSearchEngine::CacheStreamWrapper::~CacheStreamWrapper() {
    if (_pbBuff) {
        //VirtualFree(_pbBuff);
        LocalFree(_pbBuff);;
    }
}

// Read next byte from cache stream, reading in next block if necessary
BOOL CacheSearchEngine::CacheStreamWrapper::_GetNextByte(BYTE &b)
{
    //
    // If the initial read fails _pbBuffPos will be NULL.  Don't
    // allow it to be dereffed.
    //
    BOOL fSuccess = _pbBuffPos ? TRUE : FALSE;

    if (_pbBuffPos == _pbBuffLast)
        fSuccess = _ReadNextBlock();

    if (fSuccess)
        b = *(_pbBuffPos++);

    return fSuccess;
}

BOOL CacheSearchEngine::CacheStreamWrapper::GetNextChar(WCHAR &wc) {
    BOOL fSuccess = TRUE;
    if (s_IsUnicode(_dataType)) {
        BYTE b1, b2;
        LPBYTE bs = (LPBYTE)&wc;
        if (_GetNextByte(b1) && _GetNextByte(b2)) {
            switch (_dataType) {
            case UNICODE_DATA:
                bs[0] = b1;
                bs[1] = b2;
                break;
            case UNICODE_BACKWARDS_DATA:
                bs[0] = b2;
                bs[1] = b1;
                break;
            default: ASSERT(0);
            }
        }
        else
            fSuccess = FALSE;
    }
    else 
    {
       
        BYTE szData[2];

        if (_GetNextByte(szData[0]))
        {
            int cch = 1;
            if (IsDBCSLeadByte(szData[0]))
            {
                if (!_GetNextByte(szData[1]))
                {
                    fSuccess = FALSE;
                }
                cch++;
            }

            if (fSuccess)
            {
                fSuccess = (MultiByteToWideChar(CP_ACP, 0, (LPSTR)szData, cch, &wc, 1) > 0);
            }
        }
        else
        {
            fSuccess = FALSE;
        }

    }
    return fSuccess;
}


// Prepare a search target string for searching --
void CacheSearchEngine::StreamSearcher::_PrepareSearchTarget(LPCWSTR pwszSearchTarget)
{
    UINT uStrLen = lstrlenW(pwszSearchTarget);
    _pwszPreparedSearchTarget = ((LPWSTR)LocalAlloc(LPTR, (uStrLen + 1) * sizeof(WCHAR)));

    if (_pwszPreparedSearchTarget) {
        // Strip leading and trailing whitespace and compress adjacent whitespace characters
        //  into literal spaces
        LPWSTR pwszTemp  = _pwszPreparedSearchTarget;
        pwszSearchTarget = s_SkipWhiteSpace(pwszSearchTarget);
        BOOL   fAddWs    = FALSE;
        while(*pwszSearchTarget) {
            if (s_IsWhiteSpace(*pwszSearchTarget)) {
                fAddWs = TRUE;
                pwszSearchTarget = s_SkipWhiteSpace(pwszSearchTarget);
            }
            else {
                if (fAddWs) {
                    *(pwszTemp++) = L' ';
                    fAddWs = FALSE;
                }
                *(pwszTemp++) = *(pwszSearchTarget++);
            }
        }
        *pwszTemp = L'\0';
    }
}

// Search a character stream for a searchtarget
//  Does a simple strstr, but tries to be smart about whitespace and
//  ignores HTML where possible...
BOOL CacheSearchEngine::StreamSearcher::SearchCharStream(CacheSearchEngine::IWideSequentialReadStream &wsrs,
                                                         BOOL fIsHTML/* = FALSE*/)
{
    BOOL fFound = FALSE;
    
    if (_pwszPreparedSearchTarget && *_pwszPreparedSearchTarget)
    {
        WCHAR   wc;
        LPCWSTR pwszCurrent    = _pwszPreparedSearchTarget;
        BOOL    fMatchedWS     = FALSE;
#if 0
        BOOL    fIgnoreHTMLTag = FALSE;
#endif
        
        while(*pwszCurrent && wsrs.GetNextChar(wc)) {
#if 0
            if (fIsHTML && (wc == L'<'))
                fIgnoreHTMLTag = TRUE;
            else if (fIgnoreHTMLTag) {
                if (wc == L'>')
                    fIgnoreHTMLTag = FALSE;
            }
            else 
#endif

            if (s_IsWhiteSpace(wc)) {
                // matched whitespace in search stream, look for
                //  matching whitespace in target string
                if (!fMatchedWS) {
                    if (s_IsWhiteSpace(*pwszCurrent)) {
                        fMatchedWS = TRUE;
                        ++pwszCurrent;
                    }
                    else
                        pwszCurrent = _pwszPreparedSearchTarget;
                }
            }
            else {
                fMatchedWS = FALSE;
                if (!ChrCmpIW(*pwszCurrent, wc)) {
                    ++pwszCurrent;
                }
                else {
                    pwszCurrent = _pwszPreparedSearchTarget;
                }
            }
        }
        fFound = !*pwszCurrent;
    }
    return fFound;
}

BOOL CacheSearchEngine::SearchCacheStream(CacheSearchEngine::StreamSearcher &cse, HANDLE hCacheStream,
                                          BOOL fIsHTML/* = FALSE*/)
{
    CacheStreamWrapper csw(hCacheStream);
    return cse.SearchCharStream(csw, fIsHTML);
}
