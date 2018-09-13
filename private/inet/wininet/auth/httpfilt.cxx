/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    httpfilt.cxx

Abstract:

    This file contains implementation for HTTP filters.

    Contents:
        HTTPFILT methods
        HttpFiltOpen
        HttpFiltClose
        HttpFiltOnRequest
        HttpFiltOnResponse
        HttpFiltOnBlockingOps
        HttpFiltOnTransactionComplete

Author:

    Rajeev Dujari (RajeevD) 01-Jul-1996

Revision History:

TO DO
    Get urlmon to call InternetErrorDlg on ERROR_INTERNET_NEED_UI
    Deal with OnRequest returning ERROR_INTERNET_NEED_UI

    Generate list of filters from registry.
    Call filters only if matching headers.
    Wrap debug statements around filter calls
    Update documentation.

    Allow filters to modify response headers

NOTES
    Don't unload filters before shutdown because request handles
      may have references to them.

--*/

#include "wininetp.h"
#include "httpfilt.h"

struct CONTEXT_ENTRY : public SERIALIZED_LIST_ENTRY
{
    HINTERNET hRequest;
    LPVOID    lpContext;
};

class HTTPFILT
{
    HMODULE hFilter;
    LPVOID  lpFilterContext;
    SERIALIZED_LIST slContexts;      // list of CONTEXT_ENTRY

    PFN_FILTEROPEN                   pfnOpen;
    PFN_FILTERBEGINNINGTRANSACTION   pfnOnRequest;
    PFN_FILTERONRESPONSE             pfnOnResponse;
    PFN_FILTERONBLOCKINGOPS          pfnOnBlockingOps;
    PFN_FILTERONTRANSACTIONCOMPLETE  pfnOnTransactionComplete;

    LPVOID* GetContextPtr (HINTERNET hRequest);

public:
    BOOL Open (void);
    BOOL Close (void);
    BOOL OnRequest (HINTERNET hRequest);
    BOOL OnResponse (HINTERNET hRequest);
    BOOL OnTransactionComplete (HINTERNET hRequest);
    BOOL OnBlockingOps (HINTERNET hRequest, HWND hwnd);
};

PRIVATE BOOL fOpen = FALSE;
PRIVATE HTTPFILT httpfiltRPA;
PRIVATE HTTPFILT *pRPA = NULL;

/*++

Routine Description:

Arguments:

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE. Call GetLastError() for more info

--*/

LPVOID* HTTPFILT::GetContextPtr (HINTERNET hRequest)
{
    // Search for this entry.
    LockSerializedList (&slContexts);

    // Get the head of the list.  Be sure it is not the dummy entry.
    CONTEXT_ENTRY *pce = (CONTEXT_ENTRY *) HeadOfSerializedList (&slContexts);
    if ((PLIST_ENTRY) pce == &slContexts.List)
        pce = NULL;

    while (pce)
    {
        if (pce->hRequest == hRequest)
            goto done;
        pce = (CONTEXT_ENTRY *) NextInSerializedList (&slContexts, pce);
    }

    // Create a new entry.
    if (!(pce = new CONTEXT_ENTRY))
    {
        SetLastError (ERROR_NOT_ENOUGH_MEMORY);
        UnlockSerializedList (&slContexts);
        return NULL;
    }
    pce->hRequest = hRequest;
    pce->lpContext = NULL;
    InsertAtHeadOfSerializedList (&slContexts, &pce->List);

done:

    UnlockSerializedList (&slContexts);
    return &pce->lpContext;
}


/*++

Routine Description:

Arguments:

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE. Call GetLastError() for more info

--*/

BOOL HttpFiltOpen (void)
{
    if (!fOpen)
    {
        HKEY hkey;

#define SETTINGS_KEY  "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Internet Settings"

        char szRpaKey[MAX_PATH];
        strcpy (szRpaKey, SETTINGS_KEY);
        strcat (szRpaKey, "\\Http Filters\\RPA");
        if (  !RegOpenKeyEx (HKEY_CURRENT_USER,  szRpaKey, 0, KEY_READ, &hkey)
          || !RegOpenKeyEx (HKEY_LOCAL_MACHINE, szRpaKey, 0, KEY_READ, &hkey))
        {
            REGCLOSEKEY (hkey);
            if (httpfiltRPA.Open())
                pRPA = &httpfiltRPA;
        }

        fOpen = TRUE;
    }
    return TRUE;
}


BOOL HTTPFILT::Open(void)
{
    hFilter = LoadLibrary ("RPAWINET.DLL");
    if (!hFilter)
        goto err;

    // If there's an open function, call it.
    pfnOpen = (PFN_FILTEROPEN) GetProcAddress (hFilter, SZFN_FILTEROPEN);
    if (pfnOpen)
    {
        BOOL fFilter;

        __try
        {
            fFilter = (*pfnOpen) (&lpFilterContext, "RPA", NULL);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            DEBUG_PRINT(HTTP, ERROR, ("HttpFilterOpen faulted\n"));
            fFilter = FALSE;
        }
        ENDEXCEPT
        if (!fFilter)
            goto err;
    }

    InitializeSerializedList (&slContexts);

    // Look up other entry points.
    pfnOnRequest  = (PFN_FILTERBEGINNINGTRANSACTION)
        GetProcAddress (hFilter, SZFN_FILTERBEGINNINGTRANSACTION);
    pfnOnResponse = (PFN_FILTERONRESPONSE)
        GetProcAddress (hFilter, SZFN_FILTERONRESPONSE);
    pfnOnTransactionComplete  = (PFN_FILTERONTRANSACTIONCOMPLETE)
        GetProcAddress (hFilter, SZFN_FILTERONTRANSACTIONCOMPLETE);
    pfnOnBlockingOps  = (PFN_FILTERONBLOCKINGOPS)
        GetProcAddress (hFilter, SZFN_FILTERONBLOCKINGOPS);
    return TRUE;

err:

    if (hFilter)
    {
        FreeLibrary (hFilter);
        hFilter = NULL;
    }
    return FALSE;
}

/*++

Routine Description:

Arguments:

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE. Call GetLastError() for more info

--*/

HttpFiltClose(void)
{
    if (fOpen)
    {
        if (pRPA)
        {
            pRPA->Close();
            pRPA = NULL;
        }
        fOpen = FALSE;
    }
    return TRUE;
}


BOOL HTTPFILT::Close (void)
{
    TerminateSerializedList (&slContexts);

    if (hFilter)
    {
        if (pfnOpen)
        {
            PFN_FILTERCLOSE pfnClose;
            pfnClose = (PFN_FILTERCLOSE)
                GetProcAddress (hFilter, SZFN_FILTERCLOSE);

            __try
            {
                (*pfnClose) (lpFilterContext, InDllCleanup);
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                 DEBUG_PRINT(HTTP, ERROR, ("HttpFilterClose faulted\n"));
            }
            ENDEXCEPT
        }
        FreeLibrary (hFilter);
    }
    return TRUE;
}

/*++

Routine Description:

Arguments:

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE. Call GetLastError() for more info

--*/

BOOL HttpFiltOnRequest (LPVOID pRequest)
{
    INET_ASSERT (fOpen);
    if (!pRPA)
        return TRUE;
    return pRPA->OnRequest(((HTTP_REQUEST_HANDLE_OBJECT *)pRequest)->GetPseudoHandle());
}

BOOL HTTPFILT::OnRequest(HINTERNET hRequest)
{
    if (pfnOnRequest)
    {
        LPVOID *lppvContext = GetContextPtr(hRequest);
        if (!lppvContext)
            return FALSE;

        BOOL fFilter;

        __try
        {
            fFilter = (*pfnOnRequest)
                (lpFilterContext, lppvContext, hRequest, NULL);
            INET_ASSERT (fFilter);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
             DEBUG_PRINT(HTTP, ERROR, ("HttpFilterOnRequest faulted\n"));
             fFilter = FALSE;
        }
        ENDEXCEPT
    }
    return TRUE;
}

/*++

Routine Description:

Arguments:

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE. Call GetLastError() for more info

--*/


BOOL HttpFiltOnResponse (LPVOID pObject)
{
    INET_ASSERT (fOpen);
    if (!pRPA)
        return TRUE;

    HTTP_REQUEST_HANDLE_OBJECT * pRequest = (HTTP_REQUEST_HANDLE_OBJECT *)pObject;
    BOOL fFilter = pRPA->OnResponse (pRequest->GetPseudoHandle());
    if (!fFilter)
    {
        switch (GetLastError())
        {
            case ERROR_INTERNET_NEED_UI:
            {
                pRequest->SetBlockingFilter (pRPA);
                break;
            }

            case ERROR_INTERNET_FORCE_RETRY:
            {
                    // Sink any bytes before restarting the send request.
                BYTE szSink[1024];
                    DWORD dwBytesRead = 1;
                while (dwBytesRead && ERROR_SUCCESS == pRequest->ReadData
                    (szSink, sizeof(szSink), &dwBytesRead, TRUE, 0));
                SetLastError (ERROR_INTERNET_FORCE_RETRY);
                break;
            }
        }
    }

    return fFilter;
}

BOOL HTTPFILT::OnResponse(HINTERNET hRequest)
{
    BOOL fFilter;

    if (pfnOnResponse)
    {
        LPVOID *lppvContext = GetContextPtr(hRequest);
        if (!lppvContext)
            return FALSE;

        __try
        {
            fFilter = (*pfnOnResponse)
                (lpFilterContext, lppvContext, hRequest, NULL);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
             DEBUG_PRINT(HTTP, ERROR, ("HttpFilterOnResponse faulted\n"));
             fFilter = FALSE;
        }
        ENDEXCEPT
        if (!fFilter)
            return FALSE;
    }

    return TRUE;
}

/*++

Routine Description:

Arguments:

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE. Call GetLastError() for more info

--*/

BOOL HttpFiltOnBlockingOps
    (LPVOID pObject, HINTERNET hRequest, HWND hwnd)
{
    INET_ASSERT (fOpen);
    INET_ASSERT (pRPA);

    HTTP_REQUEST_HANDLE_OBJECT * pRequest =
        (HTTP_REQUEST_HANDLE_OBJECT *) pObject;
    HTTPFILT *lpFilter = (HTTPFILT *) pRequest->GetBlockingFilter();
    INET_ASSERT (lpFilter == pRPA);

    BOOL fFilter = lpFilter->OnBlockingOps(hRequest, hwnd);
    if (!fFilter && GetLastError() == ERROR_INTERNET_FORCE_RETRY)
    {
        // Sink any bytes before restarting the send request.
        BYTE szSink[1024];
        DWORD dwBytesRead = 1;
        while (dwBytesRead && ERROR_SUCCESS == pRequest->ReadData
            (szSink, sizeof(szSink), &dwBytesRead, TRUE, 0));
        SetLastError (ERROR_INTERNET_FORCE_RETRY);
    }
    return fFilter;
}


BOOL HTTPFILT::OnBlockingOps (HINTERNET hRequest, HWND hwnd)
{
    if (pfnOnBlockingOps)
    {
        LPVOID *lppvContext = GetContextPtr(hRequest);
        if (!lppvContext)
            return FALSE;
        BOOL fFilter;

        __try
        {
            fFilter = (*pfnOnBlockingOps)
                (lpFilterContext, lppvContext, hRequest, hwnd, NULL);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
             DEBUG_PRINT(HTTP, ERROR, ("HttpFilterOnBlockingOps faulted\n"));
             fFilter = FALSE;
        }
        ENDEXCEPT
        return fFilter;
    }

    return TRUE;
}


/*++

Routine Description:

Arguments:

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE. Call GetLastError() for more info

--*/

BOOL HttpFiltOnTransactionComplete (HINTERNET hRequest)
{
    INET_ASSERT (fOpen);
    if (!pRPA)
        return TRUE;
    else
        return pRPA->OnTransactionComplete (hRequest);
}

BOOL HTTPFILT::OnTransactionComplete (HINTERNET hRequest)
{
    LPVOID* lppvContext = GetContextPtr(hRequest);

    if (pfnOnTransactionComplete)
    {
        __try
        {
            (*pfnOnTransactionComplete)
                (lpFilterContext, lppvContext, hRequest, NULL);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            DEBUG_PRINT (HTTP, ERROR, ("HttpFilterOnTransactionComplete faulted\n"));
        }
        ENDEXCEPT
    }

    // Destroy any context that was created.
    if (lppvContext)
    {
        CONTEXT_ENTRY* pce =  CONTAINING_RECORD
            (lppvContext, CONTEXT_ENTRY, lpContext);
        RemoveFromSerializedList (&slContexts, &pce->List);
    }

    return TRUE;
}
