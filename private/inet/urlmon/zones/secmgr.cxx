//  File:       secmgr.cxx
//
//  Contents:   This file implements the base IInternetSecurityManager interface
//
//  Classes:    CSecurityManager
//
//  Functions:
//
//  History: 
//
//----------------------------------------------------------------------------

#include "zonepch.h" // PCH HEADER FILE, DON'T INCLUDE ANYTHING ABOVE 

PerfDbgTag(tagCSecurityManager, "Urlmon", "Log Security Mgr", DEB_SESSION)
PerfDbgTag(tagZONEMAP_COMPONENTS, "Urlmon", "Log Security URL parser", DEB_SESSION)

#define PRIVATE static

#define ZERO        TEXT('0')
#define NINE        TEXT('9')
#define DOT         TEXT('.')
#define SLASH       TEXT('/')
#define BACKSLASH   TEXT('\\')
#define COLON       TEXT(':')
#define WILDCARD    TEXT('*')
#define SPACE       TEXT(' ')
#define HYPHEN      TEXT('-')
#define BAR         TEXT('|')
#define AT          TEXT('@')
#define PERCENT     TEXT('%')

#define MAX_IPRANGE 32

BOOL  CSecurityManager::s_bIPInit = FALSE;
BYTE * CSecurityManager::s_pRanges = NULL;
DWORD CSecurityManager::s_cNumRanges = 0;
DWORD CSecurityManager::s_cbRangeItem = 0;
DWORD CSecurityManager::s_dwNextRangeIndex = 0;

PRIVATE TCHAR chWildCard = WILDCARD;

CSecurityManager::CSecMgrCache CSecurityManager::s_smcache;

BOOL  CSecurityManager::s_bcsectInit = FALSE;
CRITICAL_SECTION CSecurityManager::s_csectIP; 
HANDLE CSecurityManager::CSecMgrCache::s_hMutexCounter;

CLSID * CSecurityManager::s_clsidAllowedList = NULL;
CRITICAL_SECTION CSecurityManager::s_csectAList;
DWORD CSecurityManager::s_dwNumAllowedControls;


// HACK: See assert below.  We have to parse '*' as a valid scheme for wildcarding purposes.
// The big number is to avoid collisions with the pre-defined URL_SCHEME_* numbers that start 
// at 0 and go up sequentially. 
#define URL_SCHEME_WILDCARD (0x0000FFFF)

typedef DWORD (APIENTRY *WNETGETCONNECTION) (LPSTR, LPSTR, LPDWORD);


// Simple class to force freeing of memory pointer.
class CFreeStrPtr 
{
public:
    CFreeStrPtr(LPWSTR pwsz) { m_pwsz = pwsz; }
    ~CFreeStrPtr()  { delete [] m_pwsz; }
private:
    LPWSTR m_pwsz;
};

#ifdef UNICODE
#define IsSpace IsCharSpaceW
#else
#define IsSpace isspace
#endif

// Scans a string for number from 0 to 255 inclusive.
PRIVATE BOOL ScanByte (LPCTSTR& psz, BYTE *bOut)
{
    DWORD dw;
    
    // first char
    if (*psz < ZERO || *psz > NINE)
        return FALSE;
    dw = *psz++ - ZERO;

    // second char
    if (*psz < ZERO || *psz > NINE)
        goto done;
    dw = 10 * dw + *psz++ - ZERO;

    // third char
    if (*psz < ZERO || *psz > NINE)
        goto done;
    dw = 10 * dw + *psz++ - ZERO;
    if (dw > 255)
        return FALSE;

done:
    *bOut = (BYTE) dw;
    return TRUE;
}

// Scans a string for a range, wrapping ScanByte
PRIVATE BOOL ScanRange (LPCTSTR& psz, BYTE* pbLow, BYTE* pbHigh)
{
    if (*psz == WILDCARD)
    {
        *pbLow  = 0;
        *pbHigh = 255;
        psz++; // move past *
        return TRUE;
    }

    if (!ScanByte (psz, pbLow))
        return FALSE;

    while (*psz == SPACE)
        psz++; // trim whitespace
    if (*psz != HYPHEN)
    {
        *pbHigh = *pbLow;
        return TRUE;
    }
    else
    {
        psz++; // move past -
        while (*psz == SPACE)
            psz++; // trim whitespace
        return ScanByte (psz, pbHigh);
    }
}


PRIVATE BOOL ReadIPRule (LPCTSTR psz, BYTE *pbLow, BYTE *pbHigh)
{
    // Note: ScanRange first param passed by reference.
    return
       (    ScanRange (psz, pbLow++, pbHigh++)
        &&  *psz++ == DOT
        &&  ScanRange (psz, pbLow++, pbHigh++)
        &&  *psz++ == DOT
        &&  ScanRange (psz, pbLow++, pbHigh++)
        &&  *psz++ == DOT
        &&  ScanRange (psz, pbLow++, pbHigh++)
       );
}


// This function is copied here from the network stack code because we don't want to 
// link urlmon with winsock. Urlmon is pulled in by the shell even in cases where there
// is no network connection.

/*
 * Internet address interpretation routine.
 * All the network library routines call this
 * routine to interpret entries in the data bases
 * which are expected to be an address.
 * The value returned is in network order.
 */
PRIVATE ULONG 
inet_addr(
    IN const TCHAR *cp
    )

/*++

Routine Description:

    This function interprets the character string specified by the cp
    parameter.  This string represents a numeric Internet address
    expressed in the Internet standard ".'' notation.  The value
    returned is a number suitable for use as an Internet address.  All
    Internet addresses are returned in network order (bytes ordered from
    left to right).

    Internet Addresses

    Values specified using the "." notation take one of the following
    forms:

    a.b.c.d   a.b.c     a.b  a

    When four parts are specified, each is interpreted as a byte of data
    and assigned, from left to right, to the four bytes of an Internet
    address.  Note that when an Internet address is viewed as a 32-bit
    integer quantity on the Intel architecture, the bytes referred to
    above appear as "d.c.b.a''.  That is, the bytes on an Intel
    processor are ordered from right to left.

    Note: The following notations are only used by Berkeley, and nowhere
    else on the Internet.  In the interests of compatibility with their
    software, they are supported as specified.

    When a three part address is specified, the last part is interpreted
    as a 16-bit quantity and placed in the right most two bytes of the
    network address.  This makes the three part address format
    convenient for specifying Class B network addresses as
    "128.net.host''.

    When a two part address is specified, the last part is interpreted
    as a 24-bit quantity and placed in the right most three bytes of the
    network address.  This makes the two part address format convenient
    for specifying Class A network addresses as "net.host''.

    When only one part is given, the value is stored directly in the
    network address without any byte rearrangement.

Arguments:

    cp - A character string representing a number expressed in the
        Internet standard "." notation.

Return Value:

    If no error occurs, inet_addr() returns an in_addr structure
    containing a suitable binary representation of the Internet address
    given.  Otherwise, it returns the value INADDR_NONE.

--*/

{
        register unsigned long val, base, n;
        register TCHAR c;
        unsigned long parts[4], *pp = parts;
        const unsigned long INADDR_NONE = -1;


again:
        /*
         * Collect number up to ``.''.
         * Values are specified as for C:
         * 0x=hex, 0=octal, other=decimal.
         */
        val = 0; base = 10;
        if (*cp == '0') {
            base = 8, cp++;
            if (*cp == 'x' || *cp == 'X')
                base = 16, cp++;
        }

        while (c = *cp) {
                // If it is a decimal digit..
                if (c <= NINE && c >= ZERO) {
                        val = (val * base) + (c - '0');
                        cp++;
                        continue;
                }
                // If we are base 16 and it is a hex digit...
                if ( base == 16 && 
                     ( (c >= TEXT('a') && c <= TEXT('f')) ||
                       (c >= TEXT('A') && c <= TEXT('F'))
                     )
                   )
                {
                        val = (val << 4) + (c + 10 - (islower(c) ? TEXT('a') : TEXT('A')));
                        cp++;
                        continue;
                }
                break;
        }
        if (*cp == '.') {
                /*
                 * Internet format:
                 *      a.b.c.d
                 *      a.b.c   (with c treated as 16-bits)
                 *      a.b     (with b treated as 24 bits)
                 */
                /* GSS - next line was corrected on 8/5/89, was 'parts + 4' */
                if (pp >= parts + 3) {
                        return ((unsigned long) -1);
                }
                *pp++ = val, cp++;
                goto again;
        }
        /*
         * Check for trailing characters.
         */
        if (*cp && !IsSpace(*cp)) {
                return (INADDR_NONE);
        }
        *pp++ = val;
        /*
         * Concoct the address according to
         * the number of parts specified.
         */
        n = (unsigned long)(pp-parts);
        switch ((int) n) {

        case 1:                         /* a -- 32 bits */
                val = parts[0];
                break;

        case 2:                         /* a.b -- 8.24 bits */
                if ((parts[0] > 0xff) || (parts[1] > 0xffffff)) {
                    return(INADDR_NONE);
                }
                val = (parts[0] << 24) | (parts[1] & 0xffffff);
                break;

        case 3:                         /* a.b.c -- 8.8.16 bits */
                if ((parts[0] > 0xff) || (parts[1] > 0xff) ||
                    (parts[2] > 0xffff)) {
                    return(INADDR_NONE);
                }
                val = (parts[0] << 24) | ((parts[1] & 0xff) << 16) |
                        (parts[2] & 0xffff);
                break;

        case 4:                         /* a.b.c.d -- 8.8.8.8 bits */
                if ((parts[0] > 0xff) || (parts[1] > 0xff) ||
                    (parts[2] > 0xff) || (parts[3] > 0xff)) {
                    return(INADDR_NONE);
                }
                val = (parts[0] << 24) | ((parts[1] & 0xff) << 16) |
                      ((parts[2] & 0xff) << 8) | (parts[3] & 0xff);
                break;

        default:
                return (INADDR_NONE);
        }

#if defined(UNIX) && defined(BIG_ENDIAN)
//  IEUNIX: Dont swap on BIG_ENDIAN Unix m/c.
        return (val);
#else
        val = (val & 0xff000000) >> 24 |
              (val & 0x00ff0000) >> 8  |
              (val & 0x0000ff00) << 8  |
              (val & 0x000000ff) << 24;
        return (val);
#endif
}


// Checks if site is in form of IP address.
PRIVATE BOOL ReadAddress (LPCTSTR pwszSite, BYTE *pb)
{
    ULONG ipaddr = inet_addr(pwszSite);

    if (ipaddr != -1)
    {
#ifndef UNIX
        *(ULONG*)pb = ipaddr;
#else
        memcpy(pb, &ipaddr, sizeof(ULONG));
#endif /* UNIX */
        return TRUE;
    }
    return FALSE;
}

const TCHAR cszFileProt[] = TEXT("file");

struct ZONEMAP_COMPONENTS
{
    // pointers into buffer passed to Crack
    LPCTSTR  pszProtocol, pszSite, pszDomain; 
    DWORD    cchProtocol, cchSite, cchDomain; 

    DWORD    nScheme;   // One of URL_SCHEME_* 
    BOOL     fAddr:1;    // whether name is in form of IP address
    BOOL     fIPRange:1; // whether name is in form of an IP Range.
    BYTE     bAddr[4];  // if IP address, components of IP address
    RANGE_ITEM rangeItem; // If IP Range, components of IP Range.

    BOOL     fDrive; // URL corresponds to a drive letter that couldn't be mapped to a network share name.
    DWORD    dwDriveType; // if so, drive type

    TCHAR    szProtBuf[INTERNET_MAX_SCHEME_LENGTH];
    TCHAR    szSiteBuf[MAX_PATH]; // used for remote drives
    TCHAR    szIPAddr[16];   // room for 255.255.255.255 + NULL
    
    HRESULT Crack (LPCTSTR pwszUrl, DWORD dwFlags, BOOL bIPRange = FALSE);

    BOOL SetUNC (LPSTR pszUNC);
};


BOOL ZONEMAP_COMPONENTS::SetUNC (LPSTR pszUNC)
{
    PerfDbgLog(tagZONEMAP_COMPONENTS, this, "+ZONEMAP_COMPONENTS::SetUNC");

    // Verify and strip leading backslashes.
    if (pszUNC[0] != '\\' || pszUNC[1] != '\\')
        return FALSE;
    pszUNC += 2;

    // Strip the share name from the host.
    LPSTR pszSlash = StrChrA (pszUNC, '\\');
    if (!pszSlash)
        return FALSE;
    *pszSlash = 0;
    DWORD cchUNC = (DWORD) (pszSlash - pszUNC);

    // Convert back to unicode.
    cchSite = MultiByteToWideChar
        (CP_ACP, 0, pszUNC, cchUNC, szSiteBuf, MAX_PATH);
    szSiteBuf[cchSite] = 0;
    pszSite = szSiteBuf;

    PerfDbgLog(tagZONEMAP_COMPONENTS, this, "-ZONEMAP_COMPONENTS::SetUNC");
    return TRUE;        
}


// Helper functions to help with URL cracking.

inline BOOL IsDosPath(LPCTSTR p)
{
#ifndef unix
    return (*p == BACKSLASH 
                || 
            /* it starts with "x:" where x is from the English alphabet */
                ( (*p) && 
                  ((*p >= TEXT('a') && *p <= TEXT('z')) || (*p >= TEXT('A') && *p <= TEXT('Z'))) && 
                  p[1] == COLON) );
#else
    return (*p == SLASH);
#endif /* unix */
}
    
inline BOOL IsDrive(LPCTSTR p)
{
#ifndef unix
    return (*p && (p[1] == COLON || p[1] == BAR));
#else
    return (*p == SLASH);
#endif /* unix */
}

inline BOOL IsWildcardScheme(LPCTSTR p, BOOL &bImplicit)
{
    BOOL bRet = FALSE;
    if ( p && *p )
    {
        if (*p == WILDCARD && p[1] == COLON)
        {
            bRet = TRUE;
            bImplicit = FALSE;
        }
        else if (StrChr(p, COLON) == NULL) 
        {
            // If there is no Colon in the string the user didn't specify a scheme
            // and we will assume it is a wildcard.
            // i.e *:*.microsoft.com is treated the same as *.microsoft.com

            bRet = TRUE;
            bImplicit = TRUE;
        }
    }
    return bRet;
}

// A scheme is opaque if we cannot interpret the URL after the scheme.
inline BOOL IsOpaqueScheme(DWORD dwScheme)
{
    return (dwScheme != URL_SCHEME_FILE && dwScheme != URL_SCHEME_WILDCARD && !IsHierarchicalScheme(dwScheme));
}


// Global Init functions.

BOOL CSecurityManager::GlobalInit( )
{
    InitializeCriticalSection(&s_csectIP);
    InitializeCriticalSection(&s_csectAList);
     
    CSecurityManager::s_bcsectInit = TRUE;
    return TRUE;
}
        
BOOL CSecurityManager::GlobalCleanup( )
{
    delete [] s_pRanges; 
    s_pRanges = NULL;

    if(s_clsidAllowedList)
    {
        delete [] s_clsidAllowedList;
        s_clsidAllowedList = NULL;
    }
    

    if ( s_bcsectInit )
    {
        DeleteCriticalSection(&s_csectIP) ; 
        DeleteCriticalSection(&s_csectAList);
    }

    return TRUE; 
}

VOID CSecurityManager::IncrementGlobalCounter( )
{
    CSecurityManager::CSecMgrCache::IncrementGlobalCounter( );
}

// Helper functions to deal with caching MapUrlToZone results.
    
HRESULT ZONEMAP_COMPONENTS::Crack (LPCTSTR pszScan, DWORD dwFlags, BOOL bIPRange /* = FALSE*/)
{
    PerfDbgLog(tagZONEMAP_COMPONENTS, this, "+ZONEMAP_COMPONENTS::Crack");
    fDrive = FALSE;
    fAddr = FALSE;
    fIPRange = FALSE;

    nScheme = URL_SCHEME_INVALID;
        
    if (IsDosPath(pszScan))
        dwFlags |= PUAF_ISFILE;
    
    if (dwFlags & PUAF_ISFILE)
    {
        pszProtocol = cszFileProt;
        cchProtocol = CSTRLENW(cszFileProt);
        nScheme = URL_SCHEME_FILE;
    }
    else
    {
        PARSEDURL pu;
        pu.cbSize = sizeof(pu);

        BOOL bImplicit = FALSE;        
        if ( (dwFlags & PUAF_ACCEPT_WILDCARD_SCHEME) && 
             IsWildcardScheme(pszScan, bImplicit)
           )
        {
            nScheme = URL_SCHEME_WILDCARD;
            pszProtocol = &chWildCard;
            cchProtocol = 1;
            // Skip over the *: if the user entered this explicity.
            if (!bImplicit)
                pszScan += 2;
        }
        else
        {
            HRESULT hr = ParseURL(pszScan, &pu);

            if (SUCCEEDED(hr))
            {
                nScheme = pu.nScheme;
                pszProtocol = pu.pszProtocol;

                cchProtocol = pu.cchProtocol;
                pszScan = pu.pszSuffix;
            }
            else
            {
                return hr;
            }

        }

        // Copy protocol to null terminate it.
        if (cchProtocol >= INTERNET_MAX_SCHEME_LENGTH)
            return E_INVALIDARG;
        else
        {
            memcpy (szProtBuf, pszProtocol, sizeof(TCHAR) * cchProtocol);
            szProtBuf[cchProtocol] = 0;
            pszProtocol = szProtBuf;
        }
    }
    

    // Opaque URLs - We cannot interpret anything besides the scheme. 
    // Just Treat the rest of the string as the Site.
    if (IsOpaqueScheme(nScheme))
    {
        pszSite = pszScan;
        cchSite = lstrlen(pszSite);
        pszDomain = NULL;
        cchDomain = 0;
    }
    else 
    {
#ifndef unix
        // Scan past leading '/' and '\' before site.
        while (*pszScan == SLASH || *pszScan == BACKSLASH)
            pszScan++;
#endif /* unix */ 
        pszSite = pszScan;

        // Is this a drive letter. If so we need to figure out whether it is local or remote.
        if (nScheme == URL_SCHEME_FILE && pszSite[0] != WILDCARD && IsDrive(pszSite))
        {
            fDrive = TRUE;

            char szDriveRoot[4];
            szDriveRoot[0] = (BYTE) pszSite[0];
#ifndef unix
            szDriveRoot[1] = ':';
            szDriveRoot[2] = '\\';
            szDriveRoot[3] = 0;
#else
        szDriveRoot[1] = 0;
#endif /* unix */

            dwDriveType = GetDriveTypeFromCacheA (szDriveRoot);

            if (dwDriveType == DRIVE_REMOTE)
            {
                // Strip the trailing backslash.
                szDriveRoot[2] = 0;
            
                char szUNC[MAX_PATH];
                DWORD cchUNC;
                cchUNC = MAX_PATH;

                if (NO_ERROR == WNetGetConnectionA(szDriveRoot, szUNC, &cchUNC))
                {
                    fDrive = FALSE;
                    SetUNC (szUNC);
                }
            }
        }
        
        // SetUNC might have come back with a new site. 
        
        pszScan = pszSite;

        if (fDrive)
        {
            // Just start using the drive as is. 
            cchSite = lstrlen(pszSite);
            cchDomain = 0;
            pszDomain = NULL;
        }
        else
        {
                    
            // Scan for characters which delimit site.
            while (1)
            {
                switch (*pszScan)
                {
                    case 0:        
                    case SLASH:    
                    case BACKSLASH:
                        break;
                
                    case TEXT('@'):
                        // This happens with custom protocols. Remove assert.
                        // TransAssert(FALSE);
                    default:
                        pszScan++;
                        continue;
                }
                break;        
            }

 
            cchSite = (DWORD) (pszScan - pszSite);

            pszDomain = NULL;
            cchDomain = 0;

            // Check for IP ranges if we are asked to first. 
            if (bIPRange)
            {
                fIPRange = ReadIPRule(pszSite, rangeItem.bLow, rangeItem.bHigh);
                if (fIPRange)
                    return S_OK;
            }

            // Check for names that are form of an IP address.
            fAddr = ReadAddress (pszSite, bAddr);
            if (fAddr)
            {
                cchSite = wnsprintf(szIPAddr, ARRAYSIZE(szIPAddr), TEXT("%d.%d.%d.%d"), bAddr[0], bAddr[1], bAddr[2], bAddr[3]);
                pszSite = szIPAddr;
                return S_OK;
            }

            // Scan backward for second '.' indicating domain,
            // ignoring two-char int'l domains like "co.uk" etc.

            DWORD cDot = 0;
            LPCTSTR pszDot = NULL;

            while (pszScan > pszSite)
            {
                if ( (*pszScan == DOT) && (pszScan < pszSite + cchSite - 2) )
                {
                    ++cDot;
                    if (cDot == 1)
                    {
                        pszDot = pszScan;
                    }
                    else if (   cDot == 2
                             && pszDot    // Non-null only the 1st time
                             && pszDot == pszSite + cchSite - 3   // Only check .?? ending
                             && pszScan + 3 >= pszDot)   // Check distance between the dots
                    {
                        // The distance between the 2 dots is less than 3 chars (.?? or smaller),
                        // so don't count this as a dot and don't check again.
                        --cDot;
                        pszDot = NULL;
                    }
                    else
                    {
                        pszDomain = pszScan + 1;
                        cchDomain = (DWORD) (pszSite + cchSite - pszDomain); 
                        cchSite = (DWORD) (pszScan - pszSite);
                        break;
                    }
                }
                pszScan--;
            }
        }
    }

    PerfDbgLog(tagZONEMAP_COMPONENTS, this, "-ZONEMAP_COMPONENTS::Crack");

    return S_OK;
}
    


// Function to get a new IInternetSecurityManager from outside. We might replace this
// with a standard class factory eventually.

STDAPI 
InternetCreateSecurityManager
(
    IUnknown * pUnkOuter,
    REFIID  riid,
    void **ppvObj,
    DWORD dwReserved
)
{
    PerfDbgLog(tagCSecurityManager, NULL, "+InternetCreateSecurityManager");
   
    HRESULT hr = S_OK;      
    *ppvObj = NULL;

    if ( !IsZonesInitialized() )
        return E_UNEXPECTED;

    if (dwReserved != 0 || !ppvObj || (pUnkOuter && riid != IID_IUnknown))
    {
        // If the object has to be aggregated the caller can only ask
        // for an IUnknown back.
        hr = E_INVALIDARG;
    }
    else 
    {
        CSecurityManager * pSecMgr = new CSecurityManager(pUnkOuter, (IUnknown **)ppvObj);

        if ( pSecMgr )
        {

            if (riid == IID_IUnknown || riid == IID_IInternetSecurityManager)
            {
                // The correct pointer is in ppvObj
                *ppvObj = (IInternetSecurityManager *)pSecMgr;
            }
            else 
            {
                hr = E_NOINTERFACE;
            }
        }
        else 
        {
            hr = E_OUTOFMEMORY;
        }
    }

    PerfDbgLog1(tagCSecurityManager, NULL, "-InternetCreateSecurityManager (hr:%lx)", hr);

    return hr;
}


// Class Initialization-Destruction.


CSecurityManager::CSecurityManager(IUnknown *pUnkOuter, IUnknown** ppUnkInner)
{
    PerfDbgLog(tagCSecurityManager, this, "+CSecurityManager::CSecurityManager");

    DllAddRef();

    m_pSite = NULL;
    m_pDelegateSecMgr = NULL;

    m_pZoneManager = NULL;
    
    if (!pUnkOuter)
    {
        pUnkOuter = &m_Unknown;
    }
    else
    {
        TransAssert(ppUnkInner);
        if (ppUnkInner)
        {
            *ppUnkInner = &m_Unknown;
            m_ref = 0;
        }
    }

    m_pUnkOuter = pUnkOuter;

    if (ERROR_SUCCESS != m_regZoneMap.Open (NULL, SZZONEMAP, KEY_READ))
        goto done;

    EnterCriticalSection(&s_csectIP);
    if (!s_bIPInit)
    {
        ReadAllIPRules();
        s_bIPInit = TRUE;
    }
    LeaveCriticalSection(&s_csectIP);
    
done:
    PerfDbgLog(tagCSecurityManager, this, "-CSecurityManager::CSecurityManager");
    return;
}


CSecurityManager::~CSecurityManager()
{
    // Due to a circular dependency between wininet and urlmon, this function 
    // could get called after the dlls global uninit has happened. PerfDbgLog depends on
    // some global Mutext objects and fails because of this reason.
    // PerfDbgLog(tagCSecurityManager, this, "+~CSecurityManager::CSecurityManager");

    if (m_pZoneManager != NULL)
        m_pZoneManager->Release();

    if (m_pSite != NULL)
        m_pSite->Release();

    if (m_pDelegateSecMgr != NULL)
        m_pDelegateSecMgr->Release();
    
    DllRelease();

    // PerfDbgLog(tagCSecurityManager, this, "-~CSecurityManager::CSecurityManager");
}

BOOL CSecurityManager::EnsureZoneManager()
{
    if (m_pZoneManager == NULL)
    {
        if (SUCCEEDED(InternetCreateZoneManager(NULL, 
                          IID_IInternetZoneManager, (void **)&m_pZoneManager, NULL)))
        {
            TransAssert(m_pZoneManager != NULL);
        }
    }
    
    return (m_pZoneManager != NULL);
}
                                
STDMETHODIMP CSecurityManager::CPrivUnknown::QueryInterface(REFIID riid, void** ppvObj)
{

    HRESULT hr = S_OK;
    
    *ppvObj = NULL;

    CSecurityManager * pSecurityManager = GETPPARENT(this, CSecurityManager, m_Unknown);
        
    if (riid == IID_IUnknown || riid == IID_IInternetSecurityManager)
    {
        *ppvObj = (IInternetSecurityManager *)pSecurityManager;
        pSecurityManager->AddRef();
    }
    else 
    {
        hr = E_NOINTERFACE;
    }

    
    return hr;
}
                
STDMETHODIMP_(ULONG) CSecurityManager::CPrivUnknown::AddRef()
{
    LONG lRet = ++m_ref;

    return lRet;
}

STDMETHODIMP_(ULONG) CSecurityManager::CPrivUnknown::Release()
{

    CSecurityManager *pSecurityManager = GETPPARENT(this, CSecurityManager, m_Unknown);

    LONG lRet = --m_ref;

    if (lRet == 0)
    {
        delete pSecurityManager;
    }

    return lRet;
}

// IUnknown methods
STDMETHODIMP CSecurityManager::QueryInterface(REFIID riid, void **ppvObj)
{
    PerfDbgLog(tagCSecurityManager, this, "+CSecurityManager::QueryInterface");
    HRESULT hr = E_NOINTERFACE;

    *ppvObj = NULL;

    if (riid == IID_IUnknown || riid == IID_IInternetSecurityManager)
    {
        *ppvObj = (IInternetSecurityManager *)this;
    }

    if (*ppvObj != NULL)
    {
        ((IUnknown *)*ppvObj)->AddRef();
        hr = S_OK;
    }

    PerfDbgLog1(tagCSecurityManager, this, "-CSecurityManager::QueryInterface (hr:%lx)", hr);
    return hr;
}

STDMETHODIMP_(ULONG) CSecurityManager::AddRef()
{
    LONG lRet = m_pUnkOuter->AddRef();

    return lRet;
}

STDMETHODIMP_(ULONG) CSecurityManager::Release()
{                            
    LONG lRet = m_pUnkOuter->Release();

    // Controlling Unknown will delete the object if reqd.
        
    return lRet;
}

// IInternetSecurityManager methods

STDMETHODIMP 
CSecurityManager::MapUrlToZone
(
    LPCWSTR pwszUrl,
    DWORD *pdwZone,
    DWORD dwFlags
)
{
    PerfDbgLog(tagCSecurityManager, this, "+CSecurityManager::MapUrlToZone");
    HRESULT hr = INET_E_DEFAULT_ACTION;

    if (m_pDelegateSecMgr)
    {
        hr = m_pDelegateSecMgr->MapUrlToZone(pwszUrl, pdwZone, dwFlags);
    }

    // Check the cache to see if we already know the answer.
    /* BUGBUG - why are we stomping the delegate's result if we have a cache value?
    if ((NULL != m_pszPrevUrl) && (0 == StrCmpI(m_pszPrevUrl, pwszUrl)) && IsCounterEqual())
    { 
        hr = S_OK;
        *pdwZone = m_dwPrevZone;
    }
    */
            
    if (hr == INET_E_DEFAULT_ACTION)
    {
        LPWSTR pwszSecUrl = NULL;

        if (pdwZone == NULL || pwszUrl == NULL)
        {
            hr = E_INVALIDARG;
        }

        if (!s_smcache.Lookup(pwszUrl, pdwZone))
        {
            hr =  CoInternetGetSecurityUrl(pwszUrl, &pwszSecUrl, PSU_DEFAULT, 0);

            if (SUCCEEDED(hr))
            {
                TransAssert(pwszSecUrl);
                CFreeStrPtr freeStr(pwszSecUrl);

                // Reduce the URL here.  Need to do this up to two times.
                //
                for (int i = 1; i <= 2; i++)
                {
                    if (StrChr(pwszSecUrl, PERCENT))
                    {
                        UrlUnescapeW(pwszSecUrl, NULL, 0, URL_UNESCAPE_INPLACE);
                    }
                    else
                        break;
                }

                ZONEMAP_COMPONENTS zc;
                if (SUCCEEDED(hr = zc.Crack (pwszSecUrl, dwFlags)))
                {
                    BOOL fMarked;

                    hr = MapUrlToZone (&zc, pdwZone, dwFlags, &fMarked);

                    if (hr == S_OK && !(dwFlags & MUTZ_NOCACHE))
                    {
                        s_smcache.Add(pwszUrl, *pdwZone, fMarked ); 
                    }
                }
            }
        }
        else 
        {
            hr = S_OK;
        }
    }

    PerfDbgLog1(tagCSecurityManager, this, "-CSecurityManager::MapUrlToZone (hr:%lx)", hr);
    return hr;
}    

VOID 
CSecurityManager::PickZCString(ZONEMAP_COMPONENTS *pzc, LPCWSTR *ppwsz, DWORD *pcch, LPCWSTR pwszDocDomain)
{
    if (pzc->cchDomain)
    {
        TransAssert (!pzc->fDrive);
        // We should use the whole site even if we were able to get the
        // primary domain. i.e. security id for http://www.microsoft.com 
        // should be http:www.microsoft.com and NOT http:microsoft.com
        // We will use the fact that the domain and site are actually
        // pointing into one contiguous string to get back at the 
        // whole string.
        TransAssert((pzc->cchSite + 1) == (DWORD)(pzc->pszDomain - pzc->pszSite));
        TransAssert(pzc->pszSite[pzc->cchSite] == DOT);
        *ppwsz = pzc->pszSite;
        *pcch = pzc->cchSite + pzc->cchDomain + 1;
    }
    else if (pzc->fDrive && pzc->dwDriveType != DRIVE_REMOTE)
    {
        *ppwsz = TEXT("");
        *pcch = 0;
    }
    else if (pzc->nScheme == URL_SCHEME_FILE)
    {
        // For URL's of the nature \\server\sharename we want to include both the server and sharename in 
        // the security ID. This permits me from looking at \\server\private by putting up a page on 
        // \\server\public  At thuis point pzc->pszSite should point to the string "server\private\foo.htm"

        LPCTSTR lpszCurr = pzc->pszSite;
        BOOL bFoundFirstSlash = FALSE;
        
        for (; *lpszCurr != NULL ; lpszCurr++)
        {
            if (*lpszCurr == SLASH || *lpszCurr == BACKSLASH)
            {
                if (bFoundFirstSlash)
                {
                    // This is the second slash we are done.
                    break;
                }
                else
                {
                    bFoundFirstSlash = TRUE;
                }
            }
        }
                                        
        *ppwsz = pzc->pszSite;

        if (lpszCurr != NULL)
        {
            *pcch = (DWORD)(lpszCurr - pzc->pszSite);
        }
        else
        {
            TransAssert(FALSE);
            *pcch = pzc->cchSite;
        }
    }
    else
    {
        TransAssert (!pzc->pszDomain && !pzc->cchDomain);
        *ppwsz = pzc->pszSite;
        *pcch = pzc->cchSite;
    }

    // If the domain string passed is a suffix of the site string we will 
    // use that string instead.
    if (*pcch && pwszDocDomain != 0)
    {
        DWORD cchDocDomain = lstrlenW(pwszDocDomain);

        if (*pcch > cchDocDomain && 
            (0 == StrCmpNICW(pwszDocDomain, &((*ppwsz)[*pcch - cchDocDomain]), cchDocDomain)))
        {
            *ppwsz = pwszDocDomain;
            *pcch = cchDocDomain;
        }
    }   
}

STDMETHODIMP
CSecurityManager::GetSecurityId
(
        /* [in] */ LPCWSTR pwszUrl,
        /* [size_is][out] */ BYTE* pbSecurityId,
        /* [out][in] */ DWORD *pcbSecurityId,
        /* [in] */ DWORD_PTR dwReserved
)
{
    PerfDbgLog(tagCSecurityManager, this, "Called CSecurityManager::GetSecurityId");
    HRESULT hr = INET_E_DEFAULT_ACTION;

    // Check args ...
    if (pwszUrl == NULL || !pwszUrl[0] || !pcbSecurityId )
    {
        return E_INVALIDARG;
    }


    if (m_pDelegateSecMgr)
    {
        hr = m_pDelegateSecMgr->GetSecurityId(pwszUrl, pbSecurityId, pcbSecurityId, dwReserved);
    }

    if (hr == INET_E_DEFAULT_ACTION)
    {
        BOOL fFoundInCache;
        BOOL        fMarked = FALSE;
        DWORD       dwZone;
        DWORD       cbSecurityID = *pcbSecurityId;

        fFoundInCache = s_smcache.Lookup(pwszUrl, &dwZone, &fMarked, pbSecurityId, &cbSecurityID, (LPCWSTR)dwReserved);

        // if it wasn't in the cache, or the url and zone were there, but not the security ID,
        // then we still need to do some work.
        if ( !fFoundInCache || cbSecurityID == 0 )
        {
            LPWSTR pwszSecUrl = NULL;
            DWORD dwFlags = 0;

            hr = CoInternetGetSecurityUrl(pwszUrl, &pwszSecUrl, PSU_DEFAULT, 0);

            if (SUCCEEDED(hr))
            {
                TransAssert(pwszSecUrl != NULL);
                CFreeStrPtr freeStr(pwszSecUrl);

                // Crack the URL.
                ZONEMAP_COMPONENTS zc;
                LPWSTR pwszMarkURL = NULL;
                LPWSTR pwszSecUrl2 = NULL;

                if (S_OK != zc.Crack (pwszSecUrl, dwFlags))
                    return E_INVALIDARG;

                // Select middle portion of Id.
                LPCWSTR psz2;
                DWORD  cch2;

                PickZCString(&zc, &psz2, &cch2, (LPCWSTR)dwReserved);
                // Identify the zone and determine if the ID will bear the 
                // Mark of the Web.
                
                // if the url was found in the cache, the an earlier MapUrlToZone
                // put it there with its zone and Marked flag, 
                if (!fFoundInCache || fMarked)
                {
                    hr = MapUrlToZone (&zc, &dwZone, 0, &fMarked, &pwszMarkURL);
                    TransAssert (hr == S_OK);

                    // If the Mark of the Web is present, then take the Mark URL
                    // and substitute it for the original one in the zc, this will
                    // allow us to create a security ID that embodies the original
                    // domain, which in turn allows us to recreate the cross-domain
                    // frame security. The '*' we add to the end will prevent a
                    // potentially compromised page on the user's disk from accessing
                    // frames of the live, original site if the two should wind up
                    // in the same frameset.
                    if (fMarked)
                    {
                        TransAssert(pwszMarkURL != NULL);

                        hr = CoInternetGetSecurityUrl(pwszMarkURL, &pwszSecUrl2, PSU_DEFAULT, 0);

                        if (SUCCEEDED(hr))
                        {
                            TransAssert(pwszSecUrl2 != NULL);

                            if(SUCCEEDED(zc.Crack (pwszSecUrl2, dwFlags)))
                                PickZCString(&zc, &psz2, &cch2, (LPCWSTR)dwReserved);
                            else
                            {
                                if (pwszSecUrl2)  delete [] pwszSecUrl2;
                                if (pwszMarkURL)  LocalFree(pwszMarkURL);

                                return E_INVALIDARG;
                            }
                        }
                    }
                }

                // Calculate required size of buffer.
                DWORD cbTail = sizeof(DWORD) + ((fMarked)? sizeof(CHAR) : 0);
                DWORD cbSite = 0;

                if (cch2 != 0)
                {
                    cbSite = WideCharToMultiByte(CP_ACP, 0, psz2, cch2,
                            NULL, 0, NULL, NULL);
                    if (cbSite == 0)
                        return(HRESULT_FROM_WIN32(GetLastError()));
                }

                DWORD cbRequired = zc.cchProtocol + 1 + cbSite + cbTail;

                if (*pcbSecurityId < cbRequired)
                {
                    *pcbSecurityId = cbRequired;
                    return HRESULT_FROM_WIN32 (ERROR_INSUFFICIENT_BUFFER);
                }

                // Emit the protocol in ANSI.
                DWORD cbOut;
                cbOut = WideCharToMultiByte (CP_ACP, 0, zc.pszProtocol, zc.cchProtocol,
                    (LPSTR) pbSecurityId, *pcbSecurityId, NULL, NULL);
                if (cbOut != zc.cchProtocol)
                    return E_INVALIDARG; // non-ascii chars illegal in URL scheme
                pbSecurityId[cbOut++] = ':';

                // Emit the site/domain in ANSI.
                if (cch2 != 0)
                {
                    cbSite = WideCharToMultiByte (CP_ACP, 0, psz2, cch2,
                        (LPSTR) pbSecurityId + cbOut, *pcbSecurityId - cbOut, NULL, NULL);
                }


                // HACK: Need to figure out a better way to fix this. 
                // File: url's can come in with slashes and backslashes as seperators.
                // To prevent things from breaking we replace any slashes in the 
                // pbSecurityId with a backslash.
                if (zc.nScheme == URL_SCHEME_FILE && cch2 != 0)
                {
                    LPSTR lpszStart = (LPSTR)pbSecurityId + cbOut;
                    LPSTR lpsz = lpszStart;

                    while (lpsz < lpszStart + cbSite)
                    {
                        if (*lpsz == '/')
                            *lpsz = '\\';

                        lpsz = CharNextA(lpsz);
                    }
                }

                cbOut += cbSite;

                // Downcase the buffer.
                pbSecurityId[cbOut] = 0;
                CharLowerA ((LPSTR) pbSecurityId);

                // Add the zone.
                memcpy(pbSecurityId + cbOut, &dwZone, sizeof(DWORD));

                if (fMarked)
                {
                    CHAR chMark = WILDCARD;
                    memcpy(pbSecurityId + cbOut + sizeof(DWORD), &chMark, sizeof(CHAR));
                }

                // Report the output data size.
                *pcbSecurityId = cbRequired;

                // Now that we have all the pieces, (re)add it to the cache
                s_smcache.Add(pwszUrl, dwZone, fMarked, pbSecurityId, cbRequired, (LPCWSTR)dwReserved);

                if (pwszSecUrl2)
                    delete [] pwszSecUrl2 ;

                if (pwszMarkURL)
                    LocalFree(pwszMarkURL);

            } // got security URL 
            
        } // got security URL
        else
        {
            // Got it from the cache.
            *pcbSecurityId = cbSecurityID;
            hr = S_OK;
        }
    } // delegate missing or wants us to do the work

    return hr;
}


// Helper functions to do generic UI from within ProcessUrlAction.

struct ActionStrIDMap 
{
    DWORD dwAction;
    DWORD dwStrID;
};

ActionStrIDMap actionAlertIDMap [ ]  = 
{
    { URLACTION_DOWNLOAD_SIGNED_ACTIVEX,            IDS_ACTION_DL_SIGNED_ACTIVEX        },
    { URLACTION_DOWNLOAD_UNSIGNED_ACTIVEX,          IDS_ACTION_DL_ACTIVEX               },

    { URLACTION_ACTIVEX_RUN,                        IDS_ACTION_AX_RUN                   },
    { URLACTION_ACTIVEX_OVERRIDE_OBJECT_SAFETY,     IDS_ACTION_AX_OVERRIDE_SAFETY       },
    { URLACTION_ACTIVEX_OVERRIDE_DATA_SAFETY,       IDS_ACTION_AX_OVERRIDE_DATA_SAFETY  },
    { URLACTION_ACTIVEX_OVERRIDE_SCRIPT_SAFETY,     IDS_ACTION_AX_OVERRIDE_SCRIPT_SAFETY},
    { URLACTION_ACTIVEX_CONFIRM_NOOBJECTSAFETY,     IDS_ACTION_AX_CONFIRM_NOSAFETY      },

    { URLACTION_SCRIPT_RUN,                         IDS_ACTION_SCRIPT_RUN               },
    { URLACTION_SCRIPT_OVERRIDE_SAFETY,             IDS_ACTION_SCRIPT_OVERRIDE_SAFETY   },
    { URLACTION_SCRIPT_JAVA_USE,                    IDS_ACTION_SCRIPT_JAVA              },
    { URLACTION_SCRIPT_SAFE_ACTIVEX,                IDS_ACTION_SCRIPT_SAFE_ACTIVEX      },
    { URLACTION_CROSS_DOMAIN_DATA,                  IDS_ACTION_CROSS_DOMAIN_DATA        },
    { URLACTION_SCRIPT_PASTE,                       IDS_ACTION_SCRIPT_PASTE             },


    { URLACTION_HTML_SUBMIT_FORMS,                  IDS_ACTION_HTML_FORMS               },
    { URLACTION_HTML_SUBMIT_FORMS_FROM,             IDS_ACTION_HTML_FORMS               },
    { URLACTION_HTML_SUBMIT_FORMS_TO,               IDS_ACTION_HTML_FORMS               },
    { URLACTION_HTML_FONT_DOWNLOAD,                 IDS_ACTION_HTML_FONT_DL             },
    { URLACTION_HTML_JAVA_RUN,                      IDS_ACTION_HTML_JAVA                },
    { URLACTION_HTML_USERDATA_SAVE,                 IDS_ACTION_HTML_USERDATA            },
    { URLACTION_HTML_SUBFRAME_NAVIGATE,             IDS_ACTION_HTML_SUBFRAME_NAVIGATE   },

    { URLACTION_SHELL_INSTALL_DTITEMS,              IDS_ACTION_SHELL_INSTALL_DTITEMS    }, 
    { URLACTION_SHELL_MOVE_OR_COPY,                 IDS_ACTION_SHELL_MOVE_OR_COPY       },
    { URLACTION_SHELL_FILE_DOWNLOAD,                IDS_ACTION_SHELL_FILE_DL            },
    { URLACTION_SHELL_VERB,                         IDS_ACTION_SHELL_VERB               },
    { URLACTION_SHELL_WEBVIEW_VERB,                 IDS_ACTION_SHELL_WEBVIEW_VERB       },

    { URLACTION_CREDENTIALS_USE,                    IDS_ACTION_NW_CREDENTIALS           },
    { URLACTION_AUTHENTICATE_CLIENT,                IDS_ACTION_NW_AUTH_CLIENT           },
    { URLACTION_COOKIES,                            IDS_ACTION_NW_COOKIES               },
    { URLACTION_COOKIES_SESSION,                    IDS_ACTION_NW_COOKIES_SESSION       },

};

ActionStrIDMap actionWarnIDMap [ ]  = 
{
    { URLACTION_SHELL_INSTALL_DTITEMS,              IDS_WARN_SHELL_INSTALL_DTITEMS      }, 
    { URLACTION_SHELL_MOVE_OR_COPY,                 IDS_WARN_SHELL_MOVE_OR_COPY         },
    { URLACTION_SHELL_FILE_DOWNLOAD,                IDS_WARN_SHELL_FILE_DL              },
    { URLACTION_SHELL_VERB,                         IDS_WARN_SHELL_VERB                 },
    { URLACTION_SHELL_WEBVIEW_VERB,                 IDS_WARN_SHELL_WEBVIEW_VERB         },

    { URLACTION_HTML_SUBMIT_FORMS,                  IDS_WARN_HTML_FORMS                 },
    { URLACTION_HTML_SUBMIT_FORMS_FROM,             IDS_WARN_HTML_FORMS                 },
    { URLACTION_HTML_SUBMIT_FORMS_TO,               IDS_WARN_HTML_FORMS                 },
};   

STDMETHODIMP_(DWORD)
CSecurityManager::GetAlertIdForAction(DWORD dwAction)
{
    // The action should exist in our map.
    int count = ARRAYSIZE(actionAlertIDMap);

    for ( int i = 0 ; i < count ; i++ )
    {
        if (actionAlertIDMap[i].dwAction == dwAction)
        {
            return actionAlertIDMap[i].dwStrID;
        }
    }

    // If we get here the Action was invalid or we are missing an
    // entry in the map.
    TransAssert(FALSE);

    return IDS_ACTION_UNKNOWN;
}

STDMETHODIMP_(DWORD)
CSecurityManager::GetWarnIdForAction(DWORD dwAction)
{
    // The action should exist in our map.
    int count = ARRAYSIZE(actionWarnIDMap);

    for ( int i = 0 ; i < count ; i++ )
    {
        if (actionWarnIDMap[i].dwAction == dwAction)
        {
            return actionWarnIDMap[i].dwStrID;
        }
    }

    // If we get here the Action was invalid or we are missing an
    // entry in the map.
    TransAssert(FALSE);

    return IDS_WARN_UNKNOWN;
}

// INFRASTRUCTURE FOR REMEMBERING ANSWERS.
 
// Are the answers for this action supposed to persist. i.e. if the user says 'Yes' or 'No' for 
// these actions we will not requery them for the same URL for the duration of the security 
// manager.  

BOOL
CSecurityManager::CPersistAnswers::IsPersistentAnswerAction(DWORD dwAction)
{
    switch (dwAction)
    {
        case URLACTION_ACTIVEX_RUN:
        // This shouldn't happen because it is an aggregator
        case URLACTION_ACTIVEX_OVERRIDE_OBJECT_SAFETY:
        // These should never get called because of KludgeMapAggregatePolicy
        case URLACTION_ACTIVEX_OVERRIDE_DATA_SAFETY:
        case URLACTION_ACTIVEX_OVERRIDE_SCRIPT_SAFETY:
        case URLACTION_SCRIPT_OVERRIDE_SAFETY:

        case URLACTION_ACTIVEX_CONFIRM_NOOBJECTSAFETY:
        case URLACTION_SCRIPT_RUN:
        case URLACTION_SCRIPT_JAVA_USE:
        case URLACTION_HTML_FONT_DOWNLOAD:
        case URLACTION_SCRIPT_SAFE_ACTIVEX:
        case URLACTION_CROSS_DOMAIN_DATA:
            return TRUE;
        default:
            return FALSE;
    }
}

// CAnswerEntry methods. 

CSecurityManager::CPersistAnswers::CAnswerEntry::CAnswerEntry(LPCWSTR pszUrl, DWORD dwAction, BOOL iAnswer)
{
    m_pNext = NULL;
    m_dwAction = dwAction;
    m_iAnswer = iAnswer;
    m_pszUrl = StrDup(pszUrl);
}

CSecurityManager::CPersistAnswers::CAnswerEntry::~CAnswerEntry( )
{
    if (m_pszUrl)
        LocalFree(m_pszUrl);           
}

BOOL CSecurityManager::CPersistAnswers::CAnswerEntry::MatchEntry(LPCWSTR pszUrl, DWORD dwAction)
{
    return (dwAction == m_dwAction && (0 == StrCmp(pszUrl, m_pszUrl)));
}

// CPersistAnswers methods.
CSecurityManager::CPersistAnswers::~CPersistAnswers( )
{
    // go through the CAnswerEntries and free them up. 
    CAnswerEntry * pEntry = m_pAnswerEntry;

    while ( pEntry )
    {
        CAnswerEntry * pDelete = pEntry;
        pEntry = pEntry->GetNext( );
        delete pDelete;
    }
}                    

// Returns TRUE if the user already answered this questions. FALSE otherwise.
// 
BOOL CSecurityManager::CPersistAnswers::GetPrevAnswer(LPCWSTR pszUrl, DWORD dwAction, INT* piAnswer)
{
    BOOL bReturn = FALSE;
    CAnswerEntry * pAnswerEntry;

    if (!IsPersistentAnswerAction(dwAction))
        return FALSE;
                 
    for (pAnswerEntry = m_pAnswerEntry ; pAnswerEntry != NULL; pAnswerEntry = pAnswerEntry->GetNext())
    {
        if (pAnswerEntry->MatchEntry(pszUrl, dwAction))
        {
            if (piAnswer)
                *piAnswer = pAnswerEntry->GetAnswer( );

            bReturn = TRUE;
            break;
        }
    }

    return bReturn;
}

VOID CSecurityManager::CPersistAnswers::RememberAnswer(LPCWSTR pszUrl, DWORD dwAction, BOOL iAnswer)
{
    // Nothing to do if we are not supposed to be persisted. 
    if (!IsPersistentAnswerAction(dwAction))
        return;

    TransAssert(!GetPrevAnswer(pszUrl, dwAction, NULL));

    CAnswerEntry * pNew = new CAnswerEntry(pszUrl, dwAction, iAnswer);
    // Just don't persist answers if we don't have memory. 
    if (pNew == NULL || pNew->GetUrl() == NULL)
        return;

    pNew->SetNext(m_pAnswerEntry);
    m_pAnswerEntry = pNew;
    return;
}

// This is the dialog proc for the generic Zones Alert dialog. 
// IMPORTANT: This is an ANSI function in an otherwise unicode world.
// BE EXTREMELY CAREFUL WHEN CALLING WINDOWS API FUNCTIONS.

INT_PTR
CSecurityManager::ZonesAlertDialogProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    switch (iMsg)
    {
        case WM_INITDIALOG:
        {
            WCHAR  rgszAlert[MAX_ALERT_SIZE];
            LPDLGDATA lpDlgData = (LPDLGDATA)lParam;
            TransAssert(lpDlgData != NULL);

            DWORD dwStrId = GetAlertIdForAction(lpDlgData->dwAction);


            if (::LoadStringWrapW(g_hInst, dwStrId, rgszAlert, MAX_ALERT_SIZE) == 0)
            {
                TransAssert(FALSE);
                ::LoadStringWrapW(g_hInst, IDS_ACTION_UNKNOWN, rgszAlert, MAX_ALERT_SIZE);
            }

            HWND hwndAlertText = ::GetDlgItem(hDlg, IDC_ZONEALERTTEXT);

            TransAssert(hwndAlertText != NULL);
            ::SetWindowTextWrapW(hwndAlertText, rgszAlert);

            if (lpDlgData->dwFlags & PUAF_FORCEUI_FOREGROUND)
            {
                SetForegroundWindow(hDlg);
            }

            return TRUE;
        }
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                    EndDialog(hDlg, ZALERT_YES);
                    return TRUE;
                case IDCANCEL:
                    EndDialog(hDlg, ZALERT_NO);
                    return TRUE;
            }

            break;
    }  /* end switch */

    return FALSE;
}

// This is the dialog proc for the Alert displayed when information is posted over the net.
// This is a special case, since it is the only dialog where we let the user persist their change. 
// DO NOT START ADDING OTHER SPECIAL CASES TO THIS. IF YOU NEED TO, CONSIDER CHANGING THE 
// THE TEMPLATE POLICY FOR THE ZONE TO BE "CUSTOM" BECAUSE THE ZONE WILL START DIVERGING FROM
// THE TEMPLATE IT IS SUPPOSED TO BE BASED ON.

BOOL
CSecurityManager::IsFormsSubmitAction(DWORD dwAction)
{
    return (dwAction == URLACTION_HTML_SUBMIT_FORMS ||
            dwAction == URLACTION_HTML_SUBMIT_FORMS_FROM || 
            dwAction == URLACTION_HTML_SUBMIT_FORMS_TO
           );
}
            
INT_PTR
CSecurityManager::FormsAlertDialogProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    switch (iMsg)
    {
        case WM_INITDIALOG:
        {
            LPDLGDATA lpDlgData = (LPDLGDATA)lParam;
            TransAssert(lpDlgData != NULL);
            LPWSTR pstr = lpDlgData->pstr;
            
            if (pstr != NULL)
            {
                HWND hwnd = ::GetDlgItem(hDlg, IDC_ZONEALERTTEXT);
                TransAssert(hwnd != NULL);
                ::SetWindowTextWrapW(hwnd, pstr);
            }  
              
            if (lpDlgData->dwFlags & PUAF_FORCEUI_FOREGROUND)
            {
                SetForegroundWindow(hDlg);
            }

            if(!(lpDlgData->dwFlags & PUAF_DONTCHECKBOXINDIALOG))
            {
                CheckDlgButton(hDlg, IDC_DONT_WANT_WARNING, BST_CHECKED);
            }
            return TRUE;
        }
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                case IDYES:
                {
                    DWORD dwRet;
                    if (IsDlgButtonChecked(hDlg, IDC_DONT_WANT_WARNING) == BST_CHECKED)
                        dwRet = ZALERT_YESPERSIST;
                    else 
                        dwRet = ZALERT_YES;

                    EndDialog(hDlg, dwRet);
                    return TRUE;
                }

                case IDCANCEL:
                case IDNO:
                    EndDialog(hDlg, ZALERT_NO);
                    return TRUE;
            }

            break;
    }  /* end switch */

    return FALSE;
}

INT 
CSecurityManager::ShowFormsAlertDialog(HWND hwndParent, LPDLGDATA lpDlgData )
{
    int nRet;
    TransAssert(lpDlgData != NULL);
    TransAssert(IsFormsSubmitAction(lpDlgData->dwAction));

    // Compose the dialog string.
    LPWSTR pstr = NULL;
    WCHAR rgch[MAX_ALERT_SIZE]; 

    ZONEATTRIBUTES zc;
    zc.cbSize = sizeof(zc);
    if (SUCCEEDED(m_pZoneManager->GetZoneAttributes(lpDlgData->dwZone, &zc)))
    {
        WCHAR rgchStr[MAX_ALERT_SIZE];
        UINT uID = (lpDlgData->dwAction == URLACTION_HTML_SUBMIT_FORMS_FROM) ? IDS_ACTION_POST_FROM : IDS_ACTION_FORMS_POST;

        if (::LoadStringWrapW(g_hInst, uID, rgchStr, MAX_ALERT_SIZE) != 0)
        {
            if (wnsprintfW(rgch, MAX_ALERT_SIZE, rgchStr, zc.szDisplayName) != 0)
                pstr = rgch;
        }
    }

    lpDlgData->pstr = pstr;
                                                                    
    nRet =  (int) ::DialogBoxParamWrapW (
                        g_hInst,
                        MAKEINTRESOURCEW(IDD_WARN_ON_POST),
                        hwndParent,
                        CSecurityManager::FormsAlertDialogProc,
                        (LPARAM)lpDlgData
                    );

    return nRet;
}
    
INT_PTR
CSecurityManager::ZonesWarnDialogProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    switch (iMsg)
    {
        case WM_INITDIALOG:
        {
            WCHAR  rgszWarn[MAX_ALERT_SIZE];
            LPDLGDATA lpDlgData = (LPDLGDATA)lParam;
            TransAssert(lpDlgData != NULL);
            DWORD dwAction = lpDlgData->dwAction;
            DWORD dwStrId = GetWarnIdForAction(dwAction);

            if (::LoadStringWrapW(g_hInst, dwStrId, rgszWarn, MAX_ALERT_SIZE) == 0)
            {
                TransAssert(FALSE);
                ::LoadStringWrapW(g_hInst, IDS_WARN_UNKNOWN, rgszWarn, MAX_ALERT_SIZE);
            }

            HWND hwndWarnText = ::GetDlgItem(hDlg, IDC_ZONEALERTTEXT);

            TransAssert(hwndWarnText != NULL);
            ::SetWindowTextWrapW(hwndWarnText, rgszWarn);

            if (lpDlgData->dwFlags & PUAF_FORCEUI_FOREGROUND)
            {
                SetForegroundWindow(hDlg);
            }

            return TRUE;
        }
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                case IDCANCEL:
                    EndDialog(hDlg, ZALERT_YES);
                    return TRUE;
            }

            break;
    }  /* end switch */

    return FALSE;
}
            


STDMETHODIMP
CSecurityManager::ProcessUrlAction
(
    LPCWSTR pwszUrl,
    DWORD dwAction,
    BYTE * pPolicy,
    DWORD cbPolicy,
    BYTE * pContext,
    DWORD cbContext,
    DWORD dwFlags,
    DWORD dwReserved
)
{
    PerfDbgLog(tagCSecurityManager, this, "+CSecurityManager::ProcessUrlAction");
    HRESULT hr = INET_E_DEFAULT_ACTION;
    DWORD dwZone = ZONEID_INVALID;


// First check if the delegation interface wants to handle this.
    if (m_pDelegateSecMgr)
    {
        hr = m_pDelegateSecMgr->ProcessUrlAction(pwszUrl, dwAction, pPolicy, cbPolicy, pContext, cbContext, dwFlags, dwReserved);
    }

    if (hr != INET_E_DEFAULT_ACTION)
    {
        // Delegation interface processed the request.
        return hr;
    }

    if (!EnsureZoneManager())
    {
        return E_UNEXPECTED;
    }

    // Increment our refcount so we don't get destroyed for the duration of this 
    // function.
    AddRef();

    if (SUCCEEDED(hr = MapUrlToZone(pwszUrl, &dwZone, dwFlags)))
    {
        DWORD dwPolicy;

        hr = m_pZoneManager->GetZoneActionPolicy(dwZone, dwAction, (BYTE *)&dwPolicy, sizeof(dwPolicy), URLZONEREG_DEFAULT);        

        if (SUCCEEDED(hr))
        {
            DWORD dwPermissions = GetUrlPolicyPermissions(dwPolicy);

            // Are we supposed to be showing any UI here?
            if ( (( dwPermissions == URLPOLICY_QUERY && ( dwFlags & PUAF_NOUI) == 0 ) || 
                  ( dwPermissions == URLPOLICY_DISALLOW && ( dwFlags & PUAF_WARN_IF_DENIED) != 0))
                 && HIWORD(dwPolicy) == 0)
            {
                HWND hwndParent = NULL;
                // Show UI unless the host indicates otherwise.
                BOOL bShowUI = TRUE;
                if  (m_pSite != NULL)
                {
                    HRESULT hrGetWnd = m_pSite->GetWindow(&hwndParent);

                    // Host doesn't want us to show UI.
                    if (hrGetWnd == S_FALSE && hwndParent == INVALID_HANDLE_VALUE)
                        bShowUI = FALSE;
                    else if (FAILED(hrGetWnd))
                        hwndParent = NULL;

                    // Disable any modeless dialog boxes
                    m_pSite->EnableModeless(FALSE);
                }

                int nRet = -1;
                BOOL fRememberAnswer = FALSE;

                // structure used to pass information to the dialog proc's.
                DlgData dlgData;
                dlgData.dwAction = dwAction;
                dlgData.dwZone = dwZone;
                dlgData.pstr = NULL;
                dlgData.dwFlags = dwFlags | ((dwPolicy & URLPOLICY_DONTCHECKDLGBOX) ? PUAF_DONTCHECKBOXINDIALOG : 0);
                dwPolicy = dwPolicy & ~URLPOLICY_DONTCHECKDLGBOX; 
                if ( dwPermissions == URLPOLICY_QUERY )
                { 
                    // First check to see if the user already answered this question once.                
                    if (!m_persistAnswers.GetPrevAnswer(pwszUrl, dwAction, &nRet))
                    {

                        fRememberAnswer = TRUE;

                        if (!bShowUI)
                        {
                            // If we can't show UI just act as if the user said No.
                            nRet = ZALERT_NO;   
                        }
                        else if (IsFormsSubmitAction(dwAction))
                        {
                            nRet = ShowFormsAlertDialog(hwndParent, &dlgData);
                        }
                        else
                        {
                            nRet = (int) ::DialogBoxParamWrapW (
                                            g_hInst,
                                            MAKEINTRESOURCEW(IDD_ZONE_ALERT),
                                            hwndParent,
                                            CSecurityManager::ZonesAlertDialogProc,
                                            (LPARAM)&dlgData
                                        );
                        }
                    }
                }
                else 
                {
                    TransAssert(dwPermissions == URLPOLICY_DISALLOW);
                    if (bShowUI)
                    {
                        nRet = (int) ::DialogBoxParamWrapW (
                                        g_hInst,
                                        MAKEINTRESOURCEW(IDD_WARN_ALERT),
                                        hwndParent,
                                        CSecurityManager::ZonesWarnDialogProc,
                                        (LPARAM)&dlgData
                                    );
                    }
                }



                // If we failed to show the dialog we should just return
                // the policies unmodified.
                if (dwPermissions == URLPOLICY_QUERY && nRet != -1 )
                {
                    // Change the policy to reflect the users choice.
                    DWORD dwYesOnlyPolicy;
                    dwYesOnlyPolicy = dwPolicy | URLPOLICY_DONTCHECKDLGBOX; // copy old policy before it is changed  
                    
                    SetUrlPolicyPermissions(dwPolicy, nRet ? URLPOLICY_ALLOW : URLPOLICY_DISALLOW);
                   

                    if (fRememberAnswer)
                        m_persistAnswers.RememberAnswer(pwszUrl, dwAction, nRet);

                    // The only case where we should change the policy is we have a checkbox on the dialog
                    // we popped up that says "Don't ask me again". Today the only thing that does that is 
                    // the forms submit form. 
                    // TODO: create a more generic category name "CanDlgChangePolicy" or something like that
                    // instead of the specific IsFormsSubmitAction.

                    if (IsFormsSubmitAction(dwAction) && ((nRet == ZALERT_YESPERSIST) || (nRet == ZALERT_YES)))
                    {
                        
                        m_pZoneManager->SetZoneActionPolicy(dwZone, dwAction, 
                                                            (BYTE *)((nRet == ZALERT_YESPERSIST) ?  &dwPolicy : &dwYesOnlyPolicy), 
                                                            sizeof(dwPolicy), URLZONEREG_DEFAULT);                                 
                    }
                }
                                     
                if (m_pSite != NULL)
                {
                    m_pSite->EnableModeless(TRUE);
                }
            }
            
            TransAssert(cbPolicy == 0 || cbPolicy >= sizeof(DWORD));

            if (cbPolicy >= sizeof(DWORD) && pPolicy != NULL)                        
                *(DWORD *)pPolicy = dwPolicy;


            // Code to check for allowed list of directx objects 
            // for URLACTION_ACTIVEX_RUN
            if(dwAction == URLACTION_ACTIVEX_RUN &&
                dwPolicy == URLPOLICY_ACTIVEX_CHECK_LIST)
            {
                DWORD dwValue = 0;
                hr = CSecurityManager::GetActiveXRunPermissions(pContext, dwValue);
                *(DWORD *)pPolicy = dwValue;
            }            
            // normal allowed permissions            
            else if(GetUrlPolicyPermissions(dwPolicy) == URLPOLICY_ALLOW)
            {
                hr = S_OK ;
            }
            else
            {
                hr = S_FALSE;
            }
        
        }
    }

    Release();    
    return hr;                 
}

STDMETHODIMP
CSecurityManager::QueryCustomPolicy
(
    LPCWSTR pwszUrl,
    REFGUID guidKey,
    BYTE **ppPolicy,
    DWORD *pcbPolicy,
    BYTE *pContext,
    DWORD cbContext,
    DWORD dwReserved
)
{
    PerfDbgLog(tagCSecurityManager, this, "+CSecurityManager::QueryCustomPolicy");
    HRESULT hr = INET_E_DEFAULT_ACTION;
    DWORD dwZone = ZONEID_INVALID;
   
    if (m_pDelegateSecMgr)
    {
        hr = m_pDelegateSecMgr->QueryCustomPolicy(pwszUrl, guidKey, ppPolicy, pcbPolicy, pContext, cbContext, dwReserved);
    }

    if (hr == INET_E_DEFAULT_ACTION)
    {
        if (!EnsureZoneManager())
        {
            return E_UNEXPECTED;
        }

        if (SUCCEEDED(hr = MapUrlToZone(pwszUrl, &dwZone, NULL)))
        {
            hr = m_pZoneManager->GetZoneCustomPolicy(dwZone, guidKey, ppPolicy, pcbPolicy, URLZONEREG_DEFAULT);        
        }
        else 
        {
            hr = E_UNEXPECTED;
        }
    }

    return hr;                 
}


STDMETHODIMP
CSecurityManager::GetSecuritySite
(
    IInternetSecurityMgrSite **ppSite
)
{
    if (ppSite)
    {
        if (m_pSite)
            m_pSite->AddRef();

        *ppSite = m_pSite;
    }
    return S_OK;
}

STDMETHODIMP
CSecurityManager::SetSecuritySite
(
    IInternetSecurityMgrSite *pSite
)
{
    if (m_pSite)
    {
        m_pSite->Release();
    }

    if (m_pDelegateSecMgr)
    {
        m_pDelegateSecMgr->Release();
        m_pDelegateSecMgr = NULL;
    }

    m_pSite = pSite;

    if (m_pSite)
    {
        m_pSite->AddRef();

        IServiceProvider * pServiceProvider = NULL;

        if (SUCCEEDED(m_pSite->QueryInterface(IID_IServiceProvider, (void **)&pServiceProvider)))
        {
            TransAssert(pServiceProvider != NULL);

            if (SUCCEEDED(pServiceProvider->QueryService(
                                SID_SInternetSecurityManager, 
                                IID_IInternetSecurityManager,
                                (void **)&m_pDelegateSecMgr)))
            {
                TransAssert(m_pDelegateSecMgr != NULL);
            }
            else
            {
                m_pDelegateSecMgr = NULL;
            }
            pServiceProvider->Release();
        }
    }

    return S_OK;
}

    
// Mapping related functions

HRESULT
CSecurityManager::SetZoneMapping
(
    DWORD   dwZone,
    LPCWSTR pszPattern,
    DWORD   dwFlags
)
{
    PerfDbgLog(tagCSecurityManager, this, "+CSecurityManager::SetZoneMapping");
    HRESULT hr = INET_E_DEFAULT_ACTION;

    // Increment the counter so any cached url to zone mappings are invalidated.
    IncrementGlobalCounter( );

    if (m_pDelegateSecMgr)
    {
        hr = m_pDelegateSecMgr->SetZoneMapping(dwZone, pszPattern, dwFlags);
    }

    if (hr == INET_E_DEFAULT_ACTION)
    {
        LPWSTR pwszSecPattern = NULL;
        hr = CoInternetGetSecurityUrl(pszPattern, &pwszSecPattern, PSU_DEFAULT, 0);

        if (SUCCEEDED(hr))
        {
            CFreeStrPtr freeStr(pwszSecPattern);

            ZONEMAP_COMPONENTS zc;
            hr = zc.Crack (pwszSecPattern, PUAF_ACCEPT_WILDCARD_SCHEME, TRUE);
            if (hr != S_OK)
                return hr;

            ZONEATTRIBUTES za;
            za.cbSize = sizeof(za);

            if (!EnsureZoneManager())
                return E_OUTOFMEMORY;

            // Don't allow adding not https entries if the zone requires server verification.
            if (!(dwFlags & SZM_DELETE) && SUCCEEDED(m_pZoneManager->GetZoneAttributes(dwZone, &za)))
            {
                if (za.dwFlags & ZAFLAGS_REQUIRE_VERIFICATION)
                {
                    if (zc.nScheme != URL_SCHEME_HTTPS)
                    {
                        return E_ACCESSDENIED ;
                    }
                }
            }                           
                                
            if (zc.fIPRange)
            {
                hr = AddDeleteIPRule(&zc, dwZone, dwFlags);
                return hr;
            }
                                                                                         
            // Zone mappings for drive letters are hardcoded
            if (zc.fDrive)
                return E_INVALIDARG;        

            // Guard against buffer overflow.
            if (CSTRLENW(SZDOMAINS) + zc.cchDomain + 1 + zc.cchSite + 1 >= MAX_PATH)
                return E_INVALIDARG;

            TCHAR szKeyName[MAX_PATH];
            DWORD cchKeyName = CSTRLENW(SZDOMAINS);
            memcpy (szKeyName, SZDOMAINS, sizeof(TCHAR) * cchKeyName);

            if ((zc.cchDomain == 0 && zc.cchSite == 0) || (zc.cchProtocol == 0))
            {
                return E_INVALIDARG;
            }

            if (zc.pszDomain)
            {
                memcpy (szKeyName + cchKeyName,
                    zc.pszDomain, sizeof(TCHAR) * zc.cchDomain);
                // Null terminate for strchr.
                szKeyName[cchKeyName + zc.cchDomain] = TEXT('\0');
                if (StrChr(szKeyName + cchKeyName, WILDCARD) != NULL)
                    return E_INVALIDARG;

                cchKeyName += zc.cchDomain;
            }

            // Check for the simple wildcard case.
            // patterns such as *.microsoft.com where the only thing
            // after a * is the second-level domain.            
            if (zc.pszSite[0] == WILDCARD && zc.cchSite == 1)
            {
                if (!zc.pszDomain)
                    return E_INVALIDARG;
            }
            else
            {
                // Wildcards are only permitted at the begining of pattern.
                // So patterns such as *.foo.*.microsoft.com are invalid.

                if (zc.pszSite[0] == WILDCARD)
                {
                    // We already know that zc.cchSite is greater than 1
                    // because we would have caught it in the outer 'if' clause 
                    // otherwise.
                    if (zc.pszSite[1] != DOT)
                    {
                        return E_INVALIDARG;
                    }

                    // Skip over the leading *. and make sure there are no 
                    // other *'s in the string. 
                    if (StrRChr(zc.pszSite + 2, zc.pszSite + zc.cchSite, WILDCARD) != NULL)
                    {
                        return E_INVALIDARG;
                    }
                }

                if (zc.pszDomain)  // Add seperator only if we added a domain name.
                { 
                    szKeyName[cchKeyName++] = BACKSLASH;
                }
                else if (!IsOpaqueScheme(zc.nScheme) && 
                          (zc.pszSite[zc.cchSite - 1] == DOT || 
                           zc.pszSite[0] == DOT)
                        )

                {
                    // Catches invalid cases such as http://ohserv. or http://.inetsdk.
                    return E_INVALIDARG;
                }

                memcpy (szKeyName + cchKeyName,
                    zc.pszSite, sizeof(TCHAR) * zc.cchSite);

                if (!IsOpaqueScheme(zc.nScheme))
                { 
                    szKeyName[cchKeyName + zc.cchSite] = TEXT('\0');
                }
                cchKeyName += zc.cchSite;
            }
            szKeyName[cchKeyName] = 0;

            CRegKey regMap;

            DWORD dwErr;
    
            if (dwFlags & SZM_DELETE)
            {
                // Delete mapping if one exists.
                if (ERROR_FILE_NOT_FOUND == regMap.Open (m_regZoneMap, szKeyName, KEY_WRITE))
                    return S_OK; // nothing to delete
                if ((dwErr = regMap.DeleteValue (zc.pszProtocol)) == ERROR_SUCCESS)
                {
                    // Try reclaiming any registry key's which might be empty. 
                    regMap.Close();
                    m_regZoneMap.DeleteEmptyKey(szKeyName); 
                    if (zc.pszDomain)
                    {
                        DWORD cch = CSTRLENW(SZDOMAINS) + zc.cchDomain;
                        szKeyName[cch] = TEXT('\0');
                        m_regZoneMap.DeleteEmptyKey(szKeyName);
                    }                               
                    return S_OK;
                }
                else 
                {
                    hr = HRESULT_FROM_WIN32(dwErr);
                }
            }
            else
            {
                // Creates new mapping.
                if ((dwErr = regMap.Create (m_regZoneMap, szKeyName, KEY_READ | KEY_WRITE)) == ERROR_SUCCESS)
                {
                    DWORD dwZoneEntry;
                    if (regMap.QueryValue(&dwZoneEntry, zc.pszProtocol) == ERROR_SUCCESS)
                    {
                        hr = HRESULT_FROM_WIN32(ERROR_FILE_EXISTS);
                    }
                    else if ((dwErr = regMap.SetValue (dwZone, zc.pszProtocol)) == ERROR_SUCCESS)
                    {
                        return S_OK;
                    }
                    else 
                    {
                        hr = HRESULT_FROM_WIN32(dwErr);
                    }
                }
                else 
                {
                    hr = HRESULT_FROM_WIN32(dwErr);
                }
            }
        }
    }

    return hr;
}

// Helper functions for GetZoneMappings

// Given a site name and a domain name composes the string 
// site.domain.com

HRESULT
CSecurityManager::ComposeUrlSansProtocol
(
    LPCTSTR pszDomain,
    int     cchDomain,
    LPCTSTR pszSite,
    int     cchSite,
    LPTSTR  *ppszRet,
    int     *pcchUrlSansProtocol
)
{

    if (ppszRet == NULL)
    {
       return E_INVALIDARG;
    }

    int cchUrlSansProtocol = cchSite + 1 /* . */ + cchDomain ;

    // Create the part of the string without the protocol 
    LPTSTR szUrlSansProtocol = new TCHAR [cchUrlSansProtocol + 1];


    if ( szUrlSansProtocol == NULL )            
    {
        *ppszRet = NULL;

        if (pcchUrlSansProtocol)
            *pcchUrlSansProtocol = 0;

        return E_OUTOFMEMORY;
    }

    LPTSTR szCurrent = szUrlSansProtocol;

    // Copy over the specific parts of the name. 
    if (pszSite != NULL)
    {
        memcpy(szCurrent, pszSite, cchSite * sizeof(TCHAR));
        szCurrent += cchSite;
        memcpy(szCurrent, TEXT("."), 1 * sizeof(TCHAR));
        szCurrent += 1;
    }

    memcpy(szCurrent, pszDomain, cchDomain * sizeof(TCHAR));
    szCurrent += cchDomain;

    // Finally copy over the trailing zero.
    szCurrent[0] = TEXT('\0');

    *ppszRet = szUrlSansProtocol;

    if (pcchUrlSansProtocol)
        *pcchUrlSansProtocol = cchUrlSansProtocol;

    return S_OK;
}
    

HRESULT
CSecurityManager::ComposeUrl
(
    LPCTSTR pszUrlSansProt,
    int cchUrlSansProt,
    LPCTSTR pszProtocol,
    int cchProtocol,
    BOOL bAddWildCard,
    LPTSTR * ppszUrl,
    int *pcchUrl
)
{
    if (ppszUrl == NULL)
    {
        return E_INVALIDARG;
    }

    BOOL bWildCardScheme = FALSE;
    BOOL bOpaqueScheme   = FALSE;

    if (cchProtocol == 1 && pszProtocol[0] == WILDCARD)
    {
        bWildCardScheme = TRUE;
        bOpaqueScheme = FALSE;
    }
    else
    {
        // Figure out if this is an an opaque scheme. 
        LPWSTR pszTemp = (LPWSTR)_alloca((cchProtocol + 2) * sizeof(TCHAR));
        memcpy(pszTemp, pszProtocol, cchProtocol * sizeof(TCHAR));
        pszTemp[cchProtocol] = TEXT(':');
        pszTemp[cchProtocol + 1] = TEXT('\0');

        PARSEDURL pu; 
        pu.cbSize = sizeof(pu);

        HRESULT hr = ParseURL(pszTemp, &pu);

        if (SUCCEEDED(hr))
        {
            bOpaqueScheme = IsOpaqueScheme(pu.nScheme);
        }
        else
        {
            bOpaqueScheme = TRUE;
        }
    }

    // cchUrl will have the eventual length of the string we will send out                    
    int cchUrl = cchUrlSansProt; 
                 
    
    if (bOpaqueScheme)
    {
        cchUrl += cchProtocol + 1;    // we have to add prot: to the URL
    }
    else if (bWildCardScheme)
    {
        // If the scheme is a wildcard we don't add it to the eventual display.
    }
    else
    {
       cchUrl += cchProtocol + 3;  // we have to add prot:// to the url.
    }

    // If we are not an opaque schema, we might need to add a wildcard character as well.
    if (!bOpaqueScheme && bAddWildCard)
    {
        cchUrl += 2; /* for *. */
    }

    LPTSTR szUrl = new TCHAR [cchUrl + 1];
    
    if (szUrl == NULL)
    {
        *ppszUrl = NULL;

        if (pcchUrl)
            *pcchUrl = 0;

        return E_OUTOFMEMORY;
    }

    LPTSTR szCurrent = szUrl;

    // if the scheme is wildcard we don't want to display the scheme at all.
    // i.e we will show *.microsoft.com and *:*.microsoft.com
    if (bWildCardScheme)
    {
        if (bAddWildCard)
        {
            memcpy(szCurrent, TEXT("*."), 2 * sizeof(TCHAR));
            szCurrent += 2;
        }
    }
    else
    {
        memcpy(szCurrent, pszProtocol, cchProtocol * sizeof(TCHAR));
        szCurrent += cchProtocol;

        if (bOpaqueScheme)
        {
            memcpy(szCurrent, TEXT(":"), 1 * sizeof(TCHAR));
            szCurrent += 1;
        }
        else
        {
            memcpy(szCurrent, TEXT("://"), 3 * sizeof(TCHAR));
            szCurrent += 3;
            if (bAddWildCard)
            {
                memcpy(szCurrent, TEXT("*."), 2 * sizeof(TCHAR));
                szCurrent += 2;
            }
        }           
    }

    memcpy(szCurrent, pszUrlSansProt, cchUrlSansProt * sizeof(TCHAR));
    szCurrent += cchUrlSansProt;

    szCurrent[0] = TEXT('\0');

    *ppszUrl = szUrl;
    if (pcchUrl)
        *pcchUrl = cchUrl;

    return S_OK;
}

HRESULT
CSecurityManager::AddIPRulesToEnum
(
    DWORD dwZone,
    CEnumString *pEnumString
)
{
    HRESULT hr = NOERROR;

    if ((HUSKEY)m_regZoneMap == NULL)
        return E_UNEXPECTED;

    CRegKey regRanges;
    DWORD cNumRanges = 0;

    if (   ERROR_SUCCESS != regRanges.Open(m_regZoneMap, SZRANGES, KEY_READ)
        || ERROR_SUCCESS != regRanges.QuerySubKeyInfo(&cNumRanges, NULL, NULL)
       )
    {
        return S_OK;  // Nothing to add if we can't open the key.       
    }
    
    if (cNumRanges == 0)
        return S_OK;

    DWORD cchMaxKey = 20;
    TCHAR szKeyName[20];
    TCHAR rgchSansProtocol[MAX_PATH];
    DWORD iItem;

    for (iItem = 0 ; iItem < cNumRanges ; iItem++ )
    {
        DWORD cbName, cbRange;
        CRegKey regItem;
        cbName = cchMaxKey;
        cbRange = sizeof(rgchSansProtocol) - 3 * sizeof(TCHAR); 
                
        if  (  ERROR_SUCCESS == regRanges.EnumKey(iItem, szKeyName, &cbName)
            && ERROR_SUCCESS == regItem.Open(regRanges, szKeyName, KEY_READ)
            && ERROR_SUCCESS == regItem.QueryValue(rgchSansProtocol, SZRANGE, &cbRange) 
            )
        {
            LONG lRetProtocol = NOERROR;
            TCHAR rgchProtocol[MAX_PATH]; 
            DWORD dwZoneRead = ZONEID_INVALID;
            DWORD dwType;
            
            for ( DWORD dwIdxProt = 0 , cchP = ARRAYSIZE(rgchProtocol), dwSizeZoneId = sizeof(dwZoneRead);
                  (((lRetProtocol = regItem.EnumValue(dwIdxProt, rgchProtocol, &cchP, &dwType, &dwZoneRead, &dwSizeZoneId)) != ERROR_NO_MORE_ITEMS)
                   && (hr == NOERROR));
                   dwIdxProt++, cchP = ARRAYSIZE(rgchProtocol), dwSizeZoneId = sizeof(dwZoneRead), dwZoneRead = ZONEID_INVALID
                )
            {
#ifdef unix
                if (lRetProtocol == ERROR_MORE_DATA)
                    continue;
#endif /* unix */

                if (lRetProtocol != NOERROR)
                    break;
                
                if (dwSizeZoneId == 0 || cchP == 0 || rgchProtocol[0] == TEXT('\0')
                    || dwType != REG_DWORD || dwZoneRead == ZONEID_INVALID)
                    continue;
                                                         
                if (dwZone == dwZoneRead)
                {
                    int cchProtocol = lstrlen(rgchProtocol);
                    int cchRange = lstrlen(rgchSansProtocol);                    

                    LPTSTR szUrl = NULL;

                    if ( (SUCCEEDED(ComposeUrl(rgchSansProtocol, cchRange, rgchProtocol, cchProtocol, FALSE, &szUrl, NULL)))
                         && (SUCCEEDED(pEnumString->AddString(szUrl))))
                    {
                        if (szUrl != NULL)
                            delete [] szUrl;
                    }
                    else
                    {
                        if (szUrl != NULL)
                            delete [] szUrl;
                            
                        hr = E_OUTOFMEMORY;
                        break;
                    }
                }
            } /* for each protocol */
        } 
    } /* for each range entry */

    return hr;
}

                                                                     
// Given a Registry key and a part to the URL, this function looks through the 
// 'values' in the registry looking for a zone match. When it finds one it adds the 
// strings to the CEnumString class that is passed in. 

HRESULT
CSecurityManager::AddUrlsToEnum
(
    CRegKey * pRegKey,
    DWORD dwZone, 
    LPCTSTR pszUrlSansProt,
    int cchUrlSansProt,
    BOOL bAddWildCard,
    CEnumString *pEnumString
)
{
    HRESULT hr = NOERROR;
    // Iterate over the values and make up the strings we need.
    LONG lRetProtocol = NOERROR;
    TCHAR rgszProtocol[MAX_PATH];
    DWORD dwZoneRead = ZONEID_INVALID;
    DWORD dwType;
                
    for ( DWORD dwIdxProt = 0 , cchP = sizeof(rgszProtocol)/sizeof(TCHAR), dwSizeZoneId = sizeof(dwZoneRead);
          (((lRetProtocol = pRegKey->EnumValue(dwIdxProt, rgszProtocol, &cchP, &dwType, &dwZoneRead, &dwSizeZoneId)) != ERROR_NO_MORE_ITEMS)
           && (hr == NOERROR));
           dwIdxProt++, cchP = sizeof(rgszProtocol)/sizeof(TCHAR), dwSizeZoneId = sizeof(dwZoneRead), dwZoneRead = ZONEID_INVALID
        )
    {
        if (lRetProtocol != NO_ERROR)
        {
            // Break out of this loop but keep trying other sites.
            break;
        }

        if (  dwSizeZoneId == 0 || cchP == 0 || rgszProtocol[0] == TEXT('\0') 
                || dwType != REG_DWORD || dwZoneRead == ZONEID_INVALID)
            continue;                       

        // Yippeee, finally found a match.
        if (dwZone == dwZoneRead)
        {
            int cchProtocol = lstrlen(rgszProtocol);

            LPTSTR szUrl = NULL;

            // Compose the name of the URL.
            if ( (SUCCEEDED(ComposeUrl(pszUrlSansProt, cchUrlSansProt, rgszProtocol, cchProtocol, bAddWildCard, &szUrl, NULL)))
                 && (SUCCEEDED(pEnumString->AddString(szUrl))))
            {
                // Both succeeded we have added this string to the enumeration. 
                // Just free up the memory and move on.
                if (szUrl != NULL)
                    delete [] szUrl;
            }
            else
            {
                if (szUrl != NULL)
                    delete [] szUrl;
                hr = E_OUTOFMEMORY;
                break;
            }
        }           
    }  /* for each protocol */

    return hr;
}

                       
HRESULT
CSecurityManager::GetZoneMappings
(
    DWORD dwZone,
    IEnumString **ppEnumString,
    DWORD dwFlags
)
{
    PerfDbgLog(tagCSecurityManager, this, "+CSecurityManager::GetZoneMappings");

    HRESULT hr = NOERROR;

    CEnumString *pEnumString = NULL;

    pEnumString = new CEnumString( );
    
    if (pEnumString == NULL)
        return E_OUTOFMEMORY;

    CRegKey regDomainRoot;


    // We setup three loops below.
    // 
    //      for each domain name
    //          for each site
    //              for each protocol. 
    // The one twist is that for each domain we also have to enumerate the sites
    // to deal with wildcards such as http://*.microsoft.com
    // 
    // BUGBUG: MAX_PATH is a safe assumption, but we should change this to get the 
    // memory dynamically.

    TCHAR rgszDomain[MAX_PATH]; 
    TCHAR rgszSite[MAX_PATH];
    TCHAR rgszProtocol[MAX_PATH];
    LONG lRetDomain = NOERROR;
        
    if ( ((HUSKEY)m_regZoneMap != NULL) && 
         (regDomainRoot.Open(m_regZoneMap, SZDOMAINS, KEY_READ) == NOERROR)
       )
    {
        // If we couldn't open the root, then no rules exist for any zone.
        // Return an empty enumerator
        for ( DWORD dwIdxDomain = 0, cchD = sizeof(rgszDomain)/sizeof(TCHAR) ;
              (((lRetDomain = regDomainRoot.EnumKey(dwIdxDomain, rgszDomain, &cchD)) != ERROR_NO_MORE_ITEMS)
               && (hr == NOERROR));
              dwIdxDomain++ , cchD = sizeof(rgszDomain)/sizeof(TCHAR)
            )
        {
            if (lRetDomain != NOERROR)
            {
                TransAssert(lRetDomain != ERROR_MORE_DATA);
                break;
            }
        
            TCHAR rgszSite[MAX_PATH];
            LONG lRetSite = NOERROR;
        
            // Open the key to the domain.
            CRegKey regDomain;

            if (regDomain.Open(regDomainRoot, rgszDomain, KEY_READ) != NOERROR )
            {
                // We couldn't open this domain for some reason, but we will
                // keep trying the other domains.  
                continue;
            }

            int cchDomain = lstrlen(rgszDomain);

            TransAssert((HUSKEY)regDomain != NULL);

            for ( DWORD dwIdxSite = 0 , cchS = sizeof(rgszSite)/sizeof(TCHAR) ;
                  (((lRetSite = regDomain.EnumKey(dwIdxSite, rgszSite, &cchS)) != ERROR_NO_MORE_ITEMS)
                    && (hr == NOERROR));
                  dwIdxSite++ , cchS = sizeof(rgszSite)/sizeof(TCHAR)
                )
            {
                if (lRetSite != NOERROR)
                {
                    TransAssert(lRetSite !=  ERROR_MORE_DATA);
                    break;      // We will break out of this loop but keep trying other domains.
                }
            
                CRegKey regSite;

                if (regSite.Open(regDomain, rgszSite, KEY_READ) != NOERROR )
                {
                    // Couldn't open the site but try other sites anyway.
                    continue;
                }

                int cchSite = lstrlen(rgszSite);

                LPTSTR szUrlSansProtocol = NULL;
                int cchUrlSansProtocol = 0;

                // Get everything about the name figured out 
                if ((FAILED(ComposeUrlSansProtocol(rgszDomain, cchDomain, rgszSite, cchSite, &szUrlSansProtocol, &cchUrlSansProtocol)))
                     || szUrlSansProtocol == NULL  )
                {
                    hr = E_OUTOFMEMORY;
                    break;
                }
            
                TransAssert(cchUrlSansProtocol != 0);

                hr = AddUrlsToEnum(&regSite, dwZone, szUrlSansProtocol, cchUrlSansProtocol, FALSE, pEnumString);

                // Free up the memory we just allocated. 
                delete [] szUrlSansProtocol;

            } /* for each site */ 
                                                                     
            // At the domain level we need to look for any protocol defaults
            // An example string would look like http://*.microsoft.com
            LPTSTR szSiteWildCard = NULL;
            int cchSiteWildCard = 0;

            // If the string doesn't contain any .'s we didn't break it out as a domain/site 
            // in the first place. We shouldn't add a *. wildcard in this case. 
            BOOL bAddWildCard = (StrChr(rgszDomain, DOT) != NULL);

            if ((FAILED(ComposeUrlSansProtocol(rgszDomain, cchDomain, NULL, 0, &szSiteWildCard, &cchSiteWildCard)))
                  || szSiteWildCard == NULL)
            {
                hr = E_OUTOFMEMORY;
                break;
            }

            TransAssert(cchSiteWildCard != 0);

            hr = AddUrlsToEnum(&regDomain, dwZone, szSiteWildCard, cchSiteWildCard, bAddWildCard, pEnumString);

            delete [] szSiteWildCard;
        }
    }// opened domains root key

    // Finally add all the IP range entries to the structure.
    if (hr == NOERROR)    
    {
        hr = AddIPRulesToEnum(dwZone, pEnumString);
    }

    // Finally call the strings              
    if ( hr == NOERROR )
    {
        // Pass back the Enumeration to the caller. 
        if (ppEnumString)
            *ppEnumString = pEnumString;
    }
    else
    {
        // We need to free the object and return NULL to the caller. 
        if (ppEnumString)
            *ppEnumString = NULL;

        delete pEnumString;
    }
    
    return hr;        
                                                
}                       

//
// MapUrlToZone helper methods return S_OK if match found, otherwise S_FALSE
//

HRESULT
CSecurityManager::MapUrlToZone (ZONEMAP_COMPONENTS* pzc, DWORD* pdwZone, DWORD dwFlags,
                                BOOL *pfMarked, LPWSTR *ppszMarkURL)
{
    HRESULT hr;
    CRegKey regProtocols;
    BOOL    fMarked = FALSE;

    // Guard against buffer overflow.
    if (CSTRLENW(SZDOMAINS) + pzc->cchDomain + 1 + pzc->cchSite + 1 >= MAX_PATH)
        goto default_zone;

    // do the Mark of the Web stuff, if we have a file:
    if (pzc->nScheme == URL_SCHEME_FILE)
    {
        LPWSTR pwszMarkURL = NULL;
        TCHAR  *pszExt = PathFindExtension(pzc->pszSite);
        LPCTSTR pszPath;
        TCHAR  szPath[MAX_PATH];

        if (!pzc->fDrive)
        {
            StrCpy( szPath, TEXT("\\\\") );
            StrCatBuff(szPath, pzc->pszSite, ARRAYSIZE(szPath));
            pszPath = szPath;
        }
        else
            pszPath = pzc->pszSite;

        // Don't look for the mark if flags say not to.
        // We only want to pursue the Mark of the Web for htm(l) files.
        // If Marked, we want to be sure we're not chasing our tail recursively.
        if ( !(dwFlags & MUTZ_NOSAVEDFILECHECK) &&
             (StrCmpI(pszExt,TEXT(".htm")) == 0 || StrCmpI(pszExt,TEXT(".html")) == 0) &&
             FileBearsMarkOfTheWeb(pszPath, &pwszMarkURL) &&
             StrCmp(pszPath, pwszMarkURL) != 0)
        {
            MapUrlToZone( pwszMarkURL, pdwZone, dwFlags | MUTZ_NOSAVEDFILECHECK | MUTZ_NOCACHE );

            // Don't take the mark's word for it if it's asserting local machine
            if ( *pdwZone != URLZONE_LOCAL_MACHINE )
            {
                fMarked = TRUE;
                if (ppszMarkURL)
                {
                    *ppszMarkURL = pwszMarkURL;
                    pwszMarkURL = NULL; // give mark string to caller, don't free
                }
            }
        }

        if (pwszMarkURL)
            LocalFree(pwszMarkURL);

        if (fMarked)
            goto done;
        // if not marked, or ignoring the mark, process as normal.
    }

    if (pzc->fDrive)
    {
        switch (pzc->dwDriveType)
        {
            case 0:
            case 1:
                break;
            case DRIVE_REMOTE:
                TransAssert(FALSE);
                *pdwZone = URLZONE_INTRANET;
                goto done;
            default:
            {
                BOOL bCacheFile = IsFileInCacheDir(pzc->pszSite);

                *pdwZone = bCacheFile ? URLZONE_INTERNET : URLZONE_LOCAL_MACHINE;
                goto done;
            }
        }
    }
    else if (IsOpaqueScheme(pzc->nScheme))
    {
        if (S_OK == CheckSiteAndDomainMappings (pzc, pdwZone, pzc->pszProtocol))
            goto done;

        if (S_OK == CheckMKURL(pzc, pdwZone, pzc->pszProtocol))
            goto done;
    }
    else 
    {
        if (pzc->fAddr)
        {
            // Check name in form of IP address against range rules.
            if (S_OK == CheckAddressAgainstRanges (pzc, pdwZone, pzc->pszProtocol))
                goto done;
        }                

        if ((HUSKEY) m_regZoneMap)
        {
            // Check for a mapping for the site (or domain, if applicable)
            if (S_OK == CheckSiteAndDomainMappings (pzc, pdwZone, pzc->pszProtocol))
                goto done;

            if (S_OK == CheckUNCAsIntranet(pzc, pdwZone, pzc->pszProtocol))
                goto done;

            // Check for Local Intranet name rules.
            if (S_OK == CheckIntranetName (pzc, pdwZone, pzc->pszProtocol))
                goto done;
                          
            // Check for proxy bypass rule.
            if (S_OK == CheckProxyBypassRule (pzc, pdwZone, pzc->pszProtocol))
                goto done;
        }
    }        

    // Check for protocol defaults.
    if (    ERROR_SUCCESS == regProtocols.Open (m_regZoneMap, SZPROTOCOLS, KEY_READ)
        &&  ERROR_SUCCESS == regProtocols.QueryValueOrWild (pdwZone, pzc->pszProtocol)
       )
    {       
        goto done;
    }

default_zone:
        *pdwZone = URLZONE_INTERNET;
done:   
        if (pfMarked)
            *pfMarked = fMarked;

        hr = S_OK; 
        return hr;
}

HRESULT
CSecurityManager::ReadAllIPRules( )
{
    DWORD* pdwIndexes = NULL;

    EnterCriticalSection(&s_csectIP);
    if (s_pRanges != NULL)
    {
        delete [] s_pRanges;
        s_pRanges = NULL;
        s_cNumRanges = 0;
    }

    // We always start with the key "Range1" if nothing is found.
    s_dwNextRangeIndex = 1;  

    CRegKey regRanges, regItem;

    if ((HUSKEY)m_regZoneMap == NULL)
    {
        if (ERROR_SUCCESS != m_regZoneMap.Open (NULL, SZZONEMAP, KEY_READ))
            goto done;
    }

    DWORD cchMaxKey;

    // Read in ranges from registry.
    if (   ERROR_SUCCESS != regRanges.Open (m_regZoneMap, SZRANGES, KEY_READ)
        || ERROR_SUCCESS != regRanges.QuerySubKeyInfo (&s_cNumRanges, &cchMaxKey, NULL)
        || 0 == s_cNumRanges
       )
    {
        goto done;
    }

    // BUGBUG: TODO: Figure out why QuerySubKeyInfo is returning the wrong information. 
    cchMaxKey = 20; 
    // Calculate size of range item and allocate array (no alignment padding)
    s_cbRangeItem = sizeof(RANGE_ITEM) + sizeof(TCHAR) * (cchMaxKey + 1);
    s_pRanges = new BYTE [s_cbRangeItem * s_cNumRanges];
    pdwIndexes = new DWORD[s_cNumRanges];
        
    if (!s_pRanges || !pdwIndexes)
    {
        s_cNumRanges = 0;
        goto done;
    }

    // Loop through the ranges.
    TCHAR szRange[MAX_IPRANGE]; // 4x "###-###."
    RANGE_ITEM* pItem;
    DWORD iItem, cItem;

    pItem = (RANGE_ITEM *) s_pRanges;
    cItem = s_cNumRanges;
    s_cNumRanges = 0;
    
    for (iItem = 0; iItem < cItem; iItem++)
    {
        // Reset output buffer sizes.
        DWORD cbName, cbRange;
        cbName = cchMaxKey;
        cbRange = sizeof(szRange);
                
        // Get range from next key.
        if (  ERROR_SUCCESS != regRanges.EnumKey (iItem, pItem->szName, &cbName)
           || ERROR_SUCCESS != regItem.Open (regRanges, pItem->szName, KEY_READ)
           || ERROR_SUCCESS != regItem.QueryValue (szRange, SZRANGE, &cbRange)
           )
        {
            break;
        }

        // Figure out the index for the named Range entry. Ignore it is not of the 
        // form Range followed by Number. Range####
        DWORD chRange = lstrlen(SZRANGEPREFIX);

        if (0 == StrCmpNI(pItem->szName, SZRANGEPREFIX, chRange))
        {
            pdwIndexes[iItem] = StrToInt(pItem->szName + chRange);            
        }

        if (!ReadIPRule (szRange, pItem->bLow, pItem->bHigh))
            continue;
        
        // Advance to next range item in array.
        pItem = (RANGE_ITEM*) (((LPBYTE) pItem) + s_cbRangeItem);
        s_cNumRanges++;
    }

    // Find an empty slot or if we don't find one
    for (s_dwNextRangeIndex = 1 ; s_dwNextRangeIndex <= cItem; s_dwNextRangeIndex++)    
    {
        DWORD i;
        // Go through the entries and see if the index exists.
        for (i = 0; i < cItem ; i++ )
        {
            if (pdwIndexes[i] == s_dwNextRangeIndex)
                break;
        }

        if (i == cItem) // This range item is available.
            break;
    }

    TransAssert(s_dwNextRangeIndex >= 1 && s_dwNextRangeIndex <= (cItem + 1));
    delete [] pdwIndexes;
                     
done:
    LeaveCriticalSection(&s_csectIP);
    return S_OK;
}

HRESULT
CSecurityManager::AddDeleteIPRule
    (ZONEMAP_COMPONENTS* pzc, DWORD dwZone, DWORD dwFlags)
{
    HRESULT hr = S_OK;
    DWORD dwError = ERROR_SUCCESS;
    BOOL bFoundItem = FALSE;
    TCHAR szItemName[MAX_PATH];

    TransAssert(s_dwNextRangeIndex != 0);
    TransAssert(pzc->fIPRange);

    if (s_dwNextRangeIndex == 0)
        return E_UNEXPECTED;

    EnterCriticalSection(&s_csectIP);

    RANGE_ITEM *pItem = (RANGE_ITEM *)s_pRanges;

    // First figure out if this item already exists in our list. 
    // This is useful in both the delete and add case.
    for (DWORD iRange = 0; 
               iRange < s_cNumRanges ; 
               iRange++, pItem = (RANGE_ITEM*) (((LPBYTE) pItem) + s_cbRangeItem))
    {
        if  ( ( 0 == memcmp(pItem->bLow, pzc->rangeItem.bLow, sizeof(pItem->bLow)))
              && (0 == memcmp(pItem->bHigh, pzc->rangeItem.bHigh, sizeof(pItem->bHigh)))
            )
        {
            break;
        }
    }

    // If we have a valid "named" entry in the registry.        
    if (iRange < s_cNumRanges && pItem->szName[0] != TEXT('\0'))
    {
        bFoundItem = TRUE;
        StrCpy(szItemName, SZRANGES);
        StrCat(szItemName, pItem->szName);
    }
    else 
    {
        bFoundItem = FALSE;
        pItem = NULL;
    }

    // Are we trying to do an add or a delete.
    if  (dwFlags & SZM_DELETE)
    {
        // If we have a valid "named" entry in the registry delete it now.        
        if (bFoundItem)
        { 
            TransAssert(pItem != NULL);
            
            CRegKey regItem;        
            if ((dwError = regItem.Open(m_regZoneMap, szItemName, KEY_READ | KEY_WRITE)) != ERROR_SUCCESS)
            {
                hr = HRESULT_FROM_WIN32(dwError);
            }
            else 
            {
                // Get the protocol name and delete the protocol related value.
                if ((dwError = regItem.DeleteValue (pzc->pszProtocol)) == ERROR_SUCCESS)
                {
                    DWORD dwNumValues = 0;

                    // Is this the last entry for this range? If so delete the range & nuke the key.
                    if (ERROR_SUCCESS == regItem.QuerySubKeyInfo(NULL, NULL, &dwNumValues) &&
                        dwNumValues == 1 &&
                        ERROR_SUCCESS == regItem.DeleteValue(SZRANGE)
                       )
                    {
                        regItem.Close();
                        m_regZoneMap.DeleteEmptyKey(szItemName);
                    }
                }
                else 
                {
                    hr = HRESULT_FROM_WIN32(dwError);
                }
            }
        }
        else 
        {
            hr =  HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        }
    }
    else
    {
        if (bFoundItem)
        {
            TransAssert(pItem != NULL);            
            // See if an entry with the given name already
            CRegKey regItem;                        

            if ((dwError = regItem.Open(m_regZoneMap, szItemName, KEY_READ | KEY_WRITE)) != ERROR_SUCCESS)
            {
                hr = HRESULT_FROM_WIN32(dwError);
            }
            else 
            {
                DWORD dwZoneExists;
                // If we were able to read the value, fail because entry already exists.  
                if (regItem.QueryValue(&dwZoneExists, pzc->pszProtocol) == ERROR_SUCCESS)
                {
                    hr = HRESULT_FROM_WIN32(ERROR_FILE_EXISTS);
                }
            }
        }
        else 
        {
            // Create the new item name. 
            StrCpy(szItemName, SZRANGES);
            StrCat(szItemName, SZRANGEPREFIX);            
            if (!DwToWchar(s_dwNextRangeIndex, szItemName + lstrlen(SZRANGES) + lstrlen(SZRANGEPREFIX), 10))
            {
                TransAssert(FALSE);
                hr = E_UNEXPECTED;
            }
        }
            
        // Okay to go ahead and create the entry.
        if (SUCCEEDED(hr))
        {
            TCHAR szIPRule[MAX_IPRANGE];
            // We shouldn't have any domain part for IP Rules.
            TransAssert(pzc->pszDomain == NULL);
            memcpy(szIPRule, pzc->pszSite, sizeof(TCHAR) * pzc->cchSite);
            szIPRule[pzc->cchSite] = TEXT('\0');

            CRegKey regMap;
                                    
            // Now add the entry to the registry.
            if ( ((dwError = regMap.Create(m_regZoneMap, szItemName, KEY_WRITE)) == ERROR_SUCCESS) &&
                 ((dwError = regMap.SetValue(dwZone, pzc->pszProtocol)) == ERROR_SUCCESS) &&
                 ((dwError = regMap.SetValue(szIPRule, SZRANGE)) == ERROR_SUCCESS)
               ) 
            {
                hr = S_OK;
            }
            else
            { 
                hr = HRESULT_FROM_WIN32(dwError);                                     
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = ReadAllIPRules();
    }

    LeaveCriticalSection(&s_csectIP);

    return hr;
}
            
HRESULT 
CSecurityManager::CheckAddressAgainstRanges
    (ZONEMAP_COMPONENTS* pzc, DWORD* pdwZone, LPCTSTR pszProt)
{
    TCHAR szKeyName[MAX_PATH];
    const DWORD cchRanges = CSTRLENW(SZRANGES);
    memcpy (szKeyName, SZRANGES, sizeof(TCHAR) * cchRanges);

    CRegKey regItem;

    EnterCriticalSection(&s_csectIP);
    RANGE_ITEM* pItem = (RANGE_ITEM *) s_pRanges;

    for (DWORD iRange=0; iRange < s_cNumRanges; iRange++)
    {
        for (DWORD iByte=0; iByte<4; iByte++)
        {
            if (   pzc->bAddr[iByte] < pItem->bLow[iByte]
                || pzc->bAddr[iByte] > pItem->bHigh[iByte]
               )
            {
                goto next_range; // much cleaner than a break and test
            }
        }

        StrCpyW (szKeyName + cchRanges, pItem->szName);
        
        if (    ERROR_SUCCESS == regItem.Open (m_regZoneMap, szKeyName, KEY_READ)
            &&  ERROR_SUCCESS == regItem.QueryValueOrWild (pdwZone, pszProt)
           )
        {
            LeaveCriticalSection(&s_csectIP);
            return S_OK;
        }

next_range:
        pItem = (RANGE_ITEM*) (((LPBYTE) pItem) + s_cbRangeItem);
    }

    LeaveCriticalSection(&s_csectIP);
    return S_FALSE;
}


HRESULT CSecurityManager::CheckSiteAndDomainMappings
    (ZONEMAP_COMPONENTS* pzc, DWORD* pdwZone, LPCTSTR pszProt)
{
    CRegKey regDomain, regSite;
    DWORD dwRegErr;

    TCHAR szKeyName[MAX_PATH];
    const DWORD cchKeyName = CSTRLENW(SZDOMAINS);
    memcpy (szKeyName, SZDOMAINS, sizeof(TCHAR) * cchKeyName);

    TransAssert(!pzc->fDrive);

    if (pzc->pszDomain)
    {
        // First, look for domain rule.
        memcpy (szKeyName + cchKeyName, pzc->pszDomain, sizeof(TCHAR) * pzc->cchDomain);
        szKeyName[cchKeyName + pzc->cchDomain] = 0;
        if (ERROR_SUCCESS != regDomain.Open (m_regZoneMap, szKeyName, KEY_READ))
            return S_FALSE;
            
        // Now add the site.
        memcpy (szKeyName, pzc->pszSite, sizeof(TCHAR) * pzc->cchSite);
        szKeyName[pzc->cchSite] = 0;
        dwRegErr = regSite.Open (regDomain, szKeyName, KEY_READ);

        // For IE5.0 we support wildcard's beyond the second level domain.
        // For example if you had an intranet address www.internal.mycorp.com
        // you can specify a zone mapping for *.internal.mycorp.com.
        // In IE4 we would have flagged this as an error because we allowed
        // wildcards only at the second level domain.
        //  IE5 since we lifted this restriction, we have to search the sub-keys 
        // and look for strings such as "*.internal" under the mycorp.com key. 
        // If we find one we see if the wildcard pattern matches the site whose
        // zone we are trying to determine.
        
        if (dwRegErr != ERROR_SUCCESS)
        {
            TCHAR rgchSubKeyName[MAX_PATH];
            LONG lRet = NOERROR;
            
            for ( DWORD dwIndex = 0 , cchSubKey = ARRAYSIZE(rgchSubKeyName) ;
                   ((lRet = regDomain.EnumKey(dwIndex, rgchSubKeyName, &cchSubKey)) != ERROR_NO_MORE_ITEMS) ;
                   dwIndex++ , cchSubKey = ARRAYSIZE(rgchSubKeyName)
                )
            {
                if (lRet != NOERROR)                   
                {
                    TransAssert(lRet != ERROR_MORE_DATA);
                    break;
                }

                // For patterns that finish with a *. we will do a suffix 
                // match to see if the wildcard sequence is valid.                
                if (cchSubKey > 2 && rgchSubKeyName[0] == WILDCARD && rgchSubKeyName[1] == DOT)
                {
                    // First condition
                    //    for xyz.foo.microsoft.com to match *.foo.microsoft.com
                    //        www.foo has to be greater than or equal to foo.microsoft.com
                    //        note that we allow just foo.microsoft.com as well.
                    // Second condition
                    //     cchSubkey is the length of *.foo, therefore the last 
                    //     cchSubKey - 2 characters of the two strings should match.
                    if (pzc->cchSite >= (cchSubKey - 2) &&
                        ( StrCmpNI (rgchSubKeyName + 2, /* skip *. */
                                    pzc->pszSite + pzc->cchSite - cchSubKey + 2,
                                    cchSubKey - 2 
                                   )  == 0
                         )
                       )
                    {
                        dwRegErr = regSite.Open(regDomain, rgchSubKeyName, KEY_READ);
                        break;
                    }
                }
            }
        }                                    
    }
    else
    {
        // There was no domain.  Look for a site rule.
        memcpy (szKeyName + cchKeyName, pzc->pszSite, sizeof(TCHAR) * pzc->cchSite);
        szKeyName[cchKeyName + pzc->cchSite] = 0;
        dwRegErr = regSite.Open (m_regZoneMap, szKeyName, KEY_READ);
    }

    // Look for matching protocols under site key.
    if (    ERROR_SUCCESS == dwRegErr
        &&  ERROR_SUCCESS == regSite.QueryValueOrWild (pdwZone, pszProt)
       )
    {           
        return S_OK;
    }

    // Now fall back to domain if there was one.
    else if (   pzc->pszDomain
            &&  ERROR_SUCCESS == regDomain.QueryValueOrWild (pdwZone, pszProt)
       )
    {            
        return S_OK;
    }

    else return S_FALSE;
}


HRESULT CSecurityManager::CheckUNCAsIntranet
    (ZONEMAP_COMPONENTS* pzc, DWORD* pdwZone, LPCTSTR pszProt)
{
    HRESULT hr = S_FALSE;
    DWORD dwUNCAsIntranet;

    TransAssert(!pzc->fDrive);
    TransAssert(!pzc->fIPRange);

    if (pzc->fAddr || pzc->nScheme != URL_SCHEME_FILE)
        return hr;

    if (ERROR_SUCCESS != m_regZoneMap.QueryValue(&dwUNCAsIntranet, SZUNCASINTRANET))
        return hr;

    if (dwUNCAsIntranet == 0)
    {
        hr = S_OK;
        if (pdwZone)
            *pdwZone = URLZONE_INTERNET;
    }

    return hr;
}

HRESULT CSecurityManager::CheckIntranetName
    (ZONEMAP_COMPONENTS* pzc, DWORD* pdwZone, LPCTSTR pszProt)
{
    HRESULT hr = S_FALSE;
    DWORD dwZone = ZONEID_INVALID;
    
    TransAssert(!pzc->fDrive);
    TransAssert(!pzc->fIPRange);

    if (pzc->fAddr)
        return hr;

    // Check if there is a local intranet rule.
    if (ERROR_SUCCESS != m_regZoneMap.QueryValue(&dwZone, SZINTRANETNAME))
        return hr;

    if (dwZone != URLZONE_INTRANET)
    {
        TransAssert(FALSE);
        dwZone = URLZONE_INTRANET;
    }

        
    if (pzc->pszSite && !pzc->pszDomain)
    {
        BOOL bFoundDot = FALSE;

        for (DWORD dwIndex = 0 ; dwIndex < pzc->cchSite ; dwIndex++ )
        {
            if (pzc->pszSite[dwIndex] == DOT)
            {
                bFoundDot = TRUE;
                break;
            }
        }

        hr = bFoundDot ? S_FALSE : S_OK;
    }

    if (hr == S_OK && pdwZone)
        *pdwZone = dwZone;

    return hr;
}            


HRESULT CSecurityManager::CheckProxyBypassRule
    (ZONEMAP_COMPONENTS* pzc, DWORD* pdwZone, LPCTSTR pszProt)
{    
    TransAssert(!pzc->fDrive);

    // Check if there is a proxy bypass rule.
    if (ERROR_SUCCESS != m_regZoneMap.QueryValue(pdwZone, SZPROXYBYPASS))
        return S_FALSE;
        
    // Calculate length of hostname = site (+ . + domain)
    DWORD cchTotal;
    cchTotal = pzc->cchSite;
    if (pzc->cchDomain)
        cchTotal += 1 + pzc->cchDomain;

    // Convert from unicode to ansi.
    char szHost[MAX_PATH];
    DWORD cbHost;
    cbHost = WideCharToMultiByte
        (CP_ACP, 0, pzc->pszSite, cchTotal, szHost, sizeof(szHost), NULL, NULL);
    if (!cbHost)
        return S_FALSE;
        
    // WideCharToMultiByte won't null terminate szHost,
    // IsHostInProxyBypassList shouldn't need it,
    // but just do it anyway to play it safe.
    szHost[cbHost] = 0;
    INTERNET_SCHEME tScheme;
    BOOL bCheckByPassRules = TRUE;
    switch(pzc->nScheme)
    {
        case URL_SCHEME_HTTP:
            tScheme = INTERNET_SCHEME_HTTP;
            break;
        case URL_SCHEME_HTTPS:
            tScheme = INTERNET_SCHEME_HTTPS;
            break;
        case URL_SCHEME_GOPHER:
            tScheme = INTERNET_SCHEME_GOPHER;
            break;
        case URL_SCHEME_FTP:
            tScheme = INTERNET_SCHEME_FTP;
            break;
        default:
            bCheckByPassRules = FALSE;
            break;
    }

    return bCheckByPassRules && IsHostInProxyBypassList (tScheme, szHost, cbHost) ? S_OK : S_FALSE;
}

HRESULT CSecurityManager::CheckMKURL
    (ZONEMAP_COMPONENTS* pzc, DWORD* pdwZone, LPCTSTR pszProt)
{
    HRESULT hr = S_FALSE;
    DWORD dwZone = ZONEID_INVALID;    
    TransAssert(!pzc->fDrive);

    // First check if it looks like a valid mk: string. 
    if (pzc->nScheme == URL_SCHEME_MK && 
        pzc->pszDomain == NULL && 
        pzc->pszSite != NULL && 
        pzc->pszSite[0] == AT)
    {
        // look for a : in the domain string.
        LPTSTR pszColon = StrChr(pzc->pszSite, COLON);
        if ( pszColon != NULL)
        {
            CRegKey regProtocols;
            *pszColon = TEXT('\0'); // Temporarily overwrite the colon.
            if ((ERROR_SUCCESS == regProtocols.Open(m_regZoneMap, SZPROTOCOLS, KEY_READ)) &&
                (ERROR_SUCCESS == regProtocols.QueryValue(&dwZone, pzc->pszSite))
               )
            {
                *pdwZone = dwZone;
                hr = S_OK;
            }
            *pszColon = COLON;     // Set the domain string back to its original state.
        }
    }
    
    return hr;
}          


CSecurityManager::CSecMgrCache::CSecMgrCache(void)
{
    InitializeCriticalSection(&m_csectZoneCache);
    
    // single static object, so this only gets inited once per
    // process.
    s_hMutexCounter = CreateMutexA(NULL, FALSE, "ZonesCounterMutex");

    m_iAdd = 0;
}


CSecurityManager::CSecMgrCache::~CSecMgrCache(void)
{
    Flush();
    DeleteCriticalSection(&m_csectZoneCache) ; 

    CloseHandle(s_hMutexCounter);
}

BOOL
CSecurityManager::CSecMgrCache::Lookup(LPCWSTR pwszURL,
                                       DWORD *pdwZone,
                                       BOOL *pfMarked,
                                       BYTE* pbSecurityID,
                                       DWORD *pcbSecurityID,
                                       LPCWSTR pwszDocDomain)
{
    BOOL fFound = FALSE;

    EnterCriticalSection(&m_csectZoneCache);

    if ( !IsCounterEqual() )
    {
        Flush();
    }
    else
    {
        int i;

        fFound = FindCacheEntry( pwszURL, i );
        if (fFound)
        {
            if ( pbSecurityID )
            {
                TransAssert(pcbSecurityID);
                if ( m_asmce[i].m_pbSecurityID && 
                     (  (m_asmce[i].m_pwszDocDomain == NULL && pwszDocDomain == NULL) || /* both are NULL */
                        (   m_asmce[i].m_pwszDocDomain && pwszDocDomain &&
                            (0 == StrCmpW(m_asmce[i].m_pwszDocDomain, pwszDocDomain))  /* the strings match */
                        )
                     ) &&
                     m_asmce[i].m_cbSecurityID <= *pcbSecurityID)
                {

                    memcpy( pbSecurityID, m_asmce[i].m_pbSecurityID, m_asmce[i].m_cbSecurityID  );
                    *pcbSecurityID = m_asmce[i].m_cbSecurityID;
                }
                else 
                    *pcbSecurityID = 0;
            }

            if (pdwZone)
            {
                *pdwZone = m_asmce[i].m_dwZone;

                if (pfMarked)
                    *pfMarked = m_asmce[i].m_fMarked;
            }
        }
    }

    LeaveCriticalSection(&m_csectZoneCache);

    return fFound;
}

void 
CSecurityManager::CSecMgrCache::Add(LPCWSTR pwszURL,
                                    DWORD dwZone,
                                    BOOL fMarked,
                                    const BYTE *pbSecurityID,
                                    const DWORD cbSecurityID, 
                                    LPCWSTR pwszDocDomain)
{
    int     i;
    BOOL    fFound;

    EnterCriticalSection(&m_csectZoneCache);

    if ( !IsCounterEqual() )
        Flush();

    fFound = FindCacheEntry( pwszURL, i ); // found or not, i will be the right place to set it.
    m_asmce[i].Set(pwszURL, dwZone, fMarked, pbSecurityID, cbSecurityID, pwszDocDomain);
    if (!fFound)
        m_iAdd = (m_iAdd + 1) % MAX_SEC_MGR_CACHE;

    SetToCurrentCounter(); // validate this cache.

    LeaveCriticalSection(&m_csectZoneCache);
}

void 
CSecurityManager::CSecMgrCache::Flush(void)
{
    int i;

    EnterCriticalSection(&m_csectZoneCache);

    for ( i = 0; i < MAX_SEC_MGR_CACHE; i++ )
        m_asmce[i].Flush();

    m_iAdd = 0;

    LeaveCriticalSection(&m_csectZoneCache);
}

// Is the counter we saved with the cache entry, equal to the current counter.
BOOL
CSecurityManager::CSecMgrCache::IsCounterEqual( ) const 
{
    CExclusiveLock lock(s_hMutexCounter);
    LPDWORD lpdwCounter = (LPDWORD) g_SharedMem.GetPtr(SM_SECMGRCHANGE_COUNTER);
    // If we couldn't create the shared memory for some reason, we just assume our cache is up to date.
    if (lpdwCounter == NULL)
        return TRUE;

    return (m_dwPrevCounter == *lpdwCounter);
}

VOID
CSecurityManager::CSecMgrCache::SetToCurrentCounter( ) 
{
    CExclusiveLock lock(s_hMutexCounter);
    LPDWORD lpdwCounter = (LPDWORD) g_SharedMem.GetPtr(SM_SECMGRCHANGE_COUNTER);
    if (lpdwCounter == NULL)
        return;

    m_dwPrevCounter = *lpdwCounter;
}

VOID
CSecurityManager::CSecMgrCache::IncrementGlobalCounter( )
{
    CExclusiveLock lock(s_hMutexCounter);
    LPDWORD lpdwCounter = (LPDWORD) g_SharedMem.GetPtr(SM_SECMGRCHANGE_COUNTER);
    if (lpdwCounter == NULL)
        return;

    (*lpdwCounter)++;
}

BOOL
CSecurityManager::CSecMgrCache::FindCacheEntry( LPCWSTR pwszURL, int& riEntry )
{
    BOOL fFound = FALSE;
    riEntry = m_iAdd - 1 % MAX_SEC_MGR_CACHE;

    // our cache is a circular buffer. We scan it from the last entry
    // we added backwards to the next slot to add to, createing a quasi-
    // MRU.
    if ( riEntry < 0 )
        riEntry = MAX_SEC_MGR_CACHE + riEntry;

    // check below us, starting with the most recent addition, if any.
    for ( ; riEntry >= 0; riEntry-- )
    {
        if ( m_asmce[riEntry].m_pwszURL &&
             StrCmpW( m_asmce[riEntry].m_pwszURL, pwszURL ) == 0 )
        {
            fFound = TRUE;
            break;
        }
    }

    if (!fFound)
    {
        for ( riEntry = MAX_SEC_MGR_CACHE - 1; riEntry >= m_iAdd; riEntry-- )
        {
            if (m_asmce[riEntry].m_pwszURL == NULL)
                break; // hasn't been used yet.
            else if ( m_asmce[riEntry].m_pwszURL &&
                      StrCmpW( m_asmce[riEntry].m_pwszURL, pwszURL ) == 0 )
            {
                fFound = TRUE;
                break;
            }
        }
    }

    if (!fFound)
        riEntry = m_iAdd;

    return fFound;
}
  
void 
CSecurityManager::CSecMgrCache::CSecMgrCacheEntry::Set(LPCWSTR pwszURL,
                                                       DWORD dwZone,
                                                       BOOL fMarked,
                                                       const BYTE *pbSecurityID,
                                                       DWORD cbSecurityID,
                                                       LPCWSTR pwszDocDomain)
{
    if ( pwszURL )
    {
        // Only replace if the string has changed.
        // We may see the same string if the entry is
        // set by MapUrlToZone before GetSecurityID is called.
        if (m_pwszURL && StrCmpW(pwszURL, m_pwszURL))
        {
            delete [] m_pwszURL;
            m_pwszURL = NULL;
        }

        if (!m_pwszURL)
        {
            int cchURL = lstrlenW( pwszURL );

            m_pwszURL = new WCHAR[cchURL+1];
            if ( m_pwszURL )
                StrCpyW( m_pwszURL, pwszURL );
            else
                return;
        }
    }

    // We always set the url zone mark first, then come back later and
    // add the security ID, than means that on any set operation, we're
    // either changing the url or adding the security ID. Either way, if
    // we have a security ID, its invalid now, so flush it.
    if (m_pbSecurityID)
    {
        delete [] m_pbSecurityID;
        m_pbSecurityID = NULL;
        m_cbSecurityID = 0;
    }

    if (m_pwszDocDomain)
    {
        delete [] m_pwszDocDomain;
        m_pwszDocDomain = NULL;
    }

    if ( pbSecurityID )
    {
        m_pbSecurityID = new BYTE[cbSecurityID];
        if ( m_pbSecurityID )
        {
            memcpy( m_pbSecurityID, pbSecurityID, cbSecurityID  );
            if (pwszDocDomain)
            {
                m_pwszDocDomain = new WCHAR[lstrlenW(pwszDocDomain) + 1];
                if (m_pwszDocDomain != NULL)
                {
                    StrCpyW(m_pwszDocDomain, pwszDocDomain);
                }
                else
                {
                    // If we don't have memory for the Document's domain property
                    // we better not remember the security ID either.
                    delete [] m_pbSecurityID;
                    m_pbSecurityID = NULL;
                    cbSecurityID = 0;
                }
            }
        }
        else
        {
            cbSecurityID = 0;
        }

        m_cbSecurityID = cbSecurityID;
    }

    if (dwZone != URLZONE_INVALID)
    {
        m_dwZone = dwZone;
        m_fMarked = fMarked;
    }
}

void 
CSecurityManager::CSecMgrCache::CSecMgrCacheEntry::Flush(void)
{
    if (m_pwszURL)
        delete[] m_pwszURL;
    m_pwszURL = NULL;

    if (m_pbSecurityID)
        delete[] m_pbSecurityID;
    m_pbSecurityID = NULL;
    
    m_cbSecurityID = 0;

    if (m_pwszDocDomain)
    {
        delete [] m_pwszDocDomain;
        m_pwszDocDomain = NULL;
    }

    m_dwZone = URLZONE_INVALID;
    m_fMarked = FALSE;
}
          
BOOL
CSecurityManager::EnsureListReady(BOOL bForce)
// Make sure the list of allowed controls is ready
// Returns whether or not the list had to be made
// bForce is whether to force a reinitialization
{
    if(CSecurityManager::s_clsidAllowedList == NULL || bForce == TRUE)
    {
        CSecurityManager::IntializeAllowedControls();
        return TRUE;
    }
    else
        return FALSE;
}

void 
CSecurityManager::IntializeAllowedControls()
{
    DWORD i = 0;
    DWORD dwNumKeys=0;
    DWORD dwMaxLen=0;
    DWORD dwNumValues=0;
    // this buffer size should be long enough to hold a string-form
    // CLSID, plus the two end braces, plus a null terminator
    TCHAR szValueName[40];
    DWORD dwNameLength = 40;
    DWORD dwType = 0;
    DWORD dwData = 0;
    DWORD dwDataLength = sizeof(DWORD);

    // In case we somehow get multiply initialized
    if(CSecurityManager::s_clsidAllowedList != NULL)
    {
        delete [] CSecurityManager::s_clsidAllowedList;
        CSecurityManager::s_clsidAllowedList = NULL;
    }
    CSecurityManager::s_dwNumAllowedControls = 0;


    //open key
    // look at HKLM only, first
    CRegKey * prkey_AllowedControls;
    CRegKey rkey_AllowedControls(TRUE);
    CRegKey rkey_AllowedControlsCU(FALSE);

    LONG lRes = rkey_AllowedControls.Open(NULL, ALLOWED_CONTROLS_KEY, KEY_READ);
    if(lRes != ERROR_SUCCESS)
    {
        // List not found in HKLM, check HKCU
        
        lRes = rkey_AllowedControlsCU.Open(NULL, ALLOWED_CONTROLS_KEY, KEY_READ);

        if(lRes != ERROR_SUCCESS)
        {
            // AllowedControls Key not able to be opened
            return;
        }
        else
        {
            prkey_AllowedControls = &rkey_AllowedControlsCU;
        }
    }
    else
    {
        prkey_AllowedControls = &rkey_AllowedControls;
    }


    lRes = prkey_AllowedControls->QuerySubKeyInfo(&dwNumKeys, &dwMaxLen, &dwNumValues);
    if(lRes != ERROR_SUCCESS)
        return;

    // prepare space in data structure
    // array will not need to be resized, since the maximum number of allowed
    // CLSIDs is the number of values in the key
    CSecurityManager::s_clsidAllowedList = new CLSID[dwNumValues];
    if(CSecurityManager::s_clsidAllowedList == NULL) // new failed
        return;

	// loop through all values in the key
    for(i = 0; i < dwNumValues; i++)
    {
        // at every loop, these values get changed and must be reset to the 
        // length of the name and data buffers, respectively
        dwNameLength = ARRAYSIZE(szValueName);
        dwDataLength = sizeof(DWORD);

        // Get the (DWORD) value for the current value name     
        LONG lResult = prkey_AllowedControls->EnumValue(i, szValueName, &dwNameLength, 
                                                 &dwType, &dwData, &dwDataLength);

        if(lResult == ERROR_SUCCESS && dwType == REG_DWORD
            && GetUrlPolicyPermissions(dwData) == URLPOLICY_ALLOW)
        {
	        // found a value for the CLSID given, and it is set to allow the CLSID
            // add the CLSID to the list
            CLSID * p_id = CSecurityManager::s_clsidAllowedList +   //pointer + 
                           CSecurityManager::s_dwNumAllowedControls;//offset
            HRESULT hr = CLSIDFromString(szValueName, p_id);
            if(hr != NOERROR)
                continue;

            CSecurityManager::s_dwNumAllowedControls++;
        }
    }
}

HRESULT 
CSecurityManager::GetControlPermissions(BYTE * raw_CLSID, DWORD & dwPerm)
{
    CLSID * id = (CLSID *)(raw_CLSID);
    dwPerm = 0;

    // If the list is not initialized (something's wrong) leave function
    if(CSecurityManager::s_clsidAllowedList == NULL)
    {
        return E_UNEXPECTED;
    }


    DWORD index = 0;
    // Search for the given CLSID in the list of allowed Controls
    for(index = 0; index < CSecurityManager::s_dwNumAllowedControls; index++)
    {
        if(*id == (CSecurityManager::s_clsidAllowedList[index]))
        {
            dwPerm = URLPOLICY_ALLOW; // not necesarry, since currently only allowed controls
                                      // are in the list, but this may change later
            return S_OK;
        }
    }
    
    // Not found, return false to indicate not in list
    return S_FALSE;
}

HRESULT
CSecurityManager::GetActiveXRunPermissions(BYTE * raw_CLSID, DWORD & dwPerm)
{
    HRESULT hr = S_FALSE;
    DWORD dwValue;

    EnterCriticalSection(&s_csectAList);
    // Initialize the Allowed Controls list if it is not already
    CSecurityManager::EnsureListReady(FALSE);    
    // get the list permission for pContext, if it is in the list
    HRESULT permHR = CSecurityManager::GetControlPermissions(raw_CLSID,dwValue);
    LeaveCriticalSection(&s_csectAList);

    // interpret results, (zone dependent interpretation not yet implemented)
    if(SUCCEEDED(permHR))
    {
        if(permHR == S_OK) // found in list
        {
            if(dwValue == URLPOLICY_ALLOW)
            {
                hr = S_OK;
                dwPerm = URLPOLICY_ALLOW;
            }
            else
            {
                hr = S_FALSE;
                dwPerm = URLPOLICY_DISALLOW;
            }
        }
        else  // not in list; default is to disallow
        {
            hr = S_FALSE;
            dwPerm = URLPOLICY_DISALLOW;
        }
    }
    else // Unknown error.  Disallow by default
    {
        hr = S_FALSE;
        dwPerm = URLPOLICY_DISALLOW;
    }

    return hr;
}
    
