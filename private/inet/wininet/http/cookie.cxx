//---------------------------------------------------------------------------
//
// COOKIE.CXX
//
//     Cookie Jar
//
//     This file implements cookies as defined by Navigator 4 behavior and the
//     specification at http://www.netscape.com/newsref/std/cookie_spec.html.
//     If Navigator 4 and the specification are not in agreement, we try to
//     match the Navigator 4 behavior.
//
//     The following describes some interesting aspects of cookie behavior.
//
// SYNTAX
//
//    Syntax for cookie is
//
//          [[name]=] value [; options]
//
//    The name is everything before "=" with leading and  trailing whitespace
//    removed.  The value is everything after "=" and before ";" with leading
//    and trailing whitespace removed.  The name and value can contain spaces,
//    quotes or any other character except ";" and "=".  The name and equal
//    sign are optional.
//
//    Example:  =foo  ->  name: <blank> value: foo
//              foo   ->  name: <blank> value: foo
//              foo=  ->  name: foo     value: <blank>
//              ;     ->  name: <blank> value: <blank>
//
// ORDER
//
//    Cookies with a more specific path are sent before cookies with
//    a less specific path mapping.  The domain does not contibute
//    to the ordering of cookies.
//
//    If the path length of two cookies are equal, then the cookies
//    are ordered by time of creation.  Navigator maintains this
//    ordering across domain and path boundaries.  IE maintains this
//    ordering for a specific domain and path. It is difficult to match
//    the Navigator behavior and there are no known bugs because of
//    this difference.
//
// MATCHING
//
//    Path matches are done at the character level.  Any
//    directory structure in the path is ignored.
//
//    Navigator matches domains at the character level and ignores
//    the structure of the domain name.
//
//    Previous versions of IE tossed the leading "." on a domain name.
//    With out this information, character by character compares are
//    can produce incorrect results.  For backwards compatibilty with
//    old cookie we continue to match on a component by component basis.
//
//    Some examples of the difference are:
//
//       Cookie domain   Document domain  Navigator match  IE match
//       .foo.com        foo.com          no               yes
//       bar.x.com       foobar.x.com     yes              no
//
// ACCEPTING COOKIES
//
//    A cookie is rejected if the path specified in the set cookie
//    header is not a prefix of document's path.
//
//    Navigator rejects a cookie if the domain specified in the
//    set cookie header does not contain at least two periods
//    or the domain is not a suffix of the document's domain.
//    The suffix match is done on a character by character basis.
//
//    Navigator ignores all the stuff in the specification about
//    three period matching and the seven special top level domains.
//
//    IE rejects a cookie if the domain specified by the cookie
//    header does not contain at least one embedded period or the
//    domain is not a suffix of the documents domain.
//
//    Cookies are accepted if the path specified in the set cookie
//    header is a prefix of the document's path and the domain
//    specified in the set cookie header.
//
//    The difference in behavior is a result of the matching rules
//    described in the previous section.
//
//---------------------------------------------------------------------------

#include <wininetp.h>
#include "httpp.h"

extern DWORD ConfirmCookie(HWND hwnd, HTTP_REQUEST_HANDLE_OBJECT *lpRequest, DWORD dwFlags, LPVOID *lppvData, LPDWORD pdwStopWarning);

#define CCH_COOKIE_MAX  (5 * 1024)

CRITICAL_SECTION s_csCookieJar;

static class CCookieJar *s_pJar;
static char s_achEmpty[] = "";
static char s_cCacheModify;
static const char s_achCookieScheme[] = "Cookie:";
static DWORD s_dwCacheVersion;
static BOOL s_fFirstTime = TRUE;

// Hard-coded list of special domains. If any of these are present between the 
// second-to-last and last dot we will require 2 embedded dots.
// The domain strings are reversed to make the compares easier

static const char *s_pachSpecialDomains[] = 
    {"MOC", "UDE", "TEN", "GRO", "VOG", "LIM", "TNI" };  

#if INET_DEBUG
DWORD s_dwThreadID;
#endif

//---------------------------------------------------------------------------
//
// CACHE_ENTRY_INFO_BUFFER
//
//---------------------------------------------------------------------------

class CACHE_ENTRY_INFO_BUFFER : public INTERNET_CACHE_ENTRY_INFO
{
    BYTE _ab[5 * 1024];
};

//---------------------------------------------------------------------------
//
// CCookieCriticalSection
//
// Enter / Exit critical section.
//
//---------------------------------------------------------------------------

class CCookieCriticalSection
{
private:
    int Dummy; // Variable needed to force compiler to generate code for const/dest.
public:
    CCookieCriticalSection()
        {
            EnterCriticalSection(&s_csCookieJar);
            #if INET_DEBUG
                s_dwThreadID = GetCurrentThreadId();
            #endif
        }
    ~CCookieCriticalSection()
        {
            #if INET_DEBUG
                s_dwThreadID = 0;
            #endif
            LeaveCriticalSection(&s_csCookieJar);
        }
};

#define ASSERT_CRITSEC() INET_ASSERT(GetCurrentThreadId() == s_dwThreadID)

//---------------------------------------------------------------------------
//
// CCookieBase
//
// Provides operator new which allocates extra memory
// after object and initializes the memory to zero.
//
//---------------------------------------------------------------------------

class CCookieBase
{
public:

    void * operator new(size_t cb, size_t cbExtra);
    void operator delete(void *pv);
};

//---------------------------------------------------------------------------
//
// CCookie
//
// Holds a single cookie value.
//
//---------------------------------------------------------------------------


class CCookie : public CCookieBase
{
public:

    ~CCookie();
    static CCookie *Construct(const char *pchName);

    BOOL            SetValue(const char *pchValue);
    BOOL            WriteCacheFile(HANDLE hFile, char *pchRDomain, char *pchPath);
    BOOL            CanSend(FILETIME *pftCurrent, BOOL fSecure);
    BOOL            IsPersistent() { return (_dwFlags & COOKIE_SESSION) == 0; }

    BOOL            PurgePersistent(void *);
    BOOL            PurgeSession(void *);
    BOOL            PurgeAll(void *);
    BOOL            PurgeByName(void *);
    BOOL            PurgeThis(void *);
    BOOL            PurgeExpired(void *);

    FILETIME        _ftExpire;
    FILETIME        _ftLastModified;
    DWORD           _dwFlags;
    CCookie *       _pCookieNext;
    char *          _pchName;
    char *          _pchValue;
};

//---------------------------------------------------------------------------
//
// CCookieLocation
//
// Holds all cookies for a given domain and path.
//
//---------------------------------------------------------------------------

class CCookieLocation : public CCookieBase
{
public:

    ~CCookieLocation();
    static CCookieLocation *Construct(const char *pchRDomain, const char *pchPath);

    CCookie *       GetCookie(const char *pchName, BOOL fCreate);
    BOOL            WriteCacheFile();
    BOOL            ReadCacheFile();
    BOOL            ReadCacheFileIfNeeded();
    BOOL            Purge(BOOL (CCookie::*)(void *), void *);
    BOOL            Purge(FILETIME *pftCurrent, BOOL fSession);
    BOOL            IsMatch(char *pchRDomain, char *pchPath);
    char *          GetCacheURL();

    FILETIME        _ftCacheFileLastModified;
    CCookie *       _pCookieKids;
    CCookieLocation*_pLocationNext;
    char *          _pchRDomain;
    char *          _pchPath;
    int             _cchPath;
    BYTE            _fCacheFileExists;
    BYTE            _fReadFromCacheFileNeeded;
};


//---------------------------------------------------------------------------
//
// CCookieJar
//
// Maintains fixed size hash table of cookie location objects.
//
//---------------------------------------------------------------------------
enum SET_COOKIE_RESULT
{
    SET_COOKIE_FAIL     = 0,
    SET_COOKIE_SUCCESS  = 1,
    SET_COOKIE_DISALLOW = 2,
    SET_COOKIE_PENDING  = 3
};

class CCookieJar : public CCookieBase
{
public:

    static CCookieJar * Construct();
    ~CCookieJar();

    DWORD
    OkToSetCookie(
        HTTP_REQUEST_HANDLE_OBJECT *pRequest,
        const char *pchURL,
        char *pchRDomain,
        char *pchPath,
        char *pchName,
        char *pchValue,
        DWORD dwFlags,
        FILETIME ftExpire
        );


    void              EnforceCookieLimits(CCookieLocation *pLocation, char *pchName, BOOL *pfWriteCacheFileNeeded);
    DWORD             SetCookie(HTTP_REQUEST_HANDLE_OBJECT *pRequest, const char *pchURL, char *pchHeader, DWORD dwFlags);
    void              Purge(FILETIME *pftCurrent, BOOL fSession);
    BOOL              SyncWithCache();
    BOOL              SyncWithCacheIfNeeded();
    void              CacheFilesModified();
    CCookieLocation** GetBucket(const char *pchRDomain);
    CCookieLocation * GetLocation(const char *pchRDomain, const char *pchPath, BOOL fCreate);

    CCookieLocation * _apLocation[128];
};

//---------------------------------------------------------------------------
//
// Track cache modificaitons.
//
//---------------------------------------------------------------------------

inline void
MarkCacheModified()
{
    IncrementUrlCacheHeaderData(CACHE_HEADER_DATA_COOKIE_CHANGE_COUNT, &s_dwCacheVersion);
}

inline BOOL
IsCacheModified()
{
    DWORD dwCacheVersion;

    if (s_fFirstTime)
    {
        s_fFirstTime = FALSE;
        GetUrlCacheHeaderData(CACHE_HEADER_DATA_COOKIE_CHANGE_COUNT, &s_dwCacheVersion);
        return TRUE;
    }
    else
    {
        dwCacheVersion = s_dwCacheVersion;
        GetUrlCacheHeaderData(CACHE_HEADER_DATA_COOKIE_CHANGE_COUNT, &s_dwCacheVersion);

        return dwCacheVersion != s_dwCacheVersion;
    }
}

//---------------------------------------------------------------------------
//
// String utilities
//
//---------------------------------------------------------------------------

static BOOL
IsZero(FILETIME *pft)
{
    return pft->dwLowDateTime == 0 && pft->dwHighDateTime == 0;
}

static char *
StrnDup(const char *pch, int cch)
{
    char *pchAlloc = (char *)ALLOCATE_MEMORY(LMEM_FIXED, cch + 1);
    if (!pchAlloc)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }

    memcpy(pchAlloc, pch, cch);
    pchAlloc[cch] = 0;

    return pchAlloc;
}

static BOOL
IsPathMatch(const char *pchPrefix, const char *pchStr)
{
    while (*pchPrefix == *pchStr && *pchStr)
    {
        pchPrefix += 1;
        pchStr += 1;
    }

    return *pchPrefix == 0;
}

static BOOL
IsDomainMatch(const char *pchPrefix, const char *pchStr)
{
    while (*pchPrefix == *pchStr && *pchStr)
    {
        pchPrefix += 1;
        pchStr += 1;
    }

    return *pchPrefix == 0 && (*pchStr == 0 || *pchStr == '.');
}

static BOOL
IsPathLegal(const char *pchHeader, const char *pchDocument)
{
    return TRUE;

    /*

    We attempted to implement the specification here.
    It looks like Navigator does not reject cookies
    based on the path attribute.  We now consider
    all path attributes to be legal.

    while (*pchHeader == *pchDocument && *pchDocument)
    {
        pchHeader += 1;
        pchDocument += 1;
    }

    if (*pchDocument == 0)
    {
        while (*pchHeader && *pchHeader != '/' && *pchHeader != '\\')
        {
            pchHeader += 1;
        }
    }

    return *pchHeader == 0;
    */
}

static BOOL
IsSpecialDomain(const char *pch, int nCount)
{
    // Currently all the special strings are exactly 3 characters long.
    if (pch == NULL || nCount != 3)
        return FALSE;

    for (int i = 0 ; i < ARRAY_ELEMENTS(s_pachSpecialDomains) ; i++ )
    {
        if (StrCmpNIC(pch, s_pachSpecialDomains[i], nCount) == 0)
            return TRUE;
    }

    return FALSE;
}

static BOOL
IsDomainLegal(const char *pchHeader, const char *pchDocument)
{
    const char *pch = pchHeader;
    int c = 0;
    int d = 0;
    int rgcch[2] = { 0, 0 };  // How many characters between dots

    // Must have at least one period in name.
    // and contains nothing but '.' is illegal 

    int countChars = 0;
    const char * pchSecondPart = NULL; // for a domain string such as 
    for (; *pch; pch++)
    {
        if (*pch == '.')
        {
            if ( c < 2 )
            {
                // Remember how many characters we have between the last two dots
                // For example if domain header is .microsoft.com
                // rgcch[0] should be 3 for "com"
                // rgcch[1] should be 9 for "microsoft"
                rgcch[c] = countChars;

                if (c == 1)
                {
                    pchSecondPart = pch - countChars;
                }
            }
            countChars = 0;
            c += 1;
            
        }
        else
        {
            countChars++;
        }
        d++;
    }

    // The code below depends on the leading dot being removed from the domain header.
    // The parse code does that, but an assert here just in case something changes in the 
    // parse code.
    INET_ASSERT(*(pch - 1) != '.');

    // Remember the count of the characters between the begining of the header and 
    // the first dot. So for domain=abc.com this will set rgch[1] = 3. 
    // Note that this assumes that if domain=.abc.com the leading dot has been stripped
    // out in the parse code. See assert above.
    if (c < 2 )
    {
        rgcch[c] = countChars;
        if (c == 1)
        {
            pchSecondPart = pch - countChars;
        }
    }

    // If the domain name is of the form abc.xx.yy where the number of characters between the last two dots is less than 
    // 2 we require a minimum of two embedded dots. This is so you are not allowed to set cookies readable by all of .co.nz for 
    // example. However this rule is not sufficient and we special case things like .edu.nz as well. 

    int cEmbeddedDotsNeeded = 1;

    if (rgcch[0] <= 2)
    {
        if (rgcch[1] <= 2 || (pchSecondPart && IsSpecialDomain(pchSecondPart, rgcch[1])))
            cEmbeddedDotsNeeded = 2;
    }

    if (c < cEmbeddedDotsNeeded || d == c)
        return FALSE;

    // Mismatch between header and document not allowed.
    // Must match full components of domain name.

    while (*pchHeader == *pchDocument && *pchDocument)
    {
        pchHeader += 1;
        pchDocument += 1;
    }

    return *pchHeader == 0 && (*pchDocument == 0 || *pchDocument == '.' );
}


void
LowerCaseString(char *pch)
{
    for (; *pch; pch++)
    {
        if (*pch >= 'A' && *pch <= 'Z')
            *pch += 'a' - 'A';
    }
}

static void
ReverseString(char *pchFront)
{
    char *pchBack;
    char  ch;
    int   cch;

    cch = strlen(pchFront);

    pchBack = pchFront + cch - 1;

    cch = cch / 2;
    while (--cch >= 0)
    {
        ch = *pchFront;
        *pchFront = *pchBack;
        *pchBack = ch;

        pchFront += 1;
        pchBack -= 1;
    }
}

static BOOL
PathAndRDomainFromURL(const char *pchURL, char **ppchRDomain, char **ppchPath, BOOL *pfSecure, BOOL bStrip = TRUE)
{
    char *pchDomainBuf;
    char *pchRDomain = NULL;
    char *pchPathBuf;
    char *pchPath = NULL;
    char *pchExtra;
    DWORD cchDomain;
    DWORD cchPath;
    DWORD cchExtra;
    BOOL  fSuccess = FALSE;
    DWORD dwError;
    INTERNET_SCHEME ustScheme;
    char *pchURLCopy = NULL;

    pchURLCopy = NewString(pchURL);
    if (!pchURLCopy)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto Cleanup;
    }

    dwError = CrackUrl((char *)pchURLCopy,
             0,
             FALSE,
             &ustScheme,
             NULL,          //  Scheme Name
             NULL,          //  Scheme Lenth
             &pchDomainBuf,
             &cchDomain,
             NULL,          //  Internet Port
             NULL,          //  UserName
             NULL,          //  UserName Length
             NULL,          //  Password
             NULL,          //  Password Lenth
             &pchPathBuf,
             &cchPath,
             &pchExtra,     //  Extra
             &cchExtra,     //  Extra Length
             NULL);

    if (dwError != ERROR_SUCCESS)
    {
        SetLastError(dwError);
        goto Cleanup;
    }

    if ( ustScheme != INTERNET_SCHEME_HTTP &&
         ustScheme != INTERNET_SCHEME_HTTPS &&
         ustScheme != INTERNET_SCHEME_FILE)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto Cleanup;
    }

    *pfSecure = ustScheme == INTERNET_SCHEME_HTTPS;

    if (ustScheme == INTERNET_SCHEME_FILE)
    {
        pchDomainBuf = "~~local~~";
        cchDomain = sizeof("~~local~~") - 1;
    }
    else
    {
        // SECURITY:  It's possible for us to navigate to a carefully
        //            constructed URL such as http://server%3F.microsoft.com.
        //            This results in a cracked hostname of server?.microsoft.com.
        //            Given the current architecture, it would probably be best to
        //            make CrackUrl smarter.  However, the minimal fix below prevents
        //            the x-domain security violation without breaking escaped cases
        //            that work today that customers may expect to be allowed.
        DWORD n;
        for (n = 0; n < cchDomain; n++)
        {
            // RFC 952 as amended by RFC 1123: the only valid chars are
            // a-z, A-Z, 0-9, '-', and '.'.  The last two are delimiters
            // which cannot start or end the name, but this detail doesn't
            // matter for the security fix.
            if (!((pchDomainBuf[n] >= 'a' && pchDomainBuf[n] <= 'z') ||
                  (pchDomainBuf[n] >= 'A' && pchDomainBuf[n] <= 'Z') ||
                  (pchDomainBuf[n] >= '0' && pchDomainBuf[n] <= '9') ||
                  (pchDomainBuf[n] == '-') ||
                  (pchDomainBuf[n] == '.')))
            {
                // REVIEW
                //
                // What if this was incorrectly cracked and the escaped
                // path, query string, or fragment was incorrectly dumped
                // here?  Do we care and/or want to support this case?
                // After all, we could get CrackUrl to recognize %2f characters
                // for slashes, or we could reconstruct the path here if
                // a lower impact is desired...
                pchDomainBuf[n] = '\0';
                cchDomain = n;
                break;
            }
        }
    }

    if(bStrip)
    {
        while (cchPath > 0)
        {
            if (pchPathBuf[cchPath - 1] == '/' || pchPathBuf[cchPath - 1] == '\\')
            {
                break;
            }
        cchPath -= 1;
        }
    }

    pchRDomain = StrnDup(pchDomainBuf, cchDomain);
    if (!pchRDomain)
        goto Cleanup;

    LowerCaseString(pchRDomain);
    ReverseString(pchRDomain);

    pchPath = (char *)ALLOCATE_MEMORY(LMEM_FIXED, cchPath + 2);
    if (!pchPath)
        goto Cleanup;

    if (*pchPathBuf != '/')
    {
        *pchPath = '/';
        memcpy(pchPath + 1, pchPathBuf, cchPath);
        pchPath[cchPath + 1] = 0;
    }
    else
    {
        memcpy(pchPath, pchPathBuf, cchPath);
        pchPath[cchPath] = 0;
    }

    fSuccess = TRUE;

Cleanup:
    if (!fSuccess)
    {
        if (pchRDomain)
            FREE_MEMORY(pchRDomain);
        if (pchPath)
            FREE_MEMORY(pchPath);
    }
    else
    {
        *ppchRDomain = pchRDomain;
        *ppchPath = pchPath;
    }

    if (pchURLCopy)
        FREE_MEMORY(pchURLCopy);

    return fSuccess;
}

//---------------------------------------------------------------------------
//
// CCookieBase implementation
//
//---------------------------------------------------------------------------

void *
CCookieBase::operator new(size_t cb, size_t cbExtra)
{
    void *pv = ALLOCATE_MEMORY(LMEM_FIXED, cb + cbExtra);
    if (!pv)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }

    memset(pv, 0, cb);
    return pv;
}

inline void
CCookieBase::operator delete(void *pv)
{
    FREE_MEMORY(pv);
}

//---------------------------------------------------------------------------
//
// CCookie implementation
//
//---------------------------------------------------------------------------

CCookie *
CCookie::Construct(const char *pchName)
{
    CCookie *pCookie = new(strlen(pchName) + 1) CCookie();
    if (!pCookie)
        return NULL;

    pCookie->_pchName = (char *)(pCookie + 1);
    pCookie->_pchValue = s_achEmpty;
    strcpy(pCookie->_pchName, pchName);

    pCookie->_dwFlags = COOKIE_SESSION;

    return pCookie;
}

CCookie::~CCookie()
{
    if (_pchValue != s_achEmpty)
        FREE_MEMORY(_pchValue);
}

BOOL
CCookie::SetValue(const char *pchValue)
{
    int   cchValue;

    if (_pchValue != s_achEmpty)
        FREE_MEMORY(_pchValue);

    if (!pchValue || !*pchValue)
    {
        _pchValue = s_achEmpty;
    }
    else
    {
        cchValue = strlen(pchValue) + 1;
        _pchValue = (char *)ALLOCATE_MEMORY(LMEM_FIXED, cchValue);
        if (!_pchValue)
        {
            _pchValue = s_achEmpty;
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }

        memcpy(_pchValue, pchValue, cchValue);
    }
    return TRUE;
}

BOOL
CCookie::CanSend(FILETIME *pftCurrent, BOOL fSecure)
{
    return (fSecure || !(_dwFlags & COOKIE_SECURE)) &&
            (CompareFileTime(_ftExpire, *pftCurrent) >= 0);
}

BOOL CCookie::PurgePersistent(void *)
{
    return IsPersistent();
}

BOOL CCookie::PurgeAll(void *)
{
    return TRUE;
}

BOOL CCookie::PurgeByName(void *pvName)
{
    return strcmp((char *)pvName, _pchName) == 0;
}

BOOL CCookie::PurgeThis(void *pvThis)
{
    return this == (CCookie *)pvThis;
}

BOOL CCookie::PurgeExpired(void *pvCurrent)
{
    return CompareFileTime(_ftExpire, *(FILETIME *)pvCurrent) < 0;
}

static BOOL
WriteString(HANDLE hFile, const char *pch)
{
    DWORD cb;
    return pch && *pch ? WriteFile(hFile, pch, strlen(pch), &cb, NULL) : TRUE;
}

static BOOL
WriteStringLF(HANDLE hFile, const char *pch)
{
    DWORD cb;

    if (!WriteString(hFile, pch)) return FALSE;
    return WriteFile(hFile, "\n", 1, &cb, NULL);
}

BOOL
CCookie::WriteCacheFile(HANDLE hFile, char *pchRDomain, char *pchPath)
{
    BOOL fSuccess = FALSE;
    char achBuf[128];

    ReverseString(pchRDomain);

    if (!WriteStringLF(hFile, _pchName)) goto Cleanup;
    if (!WriteStringLF(hFile, _pchValue)) goto Cleanup;
    if (!WriteString(hFile, pchRDomain)) goto Cleanup;
    if (!WriteStringLF(hFile, pchPath)) goto Cleanup;

    wsprintf(achBuf, "%u\n%u\n%u\n%u\n%u\n*\n",
            _dwFlags,
            _ftExpire.dwLowDateTime,
            _ftExpire.dwHighDateTime,
            _ftLastModified.dwLowDateTime,
            _ftLastModified.dwHighDateTime);

    if (!WriteString(hFile, achBuf)) goto Cleanup;

    fSuccess = TRUE;

Cleanup:
    ReverseString(pchRDomain);
    return fSuccess;
}

//---------------------------------------------------------------------------
//
// CCookieLocation implementation
//
//---------------------------------------------------------------------------

CCookieLocation *
CCookieLocation::Construct(const char *pchRDomain, const char *pchPath)
{
    int cchPath = strlen(pchPath);

    CCookieLocation *pLocation = new(strlen(pchRDomain) + cchPath + 2) CCookieLocation();
    if (!pLocation)
        return NULL;

    pLocation->_cchPath = cchPath;
    pLocation->_pchPath = (char *)(pLocation + 1);
    pLocation->_pchRDomain = pLocation->_pchPath + cchPath + 1;

    strcpy(pLocation->_pchRDomain, pchRDomain);
    strcpy(pLocation->_pchPath, pchPath);

    return pLocation;
}

CCookieLocation::~CCookieLocation()
{
    Purge(CCookie::PurgeAll, NULL);
}

CCookie *
CCookieLocation::GetCookie(const char *pchName, BOOL fCreate)
{
    CCookie *pCookie;

    CCookie **ppCookie = &_pCookieKids;

    for (pCookie = _pCookieKids; pCookie; pCookie = pCookie->_pCookieNext)
    {
        if (strcmp(pchName, pCookie->_pchName) == 0)
            return pCookie;
        ppCookie = &pCookie->_pCookieNext;
    }

    if (!fCreate)
        return NULL;

    pCookie = CCookie::Construct(pchName);
    if (!pCookie)
        return NULL;

    //
    // Insert cookie at end of list to match Navigator's behavior.
    //

    pCookie->_pCookieNext = NULL;
    *ppCookie = pCookie;

    return pCookie;
}

BOOL
CCookieLocation::Purge(BOOL (CCookie::*pfnPurge)(void *), void *pv)
{
    CCookie **ppCookie = &_pCookieKids;
    CCookie *pCookie;
    BOOL     fPersistentDeleted = FALSE;

    while ((pCookie = *ppCookie) != NULL)
    {
        if ((pCookie->*pfnPurge)(pv))
        {
            *ppCookie = pCookie->_pCookieNext;
            fPersistentDeleted |= pCookie->IsPersistent();
            delete pCookie;
        }
        else
        {
            ppCookie = &pCookie->_pCookieNext;
        }
    }

    return fPersistentDeleted;
}

BOOL
CCookieLocation::Purge(FILETIME *pftCurrent, BOOL fSession)
{
    if (!_fCacheFileExists)
    {
        // If cache file is gone, then delete all persistent
        // cookies. If there's no cache file, then it's certainly
        // the case that we do not need to read the cache file.

        Purge(CCookie::PurgePersistent, NULL);
        _fReadFromCacheFileNeeded = FALSE;
    }

    // This is a good time to check for expired persistent cookies.

    if (!_fReadFromCacheFileNeeded && Purge(CCookie::PurgeExpired, pftCurrent))
    {
        WriteCacheFile();
    }

    if (fSession)
    {
        // If we are purging because a session ended, nuke
        // everything in sight.  If we deleted a persistent
        // cookie, note that we need to read the cache file
        // on next access.

        _fReadFromCacheFileNeeded |= Purge(CCookie::PurgeAll, NULL);
    }

    return !_fReadFromCacheFileNeeded && _pCookieKids == NULL;
}

char *
CCookieLocation::GetCacheURL()
{
    char *pchURL;
    char *pch;
    int cchScheme = sizeof(s_achCookieScheme) - 1;

    int cchUser = vdwCurrentUserLen;
    int cchAt = 1;
    int cchDomain = strlen(_pchRDomain);
    int cchPath = strlen(_pchPath);

    pchURL = (char *)ALLOCATE_MEMORY(LMEM_FIXED, cchScheme + cchUser + cchAt + cchDomain + cchPath + 1);
    if (!pchURL)
        return NULL;

    pch = pchURL;

    memcpy(pch, s_achCookieScheme, cchScheme);
    pch += cchScheme;

    memcpy(pch, vszCurrentUser, cchUser);
    pch += cchUser;

    memcpy(pch, "@", cchAt);
    pch += cchAt;

    ReverseString(_pchRDomain);
    memcpy(pch, _pchRDomain, cchDomain);
    ReverseString(_pchRDomain);
    pch += cchDomain;

    strcpy(pch, _pchPath);

    return pchURL;
}

BOOL
CCookieLocation::WriteCacheFile()
{
    HANDLE      hFile = INVALID_HANDLE_VALUE;
    char        achFile[MAX_PATH];
    char *      pchURL = NULL;
    BOOL        fSuccess = FALSE;
    CCookie *   pCookie;
    FILETIME    ftLastExpire =  { 0, 0 };

    achFile[0] = 0;

    GetCurrentGmtTime(&_ftCacheFileLastModified);

    //
    // Determine the latest expiry time and if we have something to write.
    //

    for (pCookie = _pCookieKids; pCookie; pCookie = pCookie->_pCookieNext)
    {
        if (pCookie->IsPersistent() && CompareFileTime(pCookie->_ftExpire, ftLastExpire) > 0)
        {
            ftLastExpire = pCookie->_ftExpire;
        }
    }

    pchURL = GetCacheURL();
    if (!pchURL)
        goto Cleanup;

    if (CompareFileTime(ftLastExpire, _ftCacheFileLastModified) < 0)
    {
        fSuccess = TRUE;
        DeleteUrlCacheEntry(pchURL);
        _fCacheFileExists = FALSE;
        goto Cleanup;
    }

    _fCacheFileExists = TRUE;

    if (!CreateUrlCacheEntry(pchURL,
            0,              // Estimated size
            "txt",          // File extension
            achFile,
            0))
        goto Cleanup;

    hFile = CreateFile(
            achFile,
            GENERIC_WRITE,
            0, // no sharing.
            NULL,
            TRUNCATE_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL );
    if (hFile == INVALID_HANDLE_VALUE)
        goto Cleanup;

    for (pCookie = _pCookieKids; pCookie; pCookie = pCookie->_pCookieNext)
    {
        if (pCookie->IsPersistent() && CompareFileTime(pCookie->_ftExpire, _ftCacheFileLastModified) >= 0)
        {
            if (!pCookie->WriteCacheFile(hFile, _pchRDomain, _pchPath))
                goto Cleanup;
        }
    }

    CloseHandle(hFile);
    hFile = INVALID_HANDLE_VALUE;

    if (!CommitUrlCacheEntry(pchURL,
            achFile,
            ftLastExpire,
            _ftCacheFileLastModified,
            NORMAL_CACHE_ENTRY | COOKIE_CACHE_ENTRY,
            NULL,
            0,
            NULL,
            0 ))
        goto Cleanup;

    MarkCacheModified();

    fSuccess = TRUE;

Cleanup:

    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);

    if (!fSuccess)
    {
        if (achFile[0])
            DeleteFile(achFile);

        if (pchURL)
            DeleteUrlCacheEntry(pchURL);
    }

    if (pchURL)
        FREE_MEMORY(pchURL);

    return fSuccess;
}

static char *
ScanString(char *pch, char **pchStr)
{
    *pchStr = pch;

    for (; *pch; *pch++)
    {
        if (*pch == '\n')
        {
            *pch = 0;
            pch += 1;
            break;
        }
    }

    return pch;
}

static char *
ScanNumber(char *pch, DWORD *pdw)
{
    DWORD dw = 0;
    char *pchJunk;

    for (; *pch >= '0' && *pch <= '9'; *pch++)
    {
        dw = (dw * 10) + *pch - '0';
    }

    *pdw = dw;

    return ScanString(pch, &pchJunk);
}

BOOL
CCookieLocation::ReadCacheFile()
{
    char *      pchURL = NULL;
    char *      pch;
    DWORD       cbCEI;
    HANDLE      hCacheStream = NULL;
    char *      pchBuffer = NULL;
    CCookie *   pCookie;
    CACHE_ENTRY_INFO_BUFFER cei;

    _fReadFromCacheFileNeeded = FALSE;

    pchURL = GetCacheURL();
    if (!pchURL)
        goto Cleanup;

    cbCEI = sizeof(cei);

    hCacheStream = RetrieveUrlCacheEntryStream(
            pchURL,
            &cei,
            &cbCEI,
            FALSE, // sequential access
            0);

    if (!hCacheStream)
    {
        // If we failed to get the entry, try to nuke it so it does not
        // bother us in the future.

        DeleteUrlCacheEntry(pchURL);
        goto Cleanup;
    }

    // Old cache files to not have last modified time set.
    // Bump the time up so that we can use file times to determine
    // if we need to resync a file.

    if (IsZero(&cei.LastModifiedTime))
    {
        cei.LastModifiedTime.dwLowDateTime = 1;
    }

    _ftCacheFileLastModified = cei.LastModifiedTime;

    // Read cache file into a null terminated buffer.

    pchBuffer = (char *)ALLOCATE_MEMORY(LMEM_FIXED, cei.dwSizeLow + 1 * sizeof(char));
    if (!pchBuffer)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto Cleanup;
    }

    if (!ReadUrlCacheEntryStream(hCacheStream, 0, pchBuffer, &cei.dwSizeLow, 0))
        goto Cleanup;

    pchBuffer[cei.dwSizeLow] = 0;

    // Blow away all existing persistent cookies.

    Purge(CCookie::PurgePersistent, NULL);

    // Parse cookies from the buffer;

    for (pch = pchBuffer; *pch; )
    {
        char *pchName;
        char *pchValue;
        char *pchLocation;
        char *pchStar;
        DWORD dwFlags;
        FILETIME ftExpire;
        FILETIME ftLast;

        pch = ScanString(pch, &pchName);
        pch = ScanString(pch, &pchValue);
        pch = ScanString(pch, &pchLocation);

        pch = ScanNumber(pch, &dwFlags);
        pch = ScanNumber(pch, &ftExpire.dwLowDateTime);
        pch = ScanNumber(pch, &ftExpire.dwHighDateTime);
        pch = ScanNumber(pch, &ftLast.dwLowDateTime);
        pch = ScanNumber(pch, &ftLast.dwHighDateTime);

        pch = ScanString(pch, &pchStar);

        if (strcmp(pchStar, "*"))
        {
            goto Cleanup;
        }

        pCookie = GetCookie(pchName, TRUE);
        if (!pCookie)
            goto Cleanup;

        // Initialize the cookie.

        pCookie->SetValue(pchValue);
        pCookie->_ftExpire = ftExpire;
        pCookie->_ftLastModified = ftLast;
        pCookie->_dwFlags = dwFlags;
    }

Cleanup:
    if (hCacheStream)
        UnlockUrlCacheEntryStream(hCacheStream, 0);

    if (pchURL)
        FREE_MEMORY(pchURL);

    if (pchBuffer)
        FREE_MEMORY(pchBuffer);

    return TRUE;
}

BOOL
CCookieLocation::IsMatch(char *pchRDomain, char *pchPath)
{
    return IsDomainMatch(_pchRDomain, pchRDomain) &&
        IsPathMatch(_pchPath, pchPath);
}

BOOL
CCookieLocation::ReadCacheFileIfNeeded()
{
    return _fReadFromCacheFileNeeded ? ReadCacheFile() : TRUE;
}


//---------------------------------------------------------------------------
//
// CCookieJar implementation
//
//---------------------------------------------------------------------------


CCookieJar *
CCookieJar::Construct()
{
    CCookieJar *s_pJar = new(0) CCookieJar();
    if (!s_pJar)
        return NULL;

    return s_pJar;
}

CCookieJar::~CCookieJar()
{
    for (int i = ARRAY_ELEMENTS(_apLocation); --i >= 0; )
    {
        CCookieLocation *pLocation = _apLocation[i];
        while (pLocation)
        {
            CCookieLocation *pLocationT = pLocation->_pLocationNext;
            delete pLocation;
            pLocation = pLocationT;
        }
    }
}

CCookieLocation **
CCookieJar::GetBucket(const char *pchRDomain)
{
    int ch;
    int cPeriod = 0;
    unsigned int hash = 0;

    ASSERT_CRITSEC();

    for (; (ch = *pchRDomain) != 0; pchRDomain++)
    {
        if (ch == '.')
        {
            cPeriod += 1;
            if (cPeriod >= 2)
                break;
        }
        hash = (hash * 29) + ch;
    }

    hash = hash % ARRAY_ELEMENTS(_apLocation);

    return &_apLocation[hash];
}

CCookieLocation *
CCookieJar::GetLocation(const char *pchRDomain, const char *pchPath, BOOL fCreate)
{
    ASSERT_CRITSEC();

    int cchPath = strlen(pchPath);
    CCookieLocation *pLocation = NULL;
    CCookieLocation **ppLocation = GetBucket(pchRDomain);

    // To support sending more specific cookies before less specific,
    // we keep list sorted by path length.

    while ((pLocation = *ppLocation) != NULL)
    {
        if (pLocation->_cchPath < cchPath)
            break;

        if (strcmp(pLocation->_pchPath, pchPath) == 0 &&
            strcmp(pLocation->_pchRDomain, pchRDomain) == 0)
            return pLocation;

        ppLocation = &pLocation->_pLocationNext;
    }

    if (!fCreate)
        goto Cleanup;

    pLocation = CCookieLocation::Construct(pchRDomain, pchPath);
    if (!pLocation)
        goto Cleanup;

    pLocation->_pLocationNext = *ppLocation;
    *ppLocation = pLocation;

Cleanup:
    return pLocation;
}

void
CCookieJar::Purge(FILETIME *pftCurrent, BOOL fSession)
{
    ASSERT_CRITSEC();

    for (int i = ARRAY_ELEMENTS(_apLocation); --i >= 0; )
    {
        CCookieLocation **ppLocation = &_apLocation[i];
        CCookieLocation *pLocation;

        while ((pLocation = *ppLocation) != NULL)
        {
            if (pLocation->Purge(pftCurrent, fSession))
            {
                *ppLocation = pLocation->_pLocationNext;
                delete pLocation;
            }
            else
            {
                ppLocation = &pLocation->_pLocationNext;
            }
        }
    }
}

BOOL
CCookieJar::SyncWithCache()
{
    DWORD       dwBufferSize;
    HANDLE      hEnum = NULL;
    int         cchUserNameAt;
    char        achUserNameAt[MAX_PATH + 2];
    FILETIME    ftCurrent;
    char *      pchRDomain;
    char *      pchPath;
    char *      pch;
    CACHE_ENTRY_INFO_BUFFER cei;
    CCookieLocation *pLocation;

    ASSERT_CRITSEC();

    if (!vdwCurrentUserLen)
        GetWininetUserName();

    strcpy(achUserNameAt, vszCurrentUser);
    strcat(achUserNameAt, "@");
    cchUserNameAt = vdwCurrentUserLen+1;

    dwBufferSize = sizeof(cei);
    hEnum = FindFirstUrlCacheEntry(s_achCookieScheme, &cei, &dwBufferSize);

    for (int i = ARRAY_ELEMENTS(_apLocation); --i >= 0; )
    {
        for (pLocation = _apLocation[i];
                pLocation;
                pLocation = pLocation->_pLocationNext)
        {
            pLocation->_fCacheFileExists = FALSE;
        }
    }

    if (hEnum)
    {
        do
        {
            if ( cei.lpszSourceUrlName &&
                (strnicmp(cei.lpszSourceUrlName, s_achCookieScheme, sizeof(s_achCookieScheme) - 1 ) == 0) &&
                (strnicmp(cei.lpszSourceUrlName+sizeof(s_achCookieScheme) - 1,achUserNameAt, cchUserNameAt) == 0))
            {

                // Split domain name from path in buffer.
                // Slide domain name down to make space for null terminator
                // between domain and path.

                pchRDomain = cei.lpszSourceUrlName+sizeof(s_achCookieScheme) - 1 + cchUserNameAt - 1;

                for (pch = pchRDomain + 1; *pch && *pch != '/'; pch++)
                {
                    pch[-1] = pch[0];
                }
                pch[-1] = 0;

                pchPath = pch;

                ReverseString(pchRDomain);

                pLocation = GetLocation(pchRDomain, pchPath, TRUE);
                if (!pLocation)
                {
                    continue;
                }

                // Old cache files to not have last modified time set.
                // Bump the time up so that we can use file times to determine
                // if we need to resync a file.

                if (IsZero(&cei.LastModifiedTime))
                {
                    cei.LastModifiedTime.dwLowDateTime = 1;
                }

                if (CompareFileTime(pLocation->_ftCacheFileLastModified, cei.LastModifiedTime) < 0)
                {
                    pLocation->_fReadFromCacheFileNeeded = TRUE;
                }

                pLocation->_fCacheFileExists = TRUE;

            }

            dwBufferSize = sizeof(cei);

        } while (FindNextUrlCacheEntryA(hEnum, &cei, &dwBufferSize));

        FindCloseUrlCache(hEnum);
    }

    // Now purge everthing we didn't get .

    GetCurrentGmtTime(&ftCurrent);
    Purge(&ftCurrent, FALSE);

    return TRUE;
}

BOOL
CCookieJar::SyncWithCacheIfNeeded()
{
    return IsCacheModified() ? SyncWithCache() : TRUE;
}

struct PARSE
{
    char *pchBuffer;
    char *pchToken;
    BOOL fEqualFound;
};

static char *
SkipWS(char *pch)
{
    while (*pch == ' ' || *pch == '\t')
        pch += 1;

    return pch;
}

static BOOL
ParseToken(PARSE *pParse, BOOL fBreakOnSpecialTokens, BOOL fBreakOnEqual)
{
    char ch;
    char *pch;
    char *pchEndToken;

    pParse->fEqualFound = FALSE;

    pch = SkipWS(pParse->pchBuffer);
    if (*pch == 0)
    {
        pParse->pchToken = pch;
        return FALSE;
    }

    pParse->pchToken = pch;
    pchEndToken = pch;

    while ((ch = *pch) != 0)
    {
        pch += 1;
        if (ch == ';')
        {
            break;
        }
        else if (fBreakOnEqual && ch == '=')
        {
            pParse->fEqualFound = TRUE;
            break;
        }
        else if (ch == ' ' || ch == '\t')
        {
            if (fBreakOnSpecialTokens)
            {
                if ((strnicmp(pch, "expires", sizeof("expires") - 1) == 0) ||
                    (strnicmp(pch, "path", sizeof("path") - 1) == 0) ||
                    (strnicmp(pch, "domain", sizeof("domain") - 1) == 0) ||
                    (strnicmp(pch, "secure", sizeof("secure") - 1) == 0))
                {
                    break;
                }
            }
        }
        else
        {
            pchEndToken = pch;
        }
    }

    *pchEndToken = 0;
    pParse->pchBuffer = pch;
    return TRUE;
}


static void
ParseHeader(
    char *pchHeader,
    char **ppchName,
    char **ppchValue,
    char **ppchPath,
    char **ppchRDomain,
    DWORD *pdwFlags,
    FILETIME *pftExpire)
{
    PARSE parse;

    parse.pchBuffer = pchHeader;

    *ppchName = NULL;
    *ppchValue = NULL;
    *ppchPath = NULL;
    *ppchRDomain = NULL;
    *pdwFlags = COOKIE_SESSION;

    // If only one of name or value is specified, Navigator
    // uses name=<blank> and value as what ever was specified.
    // Example:  =foo  ->  name: <blank> value: foo
    //           foo   ->  name: <blank> value: foo
    //           foo=  ->  name: foo     value: <blank>

    if (ParseToken(&parse, FALSE, TRUE))
    {
        *ppchName = parse.pchToken;
        if (parse.fEqualFound)
        {
            if (ParseToken(&parse, FALSE, FALSE))
            {
                *ppchValue = parse.pchToken;
            }
            else
            {
                *ppchValue = s_achEmpty;
            }
        }
        else
        {
            *ppchValue = *ppchName;
            *ppchName = s_achEmpty;
        }
    }

    while (ParseToken(&parse, FALSE, TRUE))
    {
        if (stricmp(parse.pchToken, "expires") == 0)
        {
            if (parse.fEqualFound && ParseToken(&parse, TRUE, FALSE))
            {
                FParseHttpDate(pftExpire, parse.pchToken);
                *pdwFlags &= ~COOKIE_SESSION;
            }
        }
        else if (stricmp(parse.pchToken, "domain") == 0)
        {
            if (parse.fEqualFound )
            {
                if( ParseToken(&parse, TRUE, FALSE))
                {
                    // Previous versions of IE tossed the leading
                    // "." on domain names.  We continue this behavior
                    // to maintain compatiblity with old cookie files.
                    // See comments at the top of this file for more
                    // information.

                    if (*parse.pchToken == '.') parse.pchToken += 1;
                    ReverseString(parse.pchToken);
                    *ppchRDomain = parse.pchToken;
                }
                else
                {
                    *ppchRDomain = parse.pchToken;
                }
            }
        }
        else if (stricmp(parse.pchToken, "path") == 0)
        {
            if (parse.fEqualFound && ParseToken(&parse, TRUE, FALSE))
            {
                *ppchPath = parse.pchToken;
            }
            else
            {
                *ppchPath = s_achEmpty;
            }
        }
        else if (stricmp(parse.pchToken, "secure") == 0)
        {
            *pdwFlags |= COOKIE_SECURE;

            if (parse.fEqualFound)
            {
                ParseToken(&parse, TRUE, FALSE);
            }
        }
        else
        {
            if (parse.fEqualFound)
            {
                ParseToken(&parse, TRUE, FALSE);
            }
        }
    }

    if (!*ppchName)
    {
        *ppchName = *ppchValue = s_achEmpty;
    }

    if (*pdwFlags & COOKIE_SESSION)
    {
        pftExpire->dwLowDateTime = 0xFFFFFFFF;
        pftExpire->dwHighDateTime = 0x7FFFFFFF;
    }
}

// free's an INTERNET_COOKIE structure
static VOID
DestroyInternetCookie(INTERNET_COOKIE *pic)
{
    if ( pic != NULL ) 
    {
        if ( pic->pszDomain ) {
            FREE_MEMORY(pic->pszDomain);
        }
        if ( pic->pszPath ) {
            FREE_MEMORY(pic->pszPath);
        }
        if ( pic->pszName ) {
            FREE_MEMORY(pic->pszName);
        }
        if ( pic->pszData ) {
            FREE_MEMORY(pic->pszData);
        }
        if ( pic->pszUrl ) {
            FREE_MEMORY(pic->pszUrl);
        }
        if( pic->pftExpires ) {
            delete pic->pftExpires; 
            pic->pftExpires = NULL;
        }

        FREE_MEMORY(pic);
    }
}

// allocate's an INTERNET_COOKIE structure
static INTERNET_COOKIE *
MakeInternetCookie(
    const char *pchURL,
    char *pchRDomain,
    char *pchPath,
    char *pchName,
    char *pchValue,
    DWORD dwFlags,
    FILETIME ftExpire
    )
{
    INTERNET_COOKIE *pic = NULL;

    pic = (INTERNET_COOKIE *) ALLOCATE_MEMORY(LMEM_ZEROINIT, sizeof(INTERNET_COOKIE));

    if ( pic == NULL ) {
        return NULL;
    }
    
    pic->cbSize = sizeof(INTERNET_COOKIE);

    pic->pszDomain = pchRDomain ? NewString(pchRDomain) : NULL;
    if (pic->pszDomain) {
        ReverseString(pic->pszDomain);
    }
    pic->pszPath = pchPath ? NewString(pchPath) : NULL;
    pic->pszName = pchName ? NewString(pchName) : NULL;
    pic->pszData = pchValue ? NewString(pchValue) : NULL;
    pic->pszUrl = pchURL ? NewString(pchURL) : NULL;

#if COOKIE_SECURE != INTERNET_COOKIE_IS_SECURE
#error MakeInternetCookie depends on cookie flags to remain the same
#endif 
    pic->dwFlags = dwFlags;

    if( dwFlags & COOKIE_SESSION )
    {
        pic->pftExpires = NULL;
    }
    else
    {
        pic->pftExpires = new FILETIME;
        if( pic->pftExpires )
        {
            memcpy(pic->pftExpires, &ftExpire, sizeof(FILETIME));
        }
    }
    
    return pic;
}


DWORD
CCookieJar::OkToSetCookie(
    HTTP_REQUEST_HANDLE_OBJECT *pRequest,
    const char *pchURL,
    char *pchRDomain,
    char *pchPath,
    char *pchName,
    char *pchValue,
    DWORD dwFlags,
    FILETIME ftExpire
    )
{
    DWORD dwError;
    DWORD dwCookiesPolicy;
    BOOL fCleanupPic = FALSE;
    INTERNET_COOKIE *pic = NULL, *pic_result;

    //
    // Deal first with the basic quick cases to determine if we need UI here.
    //  they are:
    //   - Do we require UI or cookies to be set all, for the current Url Zone?
    ///  - Do we allow UI for this given request?
    //

    if( pchURL )
    {
        dwCookiesPolicy = ::GetCookiePolicy(pchURL, dwFlags & COOKIE_SESSION);
    }
    else
    {
        return SET_COOKIE_FAIL;
    }


    if( dwCookiesPolicy == URLPOLICY_ALLOW 
        || (dwCookiesPolicy == URLPOLICY_QUERY && (dwFlags & COOKIE_NOUI) != 0))
    {
        return SET_COOKIE_SUCCESS;
    }
    else if( dwCookiesPolicy == URLPOLICY_DISALLOW )
    {
        return SET_COOKIE_DISALLOW;
    }

    if (pRequest && (pRequest->GetOpenFlags() & INTERNET_FLAG_NO_UI))
    {
        return SET_COOKIE_FAIL;
    }

    //
    // Now look up the cookie and confirm that it hasn't just been added,
    //  if its already added to the Cookie list, then we don't show UI, 
    //  since once the user has chosen to add a given Cookie, we don't repeatly re-prompt
    //

    {
        CCookieCriticalSection cs;
        CCookieLocation *pLocation;

        if (!SyncWithCacheIfNeeded())
            return SET_COOKIE_FAIL;
                                
        pLocation = GetLocation(pchRDomain, pchPath, FALSE /* no creation*/);

        if (pLocation)
        {
            CCookie *pCookie;

            pLocation->ReadCacheFileIfNeeded();            
            pCookie = pLocation->GetCookie(pchName, FALSE /* no creation */);
            if (pCookie) {
                return SET_COOKIE_SUCCESS;
            }
        }
    }

    //
    // Now make the async request, to see if we can put up UI
    //

    {
        DWORD dwStopWarning = 0;
        DWORD dwAction;
        DWORD dwResult;

        pic = MakeInternetCookie(pchURL,
                                 pchRDomain,
                                 pchPath,
                                 pchName,
                                 pchValue,
                                 dwFlags,
                                 ftExpire
                                 );

        if ( pic == NULL )  {
            return SET_COOKIE_FAIL;
        }

        fCleanupPic = TRUE;
        pic_result = pic;

        dwError = ChangeUIBlockingState( 
                    (HINTERNET) pRequest,
                    ERROR_HTTP_COOKIE_NEEDS_CONFIRMATION,                    
                    &dwAction,
                    &dwResult,
                    (LPVOID *) &pic_result
                    );      

        switch (dwAction)
        {
            case UI_ACTION_CODE_NONE_TAKEN:

                //
                // fallback to old behavior
                //

                dwError = ConfirmCookie(NULL, pRequest, 0, (void **)&pic, &dwStopWarning);

                if( dwStopWarning )
                {
                    // call zone mgr to report user want stop warning...
                    ::SetStopWarning(pchURL, URLPOLICY_ALLOW, dwFlags & COOKIE_SESSION);
                }
                break;

            case UI_ACTION_CODE_USER_ACTION_COMPLETED:

                //
                // UI has been completed, cleanup, and then continue
                //

                if ( pic_result && (pic_result != pic) ) {
                    DestroyInternetCookie(pic_result);
                }

                INET_ASSERT(fCleanupPic);
                    
                dwError = dwResult;
                break;

            case UI_ACTION_CODE_BLOCKED_FOR_USER_INPUT:
            
                //
                // Go pending while we wait for the UI to be ours.
                //

                INET_ASSERT(pic_result == pic);
                fCleanupPic = FALSE; // the UI needs this info, don't delete

                // fall through ...
                            
            case UI_ACTION_CODE_BLOCKED_FOR_INTERNET_HANDLE:

                INET_ASSERT(dwError == ERROR_IO_PENDING);                
                    
                break;
        }
    }

#if 0
    else
    {
        BOOL fRet = FALSE;
        LPSTR lpszMsgBuf = NULL;
        LPSTR lplpArr[3];

        char buff[INTERNET_RFC1123_BUFSIZE], msg[MAX_PATH], titlebuff[MAX_PATH];

        SYSTEMTIME sSysTime;

        if (!LoadString(GlobalDllHandle, IDS_SETTING_COOKIE, msg, sizeof(msg))) {
            return SET_COOKIE_FAIL;
        }

        if (!LoadString(GlobalDllHandle, IDS_SETTING_COOKIE_TITLE, titlebuff, sizeof(titlebuff))) {
            titlebuff[0] = 0;
        }

        buff[0] = 0;

        // get a prettified time

        if (FileTimeToSystemTime(&ftExpire, &sSysTime) &&
            InternetTimeFromSystemTime(&sSysTime, INTERNET_RFC1123_FORMAT, buff, INTERNET_RFC1123_BUFSIZE)){
        }

        ReverseString(pchRDomain);

        lplpArr[0] = pchRDomain;
        lplpArr[1] = (pchValue)?pchValue:"";
        lplpArr[2] = buff;

        if(FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
            msg,
            IDS_SETTING_COOKIE,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
            (LPTSTR) &lpszMsgBuf,
            0,
            (va_list *)lplpArr)) { // BUGBUG Cast for ALPHA, dont know if it will work

            fRet = (MessageBox( NULL,
                                (LPCSTR)lpszMsgBuf,
                                (LPCSTR)titlebuff,
                                MB_ICONQUESTION|MB_YESNO|MB_SETFOREGROUND|MB_TOPMOST) == IDYES);

            FREE_MEMORY(lpszMsgBuf);
        }
        else {

            dwError = GetLastError();

        }

        ReverseString(pchRDomain);
        if( fRet )
            return SET_COOKIE_SUCCESS;
        else
            return SET_COOKIE_FAIL;
    }
#endif    

    if ( fCleanupPic )
    {
        DestroyInternetCookie(pic);
    }

    if (dwError != ERROR_SUCCESS)
    {
        SetLastError(dwError);
        
        if ( dwError == ERROR_IO_PENDING ) {
            return SET_COOKIE_PENDING;
        } else {
            return SET_COOKIE_FAIL;
        }
    }
    else
    {
        return SET_COOKIE_SUCCESS;
    }

}


DWORD
CCookieJar::SetCookie(HTTP_REQUEST_HANDLE_OBJECT *pRequest, const char *pchURL, char *pchHeader, DWORD dwFlags = 0)
{
    FILETIME ftExpire;
    FILETIME ftCurrent;
    char *pchName;
    char *pchValue;
    char *pchHeaderPath;
    char *pchHeaderRDomain;
    char *pchDocumentRDomain = NULL;
    char *pchDocumentPath = NULL;
    DWORD dwFlagsFromParse;
    BOOL  fDocumentSecure;
    BOOL  fDelete;
    DWORD dwRet = SET_COOKIE_FAIL;
    BOOL  fWriteToCacheFileNeeded;
    CCookieLocation *pLocation;

    ParseHeader(pchHeader, &pchName, &pchValue, &pchHeaderPath, &pchHeaderRDomain, &dwFlagsFromParse, &ftExpire);
    // merge flags given with those found by the parser.
    dwFlags |= dwFlagsFromParse;

    if (!PathAndRDomainFromURL(pchURL, &pchDocumentRDomain, &pchDocumentPath, &fDocumentSecure))
        goto Cleanup;

    //
    // Verify domain and path
    //

    if ((pchHeaderRDomain && !IsDomainLegal(pchHeaderRDomain, pchDocumentRDomain)) ||
        (pchHeaderPath && !IsPathLegal(pchHeaderPath, pchDocumentPath)))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto Cleanup;
    }

    if (!pchHeaderRDomain)
        pchHeaderRDomain = pchDocumentRDomain;

    if (!pchHeaderPath)
        pchHeaderPath = pchDocumentPath;

    //
    // Delete the cookie?
    //

    GetCurrentGmtTime(&ftCurrent);
    fDelete = CompareFileTime(ftCurrent, ftExpire) > 0;

    //
    // Ok to add?
    //

    if (!fDelete) {
        dwRet = OkToSetCookie(
                    pRequest,
                    pchURL,
                    pchHeaderRDomain,
                    pchHeaderPath,
                    pchName,
                    pchValue,
                    dwFlags,
                    ftExpire);

        if (dwRet != SET_COOKIE_SUCCESS )
            goto Cleanup;
    }

    //
    // Finally, we can add the cookie!
    //

    {
        CCookieCriticalSection cs;

        if (!SyncWithCacheIfNeeded())
            goto Cleanup;

        pLocation = GetLocation(pchHeaderRDomain, pchHeaderPath, !fDelete);

        if (pLocation)
        {
            pLocation->ReadCacheFileIfNeeded();
            fWriteToCacheFileNeeded = FALSE;

            if (fDelete)
            {
                fWriteToCacheFileNeeded |= pLocation->Purge(CCookie::PurgeByName, pchName);
            }
            else
            {
                CCookie *pCookie;

                EnforceCookieLimits(pLocation, pchName, &fWriteToCacheFileNeeded);

                pCookie = pLocation->GetCookie(pchName, TRUE);
                if (!pCookie)
                    goto Cleanup;

                pCookie->_ftLastModified = ftCurrent;

                if (memcmp(&ftExpire, &pCookie->_ftExpire, sizeof(FILETIME)) ||
                    strcmp(pchValue, pCookie->_pchValue) ||
                    dwFlags != pCookie->_dwFlags)
                {
                    fWriteToCacheFileNeeded |= pCookie->IsPersistent();

                    pCookie->_ftExpire = ftExpire;
                    pCookie->_dwFlags = dwFlags;
                    pCookie->SetValue(pchValue);

                    fWriteToCacheFileNeeded |= pCookie->IsPersistent();
                }
            }

            if (fWriteToCacheFileNeeded)
            {
                if (!pLocation->WriteCacheFile())
                    goto Cleanup;
            }
        }
    }

    dwRet = SET_COOKIE_SUCCESS;

Cleanup:

    if (pchDocumentRDomain)
        FREE_MEMORY(pchDocumentRDomain);
    if (pchDocumentPath)
        FREE_MEMORY(pchDocumentPath);

    return dwRet;
}

void
CCookieJar::EnforceCookieLimits(CCookieLocation *pLocationNew, char *pchNameNew, BOOL *fWriteToCacheFileNeeded)
{
    CCookieLocation *pLocation;
    CCookieLocation *pLocationVictim;
    CCookie *pCookie;
    CCookie *pCookieVictim = NULL;
    int nCookie = 0;

    for (pLocation = *GetBucket(pLocationNew->_pchRDomain); pLocation; pLocation = pLocation->_pLocationNext)
    {
        // Same domain?

        if (stricmp(pLocationNew->_pchRDomain, pLocation->_pchRDomain) == 0)
        {
            pLocation->ReadCacheFileIfNeeded();
            for (pCookie = pLocation->_pCookieKids; pCookie; pCookie = pCookie->_pCookieNext)
            {
                nCookie += 1;

                if (pLocation == pLocationNew && strcmp(pCookie->_pchName, pchNameNew) == 0)
                {
                    // No need to enforce limits when resetting existing cookie value.
                    return;
                }

                if (!pCookieVictim ||
                    CompareFileTime(pCookie->_ftLastModified, pCookieVictim->_ftLastModified) < 0)
                {
                    pCookieVictim = pCookie;
                    pLocationVictim = pLocation;
                }
            }
        }
    }

    if (nCookie >= 20)
    {
        INET_ASSERT(pCookieVictim != NULL && pLocationVictim != NULL);

        if (pLocationVictim->Purge(CCookie::PurgeThis, pCookieVictim))
        {
            pLocationVictim->WriteCacheFile();
        }
    }
}

//---------------------------------------------------------------------------
//
// External APIs
//
//---------------------------------------------------------------------------

BOOL
OpenTheCookieJar()
{
    if (s_pJar)
        return TRUE;

    s_pJar = CCookieJar::Construct();
    if (!s_pJar)
        return FALSE;

    InitializeCriticalSection(&s_csCookieJar);

    return TRUE;
}

void
CloseTheCookieJar()
{
    if (s_pJar)
    {
        DeleteCriticalSection(&s_csCookieJar);
        delete s_pJar;
    }

    s_pJar = NULL;
}

void
PurgeCookieJarOfStaleCookies()
{
    FILETIME ftCurrent;

    if (s_pJar)
    {
        CCookieCriticalSection cs;
        GetCurrentGmtTime(&ftCurrent);
        s_pJar->Purge(&ftCurrent, TRUE);
    }
}

INTERNETAPI BOOL WINAPI
InternetGetCookieW(
    LPCWSTR  lpszUrl,
    LPCWSTR  lpszCookieName,
    LPWSTR   lpszCookieData,
    LPDWORD lpdwSize
    )
{
    DEBUG_ENTER_API((DBG_INET,
                     Bool,
                     "InternetGetCookieW",
                     "%wq, %#x, %#x, %#x",
                     lpszUrl,
                     lpszCookieName,
                     lpszCookieData,
                     lpdwSize
                     ));

    DWORD dwErr = ERROR_SUCCESS;
    BOOL fResult = FALSE;
    MEMORYPACKET mpUrl, mpCookieName, mpCookieData;

    ALLOC_MB(lpszUrl,0,mpUrl);
    if (!mpUrl.psStr)
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }
    UNICODE_TO_ANSI(lpszUrl,mpUrl);
    if (lpszCookieName)
    {
        ALLOC_MB(lpszCookieName,0,mpCookieName);
        if (!mpCookieName.psStr)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        UNICODE_TO_ANSI(lpszCookieName,mpCookieName);
    }
    if (lpszCookieData)
    {
        mpCookieData.dwAlloc = mpCookieData.dwSize = *lpdwSize;
        mpCookieData.psStr = (LPSTR)ALLOC_BYTES(*lpdwSize);
        if (!mpCookieData.psStr)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
    }

    fResult = InternetGetCookieA(mpUrl.psStr, mpCookieName.psStr, mpCookieData.psStr, &mpCookieData.dwSize);

    *lpdwSize = mpCookieData.dwSize*sizeof(WCHAR);
    if (lpszCookieData)
    {
        if (mpCookieData.dwSize <= mpCookieData.dwAlloc)
        {
            MAYBE_COPY_ANSI(mpCookieData,lpszCookieData,*lpdwSize);
        }
        else
        {
            dwErr = ERROR_INSUFFICIENT_BUFFER;
            fResult = FALSE;
        }
    }

cleanup: 
    if (dwErr!=ERROR_SUCCESS) 
    { 
        SetLastError(dwErr); 
        DEBUG_ERROR(INET, dwErr);
    }
    DEBUG_LEAVE_API(fResult);
    return fResult;
}


//
//  InternetGetCookieEx only returns those cookies within domain pchURL
//with a name that maches pchCookieName
//  

BOOL InternetGetCookieEx(
    IN LPCSTR pchURL,
    IN LPCSTR pchCookieName OPTIONAL,
    IN LPSTR pchCookieData OPTIONAL,
    IN OUT LPDWORD pcchCookieData,
    IN DWORD dwFlags,
    IN LPVOID lpReserved)
{
    DEBUG_ENTER_API((DBG_INET,
                     Bool,
                     "InternetGetCookieA",
                     "%q, %#x, %#x, %#x",
                     pchURL,
                     pchCookieName,
                     pchCookieData,
                     pcchCookieData
                     ));

    //  force everyone to not give anything in lpReserved
    INET_ASSERT( lpReserved == NULL);
    if( lpReserved != NULL)
    {
        DEBUG_LEAVE_API(FALSE);
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //  force everyone to not give anything in dwFlags
    INET_ASSERT( dwFlags == 0);
    if( dwFlags != 0)
    {
        DEBUG_LEAVE_API(FALSE);
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    
    BOOL    fSuccess = FALSE;
    char *  pchRDomain = NULL;
    char *  pchPath = NULL;
    BOOL    fSecure;
    DWORD   cch = 0;
    BOOL    fFirst;
    int     cchName;
    int     cchValue;
    FILETIME ftCurrent;
    CCookieLocation *pLocation;
    CCookie *pCookie;
    DWORD dwErr = ERROR_SUCCESS;
    
    if (!pcchCookieData || !pchURL)
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto done;
    }

    if (!GlobalDataInitialized) {

        dwErr = GlobalDataInitialize();
        if (dwErr!= ERROR_SUCCESS) {
            goto done;
        }
    }

    // NOTE THIS SEEMS TO BE A BUG BUG BUG
    if (!PathAndRDomainFromURL(pchURL, &pchRDomain, &pchPath, &fSecure))
        goto Cleanup;

    fFirst = TRUE;
    GetCurrentGmtTime(&ftCurrent);

    {
        CCookieCriticalSection cs;

        if (!s_pJar->SyncWithCacheIfNeeded())
            goto Cleanup;

        for (pLocation = *s_pJar->GetBucket(pchRDomain); pLocation; pLocation = pLocation->_pLocationNext)
        {
            if (pLocation->IsMatch(pchRDomain, pchPath))
            {
                pLocation->ReadCacheFileIfNeeded();

                for (pCookie = pLocation->_pCookieKids; pCookie; pCookie = pCookie->_pCookieNext)
                {

                    BOOL fAllow = TRUE;
                    if (::GetCookiePolicy(
                            pchURL,
                            !(pCookie->IsPersistent())
                         ) == URLPOLICY_DISALLOW
                    )
                    {
                        fAllow = FALSE;
                    }

                    if (fAllow 
                        && pCookie->CanSend(&ftCurrent, fSecure)
                        && (pchCookieName == NULL
                            || StrCmp( pCookie->_pchName, pchCookieName) == 0))

                    {
                        if (!fFirst) cch += 2; // for ; <space>
                        cch += cchName = strlen(pCookie->_pchName);
                        cch += cchValue = strlen(pCookie->_pchValue);
                        if (cchName && cchValue) cch += 1; // for equal sign

                        if (pchCookieData && cch < *pcchCookieData)
                        {
                            if (!fFirst)
                            {
                                *pchCookieData++ = ';';
                                *pchCookieData++ = ' ';
                            }

                            if (cchName > 0)
                            {
                                memcpy(pchCookieData, pCookie->_pchName, cchName);
                                pchCookieData += cchName;

                                if (cchValue > 0)
                                {
                                    *pchCookieData++ = '=';
                                }
                            }

                            if (cchValue > 0)
                            {
                                memcpy(pchCookieData, pCookie->_pchValue, cchValue);
                                pchCookieData += cchValue;
                            }
                        }

                        fFirst = FALSE;
                    }
                }
            }
        }
    }

//TerminateBuffer:

    cch += 1;

    if (pchCookieData)
    {
        if (cch > *pcchCookieData)
        {
            dwErr = ERROR_INSUFFICIENT_BUFFER;
        }
        else
        {
            *pchCookieData = 0;
            fSuccess = TRUE;
        }
    }
    else
    {
        fSuccess = TRUE;
    }

    if (cch == 1)
    {
        dwErr = ERROR_NO_MORE_ITEMS;
        fSuccess = FALSE;
        cch = 0;
    }

    *pcchCookieData = cch;

Cleanup:

    if (pchRDomain)
        FREE_MEMORY(pchRDomain);
    if (pchPath)
        FREE_MEMORY(pchPath);

done:
    if (dwErr!=ERROR_SUCCESS)
    {
        SetLastError(dwErr);
        DEBUG_ERROR(INET, dwErr);
    }
    DEBUG_LEAVE_API(fSuccess);
    return fSuccess;
}


INTERNETAPI BOOL WINAPI
InternetGetCookieA(
    IN LPCSTR pchURL,
    IN LPCSTR pchCookieName OPTIONAL,
    IN LPSTR pchCookieData OPTIONAL,
    IN OUT LPDWORD pcchCookieData
    )
{
    //  Because the value in pchCookieName had no effect on
    //the previously exported API, Ex gets NULL to ensure
    //the behavior doesn't change.
    return InternetGetCookieEx( pchURL, NULL, pchCookieData, 
                                pcchCookieData, 0, NULL);
}


INTERNETAPI BOOL WINAPI
InternetSetCookieW(
    LPCWSTR  lpszUrl,
    LPCWSTR  lpszCookieName,
    LPCWSTR  lpszCookieData)
{
    DEBUG_ENTER_API((DBG_INET,
                     Bool,
                     "InternetSetCookieW",
                     "%wq, %#x, %#x",
                     lpszUrl,
                     lpszCookieName,
                     lpszCookieData
                     ));

    DWORD dwErr = ERROR_SUCCESS;
    BOOL fResult = FALSE;
    MEMORYPACKET mpUrl, mpCookieName, mpCookieData;

    if (lpszUrl)
    {
        ALLOC_MB(lpszUrl,0,mpUrl);
        if (!mpUrl.psStr)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        UNICODE_TO_ANSI(lpszUrl,mpUrl);
    }
    if (lpszCookieName)
    {
        ALLOC_MB(lpszCookieName,0,mpCookieName);
        if (!mpCookieName.psStr)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        UNICODE_TO_ANSI(lpszCookieName,mpCookieName);
    }
    if (lpszCookieData)
    {
        ALLOC_MB(lpszCookieData,0,mpCookieData);
        if (!mpCookieData.psStr)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        UNICODE_TO_ANSI(lpszCookieData,mpCookieData);
    }

    fResult = InternetSetCookieA(mpUrl.psStr, mpCookieName.psStr, mpCookieData.psStr);

cleanup: 
    if (dwErr!=ERROR_SUCCESS) 
    { 
        SetLastError(dwErr); 
        DEBUG_ERROR(INET, dwErr);
    }
    DEBUG_LEAVE_API(fResult);
    return fResult;
}

BOOL InternetSetCookieEx(
    LPCSTR  pchURL,
    LPCSTR  pchCookieName,
    LPCSTR  pchCookieData,
    DWORD   dwFlags,
    LPVOID   lpReserved
    )
{
    DEBUG_ENTER_API((DBG_INET,
                     Bool,
                     "InternetSetCookieA",
                     "%q, %#x, %#x",
                     pchURL,
                     pchCookieName,
                     pchCookieData
                     ));

    //  force everyone to not give anything in lpReserved
    INET_ASSERT( lpReserved == NULL);
    if( lpReserved != NULL)
    {
        DEBUG_LEAVE_API(FALSE);
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    int     cch;
    char *  pch;
    char    achHeader[CCH_COOKIE_MAX];
    int     cchT;
    DWORD dwErr = ERROR_SUCCESS;
    BOOL fResult = TRUE;
    
    if (!pchURL || !pchCookieData)
    {
        fResult = FALSE;
        dwErr = ERROR_INVALID_PARAMETER;
        goto done;
    }

    if (!GlobalDataInitialized) {
        dwErr = GlobalDataInitialize();
        if (dwErr!= ERROR_SUCCESS) {
            fResult = FALSE;
            goto done;
        }
    }

    cch = CCH_COOKIE_MAX - 2;  // one for null terminator, one for "="
    pch = achHeader;
    if (pchCookieName)
    {
        cchT = strlen(pchCookieName);
        if (cchT > cch)
            cchT = cch;
        memcpy(pch, pchCookieName, cchT);

        pch += cchT;
        cch -= cchT;

        memcpy(pch, "=", 1);
        pch += 1;
        cch -= 1;
    }

    // Ensure null termination upon overflow.
    if (cch <= 0)
        cch = 1;

    // Append the cookie data.
    lstrcpyn (pch, pchCookieData, cch);

    if(s_pJar->SetCookie(NULL, pchURL, achHeader, dwFlags) == SET_COOKIE_FAIL )
    {
        fResult = FALSE;
    }

done:
    if (dwErr!=ERROR_SUCCESS)
    {
        SetLastError(dwErr);
        DEBUG_ERROR(INET, dwErr);
    }
    DEBUG_LEAVE_API(fResult);
    return fResult;
}


INTERNETAPI BOOL WINAPI
InternetSetCookieA(
    LPCSTR  pchURL,
    LPCSTR  pchCookieName,
    LPCSTR  pchCookieData
    )
{
    return InternetSetCookieEx( pchURL, pchCookieName, pchCookieData, 0, NULL);
}

//
//  rambling comments, delete before checkin...
//
// returns struc, and pending, error
//  on subsequent attempts passes back, with index, or index incremented
// perhaps can store index in fsm, and the rest in UI 
//  need to handle multi dlgs, perhaps via checking added Cookie.
//


DWORD
HTTP_REQUEST_HANDLE_OBJECT::ExtractSetCookieHeaders(LPDWORD lpdwHeaderIndex)
{
    char  achHeader[CCH_COOKIE_MAX];
    DWORD cbHeader;
    DWORD iQuery = 0;
    int   cCookies = 0;
    DWORD error = ERROR_HTTP_COOKIE_DECLINED;

    INET_ASSERT(lpdwHeaderIndex);

    cbHeader = sizeof(achHeader) - 1;

    _ResponseHeaders.LockHeaders();

    iQuery = *lpdwHeaderIndex;

    while ( QueryResponseHeader(
            HTTP_QUERY_SET_COOKIE,
            achHeader,
            &cbHeader,
            0,
            &iQuery) == ERROR_SUCCESS)

    {
        achHeader[cbHeader] = 0;

        DWORD dwRet = s_pJar->SetCookie(this, GetURL(), achHeader);
        if (dwRet == SET_COOKIE_SUCCESS)
        {
            cCookies += 1;
            *lpdwHeaderIndex = iQuery;
            error = ERROR_SUCCESS;
        } 
        else if (dwRet == SET_COOKIE_PENDING ) 
        {
            error = ERROR_IO_PENDING;

            INET_ASSERT(iQuery != 0);
            *lpdwHeaderIndex = iQuery - 1; // back up and retry this cookie

            break;
        }

        cbHeader = sizeof(achHeader) - 1;
    }

    _ResponseHeaders.UnlockHeaders();

    return error;
}

int
HTTP_REQUEST_HANDLE_OBJECT::CreateCookieHeaderIfNeeded(VOID)
{
    int     cCookie = 0;
    char *  pchRDomain = NULL;
    char *  pchPath = NULL;
    BOOL    fSecure;
    DWORD   cch;
    int     cchName;
    int     cchValue;
    char *  pchHeader;
    char    achHeader[CCH_COOKIE_MAX];
    FILETIME ftCurrent;
    CCookieLocation *pLocation;
    CCookie *pCookie;

    // remove cookie header if it exists
    // BUGBUG - we are overriding the app. Original cookie code has this.  Don't know why.

    ReplaceRequestHeader(HTTP_QUERY_COOKIE, NULL, 0, 0, 0);


    if (!PathAndRDomainFromURL(GetURL(), &pchRDomain, &pchPath, &fSecure, FALSE))
        goto Cleanup;

    fSecure = GetOpenFlags() & INTERNET_FLAG_SECURE;
    GetCurrentGmtTime(&ftCurrent);

    {
        CCookieCriticalSection cs;

        if (!s_pJar->SyncWithCacheIfNeeded())
            goto Cleanup;

        LockHeaders();

        for (pLocation = *s_pJar->GetBucket(pchRDomain); pLocation; pLocation = pLocation->_pLocationNext)
        {
            if (pLocation->IsMatch(pchRDomain, pchPath))
            {
                pLocation->ReadCacheFileIfNeeded();

                for (pCookie = pLocation->_pCookieKids; pCookie; pCookie = pCookie->_pCookieNext)
                {
                    if (pCookie->CanSend(&ftCurrent, fSecure))
                    {
                        pchHeader = achHeader;

                        cch = 0;
                        cch += cchName = strlen(pCookie->_pchName);
                        cch += cchValue = strlen(pCookie->_pchValue);
                        if (cchName && cchValue) cch += 1; // for equal sign

                        if (cch < CCH_COOKIE_MAX)
                        {
                            if (cchName > 0)
                            {
                                memcpy(pchHeader, pCookie->_pchName, cchName);
                                pchHeader += cchName;

                                if (cchValue > 0)
                                {
                                    *pchHeader++ = '=';
                                }
                            }

                            if (cchValue > 0)
                            {
                                memcpy(pchHeader, pCookie->_pchValue, cchValue);
                                pchHeader += cchValue;
                            }

                            cCookie += 1;

                            if( ::GetCookiePolicy(
                                    GetURL(),
                                    !(pCookie->IsPersistent())
                                ) != URLPOLICY_DISALLOW
                            )
                            {

                                AddRequestHeader(HTTP_QUERY_COOKIE,
                                    achHeader,
                                    cch,
                                    0,
                                    HTTP_ADDREQ_FLAG_COALESCE_WITH_SEMICOLON);
                            }

                        }
                    } // if CanSend
                } // for pCookie
            } // if IsMatch
        } // for

        UnlockHeaders();
    }

Cleanup:

    if (pchRDomain)
        FREE_MEMORY(pchRDomain);
    if (pchPath)
        FREE_MEMORY(pchPath);

    return cCookie;
}
