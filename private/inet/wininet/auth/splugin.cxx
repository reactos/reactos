/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    splugin.cxx

Abstract:

    This file contains the implementation for Plug In Authentication

    The following functions are exported by this module:

    AuthOnRequest
    AuthOnResponse
    AuthCtxClose
    AuthInDialog
    AuthNotify
    AuthUnload

Author:

    Arthur Bierer (arthurbi) 25-Dec-1995

Revision History:

    Rajeev Dujari (rajeevd)  01-Oct-1996 overhauled

    Adriaan Canter (adriaanc) 01-03-1998 :
    AUTHCTX now a virtual base class, from which derived classes
    inherit to implement the different authentication protocols:

    BASIC_CTX  (Basic auth),
    PLUG_CTX   (NTLM, MSN, DPA)
    DIGEST_CTX (Digest auth, new)


--*/

#include <wininetp.h>
#include <urlmon.h>
#include <splugin.hxx>
#include "auth.h"


//
// constants
//

#define WILDCARD 0x05 // don't use '*' since it can appear in an URL
#define INC_WAIT_INFO 10  // number of records to grow array
#define AssertHaveLock() INET_ASSERT(g_dwOwnerId == GetCurrentThreadId())


struct AUTH_WAIT_INFO
{
    PFN_AUTH_NOTIFY  pfnNotify;
    DWORD_PTR        dwContext;
    PWC *            pPWC;
};

//
//  globals
//

PRIVATE AUTH_WAIT_INFO *g_pWaitList = NULL; // array of notification records
PRIVATE DWORD g_cWaitList = 0;    // size of notification record array
PRIVATE DWORD g_cDlgThreads = 0;  // number of threads in dlg or waiting
PRIVATE BOOL  g_fNotifyInProgress = FALSE;    // Indicates if a notification is in progress.
PRIVATE const char szBasic[] = "basic";

// Global authentication providers list and state.
AUTHCTX::SPMState  AUTHCTX::g_eState;
AUTHCTX::SPMData  *AUTHCTX::g_pSPMList = NULL;


/*
Conceptually, we have a three layer structure for server auth.  The top level
is a list of hosts.  For each host, there's a list of realms.  For each realm,
there's a user/pass and a list of directories to preauthenticate.  Also for
each host, there's a list of directories without realm and a user/pass for each.

In actuality, there's a flat list of primary host/realm directories with a
single user/pass for each.  Additional directories, with or without realm,
are kept in an auxiliary list.
*/

PRIVATE PWC* g_pwcRealm = NULL;  // SLL for primary server/realm user/pass
PRIVATE PWC* g_pwcOther = NULL;  // SLL of additional dirs, with or w/o realm
PRIVATE PWC* g_pwcProxy = NULL;  // SLL of proxy  password cache entries


// Global auth crit sect.
CRITICAL_SECTION g_crstAuth;

#ifdef DBG
DWORD g_dwOwnerId = 0;
LONG g_nLockCount = 0;
#endif


//
// private prototypes
//

//-----------------------------------------------------------------------------
//
//  PWC class definition.
//
//
PRIVATE PWC *PWC_Create
(
    LPSTR lpszHost,
    LPSTR lpszUrl,
    LPSTR lpszRealm,
    AUTHCTX::SPMData* pSPM
);

void  PWC_Free (PWC *pwc);



//-----------------------------------------------------------------------------
//
//  Utilities
//
//

PRIVATE VOID SspiFlush (LPSTR pszDll);
PRIVATE BOOL TemplateMatch(LPSTR lpszTemplate, LPSTR lpszFilename);
PRIVATE LPSTR MakeTemplate (LPSTR docname);
PRIVATE LPSTR GetProxyName (HTTP_REQUEST_HANDLE_OBJECT *pRequest);

PRIVATE BOOL ReadRegKey(
    BYTE * pbRegValue,
    DWORD * pdwNumBytes,
    LPSTR  pszRegKey,
    LPSTR  pszRegValueName,
    DWORD  dwRegTypeExpected);



//-----------------------------------------------------------------------------
//
//
//      PWC functions
//
//          PWC_CREATE
//          PWC_FREE
//          SetUser
//          SetPass
//          FlushPwcList
//

PRIVATE PWC *PWC_Create // PWC constructor
(
    LPSTR lpszHost,     // Host Name to place in structure.
    LPSTR lpszUrl,      // URL to template, and place in the structure.
    LPSTR lpszRealm,    // Realm string to add.
    AUTHCTX::SPMData * pSPM
)
{
    PWC* pwc = (PWC *) ALLOCATE_ZERO_MEMORY(sizeof(*pwc));
    if (!pwc)
        return NULL;

    INET_ASSERT (!pwc->pNext);
    INET_ASSERT (!pwc->nLockCount);
    pwc->lpszHost    = lpszHost?  NewString(lpszHost)   : NULL;
    pwc->lpszUrl     = lpszUrl?   MakeTemplate(lpszUrl) : NULL;
    INET_ASSERT (!pwc->lpszUser);
    INET_ASSERT (!pwc->lpszPass);
    INET_ASSERT (!pwc->fPreAuth);
    pwc->fPreAuth    = FALSE;
    pwc->lpszRealm   = lpszRealm? NewString(lpszRealm)  : NULL;
    pwc->lpszNonce    = NULL;
    pwc->lpszOpaque   = NULL;
    pwc->pSPM         = pSPM;


    if (  (!pwc->lpszHost  && lpszHost)
       || (!pwc->lpszUrl   && lpszUrl)
       || (!pwc->lpszRealm && lpszRealm)
       )
    {
        PWC_Free(pwc);
        return NULL;
    }

    return pwc;
}

PRIVATE VOID PWC_Free(PWC *pwc) // PWC destructor
{
    if ( pwc )
    {
        if (pwc->lpszHost)
            FREE_MEMORY(pwc->lpszHost);
        if (pwc->lpszUrl)
            FREE_MEMORY(pwc->lpszUrl);
        if ( pwc->lpszUser )
            FREE_MEMORY(pwc->lpszUser);
        if ( pwc->lpszPass )
            FREE_MEMORY(pwc->lpszPass);
        if ( pwc->lpszRealm )
            FREE_MEMORY(pwc->lpszRealm);
        if ( pwc->lpszNonce )
            FREE_MEMORY(pwc->lpszNonce);
        if ( pwc->lpszOpaque )
            FREE_MEMORY(pwc->lpszOpaque);
        FREE_MEMORY(pwc);
    }
}

PUBLIC DWORD PWC::SetUser (LPSTR lpszInput)
{
    AssertHaveLock();
    INET_ASSERT (lpszInput);
    if (lpszUser && !lstrcmp (lpszUser, lpszInput))
        return ERROR_SUCCESS; // didn't change
    LPSTR lpszTemp = NewString(lpszInput);
    if (!lpszTemp)
        return ERROR_NOT_ENOUGH_MEMORY;
    if (lpszUser)
        FREE_MEMORY (lpszUser);
    lpszUser = lpszTemp;
    return ERROR_SUCCESS;
}

PUBLIC DWORD PWC::SetPass (LPSTR lpszInput)
{
    AssertHaveLock();
    INET_ASSERT (lpszInput);
    if (lpszPass && !lstrcmp (lpszPass, lpszInput))
        return ERROR_SUCCESS; // didn't change
    LPSTR lpszTemp = NewString(lpszInput);
    if (!lpszTemp)
        return ERROR_NOT_ENOUGH_MEMORY;
    if (lpszPass)
        FREE_MEMORY (lpszPass);
    lpszPass = lpszTemp;
    return ERROR_SUCCESS;
}


/*++
Delete some entries from a singly linked list.
--*/
PRIVATE void FlushPwcList (PWC **ppList)
{
    AssertHaveLock();

    PWC *pwc = *ppList;
    while (pwc)
    {
        PWC *pwcNext = pwc->pNext;

        if (!pwc->nLockCount)
            PWC_Free (pwc);
        else
        {
            *ppList = pwc;
            ppList = &(pwc->pNext);
        }

        pwc = pwcNext;
    }
    *ppList = NULL;
}


//-----------------------------------------------------------------------------
//
//
//      Auth* functions
//
//          AuthOpen
//          AuthClose
//          AuthLock
//          AuthUnlock
//          AuthInDialog
//          AuthNotify
//          AuthFlush
//
//

BOOL bAuthInitialized = FALSE;

void AuthOpen (void)
{
    INET_ASSERT (!g_dwOwnerId && !g_nLockCount);
    InitializeCriticalSection (&g_crstAuth);
    bAuthInitialized = TRUE;
}

void AuthClose (void)
{
    if (bAuthInitialized)
    {
        DeleteCriticalSection (&g_crstAuth);
        bAuthInitialized = FALSE;
    }
    INET_ASSERT (!g_dwOwnerId && !g_nLockCount);
}

void AuthLock (void)
{
    INET_ASSERT(bAuthInitialized);

    EnterCriticalSection (&g_crstAuth);
    DEBUG_ONLY (if (!g_nLockCount++) g_dwOwnerId = GetCurrentThreadId();)
}

void AuthUnlock (void)
{
    INET_ASSERT(bAuthInitialized);
    INET_ASSERT (g_nLockCount > 0);
    DEBUG_ONLY (if (!--g_nLockCount) g_dwOwnerId = 0;)
    LeaveCriticalSection (&g_crstAuth);
}


//
// DIALOG SERIALIZATION
//   AuthInDialog
//   AuthNotify
//

/*++
Description: Checks if a authentication dialog is progress.  If so,
records info to notify waiting threads when the dialog is dismissed.

Return Value: TRUE if a dialog is already in progress, FALSE if not
--*/

PUBLIC BOOL AuthInDialog
    (IN AUTHCTX *pAuthCtx, INTERNET_AUTH_NOTIFY_DATA *pNotify, BOOL * pfMustLock)
{
    INET_ASSERT(!*pfMustLock);

    BOOL fRet = FALSE;

    // Serialize access to globals.
    AuthLock();

    // NOTE - This function can be called reentrantly from AuthNotify.

    // Increment dialog request count.
    g_cDlgThreads++;

    // Check if any dialogs are in progress
    // If only one request is outstanding, service it.
    if (g_cDlgThreads == 1)
        goto done;

    // Otherwise more than one request outstanding.



    // It is possible that AuthInDialog is being called reentrantly by
    // AuthNotify. In this case we do not enqueue this request but
    // service it immediately. We release the lock so that subsequent
    // requests may be serviced and flag that the lock must be re-
    // acquired after the dialog has been dismissed so that AuthNotify
    // is not re-entered and the lock state will be correct (acquired)
    // when control is returned to AuthNotify.
    if (g_fNotifyInProgress)
    {
        // Decrement count since this request
        // will not be enqueued.
        g_cDlgThreads--;

        // Lock must be reacquired after
        // dialog is dismissed.
        *pfMustLock = TRUE;

        // We're about to unlock and service
        // this request. Any other threads
        // calling AuthInDialog will not be
        // reentrant.
        g_fNotifyInProgress = FALSE;

        // Unlock once here and once before
        // exiting function - lock is released.
        AuthUnlock();

        INET_ASSERT(g_nLockCount == 1);

        goto done;
    }

    // This is not a re-entrant call. Enqueue the
    // request.

    // Calculate the next open slot in the array.
    DWORD iWaitList;
    iWaitList = g_cDlgThreads - 2;

    // Grow the array of necessary.

    // NOTE - if a dialog is displayed as a result of a reentrant
    // call by AuthNotify then additional notifications will be
    // placed at the tail. In the case of a fixed number of requests
    // the worst case is that the last notification in the
    // list caused reentrancy and the previous (n-1) notified
    // requests are retried and subsequently queued. The leads to
    // n^2 behavior (n + (n-1) + (n-2) + ...). In the case of
    // additional incoming requests a rare case (auth UI continually
    // invoked, some requests reentrant and g_cDlgThreads never reset
    // to 0) would be that the list grows without bound.
    if (iWaitList >= g_cWaitList)
    {
        HLOCAL hTemp;

        g_cWaitList += INC_WAIT_INFO;

        if (g_cWaitList == INC_WAIT_INFO)
        {
            hTemp = ALLOCATE_FIXED_MEMORY (g_cWaitList * sizeof(*g_pWaitList));
        }
        else
        {
            hTemp = REALLOCATE_MEMORY
                (g_pWaitList, g_cWaitList * sizeof(*g_pWaitList), LMEM_MOVEABLE);
        }
        if (!hTemp)
        {
            INET_ASSERT (!fRet);
            goto done;
        }

        g_pWaitList = (AUTH_WAIT_INFO *) hTemp;
     }

     // Insert the new info at the end of the array.
     g_pWaitList[iWaitList].pfnNotify = pNotify->pfnNotify;
     g_pWaitList[iWaitList].dwContext = pNotify->dwContext;
     g_pWaitList[iWaitList].pPWC      = pAuthCtx->_pPWC;

     pAuthCtx->_pPWC->nLockCount++;

     fRet = TRUE;

done:

    AuthUnlock();
    return fRet;
}

/*++
Description: Notifies all waiting threads that a dialog has been dismissed.
--*/

PUBLIC void AuthNotify (PWC *pwc, DWORD dwErr)
{
    // Serialize access to globals.
    AuthLock();

    if (g_pWaitList)
    {
        INET_ASSERT (g_cDlgThreads >= 2);

        // Loop through the array of notifications.
        AUTH_WAIT_INFO *pWaitItem;

        for (DWORD i = 0; i < g_cDlgThreads - 1; i++)
        {
            // NOTE - Because AuthInDlg can be called reentrantly
            // it is possible that g_pWaitList is reallocated. It is
            // important that pWaitItem is always indexed directly from
            // the global array pointer.
            pWaitItem = g_pWaitList + i;

            // If the thread was waiting for the same PWC, either resend or cancel.
            // Otherwise tell the client to retry InternetErrorDlg.
            INET_ASSERT ((dwErr == ERROR_CANCELLED)
                || (dwErr == ERROR_INTERNET_FORCE_RETRY));
            DWORD dwAction = (pWaitItem->pPWC == pwc)?
                dwErr : ERROR_INTERNET_RETRY_DIALOG;

            pWaitItem->pPWC->nLockCount--;
            DEBUG_ONLY (pWaitItem->pPWC = NULL);

            // Flag that a notification is in progress in case
            // it calls AuthNotify reentrantly. This must be
            // set each time since a reentrant call will clear
            // the flag.
            g_fNotifyInProgress = TRUE;

            (*pWaitItem->pfnNotify) (pWaitItem->dwContext, dwAction, NULL);

        }

        // Notification no longer in progress.
        g_fNotifyInProgress = FALSE;

        // Free the notification array.
        FREE_MEMORY (g_pWaitList);
        g_pWaitList = NULL;
        g_cWaitList = 0;
    }

    g_cDlgThreads = 0;

    AuthUnlock();
}

/*++
Flush any server and proxy password cache entries not in use.
--*/
PUBLIC void AuthFlush (void)
{
    // Serialize access to globals.
    AuthLock();

    FlushPwcList (&g_pwcRealm);
    FlushPwcList (&g_pwcOther);
    FlushPwcList (&g_pwcProxy);

    TCHAR  pszRegValue[1024];
    DWORD  dwBufLen=sizeof(pszRegValue);

    // Read SecurityProviders regkey.

    if (ReadRegKey((BYTE*) &pszRegValue,//Buffer to store value
                    &dwBufLen, // Length of above buffer
                    TEXT("SYSTEM\\CurrentControlSet\\Control\\SecurityProviders"), // Reg Key name
                    TEXT("SecurityProviders"), // Value name
                    REG_SZ) ) // Type expected
    {
        CharUpper(pszRegValue);

        static LPSTR rszDll[] =
            {"MSNSSPC.DLL", "MSNSSPS.DLL", "MSAPSSPC.DLL", "MSAPSSPS.DLL"};

        // Flush any msn/dpa credential cache.
        for (DWORD i=0; i<ARRAY_ELEMENTS(rszDll); i++)
        {
            if (StrStr (pszRegValue, rszDll[i]))
                SspiFlush (rszDll[i]);
        }
    }

    if (!g_cSspiContexts)
        AUTHCTX::UnloadAll();

    DIGEST_CTX::FlushCreds();

    AuthUnlock();
}


PUBLIC void AuthUnload (void)
/*++
Routine Description:
    Frees all Cached URLs, and unloads any loaded DLL authentication modeles.

--*/
{
    if (bAuthInitialized) {
        AuthFlush();
        AuthLock();
        AUTHCTX::UnloadAll();
        AuthUnlock();
    }
}


//-----------------------------------------------------------------------------
//
//
//      Utility Functions:
//
//          SspiFlush
//          TemplateMatch
//          MakeTemplate
//          GetProxyName
//          ReadRegKey
//          TrimQuotes
//          TrimWhiteSpace
//          GetDelimitedToken
//          GetKeyValuePair
//
//


typedef BOOL (WINAPI * SSPI_FLUSH) (VOID) ;

void SspiFlush (LPSTR pszDll)
{
    __try
    {
        HINSTANCE hinst = GetModuleHandle (pszDll);

        if (hinst)
        {
            SSPI_FLUSH pfnFlush = (SSPI_FLUSH)
                GetProcAddress (hinst, "CleanupCredentialCache");

            if (pfnFlush)
            {
                (*pfnFlush) ();
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        INET_ASSERT (FALSE);
    }
    ENDEXCEPT
}



PRIVATE BOOL TemplateMatch(LPSTR lpszTemplate, LPSTR lpszFilename)

/*++

Routine Description:

    Attempts to match a template URL string with a URL ( FileName )

Arguments:

    lpszTemplate             - Template to match against.
    lpszFilename             - URL to match with the template

Return Value:

    BOOL
    Success - TRUE - match
    Failure - FALSE - no match

Comments:

    Note: This Legacy code from the SpyGlass IE 1.0 browser

--*/

{
    /* code for this routine cloned from HTAA_templateMatch() */

    CHAR *p = lpszTemplate;
    CHAR *q = lpszFilename;
    int m;

    if (!lpszTemplate || !lpszFilename)
        return 0;

    for (; *p && *q && *p == *q; p++, q++)  /* Find first mismatch */
        ;                                                                       /* do nothing else */

    if (!*p && !*q)
        return 1;                                                       /* Equally long equal strings */
    else if (WILDCARD == *p)
    {                                                                               /* Wildcard */
        p++;                                                            /* Skip wildcard character */
        m = strlen(q) - strlen(p);                      /* Amount to match to wildcard */
        if (m < 0)
            return 0;                                               /* No match, filename too short */
        else
        {                                                                       /* Skip the matched characters and compare */
        if (lstrcmp(p, q + m))
                return 0;                                       /* Tail mismatch */
            else
                return 1;                                       /* Tail match */
        }
    }                                                                               /* if wildcard */
    else
        return 0;                                                       /* Length or character mismatch */
}


PRIVATE LPSTR MakeTemplate (LPSTR docname)

/*++
Routine Description:
    Makes a Template String (from a URL) that can later be used to match a range of URLs.

Arguments:
    ppszTemplate             - pointer to pointer of where Template can be stored
    docname                  - URL to create a template with.

Return Value: BOOL
    Success - TRUE - created
    Failure - FALSE - error

Comments:
    Note: This Legacy code from the SpyGlass IE 1.0 browser
--*/

{
    CHAR *pszTemplate = NULL;
    unsigned long k;
    k = 0;

    if (docname)
    {
        CHAR *slash;
        CHAR *first_slash;

        //
        // Ignore everything after first reserved character.
        //

        BYTE chSave = 0;
        LPSTR lpszScan = docname;
        while (*lpszScan)
        {
            if (*lpszScan == '?' || *lpszScan == ';')
            {
                chSave = *lpszScan;
                *lpszScan = 0;
                break;
            }
            lpszScan++;
        }

        slash = strrchr(docname, '/');

        //
        // If there is a "//" and no other slashes,
        //  then make sure not to chop the hostname off
        //  the URL. ex: http://www.netscape.com
        //  should be //www.netscape.com* not //*
        //

        if (slash)
        {
            first_slash = strchr(docname, '/' );
            if ((first_slash+1) == slash)
                k = lstrlen(docname);
            else
                k = (unsigned long)(slash-docname)+1;
        }

        // Restore any reserved character (or rewrite terminating null)
        *lpszScan = chSave;
    }

    pszTemplate = (CHAR *) ALLOCATE_FIXED_MEMORY(k+2);
    if (!pszTemplate)
        return 0;

    memcpy(pszTemplate, docname, k);
    pszTemplate[k]= WILDCARD;
    pszTemplate[k+1]=0;

    DEBUG_PRINT(HTTP, INFO, ("MakeTemplate: made template [%s] from [%s]\n",
        pszTemplate, docname ));

    return pszTemplate;
}

LPSTR GetProxyName (HTTP_REQUEST_HANDLE_OBJECT *pRequest)
{
    // Get the proxy name.
    LPSTR lpszProxy;
    DWORD cbProxy;
    INTERNET_PORT port;

   pRequest->GetProxyName(
                &lpszProxy,
                &cbProxy,
                &port);

   return lpszProxy;
}


//++------------------------------------------------------------------------
//
//   Function: ReadRegKey
//
//   Synopsis: This function reads a registry key.
//
//   Arguments:
//
//   Returns:   TRUE     no error
//                 FALSE    a fatal error happened
//
//   History:      AshishS    Created     5/22/96
//------------------------------------------------------------------------

BOOL ReadRegKey(
    BYTE * pbRegValue, // The value of the reg key will be
                 // stored here
    DWORD * pdwNumBytes, // Pointer to DWORD conataining
     // the number of bytes in the above buffer - will be
     // set to actual bytes stored.
    LPSTR  pszRegKey, // Reg Key to be opened
    LPSTR  pszRegValueName, // Reg Value to query
    DWORD  dwRegTypeExpected) // Expected type of Value
{
    HKEY   hRegKey;
    DWORD  dwRegType;
    LONG lResult;

     //read registry to find out name of the file
    if ( (lResult = REGOPENKEYEX(HKEY_LOCAL_MACHINE,
                                 pszRegKey, // address of subkey name
                                 0,          // reserved
                                 KEY_READ,   // samDesired
                                 &hRegKey
                                  // address of handle of open key
        )) != ERROR_SUCCESS )
    {
        goto cleanup;
    }


    if ( (lResult =RegQueryValueEx( hRegKey,
                                    pszRegValueName,
                                    0,           // reserved
                                    &dwRegType,// address of buffer
                                     // for value type
                                    pbRegValue,
                                    pdwNumBytes)) != ERROR_SUCCESS )
    {
        REGCLOSEKEY(hRegKey);
        goto cleanup;
    }

    REGCLOSEKEY(hRegKey);

    if ( dwRegType != dwRegTypeExpected )
    {
        goto cleanup;
    }

    return TRUE;

cleanup:

    return FALSE;

}


/*-----------------------------------------------------------------------------
Inplace trim of one leading and one trailing quote.
-----------------------------------------------------------------------------*/
VOID TrimQuotes(LPSTR *psz, LPDWORD pcb)
{
    if (*pcb && (**psz == '"'))
    {
        (*psz)++;
        (*pcb)--;
    }
    if (*pcb && (*(*psz + *pcb - 1) == '"'))
        (*pcb)--;
}

/*-----------------------------------------------------------------------------
Inplace trim of leading and trailing whitespace.
-----------------------------------------------------------------------------*/
VOID TrimWhiteSpace(LPSTR *psz, LPDWORD pcb)
{
    DWORD cb = *pcb;
    CHAR* beg = *psz;
    CHAR* end = beg + cb - 1;

    while ((cb != 0) && ((*beg == ' ') || (*beg == '\t')))
    {
        beg++;
        cb--;
    }

    while ((cb != 0) && ((*end == ' ') || (*end == '\t')))
    {
        end--;
        cb--;
    }

    *psz = beg;
    *pcb = cb;
}

/*-----------------------------------------------------------------------------
Inplace strtok based on one delimiter. Ignores delimiter scoped by quotes.
-----------------------------------------------------------------------------*/
BOOL GetDelimitedToken(LPSTR* pszBuf,   LPDWORD pcbBuf,
                       LPSTR* pszTok,   LPDWORD pcbTok,
                       CHAR   cDelim)
{
    CHAR *pEnd;
    BOOL fQuote = FALSE,
         fRet   = FALSE;

    *pcbTok = 0;
    *pszTok = *pszBuf;
    pEnd = *pszBuf + *pcbBuf - 1;

    while (*pcbBuf)
    {
        if ( ((**pszBuf == cDelim) && !fQuote)
            || (*pszBuf == pEnd)
            || (**pszBuf =='\r')
            || (**pszBuf =='\n'))
        {
            fRet = TRUE;
            break;
        }

        if (**pszBuf == '"')
            fQuote = !fQuote;

        (*pszBuf)++;
        (*pcbBuf)--;
    }

    if (fRet)
    {
        *pcbBuf = (DWORD) (pEnd - *pszBuf);
        if (**pszBuf == cDelim)
        {
            *pcbTok = (DWORD)(*pszBuf - *pszTok);
            (*pszBuf)++;
        }
        else
            *pcbTok = (DWORD) (*pszBuf - *pszTok) + 1;
    }

    return fRet;
}


/*-----------------------------------------------------------------------------
Inplace retrieval of key and value from a buffer of form key = <">value<">
-----------------------------------------------------------------------------*/
BOOL GetKeyValuePair(LPSTR  szB,    DWORD cbB,
                     LPSTR* pszK,   LPDWORD pcbK,
                     LPSTR* pszV,   LPDWORD pcbV)
{
    if (GetDelimitedToken(&szB, &cbB, pszK, pcbK, '='))
    {
        TrimWhiteSpace(pszK, pcbK);

        if (cbB)
        {
            *pszV = szB;
            *pcbV = cbB;
            TrimWhiteSpace(pszV, pcbV);
        }
        else
        {
            *pszV = NULL;
            *pcbV = 0;
        }
        return TRUE;
    }

    else
    {
        *pszK  = *pszV  = NULL;
        *pcbK  = *pcbV = 0;
    }
    return FALSE;
}


//-----------------------------------------------------------------------------
//
//
//      Main authentication functions:
//
//          AddAuthorizationHeader
//          AuthOnRequest
//          ProcessResponseHeader
//          AuthOnRequest
//
//



/*-----------------------------------------------------------------------------
-----------------------------------------------------------------------------*/
PRIVATE void AddAuthorizationHeader
(
    HTTP_REQUEST_HANDLE_OBJECT *pRequest,
    AUTHCTX* pAuthCtx
)
{
    if (!pAuthCtx)
        return;

    INET_ASSERT(pAuthCtx->_pSPMData);

    AssertHaveLock();

    // Call the auth package.
    // BUGBUG - fixed size outgoing header
    CHAR szHeader[10000];
    LPSTR pBuf;
    DWORD cbBuf;
    DWORD dwPlugInError;

    // Prefix with the appropriate header.

    if (pAuthCtx->_fIsProxy)
    {
        memcpy (szHeader, HTTP_PROXY_AUTHORIZATION_SZ, HTTP_PROXY_AUTHORIZATION_LEN);
        pBuf = szHeader + HTTP_PROXY_AUTHORIZATION_LEN;

        // Don't reuse this keep-alive socket after a password cache flush.
        pRequest->SetAuthorized();
    }
    else
    {
        memcpy (szHeader, HTTP_AUTHORIZATION_SZ, HTTP_AUTHORIZATION_LEN);
        pBuf = szHeader + HTTP_AUTHORIZATION_LEN;

        // Don't reuse a keep-alive socket after a password cache flush.
        if (!pRequest->IsViaProxy())
            pRequest->SetAuthorized();

        // This item should be marked per user in the cache.
        pRequest->SetPerUserItem(TRUE);
    }

    *pBuf++ = ' ';
    cbBuf = sizeof(szHeader) - HTTP_PROXY_AUTHORIZATION_LEN - 2;
    INET_ASSERT (HTTP_PROXY_AUTHORIZATION_LEN >= HTTP_AUTHORIZATION_LEN);


    dwPlugInError =
        pAuthCtx->PreAuthUser(pBuf, &cbBuf);

    //  If the plug in did not fail, add its header to the outgoing header list
    if (dwPlugInError == ERROR_SUCCESS && pAuthCtx->GetState() != AUTHCTX::STATE_ERROR)
    {
        // Append CR-LF.
        pBuf += cbBuf;
        *pBuf++ = '\r';
        *pBuf++ = '\n';
        *pBuf = 0;
        cbBuf = (DWORD) (pBuf - szHeader);

        // Add or replace the (proxy-)authorization header.
        wHttpAddRequestHeaders (pRequest, szHeader, cbBuf,
            HTTP_ADDREQ_FLAG_ADD | HTTP_ADDREQ_FLAG_REPLACE);

    }

}


/*-----------------------------------------------------------------------------
-----------------------------------------------------------------------------*/
PUBLIC DWORD AuthOnRequest (IN HINTERNET hRequestMapped)
{
    DEBUG_ENTER ((DBG_HTTP, Dword, "AuthOnRequest", "%#x", hRequestMapped));

    DWORD dwError = ERROR_SUCCESS;
    LPSTR lpszUser, lpszPass;
    
    // Get username, password, url, and auth context from request handle.
    HTTP_REQUEST_HANDLE_OBJECT *pRequest =
        (HTTP_REQUEST_HANDLE_OBJECT *) hRequestMapped;

    LPSTR lpszUrl = pRequest->GetURL();
    LPSTR lpszHost= pRequest->GetServerName();

    AUTHCTX *pAuthCtx = pRequest->GetAuthCtx();
    PWC *pwc;
    BOOL fCredsChanged = FALSE;
    DWORD dwAuthState = AUTHSTATE_NONE;
    
    // Serialize access to globals.
    AuthLock();

    // PROXY AUTHENTICATION
    //
    // CERN proxies should remove proxy-authorization headers before forwarding
    // requests to servers.  Otherwise, don't add proxy authorization headers
    // that would be seen by servers on a direct connect or via SSL tunneling.

    // How the credentials (username + password) are retrieved from handles during
    // authentication
    //
    //             Connect      Request
    //  Server        4     <-     3
    //  Proxy         2     <-     1
    //

    if  (pRequest->IsRequestUsingProxy()
      && !pRequest->IsTalkingToSecureServerViaProxy())
    {
        if (pAuthCtx && pAuthCtx->_fIsProxy)
        {
            // We have a proxy authentication in progress.
            // If a user/pass set on handle, transfer to PWC.

            // First check for proxy credentials and fallback to server
            // for legacy wininet apps. This will invalidate the credentials
            // on the handle they were found for any subsequent calls to
            // GetUserAndPass.
            if (pRequest->GetUserAndPass(IS_PROXY, &lpszUser, &lpszPass)
                || (!GlobalIsProcessExplorer && pRequest->GetUserAndPass(IS_SERVER, &lpszUser, &lpszPass)))
            {
                pAuthCtx->_pPWC->SetUser(lpszUser);
                pAuthCtx->_pPWC->SetPass(lpszPass);
            }

            // Add the authorization header.
            AddAuthorizationHeader (pRequest, pAuthCtx);
        }

        // NO AUTHENTICATION CONTEXT -> SEE IF A PWC EXISTS.
        else  // See if we have a cached proxy user/pass.
        {
            PWC *pPWC;
            LPSTR lpszProxy = GetProxyName (pRequest);

            if (lpszProxy)
            {
                pPWC = AUTHCTX::SearchPwcList (g_pwcProxy, lpszProxy, NULL, NULL, NULL);
                if (pPWC)
                {
                    // If proxy credentials have been set on the handle, set them
                    // in the password cache entry.
                    // BUGBUG - we don't fallback to server credentials on the handle -
                    // this would invalidate them for a subsequent server auth.
                    if (pRequest->GetUserAndPass(IS_PROXY, &lpszUser, &lpszPass))
                    {
                        // Check to see if handle credentials differ from pw cache.
                        // Flush authorized sockets since we don't want to pick these up.
                        if (lstrcmpi(lpszUser, pPWC->GetUser())
                            || lstrcmp(lpszPass, pPWC->GetPass()))
                        {
                            PurgeKeepAlives(PKA_AUTH_FAILED);
                        }

                        pPWC->SetUser(lpszUser);
                        pPWC->SetPass(lpszPass);
                    }
                    // This should always create a ctx
                    pAuthCtx = AUTHCTX::CreateAuthCtx(pRequest, IS_PROXY, pPWC);
                    INET_ASSERT(pAuthCtx);
                    if (pAuthCtx->GetSchemeType() == AUTHCTX::SCHEME_BASIC)
                        AddAuthorizationHeader (pRequest, pAuthCtx);
                    delete pAuthCtx;
                    pAuthCtx = NULL;
                }
            }
        }
    }

    // SERVER AUTHENTICATION
    //
    // Don't send server authorization when initiating SSL tunneling with proxy.
    pAuthCtx = pRequest->GetAuthCtx();
    if (!pRequest->IsTunnel())
    {
        // See if we have a server authentication in progress
        if (pAuthCtx && !pAuthCtx->_fIsProxy)
        {
            // Server authentication in progress.

            // If a user/pass set on handle, transfer to PWC.
            // This will invalidate the credentials on the handle they
            // were found for any subsequent calls to GetUserAndPass.
            if (pRequest->GetUserAndPass(IS_SERVER, &lpszUser, &lpszPass))
            {
                pAuthCtx->_pPWC->SetUser(lpszUser);
                pAuthCtx->_pPWC->SetPass(lpszPass);
            }

            // Add the authorization header.
            AddAuthorizationHeader (pRequest, pAuthCtx);
        }
        else  // See if we have a cached server user/pass.
        {
            // NO AUTHENTICATION CONTEXT -> SEE IF A PWC EXISTS
            // First search the primary host/realm list.
            PWC *pPWC;
            pPWC = AUTHCTX::SearchPwcList (g_pwcRealm, lpszHost, lpszUrl, NULL, NULL);
            if (!pPWC)
            {
                // Then search the secondary directory and no-realm list.
                pPWC = AUTHCTX::SearchPwcList (g_pwcOther, lpszHost, lpszUrl, NULL, NULL);
                if (pPWC && pPWC->lpszRealm)
                {
                    // There's a realm, so this entry is merely to indicate the
                    // directory qualifies for preauthentication...
                    INET_ASSERT (!pPWC->lpszUser && !pPWC->lpszPass);
                    INET_ASSERT (!pPWC->nLockCount && !pPWC->fPreAuth);

                    // but the user/pass should come from the primary entry...
                    pPWC = AUTHCTX::SearchPwcList
                        (g_pwcRealm, lpszHost, NULL, pPWC->lpszRealm, pPWC->pSPM);

                    // ...which should exist.  The only reason it wouldn't is
                    // a password cache flush, but that would also wipe out any
                    // secondary entries with realm, which are never locked.
                    INET_ASSERT (pPWC);
                }
            }

            // No existing auth context. If we found a password cache
            // which allows pre-auth, go ahead and pre-authenticate.
            if (pPWC && pPWC->fPreAuth)
            {
                BOOL fPreauth = FALSE;
                AUTHCTX *pNewAuthCtx = NULL;

                // Basic pre-auth calls AddAuthorizationHeader with a NULL auth context.
                // For NTLM, we create a new auth context, set it in the handle and set handle
                // auth state to AUTHSTATE_NEGOTIATE. This will send out negotiate auth headers
                // on the  first request. NOTE - at this point we don't know if the handle will
                // pick up a pre-authenticated socket. If so, the auth header is still sent out :
                // In this case IIS will recognize that the header (which contain the username
                // in the negotiate header) is identical to that cached on the socket and will
                // not force re-authentication.

                // If server credentials have been set on the handle, set them
                // in the password cache entry.
                if (pRequest->GetUserAndPass(IS_SERVER, &lpszUser, &lpszPass))
                {
                    // Check to see if handle credentials differ from pw cache.
                    // Flush authorized sockets since we don't want to pick these up.
                    if (lstrcmpi(lpszUser, pPWC->GetUser())
                        || lstrcmp(lpszPass, pPWC->GetPass()))
                    {
                        fCredsChanged = TRUE;
                        PurgeKeepAlives(PKA_AUTH_FAILED);
                    }

                    pPWC->SetUser(lpszUser);
                    pPWC->SetPass(lpszPass);
                }

                // This should always create a ctx
                dwAuthState = pRequest->GetAuthState();
                pNewAuthCtx = AUTHCTX::CreateAuthCtx(pRequest, IS_SERVER, pPWC);
                INET_ASSERT(pNewAuthCtx);
                if (pNewAuthCtx->GetSchemeType() == AUTHCTX::SCHEME_NTLM
                    || pNewAuthCtx->GetSchemeType() == AUTHCTX::SCHEME_NEGOTIATE)
                {
                    // NTLM or Negotiate (in which case we don't really know the
                    // protocol yet) - create the auth context, set it in the handle
                    // and set state to AUTHSTATE_NEGOTIATE. Handle now has
                    // a valid auth context and is in the correct auth state
                    // for the remainder of the authorization sequence.

                    // It's possible that the pwc entry was created when no proxy
                    // was in use and the user set a proxy. Check that this is
                    // not the case.
                    if (!pRequest->IsMethodBody() 
                        || (pNewAuthCtx->GetSchemeType() == AUTHCTX::SCHEME_NTLM
                            && pRequest->IsDisableNTLMPreauth())
                        || (pRequest->IsRequestUsingProxy()
                            && !pRequest->IsTalkingToSecureServerViaProxy()))
                    {
                        // NTLM preauth disabled or over proxy; no preauth.
                        delete pNewAuthCtx;
                        pNewAuthCtx = NULL;
                    }
                    else
                    {
                        // Set the auth context in the handle and
                        // add the auth header.
                        pRequest->SetAuthCtx(pNewAuthCtx);
                        pRequest->SetAuthState(AUTHSTATE_NEGOTIATE);
                        AddAuthorizationHeader (pRequest, pNewAuthCtx);
                        fPreauth = TRUE;
                    }
                }
                else
                {
                    // For Basic and Digest add the header but do not set the
                    // context in the handle since we will delete it next.
                    // In this case we record that we have pre-authenticated which
                    // is necessary state info if the server challenges with a 401
                    // and we are forced to re-authenticate.
                    if ((pNewAuthCtx->GetSchemeType() == AUTHCTX::SCHEME_DIGEST && !fCredsChanged)
                        || (pNewAuthCtx->GetSchemeType() == AUTHCTX::SCHEME_BASIC))
                    {
                        AddAuthorizationHeader (pRequest, pNewAuthCtx);
                        pRequest->SetPWC(pNewAuthCtx->_pPWC);
                        fPreauth = TRUE;
                    }
                }

                if (fPreauth)
                {
                    // Bug # 57742 (Front Page)
                    // Retrieve the credentials to be used for preauthentication,
                    // set them on the handle and mark them invalid by performing
                    // a get. This ensures apps have access to the preauth creds
                    // and that they will not be used internally by wininet.
                    if (pNewAuthCtx->_pPWC->lpszUser && pNewAuthCtx->_pPWC->lpszPass)
                    {
                        LPSTR pszUser, pszPass;
                        pRequest->SetUserOrPass(pNewAuthCtx->_pPWC->lpszUser, IS_USER, IS_SERVER);
                        pRequest->SetUserOrPass(pNewAuthCtx->_pPWC->lpszPass, IS_PASS, IS_SERVER);
                        pRequest->GetUserAndPass(IS_SERVER, &pszUser, &pszPass);
                    }
                    
                    // Proceed to delete the context if basic or digest.
                    if (pNewAuthCtx->GetSchemeType() == AUTHCTX::SCHEME_DIGEST
                        || pNewAuthCtx->GetSchemeType() == AUTHCTX::SCHEME_BASIC)
                    {
                        delete pNewAuthCtx;
                        pNewAuthCtx = NULL;
                    }
                }
                // BUGBUG - For QFE, prevent the auth state from being nulled
                //          via the CreateAuthCtx for DPA only.  It turns out
                //          that DPA server connections through an NTLM authenticated
                //          proxy can end up in a bad auth state that results in
                //          unnecessary proxy auth dialogs during a 407->401->407
                //          response sequence.
                //
                //          This needs to be looked at more closely in the future because
                //          the creation/deletion of auth contexts is altering the
                //          auth state for a request, leaving us vulnerable to this
                //          type of issue.
                if (pNewAuthCtx && pNewAuthCtx->GetSchemeType() == AUTHCTX::SCHEME_DPA)
                    pRequest->SetAuthState(dwAuthState);
            }
        }
    }

    AuthUnlock();
    DEBUG_LEAVE(dwError);
    return dwError;
}


/*-----------------------------------------------------------------------------
-----------------------------------------------------------------------------*/
DWORD ProcessResponseHeaders
(
    HINTERNET hRequestMapped,
    BOOL fIsProxy
)
{
    DEBUG_ENTER ((DBG_HTTP, Dword, "ProcessResponseHeaders", "%#x", hRequestMapped));

    // Serialize access to globals.
    AuthLock();

    DWORD dwError;

    // Get context from request handle.
    HTTP_REQUEST_HANDLE_OBJECT *pRequest
        = (HTTP_REQUEST_HANDLE_OBJECT *) hRequestMapped;
    AUTHCTX* pAuthCtx = pRequest->GetAuthCtx();

    if (pAuthCtx)
    {
        if ((dwError = pAuthCtx->UpdateFromHeaders(pRequest, fIsProxy))
            != ERROR_SUCCESS)
        {
            // Delete the auth context and fail auth 
            // immediately if any other error than
            // scheme has been changed.
            delete pAuthCtx;
            pRequest->SetAuthCtx(NULL);
            if (dwError != ERROR_HTTP_HEADER_NOT_FOUND)
                goto quit;

            // Attempt to create a new auth context using
            // the challenge received from the server.
            // If this fails, we follow logic as commented
            // below.
            pAuthCtx = AUTHCTX::CreateAuthCtx(pRequest, fIsProxy);        
            if (!pAuthCtx)
            {
                dwError = ERROR_SUCCESS;
                goto quit;
            }
        }
    }
    else
    {
        // CreateAuthCtx returns NULL if auth scheme not
        // supported (fall through from HttpFiltOnResponse
        // in sendreq.cxx) or if scheme is NTLM and the
        // socket is not keep-alive or via proxy.
        // In these cases it is necessary to check for a NULL
        // return value. The correct return code for these cases is
        // ERROR_SUCCESS, which will be returned by AuthOnResponse.
        pAuthCtx = AUTHCTX::CreateAuthCtx(pRequest, fIsProxy);
        if (!pAuthCtx)
        {
            dwError = ERROR_SUCCESS;
            goto quit;
        }
    }


    LPSTR lpszUser, lpszPass;

    // First check for proxy credentials and fallback to server
    // for legacy wininet apps. This will invalidate the credentials
    // on the handle they were found for any subsequent calls to
    // GetUserAndPass.

    // I believe we should be putting the credentials in the
    // password cache at this time. The scenario is that a client
    // sets credentials on a handle, after a successful authentication
    // the pwc will have null credentials. Pre-auth will then pull up
    // credentials for the default user!!!!!!!!!!!
    if ((!pRequest->GetUserAndPass(fIsProxy, &lpszUser, &lpszPass)) && fIsProxy && !GlobalIsProcessExplorer)
    {
        pRequest->GetUserAndPass(IS_SERVER, &lpszUser, &lpszPass);
    }

    // If we retrieved credentials from the handle set
    // them in the pwc.
    if (lpszUser && lpszPass)
    {
       pAuthCtx->_pPWC->SetUser(lpszUser);
       pAuthCtx->_pPWC->SetPass(lpszPass);
    }

    // Post authenticate user.
    dwError = pAuthCtx->PostAuthUser();

    // Bug # 57742 (Front Page) - Set credentials used for any
    // authentication and invalidate them by performing a get.
    if ((pAuthCtx->GetSchemeType() == AUTHCTX::SCHEME_BASIC)
        && pAuthCtx->_pPWC->lpszUser && pAuthCtx->_pPWC->lpszPass)
    {
        LPSTR pszUser, pszPass;
        pRequest->SetUserOrPass(pAuthCtx->_pPWC->lpszUser, IS_USER, IS_SERVER);
        pRequest->SetUserOrPass(pAuthCtx->_pPWC->lpszPass, IS_PASS, IS_SERVER);
        pRequest->GetUserAndPass(IS_SERVER, &pszUser, &pszPass);
    }

    // Map all unexpected error codes to login failure.
    if (dwError != ERROR_INTERNET_FORCE_RETRY
        && dwError != ERROR_INTERNET_INCORRECT_PASSWORD
        && dwError != ERROR_INTERNET_LOGIN_FAILURE_DISPLAY_ENTITY_BODY)
    {
        dwError = ERROR_INTERNET_LOGIN_FAILURE;
    }

    pRequest->SetAuthCtx(pAuthCtx);

quit:
    AuthUnlock();
    return dwError;
}

/*-----------------------------------------------------------------------------

Routine Description:

    Validates, and Caches Authentication Request headers if needed. If a URL matches a
    cached set of templates it is assumed to require specific authentication information.

Arguments:

    hRequest                - An open HTTP request handle
                              where headers will be added if needed.

Return Value:

    DWORD
    Success - ERROR_SUCCESS

    Failure - One of Several Error codes defined in winerror.h or wininet.w

Comments:

    Need to handle mutiple authentication pages.

-----------------------------------------------------------------------------*/
PUBLIC DWORD AuthOnResponse (HINTERNET hRequestMapped)
{
    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "AuthOnResponse",
                 "%#x [%#x]",
                 hRequestMapped,
                 ((INTERNET_HANDLE_OBJECT *)hRequestMapped)->GetPseudoHandle()
                 ));

    // Get URL and password cache entry from request handle.
    HTTP_REQUEST_HANDLE_OBJECT *pRequest =
        (HTTP_REQUEST_HANDLE_OBJECT *) hRequestMapped;

    AUTHCTX *pAuthCtx = pRequest->GetAuthCtx();
    DWORD dwStatus = pRequest->GetStatusCode();

    if (pAuthCtx)
    {
        if (pAuthCtx->_fIsProxy && dwStatus != HTTP_STATUS_PROXY_AUTH_REQ)
        {
            // We are done with proxy authentication.
            pAuthCtx->_pPWC->fPreAuth = TRUE;
            delete pAuthCtx;
            pRequest->SetAuthCtx (NULL);
        }
        else if (!pAuthCtx->_fIsProxy && dwStatus != HTTP_STATUS_DENIED)
        {
            // We are done with server authentication.
            pAuthCtx->_pPWC->fPreAuth = TRUE;
            delete pAuthCtx;
            pRequest->SetAuthCtx (NULL);
        }
    }

    // Remove any stale authorization headers in case wHttpSendRequest
    // loops, for example, to handle a redirect.  To ignore trailing colon,
    // subtract 1 from header lengths passed to ReplaceRequestHeader.
    pRequest->ReplaceRequestHeader
        (HTTP_QUERY_AUTHORIZATION,
                    "", 0, 0, HTTP_ADDREQ_FLAG_REPLACE);
    pRequest->ReplaceRequestHeader
        (HTTP_QUERY_PROXY_AUTHORIZATION,
            "", 0, 0, HTTP_ADDREQ_FLAG_REPLACE);

    DWORD error;

    DWORD dwZone, dwPolicy;

    switch (dwStatus)
    {
        case HTTP_STATUS_PROXY_AUTH_REQ: // 407
            error = ProcessResponseHeaders(pRequest, IS_PROXY);
            break;

        case HTTP_STATUS_DENIED: // 401

            if (pRequest->GetCredPolicy() == URLPOLICY_CREDENTIALS_ANONYMOUS_ONLY)
                error = ERROR_SUCCESS;
            else
                error = ProcessResponseHeaders(pRequest, IS_SERVER);
            break;

        default:
            pRequest->SetAuthState(AUTHSTATE_NONE);
            error = ERROR_SUCCESS;
    }

    DEBUG_LEAVE(error);
    return error;
}




//-----------------------------------------------------------------------------
//
//
//      AUTHCTX Base class definitions
//
//
//
//      static funcs:
//          Enumerate
//          UnloadAll
//          CreateAuthCtx
//          CreateAuthCtx (using pwc*)
//          GetSPMListState
//          SearchPwcList
//          FindOrCreatePWC
//          GetAuthHeaderData
//
//      base funcs:
//          FindHdrIdxFromScheme
//          GetScheme
//          GetSchemeType - returns enum
//          GetFlags      - returns SPM flags
//          GetState      - returns state of SPM provider
//
//
//      virtual overrides: defined in basic.cxx, plug.cxx and digest.cxx
//          UpdateFromHeaders
//          PreAuthUser
//          PostAuthUser
//
//
//


/*---------------------------------------------------------------------------
AUTHCTX::SPMData constructor
---------------------------------------------------------------------------*/
AUTHCTX::SPMData::SPMData(LPSTR _szScheme, DWORD _dwFlags)
{
    if (_szScheme)
    {
        szScheme = NewString(_szScheme);
        cbScheme = strlen(_szScheme);
    }
    else
    {
        szScheme = NULL;
        cbScheme = 0;
    }

    if (szScheme)
    {
        if (!lstrcmpi(szScheme, "Basic"))
            eScheme = SCHEME_BASIC;
        else if (!lstrcmpi(szScheme, "NTLM"))
            eScheme = SCHEME_NTLM;
        else if (!lstrcmpi(szScheme, "Digest"))
            eScheme = SCHEME_DIGEST;
        else if (!lstrcmpi(szScheme, "MSN"))
            eScheme = SCHEME_MSN;
        else if (!lstrcmpi(szScheme, "DPA"))
            eScheme = SCHEME_DPA;
        else if (!lstrcmpi(szScheme, "Negotiate"))
            eScheme = SCHEME_NEGOTIATE;
        else
            eScheme = SCHEME_UNKNOWN;

        dwFlags    = _dwFlags;
        eState     = STATE_NOTLOADED;
    }
    else
    {
        dwFlags    = 0;
        eState     = STATE_ERROR;
    }
}

/*---------------------------------------------------------------------------
AUTHCTX::SPMData destructor
---------------------------------------------------------------------------*/
AUTHCTX::SPMData::~SPMData()
{ delete szScheme; }


/*---------------------------------------------------------------------------
AUTHCTX constructor
---------------------------------------------------------------------------*/
AUTHCTX::AUTHCTX(SPMData *pData, PWC *pPWC)
{
    _pSPMData = pData;
    _pPWC = pPWC;
    _pRequest = NULL;
    _fIsProxy = FALSE;
    _pvContext = NULL;
    if (_pPWC)
        _pPWC->nLockCount++;
    _eSubScheme = SCHEME_UNKNOWN;
    _dwSubFlags = 0;
}


/*---------------------------------------------------------------------------
    Destructor
---------------------------------------------------------------------------*/
AUTHCTX::~AUTHCTX()
{
    AuthLock();
    if (_pPWC)
        _pPWC->nLockCount--;
    AuthUnlock();
}


// ------------------------  Static Functions ---------------------------------


/*---------------------------------------------------------------------------
    Enumerate
---------------------------------------------------------------------------*/
VOID AUTHCTX::Enumerate()
{
    LPSTR   szScheme[] = {"NTLM",
                          "Basic"};

    DWORD   dwFlags[]  = {PLUGIN_AUTH_FLAGS_NO_REALM,
                          PLUGIN_AUTH_FLAGS_KEEP_ALIVE_NOT_REQUIRED};

    SPMData   *pNew;
    g_pSPMList = NULL;
    g_eState = STATE_ERROR;

    AssertHaveLock();

    HKEY hkSecurity = NULL; // main security key
    HKEY hkSPMKey = NULL;

    const static CHAR cszMainSecKey[]
        = "SOFTWARE\\Microsoft\\Internet Explorer\\Security";

    if (REGOPENKEY( HKEY_LOCAL_MACHINE, cszMainSecKey, &hkSecurity ) != ERROR_SUCCESS)
    {
        goto endRegEnum;
    }

    DWORD dwIndex;
    dwIndex = 0;

    do
    {
        CHAR szScheme[256];

        if (RegEnumKey( hkSecurity, dwIndex, szScheme, sizeof(szScheme))
            != ERROR_SUCCESS )
        {
            goto endRegEnum;
        }
        dwIndex++;

        if ( hkSPMKey != NULL )
        {
            REGCLOSEKEY(hkSPMKey);
            hkSPMKey = NULL;
        }

        if (REGOPENKEY( hkSecurity, szScheme, &hkSPMKey ) != ERROR_SUCCESS)
        {
            dwIndex++;
            INET_ASSERT(0);
            continue;
        }

        DWORD dwType, cbBuf;

        // Get the flags.
        DWORD dwFlags;
        cbBuf = sizeof(dwFlags);
        if (ERROR_SUCCESS != RegQueryValueEx
                (hkSPMKey, "Flags", NULL, &dwType, (LPBYTE) &dwFlags, &cbBuf)
            || ((dwType != REG_DWORD) && (dwType != REG_BINARY)))
        {
            INET_ASSERT (cbBuf == sizeof(DWORD));
            dwFlags = 0;
        }

        // Hack for 5x: Negotiate protocol requires k-a since it will fail over proxy.
        // Bug # 80939: Front Page 2000 cannot open web folders that require authentication.
        if (!lstrcmpi(szScheme, "Negotiate"))
            dwFlags = PLUGIN_AUTH_FLAGS_NO_REALM;

        // Basic, and NTLM are now hard wired - see endRegEnum.
        if (lstrcmpi(szScheme, "Basic")
            && lstrcmpi(szScheme, "NTLM"))
        {
            pNew = (AUTHCTX::SPMData*) new SPMData(szScheme, dwFlags);

            if (!pNew)
                goto quit;

            // Add to head of list.
            if (pNew->eState != STATE_ERROR)
            {
                pNew->pNext = g_pSPMList;
                g_pSPMList = pNew;
            }
        }

    } while (1);

endRegEnum:

    // Hard-wired Basic, NTLM and Digest
    for (dwIndex = 0; dwIndex < sizeof(szScheme) / sizeof(LPSTR); dwIndex++)
    {
        pNew = (AUTHCTX::SPMData*) new SPMData(szScheme[dwIndex], dwFlags[dwIndex]);
        if (!pNew)
            goto quit;

        // Add to head of list.
        if (pNew->eState != STATE_ERROR)
        {
            pNew->pNext = g_pSPMList;
            g_pSPMList = pNew;
        }
    }

    // The list is now in the correct state.
    g_eState = STATE_LOADED;

quit:


    if ( hkSecurity != NULL )
    {
        REGCLOSEKEY(hkSecurity);
    }

    if ( hkSPMKey != NULL )
    {
        REGCLOSEKEY(hkSPMKey);
    }

}


VOID AUTHCTX::UnloadAll()
{
    // BUGBUG - AuthFlush is called when the last browser session
    // is closed. First the global pwc lists are destructed, and
    // then this func (UnloadAll) is called. However, the global
    // pwc lists are not necessarily flushed (they may have out-
    // standing ref counts) and may persist across browser sessions.
    // when we destruct the SPM list in this func, SPMs reference
    // by any surviving pwc entries are bogus and can be used
    // for subsequent authentication, resulting in a fault.
    //
    // The temporary hack here is to not destruct the SPM
    // list if any pwc list is not destructed. We leak the
    // SPM list on process detach but don't fault. Put the
    // SPM destruct code in DllProcessDetach.
    if (g_pwcRealm || g_pwcOther || g_pwcProxy)
        return;

    SPMData *pData = g_pSPMList;
    while (pData)
    {
        SPMData *pNext = pData->pNext;
        delete pData;
        pData = pNext;
    }

    SSPI_Unload();
    g_eState = STATE_NOTLOADED;
    g_pSPMList = NULL;
}

/*---------------------------------------------------------------------------
    CreateAuthCtx
    Create an AUTHCTX* from headers - initially the authentication context
    is created without a PWC entry. The PWC entry will be found or created
    and possibly updated in UpdateFromHeaders.
---------------------------------------------------------------------------*/
AUTHCTX* AUTHCTX::CreateAuthCtx(HTTP_REQUEST_HANDLE_OBJECT * pRequest, BOOL fIsProxy)
{
    LPSTR szScheme;
    DWORD cbScheme, dwError, dwAuthIdx;
    AUTHCTX *pAuthCtx = NULL;

    dwAuthIdx = 0;
    szScheme = NULL;


    // Get scheme. This is assumed to be the first
    // non-ws token in the auth header info.
    do
    {
        // It is necessary to hold on to the auth index
        // in this loop because we could have gone through
        // more than one scheme.
        dwError = GetAuthHeaderData(pRequest, fIsProxy, NULL,
            &szScheme, &cbScheme, ALLOCATE_BUFFER | GET_SCHEME, dwAuthIdx);

        if (dwError != ERROR_SUCCESS)
            goto quit;

        // This will create the appropriate authentication context
        // with a NULL password cache. The password cache will be
        // created in the call to UpdateFromHeaders.
        pAuthCtx = CreateAuthCtx(pRequest, fIsProxy, szScheme);

        // If creation of an auth context is successful, update
        // the context from any header info.
        if (pAuthCtx)
        {
            if (pAuthCtx->UpdateFromHeaders(pRequest, fIsProxy) != ERROR_SUCCESS)
            {
                delete pAuthCtx;
                pAuthCtx = NULL;
            }
        }
        dwAuthIdx++;

        delete szScheme;
        szScheme = NULL;

    } while (pAuthCtx == NULL);

quit:

    if (szScheme)
        delete szScheme;

    return pAuthCtx;
}

/*---------------------------------------------------------------------------
    CreateAuthCtx
    Create an AUTHCTX without a PWC from scheme
---------------------------------------------------------------------------*/
AUTHCTX* AUTHCTX::CreateAuthCtx(HTTP_REQUEST_HANDLE_OBJECT *pRequest,
    BOOL fIsProxy, LPSTR szScheme)
{
    AUTHCTX *pNewAuthCtx = NULL;

    // If necessary, enumerate auth providers from registry.
    if (AUTHCTX::GetSPMListState() == AUTHCTX::STATE_NOTLOADED)
        AUTHCTX::Enumerate();

    if (AUTHCTX::GetSPMListState() != AUTHCTX::STATE_LOADED)
    {
        // not critical, just no authentication
        goto quit;
    }

    SPMData *pSPM;
    pSPM = g_pSPMList;

    // Find SPMData to create from scheme.
    while (pSPM)
    {
        if (!lstrcmpi(pSPM->szScheme, szScheme))
            break;

        pSPM = pSPM->pNext;
    }

    if (!pSPM)
    {
        // No matching auth scheme found.
        // Not critical, just no auth.
        goto quit;
    }

    // Create an auth context without pwc
    switch(pSPM->eScheme)
    {
        // Create BASIC_CTX with NULL PWC.
        case SCHEME_BASIC:
            pNewAuthCtx = new BASIC_CTX(pRequest, fIsProxy, pSPM, NULL);
            break;

        // Create DIGEST_CTX with NULL PWC.
        case SCHEME_DIGEST:
            pNewAuthCtx = new DIGEST_CTX(pRequest, fIsProxy, pSPM, NULL);
            break;


        // Create PLUG_CTX with NULL PWC.
        case SCHEME_NTLM:
        case SCHEME_MSN:
        case SCHEME_DPA:
        case SCHEME_NEGOTIATE:
        case SCHEME_UNKNOWN:

        default:
            pNewAuthCtx = new PLUG_CTX(pRequest, fIsProxy, pSPM, NULL);

    }


quit:
    return pNewAuthCtx;
}


/*---------------------------------------------------------------------------
    CreateAuthCtx
    Create an AUTHCTX from a PWC.
---------------------------------------------------------------------------*/
AUTHCTX* AUTHCTX::CreateAuthCtx(HTTP_REQUEST_HANDLE_OBJECT *pRequest,
                    BOOL fIsProxy, PWC* pPWC)
{
    AUTHCTX *pNewAuthCtx = NULL;

    // If necessary, enumerate auth providers from registry.
    if (AUTHCTX::GetSPMListState() == AUTHCTX::STATE_NOTLOADED)
        AUTHCTX::Enumerate();

    if (AUTHCTX::GetSPMListState() != AUTHCTX::STATE_LOADED)
    {
        // not critical, just no authentication
        goto quit;
    }

    // Handle tests (via proxy, is keep-alive)
    // will be done in UpdateFromHeaders. Here
    // we just construct the AUTHCTX.
    switch(pPWC->pSPM->eScheme)
    {
        // Create BASIC_CTX.
        case SCHEME_BASIC:

            pNewAuthCtx = new BASIC_CTX(pRequest, fIsProxy, pPWC->pSPM, pPWC);
            break;

        // Create DIGEST_CTX.
        case SCHEME_DIGEST:
            pNewAuthCtx = new DIGEST_CTX(pRequest, fIsProxy, pPWC->pSPM, pPWC);
            break;


        // Create PLUG_CTX.
        case SCHEME_NTLM:
        case SCHEME_MSN:
        case SCHEME_DPA:
        case SCHEME_NEGOTIATE:
        case SCHEME_UNKNOWN:

        default:
            pNewAuthCtx = new PLUG_CTX(pRequest, fIsProxy, pPWC->pSPM, pPWC);

    }


quit:
    return pNewAuthCtx;
}

AUTHCTX::SPMState AUTHCTX::GetSPMListState()
    { return g_eState; }


/*-----------------------------------------------------------------------------
    SearchPwcList
-----------------------------------------------------------------------------*/
PWC* AUTHCTX::SearchPwcList
    (PWC* pwc, LPSTR lpszHost, LPSTR lpszUri, LPSTR lpszRealm, SPMData *pSPM)
/*++

Routine Description:
    Scans the Linked List Cache for URLs, Realms, and Servers.  Also allows
    filter fields, to narrow searches.

Arguments:
    pwc                   - Pointer to first item to search from.
    lpszHost              - Host, or Server name to search on.
    lpszUri               - URL to search on.
    lpszRealm             - Security Realm to search on
    lpszScheme            - Authentication scheme to search on.
Return Value:

    PWC *
    Success - Pointer to found item.

    Failure - NULL pointer.

Comments:
    Note: This Legacy code from the SpyGlass IE 1.0 browser

    The PWC lists are searched on every request.  Could optimize by keeping
    a hash value of the server/proxy name.

    If an exact match isn't found on a 401 response, the list is walked again
    to search for a realm match.  Could add a parameter to do both at once.
--*/
{
    AssertHaveLock();

    while (pwc)
    {
        if (   (!pSPM || pSPM == pwc->pSPM)
            && (!lpszHost  || !lstrcmpi(pwc->lpszHost,lpszHost))
            && (!lpszRealm || !lstrcmpi(pwc->lpszRealm,lpszRealm))
            && (!lpszUri   || TemplateMatch (pwc->lpszUrl, lpszUri))
           )
        {

            DEBUG_PRINT(HTTP, INFO, ("Lookup: Found template match [%q]\n",
                pwc->lpszUser));
            return pwc;
        }
        else
        {
            pwc = pwc->pNext;
        }
    }

    return NULL;
}



/*-----------------------------------------------------------------------------
    FindOrCreatePWC
-----------------------------------------------------------------------------*/
PWC* AUTHCTX::FindOrCreatePWC(
    HTTP_REQUEST_HANDLE_OBJECT *pRequest,
    BOOL     fIsProxy,
    SPMData *pSPM,
    LPSTR    lpszRealm
)
{
    AssertHaveLock();

    // Find or create a PWC.
    PWC *pwc;
    pwc = NULL;


    if (!pSPM)
    {
        INET_ASSERT(FALSE);
        goto quit;
    }

    // Get URL from request handle.
    LPSTR lpszUrl;
    lpszUrl = pRequest->GetURL();

    // Get host from request handle.
    LPSTR lpszHost;
    lpszHost = fIsProxy?
        GetProxyName(pRequest) : pRequest->GetServerName();


    // For NTLM, use the hostname analagously to basic realm.
    if (pSPM->eScheme == SCHEME_NTLM || pSPM->eScheme == SCHEME_NEGOTIATE)
    {
        INET_ASSERT (!lpszRealm);
        lpszRealm = lpszHost;
    }


    if (fIsProxy)
    {
        //
        // PROXY AUTHENTICATION
        //

        if (lpszHost)
        {
            // See if we have a exact match.
            pwc = SearchPwcList
                (g_pwcProxy, lpszHost, NULL, lpszRealm, pSPM);
            if (!pwc)
            {
                // If not, create a new one.
                pwc = PWC_Create (lpszHost, NULL, lpszRealm, pSPM);
                if (!pwc)
                    goto quit;

                // Insert at front of list.
                pwc->pNext = g_pwcProxy;
                g_pwcProxy = pwc;
            }
        }
    }
    else // if (!fIsProxy)
    {
        //
        // SERVER AUTHENTICATION
        //

        if (!lpszRealm)
        {
            // See if we have an exact match.
            pwc = SearchPwcList
                (g_pwcOther, lpszHost, lpszUrl, NULL, pSPM);
            if (!pwc)
            {
                // If not, create a new one.
                pwc = PWC_Create (lpszHost, lpszUrl, NULL, pSPM);
                if (!pwc)
                    goto quit;

                // Insert at front of list.
                pwc->pNext = g_pwcOther;
                g_pwcOther = pwc;
            }
        }
        else // if (lpszRealm)
        {
            // First, search the primary host/realm list.
            // We will check whether the URL matches later.
            pwc = SearchPwcList
                (g_pwcRealm, lpszHost, NULL, lpszRealm, pSPM);
            if (!pwc)
            {
                // If not, create one.
                pwc = PWC_Create (lpszHost, lpszUrl, lpszRealm, pSPM);
                if (!pwc)
                    goto quit;

                // Insert at front of list.
                pwc->pNext = g_pwcRealm;
                g_pwcRealm = pwc;
            }
            else // if (pwc)
            {
                // Check if the URL matches.
                if (!TemplateMatch (lpszUrl, pwc->lpszUrl))
                {
                    // Create another pwc entry
                    PWC* pwcDir = PWC_Create (lpszHost, lpszUrl, lpszRealm, pSPM);
                    if (!pwcDir)
                        goto quit;

                    // Insert into secondary list.
                    pwcDir->pNext = g_pwcOther;
                    g_pwcOther = pwcDir;
                }
            }
        }

        INET_ASSERT (pwc);
    }
quit:
    return pwc;
}

/*-----------------------------------------------------------------------------
    GetAuthHeaderData
-----------------------------------------------------------------------------*/
DWORD AUTHCTX::GetAuthHeaderData(
    HTTP_REQUEST_HANDLE_OBJECT *pRequest,
    BOOL      fIsProxy,
    LPSTR     szItem,
    LPSTR    *pszData,
    LPDWORD   pcbData,
    DWORD     dwFlags,
    DWORD     dwIndex)
{
    LPSTR szData;
    DWORD cbData, cbItem, dwError = ERROR_SUCCESS;;
    CHAR *szTok, *szKey, *szValue;
    DWORD cbTok, cbKey, cbValue;

    szTok = szKey = szValue = NULL;
    cbTok = cbKey = cbValue = NULL;

    cbItem = szItem ? strlen(szItem) : 0;

    DWORD dwQuery = fIsProxy?
        HTTP_QUERY_PROXY_AUTHENTICATE : HTTP_QUERY_WWW_AUTHENTICATE;

    // NULL item passed in means get up to the first \r\n, or
    // possibly only the scheme is desired depending on dwFlags.
    if (!cbItem)
    {
        if ((dwError = pRequest->FastQueryResponseHeader(dwQuery,
            (LPVOID*) &szData,
            &cbData,
            dwIndex)) != ERROR_SUCCESS)
        {
            goto quit;
        }

        // Only the scheme is desired.
        if (dwFlags & GET_SCHEME)
        {
            CHAR* ptr;
            ptr = szValue = szData;
            cbValue = 0;
            while (!(*ptr == ' ' || *ptr == '\t' || *ptr == '\r' || *ptr == '\n'))
            {
                ptr++;
                cbValue++;
            }
        }
        else
        {
            // The entire header is desired.
            szValue = szData;
            cbValue = cbData;
        }

    }
    else
    {
        // An item was passed in - attempt to parse this
        // from the headers and return the corresponding
        // value.
        if ((dwError = pRequest->FastQueryResponseHeader(dwQuery,
                  (LPVOID*) &szData,
                  &cbData,
                  dwIndex)) != ERROR_SUCCESS)
        {
            goto quit;
        }

        // Successfully retrieved header. Parse for the desired item.

        // Point past scheme
        while (!(*szData == ' ' || *szData == '\t' || *szData == '\r' || *szData == '\n'))
        {
            szData++;
            cbData--;
        }

        // Attempt to parse an item of the format 'key = <">value<">'
        // from a comma delmited list of items.
        dwError = ERROR_HTTP_HEADER_NOT_FOUND;
        while (GetDelimitedToken(&szData, &cbData, &szTok, &cbTok, ','))
        {
            if (GetKeyValuePair(szTok, cbTok, &szKey, &cbKey, &szValue, &cbValue))
            {
                if ((cbItem == cbKey) && !strnicmp(szKey, szItem, cbItem))
                {
                    TrimQuotes(&szValue, &cbValue);
                    dwError = ERROR_SUCCESS;
                    break;
                }
            }
        }

    }

    if (dwError == ERROR_SUCCESS)
    {
        // Allocate buffer containing data
        // or return reference.
        if (dwFlags & ALLOCATE_BUFFER)
        {
            *pszData = new CHAR[cbValue+1];
            if (!*pszData)
            {
                dwError = ERROR_NOT_ENOUGH_MEMORY;
                goto quit;
            }
            memcpy(*pszData, szValue, cbValue);
            (*pszData)[cbValue] = '\0';
            *pcbData = cbValue;
        }
        else
        {
            *pszData = szValue;
            *pcbData = cbValue;
        }
    }

quit:

    if (dwError != ERROR_SUCCESS)
    {
        INET_ASSERT(dwIndex || dwError == ERROR_HTTP_HEADER_NOT_FOUND);
    }

    return dwError;
}


// ------------------------  Base class funcs---------------------------------




/*-----------------------------------------------------------------------------
FindHdrIdxFromScheme
-----------------------------------------------------------------------------*/
DWORD AUTHCTX::FindHdrIdxFromScheme(LPDWORD pdwIndex)
{
    LPSTR szHeader;
    DWORD cbScheme, cbHeader, dwQuery, dwError;

    dwQuery = _fIsProxy?
        HTTP_QUERY_PROXY_AUTHENTICATE : HTTP_QUERY_WWW_AUTHENTICATE;

    *pdwIndex = 0;

    while ((dwError = _pRequest->FastQueryResponseHeader(dwQuery,
            (LPVOID*) &szHeader,
            &cbHeader,
            *pdwIndex)) == ERROR_SUCCESS)
    {
        DWORD cb = 0;
        CHAR *ptr = szHeader;
        while (!(*ptr == ' ' || *ptr == '\t' || *ptr == '\r' || *ptr == '\n'))
        {
            ptr++;
            cb++;
        }

        if ((_pSPMData->cbScheme == cb)
            && (!strnicmp(_pSPMData->szScheme, szHeader, cb)))
        {
            break;
        }
        (*pdwIndex)++;
    }
    return dwError;
}

/*-----------------------------------------------------------------------------
    Get funcs.
-----------------------------------------------------------------------------*/
LPSTR AUTHCTX::GetScheme()
    { return _pSPMData->szScheme; }

AUTHCTX::SPMScheme AUTHCTX::GetSchemeType()
{
    if (_pSPMData->eScheme == SCHEME_NEGOTIATE)
    {
        if (_eSubScheme == SCHEME_NTLM || _eSubScheme == SCHEME_KERBEROS)
        {
            return _eSubScheme;
        }
    }
    return _pSPMData->eScheme;
}

DWORD AUTHCTX::GetFlags()
{
    if (_pSPMData->eScheme == SCHEME_NEGOTIATE)
    {
        if (_eSubScheme == SCHEME_NTLM || _eSubScheme == SCHEME_KERBEROS)
        {
            return _dwSubFlags;
        }
    }
    return _pSPMData->dwFlags;
}

AUTHCTX::SPMState AUTHCTX::GetState()
    { return _pSPMData->eState; }
