/****************************** Module Header ******************************\
* Module Name: ddemlcli.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* DDE Manager main client side module
*
* Created: 10/3/91 Sanford Staab
*
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

// DDEML globals

PCL_INSTANCE_INFO pciiList = NULL;
RTL_CRITICAL_SECTION gcsDDEML;
#if DBG
PVOID gpDDEMLHeap;
#endif

/***************************************************************************\
* DdeInitialize (DDEML API)
*
* Description:
* Used two different ways:
* 1) First time call (*pidInst == 0) - causes a DDEML instance to be
* created for the calling process/thread. Creates a server side
* event window, server side instance structure, DDE Access Object,
* and client side instance structure. The callback function address
* and filter flags (afCmd) are placed into these structures.
* 2) Subsequent call (*pidInst == hInst) - updates filter flags in
* client and server side structures.
*
* History:
* 11-1-91 sanfords Created.
\***************************************************************************/
UINT DdeInitializeA(
LPDWORD pidInst,
PFNCALLBACK pfnCallback,
DWORD afCmd,
DWORD ulRes)
{
    if (ulRes != 0) {
        return (DMLERR_INVALIDPARAMETER);
    }
    return (InternalDdeInitialize(pidInst, pfnCallback, afCmd, 0));
}


UINT DdeInitializeW(
LPDWORD pidInst,
PFNCALLBACK pfnCallback,
DWORD afCmd,
DWORD ulRes)
{
    if (ulRes != 0) {
        return (DMLERR_INVALIDPARAMETER);
    }
    return (InternalDdeInitialize(pidInst, pfnCallback, afCmd, 1));
}


UINT InternalDdeInitialize(
LPDWORD pidInst,
PFNCALLBACK pfnCallback,
DWORD afCmd,
BOOL fUnicode)
{
    UINT uiRet = DMLERR_MEMORY_ERROR;
    register PCL_INSTANCE_INFO pcii;

    if (afCmd & APPCLASS_MONITOR) {
        afCmd |= CBF_MONMASK;
    }

    if (afCmd & APPCMD_CLIENTONLY) {
        afCmd |= CBF_FAIL_CONNECTIONS;
    }

    EnterDDECrit;

    if (*pidInst != 0) {
        pcii = ValidateInstance((HANDLE)LongToHandle( *pidInst ));
        if (pcii == NULL) {
            uiRet = DMLERR_INVALIDPARAMETER;
            goto Exit;
        }

        // only allow certain bits to be changed on reinitialize call

        pcii->afCmd = (pcii->afCmd & ~(CBF_MASK | MF_MASK)) |
                (afCmd & (CBF_MASK | MF_MASK));

        LeaveDDECrit;
        NtUserUpdateInstance(pcii->hInstServer, &pcii->MonitorFlags, afCmd);
        return (DMLERR_NO_ERROR);
    }

    pcii = (PCL_INSTANCE_INFO)DDEMLAlloc(sizeof(CL_INSTANCE_INFO));
    if (pcii == NULL) {
        uiRet = DMLERR_MEMORY_ERROR;
        goto Exit;
    }

    pcii->plaNameService = (LATOM *)DDEMLAlloc(sizeof(LATOM));
    if (pcii->plaNameService == NULL) {
        uiRet = DMLERR_MEMORY_ERROR;
        goto Backout3;
    }
    // *pcii->plaNameService = 0; // zero init takes care of this
    pcii->cNameServiceAlloc = 1;


    /*
     * Flag this window as being create from a diff hmod as the app so
     * hotkeys don't take it as the first window created in the app and
     * assign it as the hotkey.
     */
    pcii->hwndMother =  _CreateWindowEx(0, (LPTSTR)(gpsi->atomSysClass[ICLS_DDEMLMOTHER]), L"",
            WS_POPUP, 0, 0, 0, 0, (HWND)0,
            (HMENU)0, 0, (LPVOID)NULL, CW_FLAGS_DIFFHMOD);

    if (pcii->hwndMother == 0) {
        uiRet = DMLERR_SYS_ERROR;
        goto Backout2;
    }
    SetWindowLongPtr(pcii->hwndMother, GWLP_INSTANCE_INFO, (LONG_PTR)pcii);

    pcii->afCmd = afCmd | APPCMD_FILTERINITS;
    pcii->pfnCallback = pfnCallback;
    // pcii->LastError = DMLERR_NO_ERROR; // zero init
    pcii->tid = GetCurrentThreadId();
    // pcii->aServerLookup = NULL;          // zero init
    // pcii->cServerLookupAlloc = 0;        // zero init
    // pcii->ConvStartupState = 0;          // zero init - Not blocked.
    // pcii->flags = 0;                     // zero init
    // pcii->cInDDEMLCallback = 0;          // zero init
    // pcii->pLinkCounts = NULL;            // zero init

    // Do this last when the client side is ready for whatever events
    // flying around may come charging in.

    LeaveDDECrit;
    uiRet = NtUserDdeInitialize(&pcii->hInstServer,
                            &pcii->hwndEvent,
                            &pcii->MonitorFlags,
                            pcii->afCmd,
                            pcii);
    EnterDDECrit;

    if (uiRet != DMLERR_NO_ERROR) {
Backout:
        NtUserDestroyWindow(pcii->hwndMother);
Backout2:
        DDEMLFree(pcii->plaNameService);
Backout3:
        DDEMLFree(pcii);
        goto Exit;
    }
    pcii->hInstClient = AddInstance(pcii->hInstServer);
    *pidInst = HandleToUlong(pcii->hInstClient);
    if (pcii->hInstClient == 0) {
        LeaveDDECrit;
        NtUserCallOneParam((ULONG_PTR)pcii->hInstServer, SFI__CSDDEUNINITIALIZE);
        EnterDDECrit;
        uiRet = DMLERR_MEMORY_ERROR;
        goto Backout;
    }
    SetHandleData(pcii->hInstClient, (ULONG_PTR)pcii);

    pcii->next = pciiList;
    pciiList = pcii;
    if (fUnicode) {
        pcii->flags |= IIF_UNICODE;
    }
    uiRet = DMLERR_NO_ERROR;

Exit:
    LeaveDDECrit;
    return (uiRet);
}



/***************************************************************************\
* DdeUninitialize (DDEML API)
*
* Description:
* Shuts down a DDEML instance and frees all associated resources.
*
* History:
* 11-12-91 sanfords Created.
\***************************************************************************/
BOOL DdeUninitialize(
DWORD idInst)
{
    PCL_INSTANCE_INFO pcii, pciiPrev;
    BOOL fRet = FALSE;

    CheckDDECritOut;
    EnterDDECrit;

    pcii = ValidateInstance((HANDLE)LongToHandle( idInst ));
    if (pcii == NULL) {
        BestSetLastDDEMLError(DMLERR_INVALIDPARAMETER);
        goto Exit;
    }

    /*
     * If this thread is in the middle of a synchronous transaction or
     * a callback, we need to back out of those first.
     */
    if ((pcii->flags & IIF_IN_SYNC_XACT) || pcii->cInDDEMLCallback) {
        pcii->afCmd |= APPCMD_UNINIT_ASAP;
        fRet = TRUE;
        goto Exit;
    }

    ApplyFunctionToObjects(HTYPE_CONVERSATION_LIST, InstFromHandle(pcii->hInstClient),
            (PFNHANDLEAPPLY)DdeDisconnectList);
    ApplyFunctionToObjects(HTYPE_CLIENT_CONVERSATION, InstFromHandle(pcii->hInstClient),
            (PFNHANDLEAPPLY)DdeDisconnect);
    ApplyFunctionToObjects(HTYPE_SERVER_CONVERSATION, InstFromHandle(pcii->hInstClient),
            (PFNHANDLEAPPLY)DdeDisconnect);
    ApplyFunctionToObjects(HTYPE_ZOMBIE_CONVERSATION, InstFromHandle(pcii->hInstClient),
            (PFNHANDLEAPPLY)WaitForZombieTerminate);
    ApplyFunctionToObjects(HTYPE_DATA_HANDLE, InstFromHandle(pcii->hInstClient),
            (PFNHANDLEAPPLY)ApplyFreeDataHandle);

    LeaveDDECrit;
    NtUserCallOneParam((ULONG_PTR)pcii->hInstServer, SFI__CSDDEUNINITIALIZE);
    NtUserDestroyWindow(pcii->hwndMother);
    EnterDDECrit;

    DDEMLFree(pcii->plaNameService);
    DestroyInstance(pcii->hInstClient);

    // unlink pcii from pciiList

    if (pciiList == pcii) {
        pciiList = pciiList->next;
    } else {
        for (pciiPrev = pciiList; pciiPrev != NULL && pciiPrev->next != pcii;
                pciiPrev = pciiPrev->next) {
            ;
        }
        if (pciiPrev != NULL) {
            pciiPrev->next = pcii->next;
        }
    }
    DDEMLFree(pcii);
    fRet = TRUE;

Exit:
    LeaveDDECrit;
    return (fRet);
}



/***************************************************************************\
* DdeNameService (DDEML API)
*
* Description:
* Registers, and Unregisters service names and sets the Initiate filter
* state for an instance.
*
* History:
* 11-1-91 sanfords Created.
\***************************************************************************/
HDDEDATA DdeNameService(
DWORD idInst,
HSZ hsz1, // service name
HSZ hsz2, // reserved for future enhancements
UINT afCmd) // DNS_ flags.
{
    BOOL fRet = TRUE;
    LATOM *plaNameService;
    PCL_INSTANCE_INFO pcii;

    EnterDDECrit;

    pcii = ValidateInstance((HANDLE)LongToHandle( idInst ));
    if (pcii == NULL) {
        BestSetLastDDEMLError(DMLERR_INVALIDPARAMETER);
        fRet = FALSE;
        goto Exit;
    }

    if ((hsz1 && ValidateHSZ(hsz1) == HSZT_INVALID) || hsz2 != 0) {
        SetLastDDEMLError(pcii, DMLERR_INVALIDPARAMETER);
        fRet = FALSE;
        goto Exit;
    }

    if (afCmd & DNS_FILTERON && !(pcii->afCmd & APPCMD_FILTERINITS)) {
        pcii->afCmd |= APPCMD_FILTERINITS;
        NtUserUpdateInstance(pcii->hInstServer, &pcii->MonitorFlags, pcii->afCmd);
    }
    if (afCmd & DNS_FILTEROFF && (pcii->afCmd & APPCMD_FILTERINITS)) {
        pcii->afCmd &= ~APPCMD_FILTERINITS;
        NtUserUpdateInstance(pcii->hInstServer, &pcii->MonitorFlags, pcii->afCmd);
    }

    if (afCmd & (DNS_REGISTER | DNS_UNREGISTER)) {
        GATOM ga;

        if (pcii->afCmd & APPCMD_CLIENTONLY) {
            SetLastDDEMLError(pcii, DMLERR_DLL_USAGE);
            fRet = FALSE;
            goto Exit;
        }

        if (hsz1 == 0) {
            if (afCmd & DNS_REGISTER) {

                /*
                 * registering NULL is not allowed!
                 */
                SetLastDDEMLError(pcii, DMLERR_INVALIDPARAMETER);
                fRet = FALSE;
                goto Exit;
            }

            /*
             * unregistering NULL is just like unregistering each
             * registered name.
             *
             * 10/19/90 - made this a synchronous event so that hsz
             * can be freed by calling app after this call completes
             * without us having to keep a copy around forever.
             */
            plaNameService = pcii->plaNameService;
            while (*plaNameService != 0) {
                ga = LocalToGlobalAtom(*plaNameService);
                DeleteAtom(*plaNameService);
                LeaveDDECrit;
                RegisterService(FALSE, ga, pcii->hwndMother);
                EnterDDECrit;
                GlobalDeleteAtom(ga);
                plaNameService++;
            }
            pcii->cNameServiceAlloc = 1;
            *pcii->plaNameService = 0;
            goto Exit;
        }

        if (afCmd & DNS_REGISTER) {
            plaNameService = (LATOM *)DDEMLReAlloc(pcii->plaNameService,
                    sizeof(LATOM) * ++pcii->cNameServiceAlloc);
            if (plaNameService == NULL) {
                SetLastDDEMLError(pcii, DMLERR_MEMORY_ERROR);
                pcii->cNameServiceAlloc--;
                fRet = FALSE;
                goto Exit;
            } else {
                pcii->plaNameService = plaNameService;
            }
            IncLocalAtomCount(LATOM_FROM_HSZ(hsz1)); // NameService copy
            plaNameService[pcii->cNameServiceAlloc - 2] = LATOM_FROM_HSZ(hsz1);
            plaNameService[pcii->cNameServiceAlloc - 1] = 0;

        } else { // DNS_UNREGISTER
            plaNameService = pcii->plaNameService;
            while (*plaNameService != 0 && *plaNameService != LATOM_FROM_HSZ(hsz1)) {
                plaNameService++;
            }
            if (*plaNameService == 0) {
                goto Exit; // not found just exit
            }
            //
            // fill empty slot with last entry and fill last entry with 0
            //
            pcii->cNameServiceAlloc--;
            *plaNameService = pcii->plaNameService[pcii->cNameServiceAlloc - 1];
            pcii->plaNameService[pcii->cNameServiceAlloc - 1] = 0;
        }

        ga = LocalToGlobalAtom(LATOM_FROM_HSZ(hsz1));
        LeaveDDECrit;
        RegisterService((afCmd & DNS_REGISTER) ? TRUE : FALSE, ga,
            pcii->hwndMother);
        EnterDDECrit;
        GlobalDeleteAtom(ga);
    }

Exit:
    LeaveDDECrit;
    return ((HDDEDATA)IntToPtr( fRet ));
}



/***************************************************************************\
* DdeGetLastError (DDEML API)
*
* Description:
* Returns last error code set for the instance given.
*
* History:
* 11-12-91 sanfords Created.
\***************************************************************************/
UINT DdeGetLastError(
DWORD idInst)
{
    UINT uiRet = 0;
    PCL_INSTANCE_INFO pcii;

    EnterDDECrit;

    pcii = ValidateInstance((HANDLE)LongToHandle( idInst ));
    if (pcii == NULL) {
        uiRet = DMLERR_INVALIDPARAMETER;
        goto Exit;
    }
    uiRet = pcii->LastError;
    pcii->LastError = DMLERR_NO_ERROR;

Exit:
    LeaveDDECrit;
    return (uiRet);
}



/***************************************************************************\
* DdeImpersonateClient()
*
* Description:
*   Does security impersonation for DDEML server apps.
*   This API should only be called with server side hConvs;
*
* History:
* 5-4-92 sanfords Created.
\***************************************************************************/
BOOL DdeImpersonateClient(
HCONV hConv)
{
    PCONV_INFO pcoi;
    PCL_INSTANCE_INFO pcii;
    BOOL fRet = FALSE;

    EnterDDECrit;

    pcoi = (PCONV_INFO)ValidateCHandle((HANDLE)hConv,
            HTYPE_SERVER_CONVERSATION, HINST_ANY);
    if (pcoi == NULL) {
        BestSetLastDDEMLError(DMLERR_INVALIDPARAMETER);
        goto Exit;
    }
    pcii = PciiFromHandle((HANDLE)hConv);
    if (pcii == NULL) {
        BestSetLastDDEMLError(DMLERR_INVALIDPARAMETER);
        goto Exit;
    }

    fRet = NtUserImpersonateDdeClientWindow(pcoi->hwndPartner, pcoi->hwndConv);
Exit:
    LeaveDDECrit;
    return (fRet);
}
