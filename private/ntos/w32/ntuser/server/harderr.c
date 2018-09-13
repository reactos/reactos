/**************************** Module Header ********************************\
* Module Name: harderr.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Hard error handler
*
* History:
* 07-03-91 JimA                Created scaffolding.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop
#include "ntlpcapi.h"

#include <winsta.h>
extern BOOLEAN gbExitInProgress;
BOOL gfTimedOut = FALSE;

VOID UserHardErrorEx(
    PCSR_THREAD pt,
    PHARDERROR_MSG pmsg,
    PCTXHARDERRORINFO pCtxHEInfo);

VOID UserExitWorkerThread(VOID);

VOID ProcessHardErrorRequest(
    BOOL fNewThread);


CONST UINT wIcons[] = {
    0,
    MB_ICONINFORMATION,
    MB_ICONEXCLAMATION,
    MB_ICONSTOP
};
CONST UINT wOptions[] = {
    MB_ABORTRETRYIGNORE,
    MB_OK,
    MB_OKCANCEL,
    MB_RETRYCANCEL,
    MB_YESNO,
    MB_YESNOCANCEL,
    MB_OK,              // OptionShutdownSystem
    MB_OK,              // OptionOkNoWait
    MB_CANCELTRYCONTINUE
};
CONST DWORD dwResponses[] = {
    ResponseNotHandled, // MessageBox error
    ResponseOk,         // IDOK
    ResponseCancel,     // IDCANCEL
    ResponseAbort,      // IDABORT
    ResponseRetry,      // IDRETRY
    ResponseIgnore,     // IDIGNORE
    ResponseYes,        // IDYES
    ResponseNo,         // IDNO
    ResponseNotHandled, // Error as IDCLOSE can't show up
    ResponseNotHandled, // error as IDHELP can't show up
    ResponseTryAgain,   // IDTRYAGAIN
    ResponseContinue    // IDCONTINUE
};
CONST DWORD dwResponseDefault[] = {
    ResponseAbort,      // OptionAbortRetryIgnore
    ResponseOk,         // OptionOK
    ResponseOk,         // OptionOKCancel
    ResponseCancel,     // OptionRetryCancel
    ResponseYes,        // OptionYesNo
    ResponseYes,        // OptionYesNoCancel
    ResponseOk,         // OptionShutdownSystem
    ResponseOk,         // OptionOKNoWait
    ResponseCancel      // OptionCancelTryContinue
};

/*
 *  Global timer id
 */
static UINT_PTR gTimerId = 0;

/*
 *  Citrix SendMessage entry point to harderror handler and cleanup routine
 */
VOID HardErrorInsert(PCSR_THREAD, PHARDERROR_MSG, PCTXHARDERRORINFO);
VOID HardErrorRemove(PCTXHARDERRORINFO);

FARPROC gfnRegisterEventSource;
FARPROC gfnDeregisterEventSource;
FARPROC gfnReportEvent;

/***************************************************************************\
* UserRegisterEventSource
*
* Dynamically link to the event functions in advapi32.dll and register the
* event source.
*
* History:
* 04-13-98 JerrySh      Created.
\***************************************************************************/
HANDLE
UserRegisterEventSource(
    PCWSTR pwszSourceName)
{
    /*
     * If we haven't already dynamically linked to advadpi32.dll, do it now.
     */
    if (gfnRegisterEventSource == NULL) {
        HINSTANCE hAdvApiDll;
        FARPROC fnRegisterEventSource;
        FARPROC fnDeregisterEventSource;
        FARPROC fnReportEvent;

        /*
         * Try to load the DLL and function pointers.
         */
        if ((hAdvApiDll = LoadLibrary(L"advapi32.dll")) == NULL) {
            return NULL;
        }
        fnRegisterEventSource = GetProcAddress(hAdvApiDll, "RegisterEventSourceW");
        fnDeregisterEventSource = GetProcAddress(hAdvApiDll, "DeregisterEventSource");
        fnReportEvent = GetProcAddress(hAdvApiDll, "ReportEventW");
        if (!fnRegisterEventSource || !fnDeregisterEventSource || !fnReportEvent) {
            FreeLibrary(hAdvApiDll);
            return NULL;
        }

        /*
         * Update the global function pointers if they're not set already.
         */
        EnterCrit();
        if (gfnRegisterEventSource == NULL) {
            gfnReportEvent = fnReportEvent;
            gfnDeregisterEventSource = fnDeregisterEventSource;
            // This must be last since we test it above
            gfnRegisterEventSource = fnRegisterEventSource;
            hAdvApiDll = NULL;
        }
        LeaveCrit();

        /*
         * If another thread beat us to it, free the library.
         */
        if (hAdvApiDll) {
            FreeLibrary(hAdvApiDll);
        }
    }

    /*
     * Let's be paranoid and verify we loaded everything correctly.
     */
    UserAssert(gfnRegisterEventSource != NULL);
    UserAssert(gfnDeregisterEventSource != NULL);
    UserAssert(gfnReportEvent != NULL);

    /*
     * Call the real function.
     */
    return (HANDLE)gfnRegisterEventSource(NULL, pwszSourceName);
}

/***************************************************************************\
* LogErrorPopup
*
* History:
* 09-22-97 GerardoB     Added Header
\***************************************************************************/
VOID
LogErrorPopup(
    IN LPWSTR Caption,
    IN LPWSTR Message
    )
{

    LPWSTR lps[2];

    lps[0] = Caption;
    lps[1] = Message;

    UserAssert(gEventSource != NULL);
    gfnReportEvent(gEventSource, EVENTLOG_INFORMATION_TYPE, 0,
        STATUS_LOG_HARD_ERROR, NULL, sizeof(lps) / sizeof(*lps),
        0, lps, NULL);
}

/***************************************************************************\
* SubstituteDeviceName
*
* History:
* 09-22-97 GerardoB     Added Header
\***************************************************************************/
static WCHAR wszDosDevices[] = L"\\??\\A:";
VOID
SubstituteDeviceName(
    PUNICODE_STRING InputDeviceName,
    LPSTR OutputDriveLetter
    )
{
    UNICODE_STRING LinkName;
    UNICODE_STRING DeviceName;
    OBJECT_ATTRIBUTES Obja;
    HANDLE LinkHandle;
    NTSTATUS Status;
    ULONG i;
    PWCHAR p;
    WCHAR DeviceNameBuffer[MAXIMUM_FILENAME_LENGTH];

    RtlInitUnicodeString(&LinkName,wszDosDevices);
    p = wszDosDevices + ARRAY_SIZE(wszDosDevices) - ARRAY_SIZE(L"A:");
    for(i=0;i<26;i++){
        *p = (WCHAR)('A' + i);

        InitializeObjectAttributes(
            &Obja,
            &LinkName,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL
            );
        Status = NtOpenSymbolicLinkObject(
                    &LinkHandle,
                    SYMBOLIC_LINK_QUERY,
                    &Obja
                    );
        if (NT_SUCCESS( Status )) {

            //
            // Open succeeded, Now get the link value
            //

            DeviceName.Length = 0;
            DeviceName.MaximumLength = sizeof(DeviceNameBuffer);
            DeviceName.Buffer = DeviceNameBuffer;

            Status = NtQuerySymbolicLinkObject(
                        LinkHandle,
                        &DeviceName,
                        NULL
                        );
            NtClose(LinkHandle);
            if ( NT_SUCCESS(Status) ) {
                if ( RtlEqualUnicodeString(InputDeviceName,&DeviceName,TRUE) ) {
                    OutputDriveLetter[0]=(CHAR)('A'+i);
                    OutputDriveLetter[1]=':';
                    OutputDriveLetter[2]='\0';
                    return;
                    }
                }
            }
        }
}
/***************************************************************************\
* GetErrorMode
*
* History:
* 09-22-97 GerardoB     Added Header
\***************************************************************************/
DWORD GetErrorMode(VOID)
{
    HANDLE hKey;
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES OA;
    LONG Status;
    BYTE Buf[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(DWORD)];
    DWORD cbSize;
    DWORD dwRet = 0;

    RtlInitUnicodeString(&UnicodeString,
            L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Windows");
    InitializeObjectAttributes(&OA, &UnicodeString, OBJ_CASE_INSENSITIVE, NULL, NULL);

    Status = NtOpenKey(&hKey, KEY_READ, &OA);
    if (NT_SUCCESS(Status)) {
        RtlInitUnicodeString(&UnicodeString, L"ErrorMode");
        Status = NtQueryValueKey(hKey,
                &UnicodeString,
                KeyValuePartialInformation,
                (PKEY_VALUE_PARTIAL_INFORMATION)Buf,
                sizeof(Buf),
                &cbSize);
        if (NT_SUCCESS(Status)) {
            dwRet = *((PDWORD)((PKEY_VALUE_PARTIAL_INFORMATION)Buf)->Data);
        }
        NtClose(hKey);
    }
    return dwRet;
}
/***************************************************************************\
* FreePhi
*
* History:
* 09-18-97 GerardoB     Created
\***************************************************************************/
void FreePhi (PHARDERRORINFO phi)
{
    if (phi->dwHEIFFlags & HEIF_ALLOCATEDMSG) {
        LocalFree(phi->pmsg);
    }

    RtlFreeUnicodeString(&phi->usText);
    RtlFreeUnicodeString(&phi->usCaption);

    LocalFree(phi);
}
/***************************************************************************\
* ReplyHardError
*
* This function is called when we are done with a hard error.
*
* History:
* 03-11-97 GerardoB     Created
\***************************************************************************/
void ReplyHardError (PHARDERRORINFO phi, DWORD dwResponse)
{
    phi->pmsg->Response = dwResponse;
    /*
     * Signal the event if any. If not, reply if we haven't done so
     *  already.
     */
    if (phi->hEventHardError != NULL) {
        NtSetEvent(phi->hEventHardError, NULL);
    } else if (!(phi->dwHEIFFlags & HEIF_REPLIED)) {
        NtReplyPort(((PCSR_THREAD)phi->pthread)->Process->ClientPort,
                    (PPORT_MESSAGE)phi->pmsg);
    }
    /*
     * If we had locked the thread or were holding the client port,
     *   then let it go now.
     */
    if (phi->dwHEIFFlags & HEIF_DEREFTHREAD) {
        CsrDereferenceThread(phi->pthread);
    }
    /*
     * We're done with this dude
     */
    FreePhi(phi);
}
/***************************************************************************\
* CheckDefaultDesktop
*
* This function is called by the HardErrorHandler when it's notified
*  that we've switched desktops or upon waking up.
*  If we're on the default desktop now, then we clear the HEIF_WRONGDESKTOP
*  flag; this flag is set when we find a MB_DEFAULT_DESKTOP_ONLY request but
*  we are not in the right (default) desktop.
*
* History:
* 06-02-97 GerardoB     Created
\***************************************************************************/
void CheckDefaultDesktop(void)
{
    PHARDERRORINFO phi;

    if (HEC_WRONGDESKTOP == NtUserHardErrorControl(HardErrorInDefDesktop, NULL, NULL)) {
        return;
    }

    EnterCrit();
    phi = gphiList;
    while (phi != NULL) {
        phi->dwHEIFFlags &= ~HEIF_WRONGDESKTOP;
        phi = phi->phiNext;
    }
    LeaveCrit();
}

/*****************************************************************************
 *
 *  MsgBoxTimerFunc
 *
 *  This function gets called when a timer fires on a GUI message box
 *  is up that has a timeout set for it.
 *
 *  This function cancels the message box by posting a WM_QUIT into the
 *  message queue of the Citrix Message box thread.
 *
 * ENTRY:
 *
 *
 * EXIT:
 *
 *
 ****************************************************************************/

VOID
MsgBoxTimerFunc(
    HWND  hWnd,
    UINT  MessageType,
    UINT_PTR idEvent,
    DWORD TimeWhenCalled
    )
{
    BOOL RetVal;

    RIPMSG0(RIP_WARNING, "MsgBoxTimerFunc: timer poped");

    //
    // Post a WM_QUIT message into the messagebox threads queue
    //
    if ( gdwHardErrorThreadId != 0 ) {
        gfTimedOut = TRUE;
        RetVal = PostThreadMessage(gdwHardErrorThreadId, WM_QUIT, 0, 0);
    }

    if(- !RetVal ) {
        RIPMSG0(RIP_WARNING, "MsgBoxTimerFunc: PostMessageFailed");
    }

    UNREFERENCED_PARAMETER(hWnd);
    UNREFERENCED_PARAMETER(MessageType);
    UNREFERENCED_PARAMETER(idEvent);
    UNREFERENCED_PARAMETER(TimeWhenCalled);
}

/***************************************************************************\
* GetHardErrorText
*
* This function figures out the message box title, text and flags.
* We want to do this up front so we can log this error when the hard error is
*  raised. Previously we used to log it after the user had dismissed the message
*  box -- but that was not when the error occurred (DCR Bug 107590)
*
* History:
* 09-18-97 GerardoB     Extracted (and cleaned up) from HardErrorHandler
\***************************************************************************/
void GetHardErrorText (PHARDERRORINFO phi)
{
    static WCHAR wszUnkownSoftwareException [] = L"unknown software exception";
    static WCHAR wszException [] = L"{EXCEPTION}";
    static WCHAR wszUnknownHardError [] = L"Unknown Hard Error";
    ANSI_STRING asLocal, asMessage;
    BOOL fFreeAppNameBuffer, fFreeCaption;
    BOOL fResAllocated, fResAllocated1, fErrorIsFromSystem;
    WCHAR wszErrorMessage[WSPRINTF_LIMIT + 1];
    DWORD dwCounter, dwStringsToFreeMask, dwMBFlags;
    ULONG_PTR adwParameterVector[MAXIMUM_HARDERROR_PARAMETERS];
    HANDLE hClientProcess;
    HWND hwndOwner;
    NTSTATUS Status;
    PHARDERROR_MSG phemsg;
    PMESSAGE_RESOURCE_ENTRY MessageEntry;
    PWSTR pwszCaption, pwszFormatString;
    PWSTR pwszAppName, pwszResBuffer, pwszResBuffer1;
    PWSTR pwszMsg, pwszTitle, pwszFullCaption;
    UINT uMsgLen, uCaptionLen, uTitleLen;
    UNICODE_STRING usScratch, usLocal, usMessage, usCaption;

    /*
     * Initialize working variables
     */
    fFreeAppNameBuffer = fFreeCaption = FALSE;
    hClientProcess = NULL;
    RtlInitUnicodeString(&usCaption, NULL);
    RtlInitUnicodeString(&usMessage, NULL);
    /*
     * Initialize response in case something goes wrong
     */
    phemsg = phi->pmsg;
    phemsg->Response = ResponseNotHandled;
    /*
     * Make a copy of the parameters. Initialize unused ones to point to empty
     *  strings (in case we expect a string there).
     */
    UserAssert(phemsg->NumberOfParameters <= MAXIMUM_HARDERROR_PARAMETERS);
    RtlCopyMemory(adwParameterVector, phemsg->Parameters, phemsg->NumberOfParameters * sizeof(*phemsg->Parameters));
    dwCounter = phemsg->NumberOfParameters;
    while (dwCounter < MAXIMUM_HARDERROR_PARAMETERS) {
        adwParameterVector[dwCounter++] = (ULONG_PTR)L"";
    }
    /*
     * Open the client process so we can read the strings parameters, process
     *  name, etc., from its address space
     */
    hClientProcess = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION,
                                FALSE, HandleToUlong(phemsg->h.ClientId.UniqueProcess));
    fErrorIsFromSystem = (hClientProcess == NULL);
    /*
     * If there are unicode strings, then we need to
     * convert them to ansi and store them in the
     * parameter vector
     */
    dwStringsToFreeMask = 0;
    if (phemsg->UnicodeStringParameterMask) {

        for (dwCounter = 0; dwCounter < phemsg->NumberOfParameters; dwCounter++) {
            /*
             * if there is no string in this position, continue
             */
            if (!(phemsg->UnicodeStringParameterMask & (1 << dwCounter))) {
                continue;
            }
            /*
             * Point to an empty string in case we don't have
             * a client to read from or something fails later on.
             */
            adwParameterVector[dwCounter] = (ULONG_PTR)L"";
            if (hClientProcess == NULL) {
                continue;
            }

            Status = NtReadVirtualMemory(hClientProcess,
                            (PVOID)phemsg->Parameters[dwCounter],
                            (PVOID)&usScratch,
                             sizeof(usScratch), NULL);

            if (!NT_SUCCESS(Status)) {
                RIPMSG0(RIP_WARNING, "Failed to read error string struct!");
                continue;
            }

            usLocal = usScratch;
            usLocal.Buffer = (PWSTR)LocalAlloc(LMEM_ZEROINIT, usLocal.Length + sizeof(UNICODE_NULL));
            if (usLocal.Buffer == NULL) {
                RIPMSG0(RIP_WARNING, "Failed to alloc string buffer!");
                continue;
            }

            Status = NtReadVirtualMemory(hClientProcess,
                            (PVOID)usScratch.Buffer,
                            (PVOID)usLocal.Buffer,
                            usLocal.Length,
                            NULL);

            if (!NT_SUCCESS(Status)) {
                LocalFree(usLocal.Buffer);
                RIPMSG0(RIP_WARNING, "Failed to read error string!");
                continue;
            }

            usLocal.MaximumLength = usLocal.Length;
            Status = RtlUnicodeStringToAnsiString(&asLocal, &usLocal, TRUE);
            if (!NT_SUCCESS(Status)) {
                LocalFree(usLocal.Buffer);
                RIPMSG0(RIP_WARNING, "Failed to translate error string!");
                continue;
            }

            /*
             * check to see if string contains an NT
             * device name. If so, then attempt a
             * drive letter substitution
             */

            if (strstr(asLocal.Buffer,"\\Device") == asLocal.Buffer) {
                SubstituteDeviceName(&usLocal,asLocal.Buffer);
            } else if ((asLocal.Length > 4) && !_strnicmp(asLocal.Buffer, "\\??\\", 4)) {
                strcpy( asLocal.Buffer, asLocal.Buffer+4 );
                asLocal.Length -= 4;
            } else {
                /*
                 * Processing some status code doesn't require ansi strings.
                 * Since no substitution took place, let's ignore the translation
                 * to avoid losing chars -- incorrect code page translation
                 */
                switch (phemsg->Status) {
                    case STATUS_SERVICE_NOTIFICATION:
                    case STATUS_VDM_HARD_ERROR:
                        adwParameterVector[dwCounter] = (ULONG_PTR)usLocal.Buffer;
                        RtlFreeAnsiString(&asLocal);
                        continue;
                }

            }

            LocalFree(usLocal.Buffer);

            dwStringsToFreeMask |= (1 << dwCounter);
            adwParameterVector[dwCounter] = (ULONG_PTR)asLocal.Buffer;

        } /* for (dwCounter... */

    } /* if (phemsg->UnicodeStringParameterMask) */

    /*
     * Read additional MB flags, if provided.
     */
#if (HARDERROR_PARAMETERS_FLAGSPOS >= MAXIMUM_HARDERROR_PARAMETERS)
#error Invalid HARDERROR_PARAMETERS_FLAGSPOS value.
#endif
#if (HARDERROR_FLAGS_DEFDESKTOPONLY != MB_DEFAULT_DESKTOP_ONLY)
#error Invalid HARDERROR_FLAGS_DEFDESKTOPONLY
#endif
    dwMBFlags = 0;
    if (phemsg->NumberOfParameters > HARDERROR_PARAMETERS_FLAGSPOS) {
        /*
         * Currently we only use MB_DEFAULT_DESKTOP_ONLY
         */
        UserAssert(!(adwParameterVector[HARDERROR_PARAMETERS_FLAGSPOS] & ~MB_DEFAULT_DESKTOP_ONLY));
        if (adwParameterVector[HARDERROR_PARAMETERS_FLAGSPOS] & MB_DEFAULT_DESKTOP_ONLY) {
            dwMBFlags |= MB_DEFAULT_DESKTOP_ONLY;
        }
    }
    /*
     * For some status codes, all MessageBox parameters are provided in the HardError parameters
     */
    switch (phemsg->Status) {
        case STATUS_SERVICE_NOTIFICATION:
            if (phemsg->UnicodeStringParameterMask & 0x1) {
                RtlInitUnicodeString(&usMessage, (PWSTR)adwParameterVector[0]);
            } else {
                RtlInitAnsiString(&asMessage, (PSTR)adwParameterVector[0]);
                RtlAnsiStringToUnicodeString(&usMessage, &asMessage, TRUE);
            }

            if (phemsg->UnicodeStringParameterMask & 0x2) {
                RtlInitUnicodeString(&usCaption, (PWSTR)adwParameterVector[1]);
            } else {
                RtlInitAnsiString(&asMessage, (PSTR)adwParameterVector[1]);
                RtlAnsiStringToUnicodeString(&usCaption, &asMessage, TRUE);
            }

            dwMBFlags = (DWORD)adwParameterVector[2] & ~MB_SERVICE_NOTIFICATION;
            goto CleanUpAndSaveParams;

        case STATUS_VDM_HARD_ERROR:
            /*
             * Parameters[0] = (fForWOW << 16) | wBtn1;
             * Parameters[1] = (wBtn2   << 16) | wBtn3;
             * Parameters[2] = (DWORD) szTitle;
             * Parameters[3] = (DWORD) szMessage;
             */
            phi->dwHEIFFlags |= HEIF_VDMERROR;
            /*
             * Save VDM's Button(s) info to be used later.
             */
            phi->dwVDMParam0 = (DWORD)adwParameterVector[0];
            phi->dwVDMParam1 = (DWORD)adwParameterVector[1];
            /*
             * Get caption and text.
             */
            try {
                if (phemsg->UnicodeStringParameterMask & 0x4) {
                    RtlInitUnicodeString(&usCaption, (PWSTR)adwParameterVector[2]);
                } else {
                    MBToWCS((LPSTR)adwParameterVector[2], -1, &pwszTitle, -1, TRUE);
                    RtlCreateUnicodeString(&usCaption, pwszTitle);
                    RtlFreeHeap(RtlProcessHeap(), 0, pwszTitle);
                }

                if (phemsg->UnicodeStringParameterMask & 0x8) {
                    RtlInitUnicodeString(&usMessage, (PWSTR)adwParameterVector[3]);
                } else {
                    MBToWCS((LPSTR)adwParameterVector[3], -1, &pwszMsg, -1, TRUE);
                    RtlCreateUnicodeString(&usMessage, pwszMsg);
                    RtlFreeHeap(RtlProcessHeap(), 0, pwszMsg);
                }


            } except (EXCEPTION_EXECUTE_HANDLER) {

                RIPMSG0(RIP_WARNING, "Exception reading STATUS_VDM_HARD_ERROR paramerters");

                RtlFreeUnicodeString(&usCaption);
                RtlCreateUnicodeString(&usCaption, L"VDM Internal Error");
                RtlFreeUnicodeString(&usMessage);
                RtlCreateUnicodeString(&usMessage, L"Exception retrieving error text.");
            }
            goto CleanUpAndSaveParams;
    }

    /*
     * For all other status codes, we generate the information from the status code.
     * First, Map status code and valid response to MessageBox flags.
     */
    dwMBFlags |= wIcons[(ULONG)(phemsg->Status) >> 30] | wOptions[phemsg->ValidResponseOptions];


    /*
     * If we have a client process, try to get the actual application name
     */
    pwszAppName = NULL;
    if (!fErrorIsFromSystem) {
        PPEB Peb;
        PROCESS_BASIC_INFORMATION BasicInfo;
        PLDR_DATA_TABLE_ENTRY LdrEntry;
        LDR_DATA_TABLE_ENTRY LdrEntryData;
        PLIST_ENTRY LdrHead, LdrNext;
        PPEB_LDR_DATA Ldr;
        PVOID ImageBaseAddress;
        PWSTR ClientApplicationName;

        /*
         * This is cumbersome, but basically, we locate the processes
         * loader data table and get it's name directly out of the
         * loader table
         */

        Status = NtQueryInformationProcess(hClientProcess, ProcessBasicInformation,
                    &BasicInfo, sizeof(BasicInfo), NULL);

        if (!NT_SUCCESS(Status)) {
            fErrorIsFromSystem = TRUE;
            goto noname;
        }

        Peb = BasicInfo.PebBaseAddress;
        if (Peb == NULL) {
            fErrorIsFromSystem = TRUE;
            goto noname;
        }

        /*
         * ldr = Peb->Ldr
         */
        Status = NtReadVirtualMemory(hClientProcess, &Peb->Ldr, &Ldr,
                    sizeof(Ldr), NULL);

        if (!NT_SUCCESS(Status)) {
            goto noname;
        }

        LdrHead = &Ldr->InLoadOrderModuleList;

        /*
         * LdrNext = Head->Flink;
         */
        Status = NtReadVirtualMemory(hClientProcess, &LdrHead->Flink,
                    &LdrNext, sizeof(LdrNext), NULL );

        if (!NT_SUCCESS(Status)) {
            goto noname;
        }

        if (LdrNext == LdrHead) {
            goto noname;
        }
        /*
         * This is the entry data for the image.
         */
        LdrEntry = CONTAINING_RECORD(LdrNext, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
        Status = NtReadVirtualMemory(hClientProcess, LdrEntry,
                    &LdrEntryData, sizeof(LdrEntryData), NULL);

        if (!NT_SUCCESS(Status)) {
            goto noname;
        }

        Status = NtReadVirtualMemory(hClientProcess, &Peb->ImageBaseAddress,
                    &ImageBaseAddress, sizeof(ImageBaseAddress), NULL);

        if (!NT_SUCCESS(Status)) {
            goto noname;
        }

        if (ImageBaseAddress != LdrEntryData.DllBase) {
            goto noname;
        }

        LdrNext = LdrEntryData.InLoadOrderLinks.Flink;

        ClientApplicationName = (PWSTR)LocalAlloc(LMEM_ZEROINIT, LdrEntryData.BaseDllName.MaximumLength);
        if (ClientApplicationName == NULL) {
            goto noname;
        }

        Status = NtReadVirtualMemory(hClientProcess, LdrEntryData.BaseDllName.Buffer,
                    ClientApplicationName, LdrEntryData.BaseDllName.MaximumLength,
                    NULL);

        if (!NT_SUCCESS(Status)) {
            LocalFree(ClientApplicationName);
            goto noname;
        }

        pwszAppName = ClientApplicationName;
        fFreeAppNameBuffer = TRUE;

noname:;
    } /* if (!fErrorIsFromSystem) */

    if (pwszAppName == NULL) {
        /*
         * Load default application name (to be used in the caption).
         */
        pwszAppName = ServerLoadString(ghModuleWin, STR_UNKNOWN_APPLICATION,
                                        L"System Process", &fFreeAppNameBuffer);
    }

    /*
     * Map status code to (optional) caption and format string.
     * If a caption is provided, it's enclosed in {} and it's
     *  the first thing in the format string
     */
    EnterCrit();
    if (gNtDllHandle == NULL) {
        gNtDllHandle = GetModuleHandle(TEXT("ntdll"));
        UserAssert(gNtDllHandle != NULL);
    }
    LeaveCrit();

    Status = RtlFindMessage((PVOID)gNtDllHandle, (ULONG_PTR)RT_MESSAGETABLE,
                            LANG_NEUTRAL, phemsg->Status, &MessageEntry);

    /*
     * Parse the caption (if any) and the format string.
     */
    pwszCaption = NULL;
    if (!NT_SUCCESS(Status)) {
        pwszFormatString = wszUnknownHardError;
    } else {
        pwszFormatString = (PWSTR)MessageEntry->Text;
        /*
         * If the message starts with a '{', it has a caption.
         */
        if (*pwszFormatString == L'{') {
            uCaptionLen = 0;
            pwszFormatString++;
            /*
             * Find the closing bracket
             */
            while ((*pwszFormatString != (WCHAR)0) && (*pwszFormatString++ != L'}')) {
                uCaptionLen++;
            }
            /*
             * Eat any non-printable stuff (\r\n), up to the NULL
             */
            while ((*pwszFormatString != (WCHAR)0) && (*pwszFormatString <= L' ')) {
                pwszFormatString++;
            }
            /*
             * Allocate a buffer an copy the caption string
             */
            if ((uCaptionLen++ > 0)
                && ((pwszCaption = (PWSTR)LocalAlloc(LPTR, uCaptionLen * sizeof(WCHAR))) != NULL)) {

                RtlCopyMemory(pwszCaption, (PWSTR)MessageEntry->Text + 1, (uCaptionLen - 1) * sizeof(WCHAR));
                fFreeCaption = TRUE;
            }
        } /* if (*pszParsedCaption == '{') */

        if (*pwszFormatString == (WCHAR)0) {
            pwszFormatString = wszUnknownHardError;
        }
    } /* if (!NT_SUCCESS(Status))  (Failed to read caption/format string) */


    /*
     * If the message didn't include a caption (or we didn't find the message),
     *  default to something
     */
    if (pwszCaption == NULL) {
        switch (phemsg->Status & ERROR_SEVERITY_ERROR) {
            case ERROR_SEVERITY_SUCCESS:
                pwszCaption = gpwszaSUCCESS;
                break;
            case ERROR_SEVERITY_INFORMATIONAL:
                pwszCaption = gpwszaSYSTEM_INFORMATION;
                break;
            case ERROR_SEVERITY_WARNING:
                pwszCaption = gpwszaSYSTEM_WARNING;
                break;
            case ERROR_SEVERITY_ERROR:
                pwszCaption = gpwszaSYSTEM_ERROR;
                break;
        }
    }
    UserAssert(pwszCaption != NULL);
    /*
     * If the client has a window, get its title so it can be added to
     *  the caption.
     */
    hwndOwner = NULL;
    EnumThreadWindows(HandleToUlong(phemsg->h.ClientId.UniqueThread),
                        FindWindowFromThread, (LPARAM)&hwndOwner);
    if (hwndOwner == NULL) {
        uTitleLen = 0;
    } else {
        uTitleLen = GetWindowTextLength(hwndOwner);
        if (uTitleLen != 0) {
            pwszTitle = (PWSTR)LocalAlloc(LPTR, (uTitleLen + 3) * sizeof(WCHAR));
            if (pwszTitle != NULL) {
                GetWindowText(hwndOwner, pwszTitle, uTitleLen + 1);
                /*
                 * Add format chars.
                 */
                *(pwszTitle + uTitleLen++) = (WCHAR)':';
                *(pwszTitle + uTitleLen++) = (WCHAR)' ';
            } else {
                /*
                 * We couldn't allocate a buffer to get the title
                 */
                uTitleLen = 0;
            }
        } /* if (uTitleLen != 0) */
    } /* else if (hwndOwner == NULL) */
    /*
     * If we don't have a window title, make it an empty string so we won't
     *  have to special case it later.
     */
    if (uTitleLen == 0) {
        pwszTitle = L"";
    }
    /*
     * Finally we can build the caption string now.
     * It looks like this: [WindowTile: ]ApplicationName - ErrorCaption
     */
    uCaptionLen = uTitleLen + wcslen(pwszAppName) + 3 + wcslen(pwszCaption) + 1;
    pwszFullCaption = (PWSTR)LocalAlloc(LPTR, uCaptionLen * sizeof(WCHAR));
    if (pwszFullCaption != NULL) {
        #if DBG
        int iLen =
        #endif
            wsprintfW(pwszFullCaption, L"%s%s - %s", pwszTitle, pwszAppName, pwszCaption);
        UserAssert((UINT)iLen < uCaptionLen);
        RtlCreateUnicodeString(&usCaption, pwszFullCaption);
        LocalFree(pwszFullCaption);
    }
    /*
     * Free caption working buffers, as appropriate.
     */
    if (fFreeCaption) {
        LocalFree(pwszCaption);
    }
    if (fFreeAppNameBuffer) {
        LocalFree(pwszAppName);
    }
    if (uTitleLen != 0) {
        LocalFree(pwszTitle);
    }
    /*
     * Build the error message using pszFormatString and adwParameterVector.
     * Special case UAE
     */
    if (phemsg->Status == STATUS_UNHANDLED_EXCEPTION ) {
        /*
         * The first parameter has the exception status code. Map it to a
         *  format string and build the error message with it and the
         *  parameters.
         */
        Status = RtlFindMessage( (PVOID)gNtDllHandle, (ULONG_PTR)RT_MESSAGETABLE,
                                 LANG_NEUTRAL, (ULONG)adwParameterVector[0], &MessageEntry);

        if (!NT_SUCCESS(Status)) {
            /*
             * We couldn't read the exception name so let's use unknown.
             */
            pwszResBuffer = ServerLoadString(ghModuleWin, STR_UNKNOWN_EXCEPTION,
                            wszUnkownSoftwareException, &fResAllocated);

            wsprintfW(wszErrorMessage, pwszFormatString, pwszResBuffer,
                      adwParameterVector[0], adwParameterVector[1]);

            if (fResAllocated) {
                LocalFree(pwszResBuffer);
            }

            RtlCreateUnicodeString(&usMessage, wszErrorMessage);
            UserAssert(usMessage.MaximumLength <= sizeof(wszErrorMessage));

        } else {
            /*
             * Access Violations are handled a bit differently
             */

            if (adwParameterVector[0] == STATUS_ACCESS_VIOLATION ) {

                wsprintfW(wszErrorMessage, (PWSTR)MessageEntry->Text, adwParameterVector[1],
                          adwParameterVector[3], adwParameterVector[2] ? L"written" : L"read");

            } else if (adwParameterVector[0] == STATUS_IN_PAGE_ERROR) {
                wsprintfW(wszErrorMessage, (PWSTR)MessageEntry->Text, adwParameterVector[1],
                          adwParameterVector[3], adwParameterVector[2]);

            } else {
                /*
                 * If this is a marked exception, skip the mark;
                 *  the exception name follows it.
                 */
                pwszCaption = (PWSTR)MessageEntry->Text;
                if (!wcsncmp(pwszCaption, wszException, ARRAY_SIZE(wszException) - 1)) {
                    pwszCaption += ARRAY_SIZE(wszException) - 1;
                    /*
                     * Skip not printable stuff (\r\n)
                     */
                    while ((*pwszCaption != (WCHAR)0) && (*pwszCaption <= L' ')) {
                        pwszCaption++;
                    }

                } else {
                    pwszCaption = wszUnkownSoftwareException;
                }

                wsprintfW(wszErrorMessage, pwszFormatString, pwszCaption,
                          adwParameterVector[0], adwParameterVector[1]);
            }

            UserAssert(wcslen(wszErrorMessage) < ARRAY_SIZE(wszErrorMessage));

            /*
             * Add button(s) explanation text.
             */
            pwszResBuffer = ServerLoadString(ghModuleWin, STR_OK_TO_TERMINATE,
                            L"Click on OK to terminate the application",
                            &fResAllocated);


            if (phemsg->ValidResponseOptions == OptionOkCancel ) {
                pwszResBuffer1 = ServerLoadString(ghModuleWin,
                                STR_CANCEL_TO_DEBUG, L"Click on CANCEL xx to debug the application",
                                &fResAllocated1);
            } else {
                pwszResBuffer1 = NULL;
                fResAllocated1 = FALSE;
            }

            /*
             * Conncatenate all strings, one per line.
             */
            uMsgLen = wcslen(wszErrorMessage)
                        + wcslen(pwszResBuffer) + 1
                        + (pwszResBuffer1 == NULL ? 0 : wcslen(pwszResBuffer1) + 1)
                        + 1;

            pwszMsg = (PWSTR) LocalAlloc(LPTR, uMsgLen * sizeof(WCHAR));
            if (pwszMsg != NULL) {
                #if DBG
                int iLen =
                #endif
                    wsprintfW(pwszMsg, L"%s\n%s%s%s", wszErrorMessage, pwszResBuffer,
                              (pwszResBuffer1 == NULL ? L"" : L"\n"),
                              (pwszResBuffer1 == NULL ? L"" : pwszResBuffer1));

                UserAssert((UINT)iLen < uMsgLen);

                RtlCreateUnicodeString(&usMessage, pwszMsg);
                LocalFree(pwszMsg);
            }

            /*
             * Free ServerLoadString allocations.
             */
            if (fResAllocated) {
                LocalFree(pwszResBuffer);
            }

            if (fResAllocated1) {
                LocalFree(pwszResBuffer1);
            }


        }

    } else {
        /*
         * Default message text generation for all other status codes
         */
        try {
            #if DBG
            int iLen =
            #endif
                wsprintfW(wszErrorMessage, pwszFormatString, adwParameterVector[0],
                                              adwParameterVector[1],
                                              adwParameterVector[2],
                                              adwParameterVector[3]);
            UserAssert((UINT)iLen <  ARRAY_SIZE(wszErrorMessage));

            /*
             * Remove \r\n
             */
            pwszFormatString = wszErrorMessage;
            while (*pwszFormatString != (WCHAR)0) {
                if (*pwszFormatString == (WCHAR)0xd) {
                    *pwszFormatString = L' ';
                    /*
                     * Move everything up if a CR LF sequence is found
                     */
                    if (*(pwszFormatString+1) == (WCHAR)0xa) {
                        UINT uSize = (wcslen(pwszFormatString+1) + 1) * sizeof(WCHAR);
                        RtlMoveMemory(pwszFormatString, pwszFormatString+1, uSize);
                    }
                 }

                if (*pwszFormatString == (WCHAR)0xa) {
                    *pwszFormatString = L' ';
                }

                pwszFormatString++;
            }

            RtlCreateUnicodeString(&usMessage, wszErrorMessage);
            UserAssert(usMessage.MaximumLength <= sizeof(wszErrorMessage));
        } except(EXCEPTION_EXECUTE_HANDLER) {

            wsprintfW(wszErrorMessage, L"Exception Processing Message %lx Parameters %lx %lx %lx %lx",
                      phemsg->Status, adwParameterVector[0], adwParameterVector[1],
                      adwParameterVector[2], adwParameterVector[3]);

            RtlFreeUnicodeString(&usMessage);
            RtlCreateUnicodeString(&usMessage, wszErrorMessage);
            UserAssert(usMessage.MaximumLength <= sizeof(wszErrorMessage));

        } /* try */

    } /* else if (phemsg->Status == STATUS_UNHANDLED_EXCEPTION) */


CleanUpAndSaveParams:
     if (hClientProcess != NULL) {
         NtClose(hClientProcess);
     }
    /*
     * Free string parameters.
     * Note that we're supposed to call RtlFreeAnsiString since these were
     *  allocated by RtlUnicodeStringToAnsiString.... but we only saved the buffers...
     */
    if (dwStringsToFreeMask != 0) {
        for (dwCounter = 0; dwCounter < phemsg->NumberOfParameters; dwCounter++) {
            if (dwStringsToFreeMask & (1 << dwCounter)) {
                RtlFreeHeap(RtlProcessHeap(), 0, (PVOID)adwParameterVector[dwCounter]);
            }
        }
    }
    /*
     * Save MessageBox Parameters in phi to be used and freed later.
     */
    if (fErrorIsFromSystem) {
        phi->dwHEIFFlags |= HEIF_SYSTEMERROR;
    }
    phi->usText = usMessage;
    phi->usCaption = usCaption;
    phi->dwMBFlags = dwMBFlags;
    return;
}
/***************************************************************************\
* HardErrorHandler
*
* This routine processes hard error requests from the CSR exception port
*
* History:
* 07-03-91 JimA             Created.
\***************************************************************************/
VOID HardErrorHandler(void)
{
    int idResponse;
    PHARDERRORINFO phi, *pphi;
    DWORD dwResponse;
    DESKRESTOREDATA drdRestore;
    BOOL fNuked;
    UINT uHECRet;
    DWORD dwCmd;
    HANDLE hThread;
    int aidButton[3], cButtons;
    LPWSTR apstrButton[3];
    MSGBOXDATA mbd;
    BOOL bDoBlock;
    PCTXHARDERRORINFO pCtxHEInfo = NULL;
    MSG msg;

#if DBG
    /*
     * We should have only one error handler at the time.
     */
    static long glReentered = -1;
    UserAssert(InterlockedIncrement(&glReentered) == 0);
#endif

    if (ISTS()) {
        bDoBlock = (gbExitInProgress || (HEC_ERROR == NtUserHardErrorControl(HardErrorSetup, NULL, NULL)));
    } else {
        bDoBlock = (HEC_ERROR == NtUserHardErrorControl(HardErrorSetup, NULL, NULL));
    }

    drdRestore.pdeskRestore = NULL;

    if (bDoBlock) {
        /*
         * We failed to set up to process hard errors.  Acknowledge all
         * pending errors as NotHandled.
         */
        EnterCrit();
        while (gphiList != NULL) {
            phi = gphiList;
            gphiList = phi->phiNext;
            LeaveCrit();
            ReplyHardError(phi, ResponseNotHandled);
            EnterCrit();
        }
        UserAssert(InterlockedDecrement(&glReentered) < 0);
        UserAssert(gdwHardErrorThreadId == HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread));
        gdwHardErrorThreadId = 0;
        LeaveCrit();
        return;
    }

    /*
     * Process all hard error requests.
     */

    for (;;) {
        /*
         * Grab the next request (for the current desktop)
         * If we're done, reset gdwHardErrorThreadId so any request
         *  after this point will be handled by someone else
         */
        EnterCrit();
        phi = gphiList;
        if (phi == NULL) {
            UserAssert(InterlockedDecrement(&glReentered) < 0);
            UserAssert(gdwHardErrorThreadId == HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread));
            gdwHardErrorThreadId = 0;
        } else {
            while ((phi != NULL) && (phi->dwHEIFFlags & HEIF_WRONGDESKTOP)) {
                phi = phi->phiNext;
            }
            if (phi != NULL) {
                /*
                 * We're going to show this one.
                 */
                phi->dwHEIFFlags |= HEIF_ACTIVE;
            } else {
                /*
                 * We have some requests pending but they are not
                 *  for the current desktop. Let's wait for another
                 *  request (WM_NULL posted) or a desktop switch (PostQuitMessage)
                 */
                LeaveCrit();
                MsgWaitForMultipleObjects(0, NULL, FALSE, INFINITE, QS_POSTMESSAGE);
                PeekMessage(&msg, NULL, 0, 0, PM_REMOVE);
                CheckDefaultDesktop();
                continue;
            }
        }
        LeaveCrit();
        /*
         * If no messages are pending, we're done.
         */
        if (phi == NULL) {
            NtUserHardErrorControl(HardErrorCleanup, NULL, NULL);
            return;
        }

        /*
         * The Boost routine can mess with the list, so must get citrix info now.
         */
        if (ISTS()) {
            pCtxHEInfo = phi->pCtxHEInfo;
            if (gbExitInProgress) {
                dwResponse = ResponseOk;
                goto Reply;
            }
        }

        /*
         * Get win32k attach parameters.
         */
        dwCmd = (phi->dwMBFlags & MB_DEFAULT_DESKTOP_ONLY) ? HardErrorAttachUser : HardErrorAttach;
        hThread = (phi->pthread != NULL) ? phi->pthread->ThreadHandle : NULL;
        /*
         * We have already handled the MB_SERVICE_NOTIFICATION flags.
         * Clear it to prevent recursion.
         * Also, don't let hard error boxes steal the foreground
         */
        phi->dwMBFlags &= ~(MB_SERVICE_NOTIFICATION | MB_SETFOREGROUND | MB_SYSTEMMODAL);
        /*
         * If this is a VDM error, figure out buttons, default id, style, etc
         */
        if (phi->dwHEIFFlags & HEIF_VDMERROR) {
            int i;
            WORD rgwBtn[3], wBtn;
            /*
             * Initialize MSGBOXDATA with the information we
             *  have already figured out.
             */
            RtlZeroMemory(&mbd, sizeof(MSGBOXDATA));
            mbd.cbSize = sizeof(MSGBOXPARAMS);
            mbd.lpszText = phi->usText.Buffer;
            mbd.lpszCaption = phi->usCaption.Buffer;
            /*
             * phi->dwVDMParam0 = (fForWOW << 16) | wBtn1;
             * phi->dwVDMParam1 = (wBtn2   << 16) | wBtn3;
             * Right now, only WOW does this.  If NTVDM does it,
             * fForWOW will be false.
             */
            rgwBtn[0] = LOWORD(phi->dwVDMParam0);
            rgwBtn[1] = HIWORD(phi->dwVDMParam1);
            rgwBtn[2] = LOWORD(phi->dwVDMParam1);
            cButtons = 0;
            for (i = 0; i < 3; i++) {
                wBtn = rgwBtn[i] & ~SEB_DEFBUTTON;
                if (wBtn && wBtn <= MAX_SEB_STYLES) {
                    apstrButton[cButtons] = MB_GetString(wBtn-1);
                    aidButton[cButtons] = i + 1;
                    if (rgwBtn[i] & SEB_DEFBUTTON) {
                        mbd.DefButton = cButtons;
                    }
                    if (wBtn == SEB_CANCEL) {
                        mbd.CancelId = cButtons;
                    }
                    cButtons++;
                }
            }
            mbd.dwStyle = MB_TOPMOST;
            if ((cButtons != 1) || (aidButton[0] != 1)) {
                mbd.dwStyle |= MB_OKCANCEL;
            }
            mbd.ppszButtonText = apstrButton;
            mbd.pidButton = aidButton;
            mbd.cButtons = cButtons;
        }
        /*
         * Attach to win32k and show the dialog.
         * If we switch desktops, (loop and) show it on the new desktop (if applicable)
         */
        do {
            phi->pmsg->Response = ResponseNotHandled;

            uHECRet = NtUserHardErrorControl(dwCmd, hThread, &drdRestore);

            if (uHECRet == HEC_SUCCESS) {
                if (phi->dwHEIFFlags & HEIF_VDMERROR) {
                    idResponse = SoftModalMessageBox(&mbd);
                } else {
                    /*
                     * Bring up the message box. Or in MB_TOPMOST so
                     * it comes up on top.
                     * We want to preserve the MB_DEFAULT_DESKTOP_ONLY flag
                     *  but don't want to pass it to MessageBox or we'll
                     *  recurse due to a compatibility hack
                     */

                    /*
                     *  Really a Terminal Server message
                     */
                    if (pCtxHEInfo != NULL) {

                        /*
                         * If timeout is not (-1), or 0, then we must queue
                         * a system timer to call our timer callback function
                         * to cancel the message box.
                         */
                        if ((pCtxHEInfo->Timeout != (-1)) && (pCtxHEInfo->Timeout != 0)) {

                            DWORD Timeout;

                            if (((DWORD)pCtxHEInfo->Timeout) < (MAXLONG/1000)) {
                                Timeout = (DWORD)pCtxHEInfo->Timeout * 1000L;
                            } else {
                                Timeout = MAXLONG;
                            }

                            gfTimedOut = FALSE;
                            gTimerId = SetTimer((HWND)0,
                                                (UINT)0,
                                                Timeout,
                                                MsgBoxTimerFunc);
                            if (gTimerId == 0) {
                                RIPMSG0(RIP_WARNING, "HardErrorHandler: Could not set system timer");
                            }
                        }

                        RIPMSG2(RIP_WARNING, "HardErrorHandler: MessageBoxEx( %S, %S )",
                                phi->usText.Buffer, phi->usCaption.Buffer);
                    } else {
                        RIPMSG2(RIP_WARNING, "HardErrorHandler: MessageBoxEx( %S, %S )",
                                phi->usText.Buffer, phi->usCaption.Buffer);
                    }

                    idResponse = MessageBoxEx(NULL, phi->usText.Buffer, phi->usCaption.Buffer,
                        (phi->dwMBFlags | MB_TOPMOST) & ~MB_DEFAULT_DESKTOP_ONLY, 0);
                }
                /*
                 * Restore hard error handler desktop; this will also
                 *  tell us if the input desktop has changed; if so,
                 *  we want to bring the error box again on the new
                 *  desktop.
                 */
                uHECRet = NtUserHardErrorControl(HardErrorDetach, NULL, &drdRestore);

                if (ISTS()) {
                    /*
                     * Kill the timer if set
                     */
                    if (gTimerId) {
                        KillTimer((HWND)0, gTimerId);
                        gTimerId = 0;
                    }
                    /*
                     *  Really a citrix message
                     */
                    if (uHECRet != HEC_DESKTOPSWITCH && pCtxHEInfo != NULL) {

                        /*
                         * Check for message box timeout
                         */
                        if (gfTimedOut) {
                           uHECRet = HEC_SUCCESS;
                           gfTimedOut = FALSE;
                           pCtxHEInfo->Response = IDTIMEOUT;
                        } else {
                           pCtxHEInfo->Response = idResponse;
                        }

                    }
                    if (dwResponses[idResponse] == ResponseNotHandled &&
                        uHECRet == HEC_DESKTOPSWITCH && gSessionId == 0) {

                        RIPMSG2(RIP_WARNING, "HardErrorHandler: abort harderror, idResponse %u, uHECRet %u",
                                    idResponse, uHECRet);

                        break;
                    }
                }
            } else {
                idResponse = 0;
            }
            UserAssert((UINT)idResponse < ARRAY_SIZE(dwResponses));
            dwResponse = dwResponses[idResponse];
            /*
             * If we don't want to reshow this box, done
             */
            if (uHECRet != HEC_DESKTOPSWITCH) {
                break;
            } else {
                /*
                 * We've switched desktops; if we're in the default one
                 *  now, then we can show all MB_DEFAULT_DESKTOP_ONLY
                 *  requests.
                 */
                CheckDefaultDesktop();
            }
            /*
             * If BoostHardError nuked it, don't re-show it
             */
            EnterCrit();
            fNuked = (phi->dwHEIFFlags & HEIF_NUKED);
            LeaveCrit();
        } while (!fNuked);

        /*
         * If we didn't show this box because we're not in the
         *  default desktop, mark this phi and continue
         */
        if (uHECRet == HEC_WRONGDESKTOP) {
            UserAssert(phi->dwMBFlags & MB_DEFAULT_DESKTOP_ONLY);
            EnterCrit();
            COPY_FLAG(phi->dwHEIFFlags, HEIF_WRONGDESKTOP, HEIF_ACTIVE | HEIF_WRONGDESKTOP);
            LeaveCrit();
            continue;
        }

Reply:
        /*
         * We're done with this phi.
         * Unlink it if BoostHardError haven't done so already;
         *  If unlinked, it is marked as nuked.
         */
        EnterCrit();
        UserAssert(phi->dwHEIFFlags & HEIF_ACTIVE);
        fNuked = (phi->dwHEIFFlags & HEIF_NUKED);
        if (!fNuked) {
            pphi = &gphiList;
            while ((*pphi != phi) && (*pphi != NULL)) {
                pphi = &(*pphi)->phiNext;
            }
            UserAssert(*pphi != NULL);
            *pphi = phi->phiNext;
        }

#if DBG
         else {
            /*
             * Let's make sure it was unlinked.
             */
            pphi = &gphiList;
            while ((*pphi != phi) && (*pphi != NULL)) {
                UserAssert(!((*pphi)->dwHEIFFlags & (HEIF_ACTIVE | HEIF_NUKED)));
                pphi = &(*pphi)->phiNext;
            }
            UserAssert(*pphi == NULL);
         }
#endif

        if (phi->pCtxHEInfo) {

            /*
             * Clean up
             */
            HardErrorRemove(phi->pCtxHEInfo);

            /*
             * Done
             */
            phi->pCtxHEInfo = NULL;
        }

        LeaveCrit();

        /*
         * Save the response, reply and free phi
         */
        ReplyHardError(phi, (fNuked ? ResponseNotHandled : dwResponse));

    } /* for (;;) */

    /*
     * Nobody should break out of the loop.
     */
    UserAssert(FALSE);
}


LPWSTR RtlLoadStringOrError(
    HANDLE hModule,
    UINT wID,
    LPWSTR lpDefault,
    PBOOL pAllocated,
    BOOL bAnsi
    )
{
    LPTSTR lpsz;
    int cch;
    LPWSTR lpw;
    PMESSAGE_RESOURCE_ENTRY MessageEntry;
    NTSTATUS Status;

    cch = 0;
    lpw = NULL;

    Status = RtlFindMessage((PVOID)hModule, (ULONG_PTR)RT_MESSAGETABLE,
            0, wID, &MessageEntry);
    if (NT_SUCCESS(Status)) {

        /*
         * Return two fewer chars so the crlf in the message will be
         * stripped out.
         */
        cch = wcslen((PWCHAR)MessageEntry->Text) - 2;
        lpsz = (LPWSTR)MessageEntry->Text;

        if (bAnsi) {
            int ich;

            /*
             * Add one to zero terminate then force the termination
             */
            ich = WCSToMB(lpsz, cch+1, (CHAR **)&lpw, -1, TRUE);
            ((LPSTR)lpw)[ich-1] = 0;

            }
        else {
            lpw = (LPWSTR)LocalAlloc(LMEM_ZEROINIT,(cch+1)*sizeof(WCHAR));
            if ( lpw ) {

                /*
                 * Copy the string into the buffer.
                 */
                    RtlCopyMemory(lpw, lpsz, cch*sizeof(WCHAR));
                }
            }
        }

    if ( !lpw ) {
        lpw = lpDefault;
        *pAllocated = FALSE;
        } else {
        *pAllocated = TRUE;
        }

    return lpw;
}

/***************************************************************************\
* HardErrorWorkerThread
*
* Worker thread to handle hard error requests.
*
* History:
* 05-01-98 JerrySh      Created.
\***************************************************************************/

NTSTATUS
HardErrorWorkerThread(
    PVOID ThreadParameter
    )
{
    PCSR_THREAD pt;
    UNREFERENCED_PARAMETER(ThreadParameter);

    pt = CsrConnectToUser();
    ProcessHardErrorRequest(FALSE);
    if (pt) {
        CsrDereferenceThread(pt);
    }
    UserExitWorkerThread();

    return STATUS_SUCCESS;
}

/***************************************************************************\
* ProcessHardErrorRequest
*
* Figures out who should process the hard error. There are 3 possible cases.
* - If there is already a hard error thread, hand it off to him.
* - If not and we don't want to wait, create a worker thread to deal with it.
* - If we want to wait or thread creation fails, deal with it ourselves.
*
* History:
* 05-01-98 JerrySh      Created.
\***************************************************************************/

VOID
ProcessHardErrorRequest(
    BOOL fNewThread
    )
{
    NTSTATUS Status;
    CLIENT_ID ClientId;
    HANDLE hThread;

    EnterCrit();

    /*
     * If there's already a hard error handler, make sure he's awake.
     */
    if (gdwHardErrorThreadId) {
        DWORD dwHardErrorHandler = gdwHardErrorThreadId;
        LeaveCrit();
        PostThreadMessage(dwHardErrorHandler, WM_NULL, 0, 0);
        return;
    }

    /*
     * Create a worker thread to handle the hard error.
     */
    if (fNewThread) {
        LeaveCrit();
        Status = RtlCreateUserThread(NtCurrentProcess(),
                                     NULL,
                                     TRUE,
                                     0,
                                     0,
                                     0,
                                     HardErrorWorkerThread,
                                     NULL,
                                     &hThread,
                                     &ClientId);
        if (NT_SUCCESS(Status)) {
            CsrAddStaticServerThread(hThread, &ClientId, 0);
            NtResumeThread(hThread, NULL);
            return;
        }
        EnterCrit();
    }

    /*
     * Let this thread handle the hard error.
     */
    gdwHardErrorThreadId = HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread);
    LeaveCrit();
    HardErrorHandler();

    return;
}

/***************************************************************************\
* UserHardError
*
* Called from CSR to pop up hard error messages
*
* History:
* 03/12/97 GerardoB         Rewritten to support OptionOkNoWait
* 07-03-91 JimA             Created.
\***************************************************************************/

VOID UserHardError(
    PCSR_THREAD pt,
    PHARDERROR_MSG pmsg)
{
    UserHardErrorEx(pt, pmsg, NULL);
}

VOID HardErrorInsert(
    PCSR_THREAD pt,
    PHARDERROR_MSG pmsg,
    PCTXHARDERRORINFO pCtxHEInfo)
{
    UserHardErrorEx(pt, pmsg, pCtxHEInfo);
}

/***************************************************************************\
* UserHardErrorEx
*
* Called from CSR to pop up hard error messages
*
* History:
* 07-03-91 JimA             Created.
\***************************************************************************/

VOID UserHardErrorEx(
    PCSR_THREAD pt,
    PHARDERROR_MSG pmsg,
    PCTXHARDERRORINFO pCtxHEInfo)
{
    BOOL fClientPort, fNoWait, fMsgBox, fLogEvent;
    PHARDERRORINFO phi = NULL, *pphiLast;
    HANDLE hEvent;
    DWORD dwReportMode, dwResponse;

    UserAssert((ULONG)pmsg->NumberOfParameters <= MAXIMUM_HARDERROR_PARAMETERS);
    /*
     * Allocate memory to queue request.
     */
    phi = (PHARDERRORINFO)LocalAlloc(LPTR, sizeof(HARDERRORINFO));
    if (phi == NULL) {
        goto ErrorExit;
    }
    phi->pthread = pt;

    /*
     * Set up citrix specific stuff
     */
    if (ISTS()) {
        phi->pCtxHEInfo = pCtxHEInfo;
    }

    /*
     * Determine reply type
     */
    fClientPort = ((pt != NULL) && (pt->Process->ClientPort != NULL));
    fNoWait = (pmsg->ValidResponseOptions == OptionOkNoWait);

    /*
     * Capture HARDERROR_MSG data or create wait event as needed
     */
    if (fClientPort || fNoWait) {
        phi->pmsg = (PHARDERROR_MSG)LocalAlloc(LPTR, pmsg->h.u1.s1.TotalLength);
        if (phi->pmsg == NULL) {
            goto ErrorExit;
        }
        phi->dwHEIFFlags |= HEIF_ALLOCATEDMSG;
        RtlCopyMemory(phi->pmsg, pmsg, pmsg->h.u1.s1.TotalLength);
        hEvent = NULL;
        /*
         * Set the magic response value (-1) to let CsrApiRequestThread know
         * that we'll take care of dereferencing and replying.
         */
        if (pt != NULL) {
            phi->dwHEIFFlags |= HEIF_DEREFTHREAD;
        }
        pmsg->Response = (ULONG)-1;
        if (fNoWait) {
            phi->pmsg->ValidResponseOptions = OptionOk;
        }
    } else {
        phi->pmsg = pmsg;
        /*
         * There is no reply port but we need to wait; create an event.
         */
        hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (hEvent == NULL) {
            goto ErrorExit;
        }
        phi->hEventHardError = hEvent;
    }

    /*
     * Build hard error title, message and flags. Then log the event.
     */
    GetHardErrorText(phi);

    /*
     * Reply now if we're not going to wait.
     */
    if (fNoWait) {
        phi->dwHEIFFlags |= HEIF_REPLIED;
        phi->pmsg->Response = ResponseOk;
        if (fClientPort) {
            NtReplyPort(pt->Process->ClientPort, (PPORT_MESSAGE)phi->pmsg);
        } else {
            /*
             * Must let CsrApiRequestThread reply since we don't have a port
             */
            pmsg->Response = ResponseOk;
            /*
             * If we have a thread, reference it because we're telling
             *  CsrApiRequestThread that we're done with it.
             */
            if (pt != NULL) {
                /*
                 * Later5.0 GerardoB. Let's stop to see how this happens.
                 */
                UserAssert(pt == NULL);
                CsrReferenceThread(pt);
            }
        }
    }

    /*
     * Register the event if we haven't done so already.
     * Since RegisterEventSource is supported by a service, we must not hold
     *  any locks while making this call. Hence we might have several threads
     *  registering the event simultaneously.
     */
    fLogEvent = (gEventSource != NULL);
    if (!fLogEvent) {
        HANDLE hEventSource;
        hEventSource = UserRegisterEventSource(L"Application Popup");
        /*
         * Save the first handle, deregister all others.
         */
        if (InterlockedCompareExchangePointer(&gEventSource, hEventSource, NULL) == NULL) {
            /*
             * This is the first handle. If valid, we can log events.
             */
            fLogEvent = (hEventSource != NULL);
        } else {
            /*
             * We had already saved another handle (so we can log events). Deregister this one.
             */
            if (hEventSource != NULL) {
                UserVerify(gfnDeregisterEventSource(hEventSource));
            }
            fLogEvent = TRUE;
        }
    }

    dwReportMode = (fLogEvent ? GetErrorMode() : 0);
    if (fLogEvent) {
        LogErrorPopup(phi->usCaption.Buffer, phi->usText.Buffer);
    }

    /*
     * Determine if we need to display a message box.
     */
    if ((phi->pmsg->Status == STATUS_SERVICE_NOTIFICATION) || (dwReportMode == 0)) {
        fMsgBox = TRUE;
    } else if (phi->pmsg->Status == STATUS_VDM_HARD_ERROR) {
        fMsgBox = (dwReportMode == 1);
        if (!fMsgBox) {
            dwResponse = ResponseOk;
        }
    } else {
        fMsgBox = ((dwReportMode == 1) && !(phi->dwHEIFFlags & HEIF_SYSTEMERROR));
        if (!fMsgBox) {
            UserAssert((UINT)phi->pmsg->ValidResponseOptions < ARRAY_SIZE(dwResponseDefault));
            dwResponse = dwResponseDefault[phi->pmsg->ValidResponseOptions];
        }
    }
    /*
     * If we don't have to display a message box, we're done.
     */
    if (!fMsgBox) {
        goto DontNeedErrorHandler;
    }

    /*
     * We want to display a message box. Queue the request and go for it.
     */
    EnterCrit();
    /*
     * Never mind if the process has terminated. Got to check this holding the
     * critical section to make sure that no other thread makes it to BoostHardError
     * before we add this phi to the list.
     */
    if ((pt != NULL) && (pt->Process->Flags & CSR_PROCESS_TERMINATED)) {
        LeaveCrit();
DontNeedErrorHandler:
        ReplyHardError(phi, dwResponse);
        if (hEvent != NULL) {
            NtClose(hEvent);
        }
        return;
    }
    /*
     * Add it to the end of the list.
     */
    pphiLast = &gphiList;
    while (*pphiLast != NULL) {
        pphiLast = &(*pphiLast)->phiNext;
    }
    *pphiLast = phi;
    LeaveCrit();

    /*
     * Process the hard error request. If this is a NoWait request and there
     * is no reply port, then we'll try to launch a new worker thread so this
     * one can return.
     */
    ProcessHardErrorRequest(fNoWait && !fClientPort);

    /*
     * If there is an event handle, wait for it.
     */
    if (hEvent != NULL) {
        NtWaitForSingleObject(hEvent, FALSE, NULL);
        NtClose(hEvent);
    }
    return;

ErrorExit:
    if (phi != NULL) {
        FreePhi(phi);
    }
    pmsg->Response = ResponseNotHandled;
}

/***************************************************************************\
* BoostHardError
*
* If one or more hard errors exist for the specified process, remove
* them from the list if forced, otherwise bring the first one to the
* top of the hard error list and display it.  Return TRUE if there
* is a hard error.
*
* History:
* 11-02-91 JimA             Created.
\***************************************************************************/

BOOL BoostHardError(
    ULONG_PTR dwProcessId,
    DWORD dwCode)
{
    DESKRESTOREDATA drdRestore;
    PHARDERRORINFO phi, *pphi;
    BOOL fHasError = FALSE;

    EnterCrit();
    /*
     * If the list is empty, nothing do to here.
     */
    if (gphiList == NULL) {
        LeaveCrit();
        return FALSE;
    }
    drdRestore.pdeskRestore = NULL;
    /*
     * Walk the hard error list
     */
    pphi = &gphiList;
    while (*pphi != NULL) {
        /*
         * If not not nuking all and not owned by dwProcessId, continue walking
         */
        if (dwProcessId != (ULONG_PTR)-1) {
            if (((*pphi)->pthread == NULL)
                    || ((ULONG_PTR)((*pphi)->pthread->ClientId.UniqueProcess) != dwProcessId)) {

                pphi = &(*pphi)->phiNext;
                continue;
            }
        } else {
            UserAssert(dwCode == BHE_FORCE);
        }
        /*
         * Got one so we want to return TRUE
         */
        fHasError = TRUE;
        /*
         * If nuking the request
         */
        if (dwCode == BHE_FORCE) {
            /*
             * Unlink it from the list.
             */
            phi = *pphi;
            *pphi = phi->phiNext;

            /*
             * If this box is being shown right now, signal it to go away.
             * Otherwise, nuke it
             */
            if (phi->dwHEIFFlags & HEIF_ACTIVE) {
                DWORD dwHardErrorHandler = gdwHardErrorThreadId;
                phi->dwHEIFFlags |= HEIF_NUKED;
                LeaveCrit();
                PostThreadMessage(dwHardErrorHandler, WM_QUIT, 0, 0);
            } else {
                /*
                 * Acknowledge the error as not handled, reply and free
                 */
                LeaveCrit();
                ReplyHardError(phi, ResponseNotHandled);
            }

            /*
             * Restart the search because we left the crit sect.
             */
            EnterCrit();
            pphi = &gphiList;

            /* continue */

        } else if (dwCode == BHE_ACTIVATE) {
            /*
             * If it's active, find it and show it.
             */
            phi = *pphi;
            if (phi->dwHEIFFlags & HEIF_ACTIVE) {
                HWND hwndError = NULL;
                DWORD dwHardErrorHandler = gdwHardErrorThreadId;

                LeaveCrit();
                EnumThreadWindows(dwHardErrorHandler, FindWindowFromThread, (LPARAM)&hwndError);

                if ((hwndError != NULL)
                        && (HEC_SUCCESS == NtUserHardErrorControl(HardErrorAttachNoQueue, NULL, &drdRestore))) {

                    SetForegroundWindow(hwndError);

                    NtUserHardErrorControl(HardErrorDetachNoQueue, NULL, &drdRestore);
                }
                return TRUE;
            }

            /*
             * It's not active so move it to the head of the list
             *  to make it show up next.
             */
            *pphi = phi->phiNext;
            phi->phiNext = gphiList;
            gphiList = phi;
            break;

        } else { /* BHE_TEST */
            /*
             * The caller just want to know if this process owns a hard error
             */
            break;
        }
    } /*  while (*pphi != NULL) */

    LeaveCrit();

    /*
     * Bug 284468. Wake up the hard error handler
     */
    if (dwCode == BHE_FORCE && gdwHardErrorThreadId != 0) {
        PostThreadMessage(gdwHardErrorThreadId, WM_NULL, 0, 0);
    }

    return fHasError;
}
