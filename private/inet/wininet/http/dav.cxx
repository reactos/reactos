/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    dav.cxx

Abstract:

    This file contains the implementations of 
        HttpCheckDavCompliance
        HttpCheckDavCollection
        HttpCheckCachedDavStatus
        
    The following functions are exported by this module:

        HttpCheckDavComplianceA
        HttpCheckDavCollectionA
        HttpCheckCachedDavStatusA
        HttpCheckDavComplianceW
        HttpCheckDavCollectionW
        HttpCheckCachedDavStatusW

Author:

    Mead Himelstein (Meadh) 01-Jun-1998

Revision History:


--*/


#include <wininetp.h>
#include "httpp.h"

#undef DAVCACHING //IE5 bug 15696, removing unused code but may add back in later

INTERNETAPI
BOOL
WINAPI
HttpCheckDavComplianceA(
    IN LPCSTR lpszUrl,
    IN LPCSTR lpszComplianceToken,
    IN OUT LPBOOL lpfFound,
    IN HWND hWnd,
    IN LPVOID lpvReserved
    )

/*++

Routine Description:

    Determines if the resource identified by lpszUrl is DAV compliant (level determined by lpszComplianceToken).
    Returns TRUE in lpfFound if detected. Furthermore, if the token is "1" we also cache todays date
    and whether or not we found the server to be DAV level 1 compliant in the visited links cache.
    
Arguments:

    lpszUrl             - URL to the resource to check for DAV compliance, i.e. "http://webdav/davfood/"

    lpszComplianceToken - DAV compliance class identifier (i.e. "1") BUGBUG MUST NOT CONTAIN INTERNAL WHITE SPACE "foo bar" = bad, "  foo  " = ok
    
    lpfFound            - address of a BOOL to receive TRUE if DAV compliance is found or FALSE otherwise.

    hWnd                - Handle to window for displaying authentication dialog if needed. May be NULL indicating
                          no UI is to be displayed and authentication failures should be quietly handled.
                          
    lpvReserved         - Reserved, must be NULL
    
Return Value:

    TRUE - Success, check lpfFound for results.
    FALSE - failure, GetLastError returns the error code

--*/

{
    BOOL            fRet = TRUE;


    DEBUG_ENTER((DBG_API,
                 Bool,
                 "HttpCheckDavComplianceA",
                 "%sq, %sq, %#x, %#x, %#x",
                 lpszUrl,
                 lpszComplianceToken,
                 lpfFound,
                 hWnd,
                 lpvReserved
                 ));


    DWORD           dwStatusCode, dwHeaderLength, dwIndex = 0;
    DWORD           dwError = ERROR_SUCCESS, dwStatusLength = sizeof(DWORD);
    HINTERNET       hSession = NULL, hConnection = NULL, hHTTPReq = NULL;
    LPSTR           lpszDavHeader = NULL, lpszRead = NULL, lpszWrite = NULL, lpszVisitedUrl=NULL;
   	LPCACHE_ENTRY_INFO lpCEI = NULL;
    FILETIME        ftNow;
    WORD            wDate, wTime;
    URL_COMPONENTS  ucUrl;

    #define CEI_BUFFER_SIZE 512
    #define IsWhite(c)      ((DWORD) (c) > 32 ? FALSE : TRUE)

    // Debug param checking
    INET_ASSERT(lpszUrl && lpszComplianceToken && lpfFound && !lpvReserved);

    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "HttpCheckDavComplianceA",
                     "%s %s %x %x %x",
                     (lpszUrl)?lpszUrl :"NULL Url",
                     (lpszComplianceToken)?lpszComplianceToken :"NULL ComplianceToken",
                     hWnd,
                     lpfFound, lpvReserved
                     ));

    // non-debug param checking
    if (!lpszUrl || !lpszComplianceToken || !lpfFound || lpvReserved) 
    {
        dwError = ERROR_INVALID_PARAMETER;
	    fRet = FALSE;
	    goto cleanup;
    }

    *lpfFound = FALSE;
    
    // Build the URL_COMPONENTS struct
	memset(&ucUrl, 0, sizeof(ucUrl));
    ucUrl.dwStructSize = sizeof(ucUrl);
    // non-zero length enables retrieval during CrackUrl
    ucUrl.dwSchemeLength = 1;
	ucUrl.dwHostNameLength = 1;
	ucUrl.dwUrlPathLength = 1;
	if (!InternetCrackUrl(lpszUrl, 0, 0, &ucUrl))
	{
		dwError = GetLastError();
	    fRet = FALSE;
	    goto cleanup;
    }

    DEBUG_PRINT_API(API,
                    INFO,
                    ("URL Cracked. Host = %q Path = %q\n ",
                    ucUrl.lpszHostName, ucUrl.lpszUrlPath
                    ));

    // Ensures the global vszCurrentUser is set
    GetWininetUserName();   
    INET_ASSERT(vszCurrentUser);
    
    // Perform an OPTIONS call on the server and check for a DAV header
	hSession = InternetOpen(NULL,
						  INTERNET_OPEN_TYPE_PRECONFIG,
						  NULL,
						  NULL,
						  NULL);
	if(!hSession)
	{
		dwError = GetLastError();
	    fRet = FALSE;
		goto cleanup;
	}
	
	hConnection = InternetConnect(hSession,
								   ucUrl.lpszHostName,
								   INTERNET_DEFAULT_HTTP_PORT,
								   (LPCSTR) vszCurrentUser,
								   NULL,
								   INTERNET_SERVICE_HTTP,
								   NULL,
								   NULL);

	if(!hConnection)
	{
		dwError = GetLastError();
	    fRet = FALSE;
	    goto cleanup;
	}

	hHTTPReq = HttpOpenRequest(hConnection,
							   "OPTIONS",
							   ucUrl.lpszUrlPath,
							   "HTTP/1.0",
							   NULL,
							   NULL,
							   INTERNET_FLAG_PRAGMA_NOCACHE,NULL);

	if(!hHTTPReq)
	{
		dwError = GetLastError();
	    fRet = FALSE;
	    goto cleanup;
	}

	if (!HttpSendRequest(hHTTPReq,NULL, 0L, NULL, NULL))
	{
		dwError = GetLastError();
		fRet = FALSE;
		goto cleanup;
	}
        

    // Authentication handling
	if (HttpQueryInfo(hHTTPReq,
					  HTTP_QUERY_STATUS_CODE|HTTP_QUERY_FLAG_NUMBER,
					  &dwStatusCode,
					  &dwStatusLength,
					  &dwIndex))
	{
        // fRet = TRUE at this point  
        // If request was denied or proxy auth required and callee has said
        // we can display UI, we put up the InternetErrorDlg and ask for
        // credentials. Otherwise, if dwStatus != success we cache that
        // this resource is not DAV compliant
        if ((hWnd) && ((dwStatusCode == HTTP_STATUS_DENIED) || (dwStatusCode == HTTP_STATUS_PROXY_AUTH_REQ)))
        {
            DWORD dwRetval;
            DWORD dwAuthTries = 0;
            BOOL fDone;
            fDone = FALSE;
            while ((!fDone) && (dwAuthTries < 3))
            {
                dwRetval = InternetErrorDlg(hWnd,
                                            hHTTPReq,
                                            ERROR_INTERNET_INCORRECT_PASSWORD,
                                            0L,
                                            NULL);
                if (dwRetval == ERROR_INTERNET_FORCE_RETRY) // User pressed ok on credentials dialog
                {   // Resend request, new credentials are cached and will be replayed by HSR()
                	if (!HttpSendRequest(hHTTPReq,NULL, 0L, NULL, NULL))
                	{
                		dwError = GetLastError();
                		fRet = FALSE;
                		goto cleanup;
                	}

                    dwStatusCode = 0;

                	if (!HttpQueryInfo(hHTTPReq,
                					  HTTP_QUERY_STATUS_CODE|HTTP_QUERY_FLAG_NUMBER,
                					  &dwStatusCode,
                					  &dwStatusLength,
                					  &dwIndex))
                	{
                		dwError = GetLastError();
                	    fRet = FALSE;
                	    goto cleanup;
                	}

                    if ((dwStatusCode != HTTP_STATUS_DENIED) && (dwStatusCode != HTTP_STATUS_PROXY_AUTH_REQ))
                    {
                        fDone = TRUE;
                    }
                }
                else    // User pressed cancel from dialog (note ERROR_SUCCESS == ERROR_CANCELLED from IED())
                {
                    fDone = TRUE;
                }
                dwAuthTries++;
            }
        }

        if ((dwStatusCode == HTTP_STATUS_DENIED) || (dwStatusCode == HTTP_STATUS_PROXY_AUTH_REQ))
        {   // Don't want to cache this (user might have forgotten password or we may not
            // be able to show credential UI)
            goto cleanup;
        }
        else if (dwStatusCode != HTTP_STATUS_OK)
        {   // Either initial request failed for non-auth issue, cache as not compliant
            goto update_cache; //note update_cache NOT cleanup, want to cache this as non compliant
        }
	}
	else
	{
		dwError = GetLastError();
	    fRet = FALSE;
	    goto cleanup;
	}

    // If we received a DAV header, snag it and check for compliance level
    dwHeaderLength = INTERNET_MAX_URL_LENGTH;
    lpszDavHeader = (LPSTR)ALLOCATE_MEMORY(LMEM_FIXED, dwHeaderLength);
    if (!lpszDavHeader) 
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }

    lstrcpy(lpszDavHeader,"DAV");
	if (!HttpQueryInfo(hHTTPReq,
					  HTTP_QUERY_CUSTOM,
					  lpszDavHeader,
					  &dwHeaderLength,
					  &dwIndex))
	{
   		dwError = GetLastError();
	    fRet = FALSE;
	    goto update_cache; //note update_cache NOT cleanup, want to cache this as non compliant

    }
    INET_ASSERT(fRet); // sb TRUE if it made it this far
    
    // Walk the DAV header looking for the token by itself, indicating DAV compliance
    // Note: DAV header is comma delimited, but otherwise we can make no assumptions on formatting.
    // BUGBUG Currently will not work for tokens with internal white space i.e. "foo bar" = bad, " foobar " = good

    lpszRead = lpszDavHeader;
    lpszWrite = lpszDavHeader;
    INET_ASSERT(!*lpfFound);
    INET_ASSERT(fRet);
    while(*lpszRead && !*lpfFound) 
    {
        if (IsWhite(*lpszRead)) 
        {
            while ((*(++lpszRead))&&(IsWhite(*lpszRead)));
        }
        if (*lpszRead) 
        {
            if(*lpszRead == ',') 
            {
                *(lpszRead++) = '\0';
                if (*lpfFound = (lstrcmp(lpszDavHeader, lpszComplianceToken) == 0) ? TRUE:FALSE)
                {   
                    goto update_cache;
                }
                lpszWrite = lpszRead;
                lpszDavHeader = lpszRead;
            }
            else *(lpszWrite++) = *(lpszRead++);    
        }
    }
    *lpszWrite = *lpszRead; // copy the terminator
    INET_ASSERT(!*lpfFound);
    *lpfFound = (lstrcmp(lpszDavHeader, lpszComplianceToken) == 0) ? TRUE:FALSE;    //Final compare on last fragment

update_cache :
#ifdef DAVCACHING //  Commenting out as per bug 15696

    if (lstrcmp("1", lpszComplianceToken) == 0) 
    // Cache if this is a known DAV level 1 server or atleast the last time we tried
    {

        // Update visited links cache
        // We store the cached info as:
        //  CacheEntryInfo.dwExemptDelta(HiWord) = Date DAV discovery was last run
        //  CacheEntryInfo.dwExemptDelta(LoWord) & DAV_LEVEL1_STATUS = Is DAV server (*lpfFound)
        //  CacheEntryInfo.dwExemptDelta(LoWord) & DAV_COLLECTION_STATUS = Is DAV collection (set in HttpCheckDavCollection)

        // Assemble the URL to the visited links container "VISITED: username@URL"
        DWORD cbNeededBuf = lstrlen("Visited: ") + lstrlen(vszCurrentUser) + lstrlen(lpszUrl) + 32;
        lpszVisitedUrl = (LPSTR) ALLOCATE_MEMORY(LMEM_FIXED, cbNeededBuf);
    	if (!lpszVisitedUrl)
        {
            dwError = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        lstrcpy(lpszVisitedUrl, "Visited: "); // For compatibility with urlhist.c visited URLs
        lstrcat(lpszVisitedUrl, vszCurrentUser);
        lstrcat(lpszVisitedUrl, "@");
        lstrcat(lpszVisitedUrl, lpszUrl);

        lpCEI = (LPCACHE_ENTRY_INFO) ALLOCATE_MEMORY(LMEM_FIXED, CEI_BUFFER_SIZE);
    	if (!lpCEI)
        {
            dwError = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }

        GetSystemTimeAsFileTime(&ftNow);
        if (!FileTimeToDosDateTime(&ftNow,
                                   &wDate,
                                   &wTime
                                   ))
        {                          
            INET_ASSERT(FALSE); // Should "never" hit this
            goto cleanup;                              
        }

        // Stuff our 16-bit Date into the upper 16-bits of dwExemptDelta
        lpCEI->dwExemptDelta = (DWORD)wDate <<16;

        // Set the DAV_LEVEL1_STATUS_BIT if the resource was found compliant above
        if (*lpfFound)
        {
            lpCEI->dwExemptDelta |= (DWORD) DAV_LEVEL1_STATUS;
        }
    

        if (!SetUrlCacheEntryInfo (lpszVisitedUrl, 
	  	    					   lpCEI,
		        				   CACHE_ENTRY_EXEMPT_DELTA_FC
			        				))
        {
	       	if (GetLastError() == ERROR_FILE_NOT_FOUND)
	   	    // No existing cache entry, so create one and try again
    	    {
	            FILETIME ftNone;
                if (CommitUrlCacheEntry (lpszVisitedUrl,
                                         NULL,
                                         ftNone,
                                         ftNone,
                                         0,
                                         NULL,
                                         0,
                                         NULL,
                                         0
                                         ))
                {
                    // Entry is now created, so try one more time
                    SetUrlCacheEntryInfo (lpszVisitedUrl, 
	       			    			      lpCEI,
                		    			  CACHE_ENTRY_EXEMPT_DELTA_FC
			               	    		 );
                }
  	    	}
        }
    }

#endif //DAVCACHING
    
cleanup:    
    // Free memory
    if (lpCEI) FREE_MEMORY (lpCEI);
    if (lpszVisitedUrl) FREE_MEMORY (lpszVisitedUrl);
    if (lpszDavHeader != NULL) FREE_MEMORY(lpszDavHeader);

    // Close the handles
	if (hHTTPReq) InternetCloseHandle(hHTTPReq);
	if (hConnection) InternetCloseHandle(hConnection);

    SetLastError(dwError);


    DEBUG_LEAVE_API(fRet);
	return fRet;
}


INTERNETAPI
BOOL
WINAPI
HttpCheckDavCollectionA(
    IN LPCSTR lpszUrl,
    IN OUT LPBOOL lpfFound,
    IN HWND hWnd,
    IN LPVOID lpvReserved
    )

/*++

Routine Description:

    Checks the resource at lpszUrl to see if it is a DAV collection. Typically, callee has 
    already called HttpCheckDavCompliance or HttpCheckCachedDavStatus to determine 
    the resource supports DAV level 1, although it is not necessary to do so. Note this method
    will blindly attempt either way.
    
Arguments:

    lpszUrl             - URL of the resource to check

    lpfFound            - address of a BOOL to receive TRUE if DAV compliance is found or FALSE otherwise.

    hWnd                - Handle to window for displaying authentication dialog if needed. May be NULL indicating
                          no UI is to be displayed and authentication failures should be quietly handled.
                          
    lpvReserved         - Reserved, must be NULL
    
Return Value:

    TRUE - Success, check lpfFound for results.
    FALSE - failure, GetLastError returns the error code

--*/

{
    #define CEI_BUFFER_SIZE 512
    #define READ_BUFFER_SIZE 4096
    #define READ_BUFFER_AVAILABLE  (READ_BUFFER_SIZE - sizeof(LPSTR))
    BOOL            fRet = TRUE, fDone=FALSE;

    DEBUG_ENTER((DBG_API,
                 Bool,
                 "HttpCheckDavCollectionA",
                 "%sq, %#x, %#x, %#x",
                 lpszUrl,
                 lpfFound,
                 hWnd,
                 lpvReserved
                 ));


    DWORD           dwRead=0, dwBuffers =1;
    DWORD           dwStatusCode, dwIndex = 0;
    DWORD           dwError = ERROR_SUCCESS, dwStatusLength = sizeof(DWORD);
    HINTERNET       hSession = NULL, hConnection = NULL, hHTTPReq = NULL;
    LPSTR           lpszVisitedUrl=NULL, lpszXmlResponse=NULL, lpszBufferHead=NULL;
   	LPCACHE_ENTRY_INFO lpCEI = NULL;
    FILETIME        ftNow;
    WORD            wDate, wTime;
    URL_COMPONENTS  ucUrl;
    char lpszHeaders[] = "Depth:0\r\n";

    // Debug param checking
    INET_ASSERT(lpszUrl && lpfFound && !lpvReserved);

    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "HttpCheckDavCollectionA",
                     "%s %x %x",
                     (lpszUrl)?lpszUrl :"NULL Url",
                     lpfFound, lpvReserved
                     ));

    // non-debug param checking
    if (!lpszUrl || !lpfFound || lpvReserved) 
    {
        dwError = ERROR_INVALID_PARAMETER;
	    fRet = FALSE;
	    goto cleanup;
    }

    *lpfFound = FALSE;
    
    // Build the URL_COMPONENTS struct
	memset(&ucUrl, 0, sizeof(ucUrl));
    ucUrl.dwStructSize = sizeof(ucUrl);
    // non-zero length enables retrieval during CrackUrl
    ucUrl.dwSchemeLength = 1;
	ucUrl.dwHostNameLength = 1;
	ucUrl.dwUrlPathLength = 1;
	if (!InternetCrackUrl(lpszUrl, 0, 0, &ucUrl))
	{
		dwError = GetLastError();
	    fRet = FALSE;
	    goto cleanup;
    }

    INET_ASSERT(ucUrl.lpszHostName && ucUrl.lpszUrlPath);
    DEBUG_PRINT_API(API,
                    INFO,
                    ("URL Cracked. Host = %q Path = %q\n ",
                    ucUrl.lpszHostName, ucUrl.lpszUrlPath
                    ));

    // Ensures the global vszCurrentUser is set
    GetWininetUserName();   
    INET_ASSERT(vszCurrentUser);
    
    // Build the XML request body for a PROPFIND
    //<?xml version='1.0' ?>
    //<?xml:namespace ns='DAV:' prefix='D' ?>
    //<D:propfind>
    //  <D:prop>
    //    <D:resourcetype/>
    //  </D:prop>
    //</D:propfind>    
    
    #define REQUEST_RESOURCE_TYPE   "\
                                    <?xml version='1.0' ?>\r\n \
                                    <?xml:namespace ns='DAV:' prefix='D' ?>\r\n \
                                    <D:propfind>\r\n \
                                        <D:prop>\r\n \
                                            <D:resourcetype/>\r\n \
                                        </D:prop>\r\n \
                                    </D:propfind>\r\n"
    DWORD cbReqSize;
    cbReqSize = lstrlen(REQUEST_RESOURCE_TYPE);
                                    
    LPCSTR lpszXmlRequest;
    lpszXmlRequest = (LPSTR)ALLOCATE_MEMORY(LMEM_FIXED, cbReqSize);
    
    // Perform a PROPFIND call, looking for resourcetype = DAV:collection
    hSession = InternetOpen(NULL,
						  INTERNET_OPEN_TYPE_PRECONFIG,
						  NULL,
						  NULL,
						  NULL);
	if(!hSession)
	{
		dwError = GetLastError();
	    fRet = FALSE;
		goto cleanup;
	}
	
	hConnection = InternetConnect(hSession,
								   ucUrl.lpszHostName,
								   INTERNET_DEFAULT_HTTP_PORT,
								   (LPCSTR) vszCurrentUser,
								   NULL,
								   INTERNET_SERVICE_HTTP,
								   NULL,
								   NULL);

	if(!hConnection)
	{
		dwError = GetLastError();
	    fRet = FALSE;
	    goto cleanup;
	}

	hHTTPReq = HttpOpenRequest(hConnection,
							   "PROPFIND",
							   ucUrl.lpszUrlPath,
							   "HTTP/1.0",
							   NULL,
							   NULL,
							   INTERNET_FLAG_PRAGMA_NOCACHE,NULL);

	if(!hHTTPReq)
	{
		dwError = GetLastError();
	    fRet = FALSE;
	    goto cleanup;
	}

	if (!HttpAddRequestHeaders(hHTTPReq, lpszHeaders, -1L, HTTP_ADDREQ_FLAG_ADD))
	{
		dwError = GetLastError();
		fRet = FALSE;
		goto cleanup;
	}

	if (!HttpSendRequest(hHTTPReq,NULL, 0L, (LPVOID)lpszXmlRequest, cbReqSize))
	{
		dwError = GetLastError();
		fRet = FALSE;
		goto cleanup;
	}
        
	if (HttpQueryInfo(hHTTPReq,
					  HTTP_QUERY_STATUS_CODE|HTTP_QUERY_FLAG_NUMBER,
					  &dwStatusCode,
					  &dwStatusLength,
					  &dwIndex))
	{
        // fRet = TRUE at this point  
        // If request was denied or proxy auth required and callee has said
        // we can display UI, we put up the InternetErrorDlg and ask for
        // credentials. Otherwise, if dwStatus != success we cache that
        // this resource is not DAV compliant
        if ((hWnd) && ((dwStatusCode == HTTP_STATUS_DENIED) || (dwStatusCode == HTTP_STATUS_PROXY_AUTH_REQ)))
        {
            DWORD dwRetval;
            BOOL fDone;
            fDone = FALSE;
            while (!fDone)
            {
                dwRetval = InternetErrorDlg(hWnd,
                                            hHTTPReq,
                                            ERROR_INTERNET_INCORRECT_PASSWORD,
                                            0L,
                                            NULL);
                if (dwRetval == ERROR_INTERNET_FORCE_RETRY) // User pressed ok on credentials dialog
                {   // Resend request, new credentials are cached and will be replayed by HSR()
                	if (!HttpSendRequest(hHTTPReq,NULL, 0L, NULL, NULL))
                	{
                		dwError = GetLastError();
                		fRet = FALSE;
                		goto cleanup;
                	}

                    dwStatusCode = 0;

                	if (!HttpQueryInfo(hHTTPReq,
                					  HTTP_QUERY_STATUS_CODE|HTTP_QUERY_FLAG_NUMBER,
                					  &dwStatusCode,
                					  &dwStatusLength,
                					  &dwIndex))
                	{
                		dwError = GetLastError();
                	    fRet = FALSE;
                	    goto cleanup;
                	}

                    if ((dwStatusCode != HTTP_STATUS_DENIED) && (dwStatusCode != HTTP_STATUS_PROXY_AUTH_REQ))
                    {
                        fDone = TRUE;
                    }
                }
                else    // User pressed cancel from dialog (note ERROR_SUCCESS == ERROR_CANCELLED from IED())
                {
                    fDone = TRUE;
                }
            }
        }

        if ((dwStatusCode == HTTP_STATUS_DENIED) || (dwStatusCode == HTTP_STATUS_PROXY_AUTH_REQ))
        {   // Don't want to cache this (user might have forgotten password or we may not
            // be able to show credential UI)
            goto cleanup;
        }
        else if (dwStatusCode != HTTP_STATUS_OK)
        {   // Either initial request failed for non-auth issue, cache as not compliant
            goto update_cache; //note update_cache NOT cleanup, want to cache this as non compliant
        }
	}
	else
	{
		dwError = GetLastError();
	    fRet = FALSE;
	    goto cleanup;
	}

    // Note, what we are doing here is allocating a single page of memory and reserving
    // the first sizeof(LPSTR) bytes to point to the next buffer fragment if required,
    // and so on until the last buffer fragment has NULL at the dwPtrOffset

    lpszBufferHead = (LPSTR)ALLOCATE_MEMORY(LMEM_FIXED, READ_BUFFER_SIZE);
    if (!lpszBufferHead)
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }

	memset(lpszBufferHead, 0, READ_BUFFER_SIZE);
	LPSTR lpNext;
    lpNext=lpszBufferHead+sizeof(LPSTR); // Jump over the next buffer ptr space when writing data
    while(!fDone)
    {
        if (!InternetReadFile(hHTTPReq,
                              lpNext,
                              READ_BUFFER_AVAILABLE,
                              &dwRead))
    	{
    		dwError = GetLastError();
    	    fRet = FALSE;
    	    goto cleanup;
    	}
        else 
        {
            if (dwRead < READ_BUFFER_AVAILABLE)
            {
                fDone = TRUE;
            }
            else 
            {
                dwBuffers++;
                *((LPSTR*)lpNext+READ_BUFFER_AVAILABLE) = (LPSTR)ALLOCATE_MEMORY(LMEM_FIXED, READ_BUFFER_SIZE);
                lpNext = *((LPSTR*)lpNext+READ_BUFFER_AVAILABLE);
                if (!lpNext) 
                {
                    dwError = ERROR_NOT_ENOUGH_MEMORY;
                    goto cleanup;
                }
            	memset(lpNext, 0, READ_BUFFER_SIZE);
            }
            
        }
                          
    }

    // Done reading response, time to parse it
    
    
update_cache :

#ifdef DAVCACHING // commenting out as per bug 15696

    if (TRUE) 
    // Cache if this is a known DAV level 1 server or atleast the last time we tried
    {

        // Update visited links cache
        // We store the cached info as:
        //  CacheEntryInfo.dwExemptDelta(HiWord) = Date DAV discovery was last run
        //  CacheEntryInfo.dwExemptDelta(LoWord) & DAV_LEVEL1_STATUS = Is DAV server (*lpfFound)
        //  CacheEntryInfo.dwExemptDelta(LoWord) & DAV_COLLECTION_STATUS = Is DAV collection (set in HttpCheckDavCollection)

        // Assemble the URL to the visited links container "VISITED: username@URL"
        DWORD cbNeededBuf = lstrlen("Visited: ") + lstrlen(vszCurrentUser) + lstrlen(lpszUrl) + 32;
        lpszVisitedUrl = (LPSTR) ALLOCATE_MEMORY(LMEM_FIXED, cbNeededBuf);
    	if (!lpszVisitedUrl)
        {
            dwError = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        lstrcpy(lpszVisitedUrl, "Visited: "); // For compatibility with urlhist.c visited URLs
        lstrcat(lpszVisitedUrl, vszCurrentUser);
        lstrcat(lpszVisitedUrl, "@");
        lstrcat(lpszVisitedUrl, lpszUrl);

        lpCEI = (LPCACHE_ENTRY_INFO) ALLOCATE_MEMORY(LMEM_FIXED, CEI_BUFFER_SIZE);
    	if (!lpCEI)
        {
            dwError = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }

        GetSystemTimeAsFileTime(&ftNow);
        if (!FileTimeToDosDateTime(&ftNow,
                                   &wDate,
                                   &wTime
                                   ))
        {                                   
            INET_ASSERT(FALSE); // Should "never" hit this
            goto cleanup;                              
        }

        // Stuff our 16-bit Date into the upper 16-bits of dwExemptDelta
        lpCEI->dwExemptDelta = (DWORD)wDate <<16;

        // Set the DAV_LEVEL1_STATUS_BIT if the resource was found compliant above
        if (*lpfFound) 
        {
            lpCEI->dwExemptDelta |= (DWORD) DAV_LEVEL1_STATUS;
        }
    

        if (!SetUrlCacheEntryInfo (lpszVisitedUrl, 
	  	    					   lpCEI,
		        				   CACHE_ENTRY_EXEMPT_DELTA_FC
			        				))
        {
	       	if (GetLastError() == ERROR_FILE_NOT_FOUND)
	   	    // No existing cache entry, so create one and try again
    	    {
	            FILETIME ftNone;
                if (CommitUrlCacheEntry (lpszVisitedUrl,
                                         NULL,
                                         ftNone,
                                         ftNone,
                                         0,
                                         NULL,
                                         0,
                                         NULL,
                                         0
                                         ))
                {
                    // Entry is now created, so try one more time
                    SetUrlCacheEntryInfo (lpszVisitedUrl, 
	       			    			      lpCEI,
                		    			  CACHE_ENTRY_EXEMPT_DELTA_FC
			               	    		 );
                }
  	    	}
        }
    }
#endif //DAVCACHING
    
cleanup:    
    // Free memory
    // Walk buffer chain
    if (lpszBufferHead)
    {
        lpNext = (LPSTR)*(lpszBufferHead+READ_BUFFER_AVAILABLE);
        FREE_MEMORY(lpszBufferHead);
        while (lpNext)
        {
            lpszBufferHead = lpNext;
            lpNext = (LPSTR)*(lpszBufferHead+READ_BUFFER_AVAILABLE);
            FREE_MEMORY(lpszBufferHead);
        }
    }
    if (lpszXmlRequest) FREE_MEMORY(lpszXmlRequest);
    if (lpCEI) FREE_MEMORY (lpCEI);
    if (lpszVisitedUrl) FREE_MEMORY (lpszVisitedUrl);

    // Close the handles
	if (hHTTPReq) InternetCloseHandle(hHTTPReq);
	if (hConnection) InternetCloseHandle(hConnection);

    SetLastError(dwError);
    DEBUG_LEAVE_API(fRet);
	return fRet;
}

#ifdef DAVCACHING // commenting out as per bug 15696
INTERNETAPI
BOOL
WINAPI
HttpCheckCachedDavStatusA(
    IN LPCSTR lpszUrl,
    IN OUT LPDWORD lpdwStatus
    )

/*++

Routine Description:

    Checks if there is cached DAV information for this site or if detection is required, After a successfull call,
    *lpdwStatus will contain a bitfield as described in arguments below.

    Checks HKCU\SOFTWARE\Microsoft\Windows\CurrentVersion\Internet Settings\DaysBetweenDavDetection for time in days
    to pass between performing detection. If this key is missing, a default of 10 is used.

Arguments:

    lpszUrl             - Url of the resource to check

    lpdwStatus          - address of a DWORD (which must contain zero initially!!) that on return may contain a
                          combination of the following staus bits:
        DAV_LEVEL1_STATUS       - If this bit is set, the site is known to support DAV level 1
        DAV_COLLECTION_STATUS   - If this bit is set, the resource is a collection.
        DAV_DETECTION_REQUIRED  - Either no information is available or information is stale and detection should be performed. 

                          
Return Value:

    TRUE - Success, check lpdwStatus for results.
    FALSE - failure, GetLastError returns the error code

--*/

{
    BOOL            fRet = TRUE;

    DEBUG_ENTER((DBG_API,
                 Bool,
                 "HttpCheckCachedDavStatusA",
                 "%sq, %#x",
                 lpszUrl,
                 lpdwStatus
                 ));


    DWORD           dwError;
   	LPCACHE_ENTRY_INFO lpCEI = NULL;
    FILETIME        ftNow, ftNext, ftWork;
    WORD            wDate;
    LPSTR           lpszVisitedUrl=NULL;
    DWORD           cbCEI = 512, dwDays = 0;
    #define DEFAULT_DAV_DAYS 10
    #define FILE_SEC_TICKS (10000000)
    // SECS PER DAY
    #define DAY_SECS (24*60*60)
    // Debug param checking
    INET_ASSERT(lpszUrl && lpdwStatus && (*lpdwStatus==0));

    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "HttpCheckCachedDavStatusA",
                     "%s %x %x",
                     (lpszUrl)?lpszUrl :"NULL Url",
                     lpdwStatus,
                     *lpdwStatus
                     ));

    // non-debug param checking
    if (!lpszUrl || !lpdwStatus || (*lpdwStatus != 0)) 
    {
        dwError = ERROR_INVALID_PARAMETER;
	    fRet = FALSE;
	    goto cleanup;
    }

    // Ensures the global vszCurrentUser is set
    GetWininetUserName();   
    INET_ASSERT(vszCurrentUser);

    // Check visited links cache
    // We store the cached info as:
    //  CacheEntryInfo.dwExemptDelta(HiWord) = Date DAV discovery was last run
    //  CacheEntryInfo.dwExemptDelta(LoWord) & DAV_LEVEL1_STATUS = Is DAV server (*lpfFound)
    //  CacheEntryInfo.dwExemptDelta(LoWord) & DAV_COLLECTION_STATUS = Is DAV collection (set in HttpCheckDavCollection)

    // Assemble the URL to the visited links container "VISITED: username@URL"
    DWORD cbNeededBuf = lstrlen("Visited: ") + lstrlen(vszCurrentUser) + lstrlen(lpszUrl) + 32;
    lpszVisitedUrl = (LPSTR) ALLOCATE_MEMORY(LMEM_FIXED, cbNeededBuf);
	if (!lpszVisitedUrl)
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }
    lstrcpy(lpszVisitedUrl, "Visited: "); // For compatibility with urlhist.c visited URLs
    lstrcat(lpszVisitedUrl, vszCurrentUser);
    lstrcat(lpszVisitedUrl, "@");
    lstrcat(lpszVisitedUrl, lpszUrl);

    lpCEI = (LPCACHE_ENTRY_INFO) ALLOCATE_MEMORY(LMEM_FIXED, cbCEI);
	if (!lpCEI)
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }

    if (!GetUrlCacheEntryInfo (lpszVisitedUrl, 
  	    					   lpCEI,
	        				   &cbCEI
		        				))
    {
       	if (GetLastError() == ERROR_FILE_NOT_FOUND)
   	    // No existing cache entry, so return DAV_DETECTION_REQUIRED
	    {
            *lpdwStatus = DAV_DETECTION_REQUIRED;
            goto cleanup;
    
	    }
	    else
	    {
	        dwError = GetLastError();
	        fRet = FALSE;
	        goto cleanup;
	    }
    }

    // Get registry setting for # of days between detection or use default if not found
    if ((dwError = InternetReadRegistryDword("DaysBetweenDavDetection", &dwDays)) != ERROR_SUCCESS)
    {
        dwDays = DEFAULT_DAV_DAYS;
    }

    // Get the last date we checked this URL
    wDate = HIWORD(lpCEI->dwExemptDelta);
    DosDateTimeToFileTime(wDate, WORD(0), &ftWork);

    _int64 i64Base;

    i64Base = (((_int64)ftWork.dwHighDateTime) << 32) | ftWork.dwLowDateTime;
    i64Base /= FILE_SEC_TICKS;
    i64Base /= DAY_SECS;
    i64Base += dwDays;
    i64Base *= FILE_SEC_TICKS;
    i64Base *= DAY_SECS;
    ftNext.dwHighDateTime = (DWORD) ((i64Base >> 32) & 0xFFFFFFFF);
    ftNext.dwLowDateTime = (DWORD) (i64Base & 0xFFFFFFFF);

    GetSystemTimeAsFileTime(&ftNow);
    if (CompareFileTime(ftNow, ftNext) >= 0)    // If Now >= Next detect date
    {
        *lpdwStatus = DAV_DETECTION_REQUIRED;
    }

    *lpdwStatus |= LOWORD(lpCEI->dwExemptDelta);
    dwError = ERROR_SUCCESS;
    
cleanup:    
    // Free memory
    if (lpCEI) FREE_MEMORY (lpCEI);
    if (lpszVisitedUrl) FREE_MEMORY (lpszVisitedUrl);

    SetLastError(dwError);
    DEBUG_LEAVE_API(fRet);
	return fRet;
}

#endif //DAVCACHING


INTERNETAPI
BOOL
WINAPI
HttpCheckDavComplianceW(
    IN LPCWSTR lpszUrlW,
    IN LPCWSTR lpszComplianceTokenW,
    IN OUT LPBOOL lpfFound,
    IN HWND hWnd,
    IN LPVOID lpvReserved
    )

/*++

Routine Description:

    Determines the level of DAV compliance of the server. Currently just checks for level 1
    compliance and returns TRUE in lpfFound if detected.
    
Arguments:

    lpszServer          - name of server to check

    lpszPath            - path to resource on the server to check

    lpszComplianceToken - DAV compliance class identifier (i.e. "1") BUGBUG MUST NOT CONTAIN INTERNAL WHITE SPACE "foo bar" = bad, "  foo  " = ok
    
    lpfFound            - address of a BOOL to receive TRUE if DAV compliance is found or FALSE otherwise.

    hWnd                - Handle to window for displaying authentication dialog if needed. May be NULL indicating
                          no UI is to be displayed and authentication failures should be quietly handled.
                          
    lpvReserved         - Reserved, must be NULL
    
Return Value:

    TRUE - Success, check lpfFound for results.
    FALSE - failure, GetLastError returns the error code

--*/

{

    DEBUG_ENTER((DBG_API,
                 Bool,
                 "HttpCheckDavComplianceW",
                 "%wq, %wq, %#x, %#x, %#x",
                 lpszUrlW,
                 lpszComplianceTokenW,
                 lpfFound,
                 hWnd,
                 lpvReserved
                 ));

    MEMORYPACKET mpUrlA, mpComplianceTokenA;
    BOOL            fRet = TRUE;
    DWORD           dwError = ERROR_SUCCESS;
    
    // Debug param checking
    INET_ASSERT(lpszUrlW && lpszComplianceTokenW && lpfFound && !lpvReserved);

    // non-debug param checking
    if (!lpszUrlW || !lpszComplianceTokenW || !lpfFound || lpvReserved) 
    {
        dwError = ERROR_INVALID_PARAMETER;
	    fRet = FALSE;
	    goto cleanup;
    }

    ALLOC_MB(lpszUrlW, 0, mpUrlA);
    if (!mpUrlA.psStr)
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
	    fRet = FALSE;
	    goto cleanup;
    }
    UNICODE_TO_ANSI(lpszUrlW, mpUrlA);

    //  MAKE_ANSI(lpszComplianceTokenW, 0, mpComplianceTokenA);
    ALLOC_MB(lpszComplianceTokenW, 0, mpComplianceTokenA);
    if (!mpComplianceTokenA.psStr)
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
	    fRet = FALSE;
	    goto cleanup;
    }
    UNICODE_TO_ANSI(lpszComplianceTokenW, mpComplianceTokenA);

    fRet =  HttpCheckDavComplianceA(mpUrlA.psStr, mpComplianceTokenA.psStr, lpfFound, hWnd, lpvReserved);

cleanup:    
    DEBUG_LEAVE_API(fRet);
    return(fRet);


}


#ifdef DAVCACHING //commenting out as per bug 15696
INTERNETAPI
BOOL
WINAPI
HttpCheckCachedDavStatusW(
    IN LPCWSTR lpszUrlW,
    IN OUT LPDWORD lpdwStatus
    )

/*++

Routine Description:

    Checks if there is cached DAV information for this site or if detection is required, 
Arguments:

    lpszUrl             - Url of the resource to check

    lpdwStatus          - address of a DWORD (which must contain zero initially!!) that on return may contain a
                          combination of the following staus bits:
        DAV_LEVEL1_STATUS       - If this bit is set, the site is known to support DAV level 1
        DAV_COLLECTION_STATUS   - If this bit is set, the resource is a collection.
        DAV_DETECTION_REQUIRED  - Either no information is available or information is stale and detection should be performed. 

                          
Return Value:

    TRUE - Success, check lpdwStatus for results.
    FALSE - failure, GetLastError returns the error code

--*/

{

    DEBUG_ENTER((DBG_API,
                 Bool,
                 "HttpCheckCachedDavStatusW",
                 "%wq, %#x",
                 lpszUrlW,
                 lpdwStatus
                 ));

    MEMORYPACKET mpUrlA;
    BOOL            fRet = TRUE;
    DWORD           dwError = ERROR_SUCCESS;
    // Debug param checking
    INET_ASSERT(lpszUrlW && lpdwStatus && (*lpdwStatus==0));

    // non-debug param checking
    if (!lpszUrlW || !lpdwStatus || (*lpdwStatus!=0)) 
    {
        dwError = ERROR_INVALID_PARAMETER;
	    fRet = FALSE;
	    goto cleanup;
    }

    ALLOC_MB(lpszUrlW, 0, mpUrlA);
    if (!mpUrlA.psStr)
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
	    fRet = FALSE;
	    goto cleanup;
    }
    UNICODE_TO_ANSI(lpszUrlW, mpUrlA);

    fRet = HttpCheckCachedDavStatusA(mpUrlA.psStr, lpdwStatus);

cleanup:
    DEBUG_LEAVE_API(fRet);
    return(fRet);

}
#endif //DAVCACHING


