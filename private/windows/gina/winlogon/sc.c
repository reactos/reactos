//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       sc.c
//
//  Contents:   Smart Card support.
//
//  Classes:
//
//  Functions:
//
//  History:    11-17-97   RichardW   Created
//
//----------------------------------------------------------------------------

#include "precomp.h"
#pragma hdrstop

#include <winscard.h>
#include "sclogon.h"

//
// This file contains a small number of functions to listen for
// smart card events
//

static LIST_ENTRY  ScEventList ;
static RTL_CRITICAL_SECTION ScEventLock ;

// latebind the scarddlg call
typedef LONG (WINAPI FN_GETOPENCARDNAME)(LPOPENCARDNAMEW);
typedef FN_GETOPENCARDNAME * PFN_GETOPENCARDNAME ;

// richardw:  late bind the SCard calls
typedef LONG (WINAPI FN_SCARDRELEASECONTEXT)(SCARDCONTEXT);
typedef FN_SCARDRELEASECONTEXT * PFN_SCARDRELEASECONTEXT ;

typedef LONG (WINAPI FN_SCARDGETSTATUSCHANGEW)(SCARDCONTEXT, DWORD, LPSCARD_READERSTATE_W, DWORD);
typedef FN_SCARDGETSTATUSCHANGEW * PFN_SCARDGETSTATUSCHANGEW ;

typedef LONG (WINAPI FN_SCARDESTABLISHCONTEXT)(DWORD, LPCVOID, LPCVOID, LPSCARDCONTEXT );
typedef FN_SCARDESTABLISHCONTEXT * PFN_SCARDESTABLISHCONTEXT ;

typedef LONG (WINAPI FN_SCARDFREEMEMORY)(SCARDCONTEXT, LPVOID );
typedef FN_SCARDFREEMEMORY * PFN_SCARDFREEMEMORY ;

typedef LONG (WINAPI FN_SCARDLISTREADERSW)(SCARDCONTEXT, LPCWSTR, LPWSTR, LPDWORD );
typedef FN_SCARDLISTREADERSW * PFN_SCARDLISTREADERSW ;

typedef LONG (WINAPI FN_SCARDLISTCARDSW)(SCARDCONTEXT, LPCBYTE, LPCGUID, DWORD, LPWSTR, LPDWORD );
typedef FN_SCARDLISTCARDSW * PFN_SCARDLISTCARDSW ;

typedef LONG (WINAPI FN_SCARDGETCARDTYPEPROVIDERNAMEW)(SCARDCONTEXT, LPCWSTR, DWORD, LPWSTR, LPDWORD );
typedef FN_SCARDGETCARDTYPEPROVIDERNAMEW * PFN_SCARDGETCARDTYPEPROVIDERNAMEW;

typedef LONG (WINAPI FN_SCARDCANCEL)(SCARDCONTEXT);
typedef FN_SCARDCANCEL * PFN_SCARDCANCEL ;

typedef LONG (WINAPI FN_SCARDISVALIDCONTEXT)(SCARDCONTEXT );
typedef FN_SCARDISVALIDCONTEXT * PFN_SCARDISVALIDCONTEXT ;

typedef HANDLE (WINAPI FN_SCARDACCESSSTARTEDEVENT)(VOID);
typedef FN_SCARDACCESSSTARTEDEVENT * PFN_SCARDACCESSSTARTEDEVENT ;

PFN_GETOPENCARDNAME         pGetOpenCardName ;
PFN_SCARDRELEASECONTEXT     pSCardReleaseContext ;
PFN_SCARDGETSTATUSCHANGEW   pSCardGetStatusChange ;
PFN_SCARDESTABLISHCONTEXT   pSCardEstablishContext ;
PFN_SCARDFREEMEMORY         pSCardFreeMemory ;
PFN_SCARDLISTREADERSW       pSCardListReaders ;
PFN_SCARDLISTCARDSW         pSCardListCards ;
PFN_SCARDGETCARDTYPEPROVIDERNAMEW    pSCardGetCardTypeProviderName ;
PFN_SCARDCANCEL             pSCardCancel ;
PFN_SCARDISVALIDCONTEXT     pSCardIsValidContext ;
PFN_SCARDACCESSSTARTEDEVENT	pSCardAccessStartedEvent ;

static BOOL ResetInsertionMonitor;

BOOL
ScInit(
    VOID
    )
{
    BOOL Ret ;

    __try 
    {
        InitializeCriticalSection( &ScEventLock );

        InitializeListHead( &ScEventList );

        Ret = TRUE ;
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        Ret = FALSE ;
    }

    return Ret;
}

BOOL
ScAddEvent(
    SC_EVENT_TYPE   Type,
    PSC_DATA        Data
    )
{
    PSC_EVENT Event ;

    Event = LocalAlloc( LMEM_FIXED, sizeof( SC_EVENT ) );

    if ( !Event )
    {
        return FALSE ;
    }

    Event->Type = Type ;
    Event->Data = Data ;

    RtlEnterCriticalSection( &ScEventLock );
    InsertTailList( &ScEventList, &Event->List );
    RtlLeaveCriticalSection( &ScEventLock );

    return TRUE ;
}

BOOL
ScRemoveEvent(
    SC_EVENT_TYPE * Type,
    PSC_DATA *      Data
    )
{
    PSC_EVENT Event ;
    PLIST_ENTRY Head ;

    RtlEnterCriticalSection( &ScEventLock );

    if (!IsListEmpty( &ScEventList ) )
    {
        Head = RemoveHeadList( &ScEventList );
    }
    else 
    {
        Head = NULL ;
    }

    RtlLeaveCriticalSection( &ScEventLock );

    if ( Head )
    {
        Event = CONTAINING_RECORD( Head, SC_EVENT, List );

        *Type = Event->Type ;
        *Data = Event->Data ;

        LocalFree( Event );

        return TRUE ;
    }

    return FALSE ;

}

VOID
ScFreeEventData(
    PSC_DATA Data
    )
{
    if ( Data->ScInfo.pszReader )
    {
        LocalFree( Data->ScInfo.pszReader );
    }

    if ( Data->ScInfo.pszCard )
    {
        LocalFree( Data->ScInfo.pszCard );
    }

    if ( Data->ScInfo.pszContainer )
    {
        LocalFree( Data->ScInfo.pszContainer );
    }

    if ( Data->ScInfo.pszCryptoProvider )
    {
        LocalFree( Data->ScInfo.pszCryptoProvider );
    }
}



BOOL
SnapWinscard(
    VOID
    )
{
    HMODULE hDll = NULL;

    hDll = LoadLibrary( TEXT("WINSCARD.DLL") );

    if ( !hDll )
    {
        return FALSE ;
    }

    pSCardReleaseContext = (PFN_SCARDRELEASECONTEXT) GetProcAddress( hDll, "SCardReleaseContext" );
    pSCardGetStatusChange = (PFN_SCARDGETSTATUSCHANGEW) GetProcAddress( hDll, "SCardGetStatusChangeW" );
    pSCardEstablishContext = (PFN_SCARDESTABLISHCONTEXT) GetProcAddress( hDll, "SCardEstablishContext" );
    pSCardFreeMemory = (PFN_SCARDFREEMEMORY) GetProcAddress( hDll, "SCardFreeMemory" );
    pSCardListReaders = (PFN_SCARDLISTREADERSW) GetProcAddress( hDll, "SCardListReadersW" );
    pSCardListCards = (PFN_SCARDLISTCARDSW) GetProcAddress( hDll, "SCardListCardsW" );
    pSCardGetCardTypeProviderName = (PFN_SCARDGETCARDTYPEPROVIDERNAMEW) GetProcAddress( hDll, "SCardGetCardTypeProviderNameW" );
    pSCardCancel = (PFN_SCARDCANCEL) GetProcAddress( hDll, "SCardCancel" );
	pSCardIsValidContext = (PFN_SCARDISVALIDCONTEXT) GetProcAddress( hDll, "SCardIsValidContext" );
	pSCardAccessStartedEvent = (PFN_SCARDACCESSSTARTEDEVENT) GetProcAddress(hDll, "SCardAccessStartedEvent");

    if ( !pSCardReleaseContext ||
         !pSCardGetStatusChange ||
         !pSCardEstablishContext ||
         !pSCardFreeMemory ||
         !pSCardListReaders ||
         !pSCardListCards ||
         !pSCardGetCardTypeProviderName ||
         !pSCardCancel ||
		 !pSCardIsValidContext ||
		 !pSCardAccessStartedEvent)
    {
        return FALSE ;
    }

    return TRUE ;
}

VOID
SetLogonReader(
	LPCTSTR szReader
	)
{
	HKEY hKey;

    if (RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        WINLOGON_KEY,
        0,
        KEY_WRITE,
        &hKey
        ) == ERROR_SUCCESS) {

		RegSetValueEx(
			hKey,
			TEXT("ScLogonReader"),
			0,
			REG_SZ,
			(PBYTE) szReader,
			wcslen(szReader) * sizeof(WCHAR)
			);

		RegCloseKey(hKey);
	}
}

VOID
PostSmartCardEvent(
    PTERMINAL pTerm,
    SCARDCONTEXT SCContext,
    SCARD_READERSTATE * Reader
    )
{
    SC_DATA * Data ;
    SC_EVENT_TYPE EventType ;
    WLX_SC_NOTIFICATION_INFO ScInfo ;
    PWSTR String ;
    DWORD StringLen ;
    DWORD Result ;
	static BOOL InsertionReported;
	static ULONG EventCount;

	if (ResetInsertionMonitor) {

		ResetInsertionMonitor = FALSE;
		InsertionReported = FALSE;
		EventCount = 0;
	}

    if ( ( Reader->dwEventState & SCARD_STATE_EMPTY ) &&
         ( Reader->dwCurrentState & SCARD_STATE_PRESENT ) &&
		 ( Reader->pvUserData == (PVOID) EventCount) )
    {
        EventType = ScRemove ;
		Reader->pvUserData = NULL;
		InsertionReported = FALSE;
		SetLogonReader(TEXT(""));
    }
    else if ( ( Reader->dwEventState & SCARD_STATE_PRESENT ) &&
              ( Reader->dwCurrentState & SCARD_STATE_EMPTY ) &&
			  ( InsertionReported == FALSE ))
    {
        EventType = ScInsert ;
		Reader->pvUserData = (PVOID) (++EventCount);
		InsertionReported = TRUE;
		SetLogonReader(Reader->szReader);
    }
    else 
    {
        return;
    }

    DebugLog(( DEB_TRACE_SC, "Reader %ws reports card %s\n",
               Reader->szReader, 
               ( EventType == ScRemove ? 
                    "removal" : "insertion" ) ));

    if ( ExitWindowsInProgress )
    {
        DebugLog(( DEB_TRACE_SC, "Dropping sc event due to shutdown or logoff\n" ));
        return;
    }

    ZeroMemory( &ScInfo, sizeof( ScInfo ) );

    ScInfo.pszReader = AllocAndDuplicateString( (PWSTR) Reader->szReader );

    if ( !ScInfo.pszReader )
    {
        return;
    }

    //
    // Only for correctly inserted cards can we tell what card, etc. 
    // 
    if ( EventType == ScInsert && 
		 (Reader->dwEventState & SCARD_STATE_MUTE) == 0)
    {
		// get the card name from the ATR
        StringLen = SCARD_AUTOALLOCATE ;
        Result = pSCardListCards(
                        SCContext,
                        Reader->rgbAtr,
                        NULL,
                        0,
                        (LPTSTR) &String,
                        &StringLen );

        if ( Result == SCARD_S_SUCCESS )
        {
            ScInfo.pszCard = AllocAndDuplicateString( String );

            pSCardFreeMemory( SCContext, String );

            if ( !ScInfo.pszCard )
            {
                goto PostErrorExit ;
            }

			// get CSP name from card name
            StringLen = SCARD_AUTOALLOCATE ;
            Result = pSCardGetCardTypeProviderName(
                            SCContext,
                            ScInfo.pszCard,
                            SCARD_PROVIDER_CSP,
                            (LPTSTR) &String,
                            &StringLen );

            if ( Result == SCARD_S_SUCCESS )
            {
                ScInfo.pszCryptoProvider = AllocAndDuplicateString( String );

                pSCardFreeMemory( SCContext, String );

                if ( !ScInfo.pszCryptoProvider )
                {
                    goto PostErrorExit ;
                }
            }
        }
    }

    //
    // Build the message and stick it in the queue:
    //
    Data = (PSC_DATA) LocalAlloc( LMEM_FIXED, sizeof( SC_DATA ) );

    if ( Data )
    {
        Data->ScInfo = ScInfo ;

        if ( ScAddEvent( EventType, Data ) )
        {
            PostMessage( pTerm->hwndSAS, 
                         WLX_WM_SAS, 
                         WLX_SAS_INTERNAL_SC_EVENT, 
                         0 );
            return ;
        }

        LocalFree( Data );
    }


PostErrorExit:

    if ( ScInfo.pszCard )
    {
        LocalFree( ScInfo.pszCard );
    }

    if ( ScInfo.pszContainer )
    {
        LocalFree( ScInfo.pszContainer );
    }

    if ( ScInfo.pszCryptoProvider )
    {
        LocalFree( ScInfo.pszCryptoProvider );
    }

    if ( ScInfo.pszReader )
    {
        LocalFree( ScInfo.pszReader );
    }

}


DWORD
WINAPI
SCWorkerThread(
    PVOID   Param
    )
{
    PSC_THREAD_CONTROL Control ;
    PTERMINAL pTerm ;
    PSCARD_READERSTATE ReaderStates = NULL;
    SCARDCONTEXT SCContext ;
    DWORD Result, i, MyTid, ReaderCount;
    LPWSTR ReaderNames = NULL, Scan;
    DWORD ReaderSize;
	BOOLEAN newReaderAvailable = FALSE;
    LPCTSTR  newPnPReader = TEXT("\\\\?PnP?\\Notification");
	HANDLE hCalaisStarted;


    Control = (PSC_THREAD_CONTROL) Param ;

    pTerm = Control->pTerm ;

    //
    // synch up with main thread.  If the main thread needs to
    // kill this thread, then it will reset the Tid stored in
    // the global data.  Since the call to the s/c resource mgr
    // is blocking, we check this frequently.  On startup, we will
    // spin until a non-zero tid is stored there.
    //

    MyTid = GetCurrentThreadId();

    while ( pTerm->SmartCardTid == 0 )
    {
        Sleep( 100 );
    }

    if ( pTerm->SmartCardTid != MyTid )
    {
        return 0 ;
    }

	__try {

		do {

			ResetInsertionMonitor = TRUE;
			newReaderAvailable = FALSE;
			SCContext = 0;

			Result = pSCardEstablishContext(
						SCARD_SCOPE_SYSTEM,
						NULL,
						NULL,
						&SCContext );

			if (Result != SCARD_S_SUCCESS || SCContext == 0) {

				__leave;
			}

			ReaderSize = SCARD_AUTOALLOCATE;
			Result = pSCardListReaders(
						SCContext,
						TEXT("SCard$AllReaders"),
						(LPTSTR) &ReaderNames,
						&ReaderSize );

			if ( pTerm->SmartCardTid != MyTid)
			{
				__leave;
			}

			if (Result != SCARD_S_SUCCESS) 
			{
				DebugLog(( DEB_ERROR, "Failed to list readers\n" ));
			}

			//
			// count number of readers in the system.
			// initially, we only have the 'PnP detection reader'
			//
			for (Scan = ReaderNames, ReaderCount = 1; 
				 Scan && ReaderSize != 0 && *Scan; 
				 Scan += (wcslen( Scan ) + 1)) {

				ReaderCount += 1;
			}

			ReaderStates = LocalAlloc(LPTR, sizeof(SCARD_READERSTATE) * ReaderCount);

			if (ReaderStates == NULL) {

				__leave;
			}

			ReaderStates[0].szReader = newPnPReader;
			ReaderStates[0].dwCurrentState = SCARD_STATE_UNAWARE;

			for (i = 1, Scan = ReaderNames; i < ReaderCount; i++) {

				DebugLog(( DEB_TRACE_SC, "Adding reader %ws\n", Scan ));

				ReaderStates[ i ].szReader = Scan ;
				ReaderStates[ i ].dwCurrentState = SCARD_STATE_UNAWARE;
				ReaderStates[ i ].pvUserData = NULL;

				Scan += (wcslen( Scan ) + 1);
			}

			Result = pSCardGetStatusChange(
						SCContext,
						1000,
						ReaderStates,
						ReaderCount );

			if ( Result != SCARD_S_SUCCESS )
			{
				__leave;
			}

			for ( i = 0 ; i < ReaderCount ; i++)
			{
				ReaderStates[ i ].dwCurrentState = ReaderStates[ i ].dwEventState ;
			}
			//
			// S/C array is now set up, start waiting for change notification
			//

			while ( pTerm->SmartCardTid == MyTid )
			{
				DebugLog(( DEB_TRACE_SC, "Waiting for next s/c event\n" ));

				Result = pSCardGetStatusChange(
							SCContext,
							INFINITE,
							ReaderStates,
							ReaderCount );

				DebugLog(( DEB_TRACE_SC, "SCardGetStatusChange returned %lx\n", Result ));

				if ( pTerm->SmartCardTid != MyTid )
				{
					__leave;
				}

				if (Result == SCARD_E_SYSTEM_CANCELLED) {

					// the sc system has been stopped
					DebugLog((DEB_WARN, "Smart card system stopped\n"));

					hCalaisStarted = pSCardAccessStartedEvent();

					if (hCalaisStarted == NULL) {

						// there is now way to recover. Just leave.
						__leave;             	
					}

					do
					{
						Result = WaitForSingleObjectEx(
							hCalaisStarted,
							15000,
							FALSE
							);         
            
						if ( pTerm->SmartCardTid != MyTid )
						{
							__leave;
						}

						DebugLog((DEB_WARN, "Waiting for smart card system...\n"));

					} while (Result == WAIT_TIMEOUT);

					//
					// the resource manager has been restarted
					// establish a new context and start over
					//
					DebugLog((DEB_WARN, "Smart card system re-started\n"));
					break;
				}

				if ( Result != SCARD_S_SUCCESS )
				{
					DebugLog(( DEB_ERROR, "SCardGetStatusChange returned %x\n", 
							   Result ));
					__leave;
				}

				if (ReaderStates[0].dwEventState & SCARD_STATE_CHANGED) 
				{
					DebugLog(( DEB_TRACE_SC, "New smart card reader available...\n" ));
					newReaderAvailable = TRUE;
					break;
				} 
				else 
				{
					for ( i = 1 ; i < ReaderCount ; i++ )
					{
						if ( (ReaderStates[ i ].dwEventState & 
							  SCARD_STATE_CHANGED ) )
						{
							//
							// This reader changed its state.  Could be an insertion,
							// could be a deletion.
							//

							DebugLog(( 
								DEB_TRACE_SC, 
								"Posting event for reader %ws\n", ReaderStates[i].szReader ));

							PostSmartCardEvent( pTerm,
												SCContext,
												&ReaderStates[ i ]
												);
						}

						ReaderStates[ i ].dwCurrentState = ReaderStates[ i ].dwEventState ;
					}
				}
			}

			if (ReaderStates != NULL) {

				LocalFree(ReaderStates);
				ReaderStates = NULL;
			}

			if (ReaderNames != NULL) 
			{
				pSCardFreeMemory( SCContext, ReaderNames );
				ReaderNames = NULL;
			}

			if (SCContext != 0) 
			{
				pSCardReleaseContext( SCContext );
				SCContext = 0;
			}

		} while (newReaderAvailable == TRUE);
	}
	__finally 
	{
		if (ReaderStates != NULL) {

			LocalFree(ReaderStates);
		}

		if (ReaderNames != NULL) 
		{
			pSCardFreeMemory( SCContext, ReaderNames );
		}

		if (SCContext != 0) 
		{
			pSCardReleaseContext( SCContext );
		}

		if ( pTerm->SmartCardTid == MyTid )
		{
			pTerm->SmartCardTid = 0 ;
		}
	}

    return 0 ;
}

VOID
ScThreadDeathCallback(
    PVOID Context,
    BOOLEAN Timeout
    )
{
    PSC_THREAD_CONTROL Control ;

    Control = (PSC_THREAD_CONTROL) Context ;

    NtClose( Control->Thread );

    UnregisterWait( Control->Callback );

}

VOID
StartScThread(
    PTERMINAL pTerm
    )
{
    PSC_THREAD_CONTROL Control ;

    //
    // If we failed to init due to low memory, don't worry about it.
    //

    if ( !ScEventList.Flink )
    {
        return ;
    }

    Control = (PSC_THREAD_CONTROL) LocalAlloc( LMEM_FIXED, 
                                               sizeof( SC_THREAD_CONTROL ) );

    if ( Control )
    {
        pTerm->SmartCardTid = 0 ;

        Control->pTerm = pTerm ;
        Control->Thread = CreateThread( NULL, 0,
                                        SCWorkerThread, Control,
                                        0, &pTerm->SmartCardTid );

        if ( Control->Thread )
        {
            RegisterWaitForSingleObject(
                &Control->Callback,
                Control->Thread,
                ScThreadDeathCallback,
                Control,
                INFINITE,
                WT_EXECUTEONLYONCE );

        }
        
    }

}


BOOL
IsSmartCardReaderPresent(
    PTERMINAL pTerm 
    )
{
    SCARDCONTEXT hCtx = 0;
    DWORD dwAutoAllocate = SCARD_AUTOALLOCATE;
    LPCSTR pReaders = NULL;
    LONG lResult;
    BOOL fRet = FALSE;

    // WinlogonInfoLevel = DEB_TRACE_SC | DEB_WARN | DEB_ERROR;

    if ( pTerm->SmartCardTid )
    {
        return TRUE ;
    }

    __try {

        HKEY hKey ;

        //
        // First check, if the service ever started.
        // this way we don't have to call any resource manager
        // function which could potentially start the service
        //
        lResult = RegOpenKeyEx(
            HKEY_LOCAL_MACHINE,
            CALAIS_PATH,
            0,
            KEY_READ,
            &hKey
            );

        if ( lResult != 0 ) {

            // the key does not exist which means 'no reader installed'
            leave;
        }

        RegCloseKey(hKey);
     	
		SetLogonReader(TEXT(""));

        if ( !SnapWinscard() )
        {
            leave ;
        }
     	
        lResult = pSCardEstablishContext(
            SCARD_SCOPE_SYSTEM,
            NULL,
            NULL,
            &hCtx
            );

        if (lResult != SCARD_S_SUCCESS) {
         	
            leave;
        }

        lResult = pSCardListReaders(
            hCtx,
            TEXT("SCard$DefaultReaders"),
            (LPTSTR) &pReaders,
            &dwAutoAllocate
            );

        if (lResult != SCARD_S_SUCCESS) {

            leave;
        }

        fRet = TRUE; 	
    }
    __finally {

        if (pReaders != NULL) {

            pSCardFreeMemory(hCtx, (PVOID) pReaders);
        }

        if (hCtx != 0) {

            pSCardReleaseContext(hCtx);
        }

    }

    DebugLog((
        DEB_TRACE_SC,
        "IsSmartCardReaderPresent: %s\n",
        (fRet ? "Smart card system is running" : "No smart card reader found"))
        );

    if (fRet && pTerm->SmartCardTid == 0)
    {
        StartScThread( pTerm );
    }

    return fRet;
}


BOOL
StartListeningForSC(
    PTERMINAL pTerm
    )
{
    NTSTATUS Status ;

    DebugLog(( DEB_TRACE_SC, "Start listening called\n" ));

    if ( IsSmartCardReaderPresent(pTerm))
    {
        pTerm->EnableSC = TRUE ;
        return TRUE ;
    }

    return FALSE ;
}

BOOL
StopListeningForSC(
    PTERMINAL pTerm
    )
{
    DebugLog(( DEB_TRACE_SC, "Stop listening called\n" ));

    pTerm->EnableSC = FALSE ;
    return TRUE ;
}
