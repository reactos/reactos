// copy of GetSecurityDomainFromURL from 32bit shdocvw
// Why is that in shdocvw? Shouldn't it be in shlwapi?
// This uses MemAlloc for it's mem allocation, so use MemFree to free it.

#include "headers.hxx"

#define MAX_URL_STRING 1024
#define ASSERT Assert

// HRESULT GetSecurityDomainFromURL(WCHAR *pchURL, WCHAR **ppchDomain);
//
// The pchURL argument is the URL of a page. A string representing
// the security domain of the URL is returned in ppchDomain.
// The caller is responsible for freeing the string using CoTaskMemFree.
// The caller can compare security domains using wcscmp (case sensitive compare).
// The security domain string always begins with "<url scheme>:".
// If the scheme is HTTP or HTTPS and the host is specified as an IP address,
// then the second component of the security domain string is the IP address.
//
// If the scheme is HTTP or HTTPS and the host is specified as a domain name,
// then the second component of the security domain is the host's second
// level domain name.  For example, if the host is "www.microsoft.com",
// then the second level domain name is "microsoft.com".
// If the URL is a is a UNC file name, then the second component is the
// server name.
// If the URL is a file name rooted at a drive letter and the driver letter
// is mapped to a server, then the second component is the server name.
// If the URL is a local file, then the second component is the empty string.
// If the URL uses some other scheme not mentioned above, then the second
// component is the empty string.

STDAPI GetSecurityDomainFromURL(LPCTSTR pchURL, TCHAR **ppchDomain)
{
    HRESULT hres;
    *ppchDomain = NULL;

    // BUGWIN16 this needs more apis from urlmon, so until then.....
    CHAR szIn[MAX_URL_STRING];
#ifdef WIN16
    strncpy(szIn, pchURL, MAX_URL_STRING);
#else
    UnicodeToAnsi(pchURL, szIn, ARRAYSIZE(szIn));
#endif
    CHAR szInC[MAX_URL_STRING];
    DWORD cchT = ARRAYSIZE(szInC);
    hres = UrlCanonicalize(szIn, szInC, &cchT, 0); 

    if (SUCCEEDED(hres)) {
    
        CHAR * const pszOut = szIn; // re-use
        LPTSTR pszT;

        // Find the protocol and prefix
        PARSEDURL pu;
        pu.cbSize = sizeof(pu);
        hres = ParseURL(szInC, &pu);

        if (SUCCEEDED(hres))
        {
            // Copy the protocol + separator. Make it zero-terminated.
            lstrcpyn(pszOut, pu.pszProtocol, pu.cchProtocol+2);
    
            // Copy additional protocol specific data
            if (pu.nScheme==URL_SCHEME_HTTP || pu.nScheme==URL_SCHEME_HTTPS)
            {
                LPCSTR pszSep = pu.pszSuffix;
                // skip '/'s
                while(*pszSep=='/') pszSep++;
    
                // Find the separator '/' and truncate the rest.
                pszT = strchr(pszSep, '/');
                if (pszT) {
                    *pszT = '\0';
                }
    
                // Copy the remaining.
#ifdef WIN16
                pszT = strrchr(pszSep, '.');
                if (pszT)
                {
                    // check if it's an IP address...
                    // fast check--only check last byte of address (x.x.x.lastbyte)                    
                    LPTSTR pszT2 = pszT;
                    while (*pszT2)
                    {
                        if (*pszT2 < '0' || *pszT2 > '9')
                        {
                            break;
                        }
                    }
                    if (*pszT2)
                    {
                        // not an IP address
                        // Find the second last dot.
                        while (pszT > pszSep && *pszT != '.')
                        { 
                            pszT--;
                        }
                        if (*pszT == '.')
                        {
                            pszSep = pszT + 1;
                        }
                    }
                }
#else
                if (!_IsThisIPAddress(pszSep)) {
                    // Find the last dot.
                    pszT = StrRChrA(pszSep, NULL, '.');
                    if (pszT) {
                        // Find the second last dot.
                        pszT = StrRChrA(pszSep, pszT, '.');
                        if (pszT) {
                            // Copy the "microsoft.com" portion only.
                            pszSep = pszT+1;
                        }
                    } 
                }
#endif // win16 else
                strncat(pszOut, pszSep, ARRAYSIZE(szIn));
    
                //TraceMsg(DM_SDOMAIN, "HTTP + %s", pszSep);
    
            } else if (pu.nScheme==URL_SCHEME_FILE) {
                CHAR szPath[MAX_PATH];
                cchT = MAX_PATH; //ARRAYSIZE(szPath);
                hres = PathCreateFromUrl(szInC, szPath, &cchT, 0);
    
                // Copy the server name.
                if (SUCCEEDED(hres)) {
                    if (PathIsUNC(szPath)) {
                        ASSERT(szPath[0]=='\\');
                        ASSERT(szPath[1]=='\\');
                        // Find the separator '\\' and truncate the rest.
                        LPSTR pszT = strchr(szPath+2, '\\');
                        if (pszT) {
                            *pszT = '\0';
                        }
                        strncat(pszOut, szPath, ARRAYSIZE(szIn));
                    }
                }
            }

            if (SUCCEEDED(hres)) {
#ifdef WIN16
                *ppchDomain = (char *)CoTaskMemAlloc(strlen(pszOut)+1);
                strcpy(*ppchDomain, pszOut);
#else
                TraceMsg(DM_SDOMAIN, "Returning %s", pszOut);
                *ppchDomain = MakeWideStrFromAnsi(pszOut, STR_OLESTR);
#endif
                if (*ppchDomain==NULL) {
                    hres = E_OUTOFMEMORY;
                }
            }
        } else {
            hres = E_INVALIDARG;
        }
    }

    return hres;
}

