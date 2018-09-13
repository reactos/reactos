/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    comm.c

Abstract:

    This module implements Win32 comm APIs

Author:

    Anthony V. Ercolano (tonye) 25-April-1991

Revision History:

--*/

#include "basedll.h"
#pragma hdrstop

#include "ntddser.h"
#include "cfgmgr32.h"

typedef struct _LOCALMATCHSTR {
    DWORD FoundIt;
    LPCWSTR FriendlyName;
    } LOCALMATCHSTR,*PLOCALMATCHSTR;


static
NTSTATUS
GetConfigDialogName(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    )

{

    PUNICODE_STRING dllToLoad = Context;
    if (ValueType != REG_SZ) {

        return STATUS_INVALID_PARAMETER;

    }

    //
    // Allocate heap to hold the unicode string.  We know
    // that the string is zero terminated.  Allocate that
    // much.  Set the maximum size and size to
    // sizeof(WCHAR) - ValueLength.
    //

    RtlInitUnicodeString(
        dllToLoad,
        NULL
        );

    dllToLoad->Buffer = RtlAllocateHeap(
                            RtlProcessHeap(),
                            MAKE_TAG( TMP_TAG ),
                            ValueLength
                            );

    if (!dllToLoad->Buffer) {

        return STATUS_INSUFFICIENT_RESOURCES;

    }

    RtlMoveMemory(
        dllToLoad->Buffer,
        ValueData,
        ValueLength
        );

    dllToLoad->Length = (USHORT)(ValueLength - (sizeof(WCHAR)));
    dllToLoad->MaximumLength = (USHORT)ValueLength;

    return STATUS_SUCCESS;
}

static
NTSTATUS
GetFriendlyMatchComm(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    )

{

    UNICODE_STRING s1;
    UNICODE_STRING s2;
    PLOCALMATCHSTR localMatch = Context;

    RtlInitUnicodeString(
        &s1,
        localMatch->FriendlyName
        );
    RtlInitUnicodeString(
        &s2,
        ValueData
        );

    if (RtlEqualUnicodeString(
            &s1,
            &s2,
            TRUE
            )) {

        localMatch->FoundIt = TRUE;

    }

    return STATUS_SUCCESS;
}

VOID
GetFriendlyUi(
    LPCWSTR FriendlyName,
    PUNICODE_STRING DllToInvoke
    )

{

    RTL_QUERY_REGISTRY_TABLE paramTable[2] = {0};
    LOCALMATCHSTR localMatch = {0,FriendlyName};
    HINSTANCE libHandle;


    paramTable[0].QueryRoutine = GetFriendlyMatchComm;
    paramTable[0].Flags = RTL_QUERY_REGISTRY_REQUIRED;
    //
    // First things first.  Load the cfg manager library.
    //

    libHandle = LoadLibrary("cfgmgr32.dll");

    if (libHandle) {

        FARPROC getSize;
        FARPROC getList;
        FARPROC locateDevNode;
        FARPROC openDevKey;

        try {
            getSize = GetProcAddress(
                          libHandle,
                          "CM_Get_Device_ID_List_SizeW"
                          );

            getList = GetProcAddress(
                          libHandle,
                          "CM_Get_Device_ID_ListW"
                          );

            locateDevNode = GetProcAddress(
                                libHandle,
                                "CM_Locate_DevNodeW"
                                );

            openDevKey = GetProcAddress(
                             libHandle,
                             "CM_Open_DevNode_Key"
                             );

            if (getSize && getList && locateDevNode && openDevKey) {

                PWCHAR bufferForList = NULL;
                DWORD sizeOfBuffer;

                //
                // Find how much memory for the buffer.
                //

                if (getSize(
                        &sizeOfBuffer,
                        L"MODEM",
                        CM_GETIDLIST_FILTER_SERVICE
                        ) == CR_SUCCESS) {

                    //
                    // Allocate 2 extra wchar.
                    //

                    bufferForList = RtlAllocateHeap(
                                        RtlProcessHeap(),
                                        MAKE_TAG( TMP_TAG ),
                                        (sizeOfBuffer*sizeof(WCHAR))
                                         +(sizeof(WCHAR)*2)
                                        );

                    if (bufferForList) {

                        PWCHAR currentId;

                        try {

                            if (getList(
                                    L"modem",
                                    bufferForList,
                                    sizeOfBuffer,
                                    CM_GETIDLIST_FILTER_SERVICE
                                    ) == CR_SUCCESS) {

                                for (
                                    currentId = bufferForList;
                                    *currentId;
                                    currentId += wcslen(currentId)+1
                                    ) {

                                    DWORD devInst = 0;

                                    if (locateDevNode(
                                            &devInst,
                                            currentId,
                                            CM_LOCATE_DEVINST_NORMAL
                                            ) == CR_SUCCESS) {

                                        HANDLE handleToDev;

                                        if (openDevKey(
                                                devInst,
                                                KEY_ALL_ACCESS,
                                                0,
                                                RegDisposition_OpenAlways,
                                                &handleToDev,
                                                CM_REGISTRY_SOFTWARE
                                                ) == CR_SUCCESS) {

                                            NTSTATUS statusOfQuery;

                                            localMatch.FoundIt = 0;
                                            paramTable[0].Name =
                                                L"FriendlyName";

                                            //
                                            // We now have an open
                                            // handle to a dev node.
                                            //
                                            // Check to see if it's
                                            // friendly name matches ours.
                                            //

                                            if (!NT_SUCCESS(
                                                     RtlQueryRegistryValues(
                                                         RTL_REGISTRY_HANDLE,
                                                         handleToDev,
                                                         &paramTable[0],
                                                         &localMatch,
                                                         NULL
                                                         )
                                                     )) {

                                                CloseHandle(handleToDev);
                                                continue;

                                            }

                                            if (!localMatch.FoundIt) {

                                                CloseHandle(handleToDev);
                                                continue;

                                            }

                                            //
                                            // The names match.  Now look
                                            // for the config dll name.
                                            //

                                            paramTable[0].QueryRoutine =
                                                GetConfigDialogName;
                                            paramTable[0].Name =
                                                L"ConfigDialog";
                                            statusOfQuery =
                                                RtlQueryRegistryValues(
                                                    RTL_REGISTRY_HANDLE,
                                                    handleToDev,
                                                    &paramTable[0],
                                                    DllToInvoke,
                                                    NULL
                                                    );

                                            paramTable[0].QueryRoutine =
                                                GetFriendlyMatchComm;

                                            if (!NT_SUCCESS(statusOfQuery)) {

                                                //
                                                // We had a bad status
                                                // back from getting the dll
                                                // name we should have gotten.
                                                //
                                                // There is no point in
                                                // looking for anymore
                                                //

                                                BaseSetLastNTError(
                                                    statusOfQuery
                                                    );
                                                CloseHandle(handleToDev);
                                                return;

                                            }

                                            //
                                            // We know that we are dealing
                                            // with a local registry here.
                                            // we just call closehandle.
                                            //

                                            CloseHandle(handleToDev);

                                            if (DllToInvoke->Buffer) {

                                                //
                                                // We have found a dll for
                                                // the friendly name.  Just
                                                // leave.  The finally
                                                // handlers will clean up
                                                // our allocations.
                                                //

                                                return;

                                            }

                                        }

                                    }

                                }

                            }


                        } finally {

                            //
                            // Free the idlist memory.
                            //

                            RtlFreeHeap(
                                RtlProcessHeap(),
                                0,
                                bufferForList
                                );

                        }

                    }

                }

            }

        } finally {

            FreeLibrary(libHandle);

        }

    }

    if (!DllToInvoke->Buffer) {

        //
        // Couldn't find the friendly name in the enum tree.
        // See if the value is a valid comm port name.  If
        // it is, default return serialui.dll
        //

        paramTable[0].Name = NULL;
        RtlQueryRegistryValues(
            RTL_REGISTRY_DEVICEMAP,
            L"SERIALCOMM",
            paramTable,
            &localMatch,
            NULL
            );

        if (localMatch.FoundIt) {

            ANSI_STRING ansiString;

            RtlInitAnsiString(
                &ansiString,
                "serialui.dll"
                );

            DllToInvoke->Buffer = RtlAllocateHeap(
                                      RtlProcessHeap(),
                                      MAKE_TAG( TMP_TAG ),
                                      (ansiString.Length+2)*sizeof(WCHAR)
                                      );

            if (!DllToInvoke->Buffer) {

                BaseSetLastNTError(STATUS_INSUFFICIENT_RESOURCES);
                return;

            }

            DllToInvoke->Length = 0;
            DllToInvoke->MaximumLength = (ansiString.Length+1)*sizeof(WCHAR);

            RtlAnsiStringToUnicodeString(
                DllToInvoke,
                &ansiString,
                FALSE
                );
            *(DllToInvoke->Buffer+ansiString.Length) = 0;

        } else {

            SetLastError(ERROR_INVALID_PARAMETER);

        }

    }

}


BOOL
CommConfigDialogW(
    LPCWSTR lpszName,
    HWND hWnd,
    LPCOMMCONFIG lpCC
    )

{


    UNICODE_STRING dllName = {0};
    BOOL boolToReturn = TRUE;
    HINSTANCE libInstance = 0;
    DWORD statOfCall = 0;


    //
    // Given the "friendly name" get the name of the dll to load.
    //

    GetFriendlyUi(
        lpszName,
        &dllName
        );

    try {

        if (dllName.Buffer) {

            //
            // Got the new library name.  Try to load it.
            //

            libInstance = LoadLibraryW(dllName.Buffer);

            if (libInstance) {

                FARPROC procToCall;

                //
                // Got the lib.  Get the proc address we need.
                //

                procToCall = GetProcAddress(
                                 libInstance,
                                 "drvCommConfigDialogW"
                                 );

                statOfCall = (DWORD)procToCall(
                                 lpszName,
                                 hWnd,
                                 lpCC
                                 );

            } else {

                boolToReturn = FALSE;

            }

        } else {

            //
            // Assume that an appropriate error has been set.
            //

            boolToReturn = FALSE;

        }


    } finally {

        if (dllName.Buffer) {

            RtlFreeHeap(
                RtlProcessHeap(),
                0,
                dllName.Buffer
                );

        }

        if (libInstance) {

            FreeLibrary(libInstance);

        }

        if (statOfCall) {

            SetLastError(statOfCall);
            boolToReturn = FALSE;

        }

    }

    return boolToReturn;


}

BOOL
CommConfigDialogA(
    LPCSTR lpszName,
    HWND hWnd,
    LPCOMMCONFIG lpCC
    )

{

    PWCHAR unicodeName;
    UNICODE_STRING tmpString;
    ANSI_STRING ansiString;
    BOOL uniBool;

    RtlInitAnsiString(
        &ansiString,
        lpszName
        );

    unicodeName = RtlAllocateHeap(
                      RtlProcessHeap(),
                      MAKE_TAG( TMP_TAG ),
                      (ansiString.Length+2)*sizeof(WCHAR)
                      );

    if (!unicodeName) {

        BaseSetLastNTError(STATUS_INSUFFICIENT_RESOURCES);
        return FALSE;

    }

    tmpString.Length = 0;
    tmpString.MaximumLength = (ansiString.Length+1)*sizeof(WCHAR);
    tmpString.Buffer = unicodeName;

    RtlAnsiStringToUnicodeString(
        &tmpString,
        &ansiString,
        FALSE
        );
    *(unicodeName+ansiString.Length) = 0;

    try {

        uniBool = CommConfigDialogW(
                      unicodeName,
                      hWnd,
                      lpCC
                      );


    } finally {

        RtlFreeHeap(
            RtlProcessHeap(),
            0,
            unicodeName
            );

    }

    return uniBool;

}

BOOL
GetDefaultCommConfigW(
    LPCWSTR lpszName,
    LPCOMMCONFIG lpCC,
    LPDWORD lpdwSize
    )
{

    UNICODE_STRING dllName = {0};
    BOOL boolToReturn = TRUE;
    HINSTANCE libInstance = 0;
    DWORD statOfCall = 0;


    //
    // Given the "friendly name" get the name of the dll to load.
    //

    GetFriendlyUi(
        lpszName,
        &dllName
        );

    try {

        if (dllName.Buffer) {

            //
            // Got the new library name.  Try to load it.
            //

            libInstance = LoadLibraryW(dllName.Buffer);

            if (libInstance) {

                FARPROC procToCall;

                //
                // Got the lib.  Get the proc address we need.
                //

                procToCall = GetProcAddress(
                                 libInstance,
                                 "drvGetDefaultCommConfigW"
                                 );

                statOfCall = (DWORD)procToCall(
                                 lpszName,
                                 lpCC,
                                 lpdwSize
                                 );

            } else {

                boolToReturn = FALSE;

            }

        } else {

            //
            // Assume that an appropriate error has been set.
            //

            boolToReturn = FALSE;

        }


    } finally {

        if (dllName.Buffer) {

            RtlFreeHeap(
                RtlProcessHeap(),
                0,
                dllName.Buffer
                );

        }

        if (libInstance) {

            FreeLibrary(libInstance);

        }

        if (statOfCall) {

            SetLastError(statOfCall);
            boolToReturn = FALSE;

        }

    }

    return boolToReturn;

}

BOOL
GetDefaultCommConfigA(
    LPCSTR lpszName,
    LPCOMMCONFIG lpCC,
    LPDWORD lpdwSize
    )
{

    PWCHAR unicodeName;
    UNICODE_STRING tmpString;
    ANSI_STRING ansiString;
    BOOL uniBool;

    RtlInitAnsiString(
        &ansiString,
        lpszName
        );

    unicodeName = RtlAllocateHeap(
                      RtlProcessHeap(),
                      MAKE_TAG( TMP_TAG ),
                      (ansiString.Length+2)*sizeof(WCHAR)
                      );

    if (!unicodeName) {

        BaseSetLastNTError(STATUS_INSUFFICIENT_RESOURCES);
        return FALSE;

    }

    tmpString.Length = 0;
    tmpString.MaximumLength = (ansiString.Length+1)*sizeof(WCHAR);
    tmpString.Buffer = unicodeName;

    RtlAnsiStringToUnicodeString(
        &tmpString,
        &ansiString,
        FALSE
        );
    *(unicodeName+ansiString.Length) = 0;

    try {

        uniBool = GetDefaultCommConfigW(
                      unicodeName,
                      lpCC,
                      lpdwSize
                      );

    } finally {

        RtlFreeHeap(
            RtlProcessHeap(),
            0,
            unicodeName
            );

    }

    return uniBool;

}

BOOL
SetDefaultCommConfigW(
    LPCWSTR lpszName,
    LPCOMMCONFIG lpCC,
    DWORD dwSize
    )
{

    UNICODE_STRING dllName = {0};
    BOOL boolToReturn = TRUE;
    HINSTANCE libInstance = 0;
    DWORD statOfCall = 0;


    //
    // Given the "friendly name" get the name of the dll to load.
    //

    GetFriendlyUi(
        lpszName,
        &dllName
        );

    try {

        if (dllName.Buffer) {

            //
            // Got the new library name.  Try to load it.
            //

            libInstance = LoadLibraryW(dllName.Buffer);

            if (libInstance) {

                FARPROC procToCall;

                //
                // Got the lib.  Get the proc address we need.
                //

                procToCall = GetProcAddress(
                                 libInstance,
                                 "drvSetDefaultCommConfigW"
                                 );

                statOfCall = (DWORD)procToCall(
                                 lpszName,
                                 lpCC,
                                 dwSize
                                 );

            } else {

                boolToReturn = FALSE;

            }

        } else {

            //
            // Assume that an appropriate error has been set.
            //

            boolToReturn = FALSE;

        }


    } finally {

        if (dllName.Buffer) {

            RtlFreeHeap(
                RtlProcessHeap(),
                0,
                dllName.Buffer
                );

        }

        if (libInstance) {

            FreeLibrary(libInstance);

        }

        if (statOfCall) {

            SetLastError(statOfCall);
            boolToReturn = FALSE;

        }

    }
    return boolToReturn;
}

BOOL
SetDefaultCommConfigA(
    LPCSTR lpszName,
    LPCOMMCONFIG lpCC,
    DWORD dwSize
    )
{

    PWCHAR unicodeName;
    UNICODE_STRING tmpString;
    ANSI_STRING ansiString;
    BOOL uniBool = TRUE;

    RtlInitAnsiString(
        &ansiString,
        lpszName
        );

    unicodeName = RtlAllocateHeap(
                      RtlProcessHeap(),
                      MAKE_TAG( TMP_TAG ),
                      (ansiString.Length+2)*sizeof(WCHAR)
                      );

    if (!unicodeName) {

        BaseSetLastNTError(STATUS_INSUFFICIENT_RESOURCES);
        return FALSE;

    }

    tmpString.Length = 0;
    tmpString.MaximumLength = (ansiString.Length+1)*sizeof(WCHAR);
    tmpString.Buffer = unicodeName;

    RtlAnsiStringToUnicodeString(
        &tmpString,
        &ansiString,
        FALSE
        );
    *(unicodeName+ansiString.Length) = 0;

    try {

        uniBool = SetDefaultCommConfigW(
                      unicodeName,
                      lpCC,
                      dwSize
                      );

    } finally {

        RtlFreeHeap(
            RtlProcessHeap(),
            0,
            unicodeName
            );

    }

    return uniBool;

}

BOOL
ClearCommBreak(
    HANDLE hFile
    )

/*++

Routine Description:

    The function restores character transmission and places the transmission
    line in a nonbreak state.

Arguments:

    hFile - Specifies the communication device to be adjusted.

Return Value:

    The return value is TRUE if the function is successful or FALSE
    if an error occurs.

--*/

{

    return EscapeCommFunction(hFile,CLRBREAK);

}

BOOL
ClearCommError(
    HANDLE hFile,
    LPDWORD lpErrors,
    LPCOMSTAT lpStat
    )

/*++

Routine Description:

    In case of a communications error, such as a buffer overrun or
    framing error, the communications software will abort all
    read and write operations on the communication port.  No further
    read or write operations will be accepted until this function
    is called.

Arguments:

    hFile - Specifies the communication device to be adjusted.

    lpErrors - Points to the DWORD that is to receive the mask of the
               error that occured.

    lpStat - Points to the COMMSTAT structure that is to receive
             the device status.  The structure contains information
             about the communications device.

Return Value:

    The return value is TRUE if the function is successful or FALSE
    if an error occurs.

--*/

{

    NTSTATUS Status;
    HANDLE SyncEvent;
    IO_STATUS_BLOCK Iosb;
    SERIAL_STATUS LocalStat;

    RtlZeroMemory(&LocalStat, sizeof(SERIAL_STATUS));

    if (!(SyncEvent = CreateEvent(
                          NULL,
                          TRUE,
                          FALSE,
                          NULL
                          ))) {

        return FALSE;

    }

    Status = NtDeviceIoControlFile(
                 hFile,
                 SyncEvent,
                 NULL,
                 NULL,
                 &Iosb,
                 IOCTL_SERIAL_GET_COMMSTATUS,
                 NULL,
                 0,
                 &LocalStat,
                 sizeof(LocalStat)
                 );

    if ( Status == STATUS_PENDING) {

        // Operation must complete before return & IoStatusBlock destroyed

        Status = NtWaitForSingleObject( SyncEvent, FALSE, NULL );
        if ( NT_SUCCESS(Status)) {

            Status = Iosb.Status;

        }
    }

    if (NT_ERROR(Status)) {

        CloseHandle(SyncEvent);
        BaseSetLastNTError(Status);
        return FALSE;

    }

    if (lpStat) {

        //
        // All is well up to this point.  Translate the NT values
        // into win32 values.
        //

        if (LocalStat.HoldReasons & SERIAL_TX_WAITING_FOR_CTS) {

            lpStat->fCtsHold = TRUE;

        } else {

            lpStat->fCtsHold = FALSE;

        }

        if (LocalStat.HoldReasons & SERIAL_TX_WAITING_FOR_DSR) {

            lpStat->fDsrHold = TRUE;

        } else {

            lpStat->fDsrHold = FALSE;

        }

        if (LocalStat.HoldReasons & SERIAL_TX_WAITING_FOR_DCD) {

            lpStat->fRlsdHold = TRUE;

        } else {

            lpStat->fRlsdHold = FALSE;

        }

        if (LocalStat.HoldReasons & SERIAL_TX_WAITING_FOR_XON) {

            lpStat->fXoffHold = TRUE;

        } else {

            lpStat->fXoffHold = FALSE;

        }

        if (LocalStat.HoldReasons & SERIAL_TX_WAITING_XOFF_SENT) {

            lpStat->fXoffSent = TRUE;

        } else {

            lpStat->fXoffSent = FALSE;

        }

        lpStat->fEof = LocalStat.EofReceived;
        lpStat->fTxim = LocalStat.WaitForImmediate;
        lpStat->cbInQue = LocalStat.AmountInInQueue;
        lpStat->cbOutQue = LocalStat.AmountInOutQueue;

    }

    if (lpErrors) {

        *lpErrors = 0;

        if (LocalStat.Errors & SERIAL_ERROR_BREAK) {

            *lpErrors = *lpErrors | CE_BREAK;

        }

        if (LocalStat.Errors & SERIAL_ERROR_FRAMING) {

            *lpErrors = *lpErrors | CE_FRAME;

        }

        if (LocalStat.Errors & SERIAL_ERROR_OVERRUN) {

            *lpErrors = *lpErrors | CE_OVERRUN;

        }

        if (LocalStat.Errors & SERIAL_ERROR_QUEUEOVERRUN) {

            *lpErrors = *lpErrors | CE_RXOVER;

        }

        if (LocalStat.Errors & SERIAL_ERROR_PARITY) {

            *lpErrors = *lpErrors | CE_RXPARITY;

        }

    }

    CloseHandle(SyncEvent);
    return TRUE;

}

BOOL
SetupComm(
    HANDLE hFile,
    DWORD dwInQueue,
    DWORD dwOutQueue
    )

/*++

Routine Description:

    The communication device is not initialized until SetupComm is
    called.  This function allocates space for receive and transmit
    queues.  These queues are used by the interrupt-driven transmit/
    receive software and are internal to the provider.

Arguments:

    hFile - Specifies the communication device to receive the settings.
            The CreateFile function returns this value.

    dwInQueue - Specifies the recommended size of the provider's
                internal receive queue in bytes.  This value must be
                even.  A value of -1 indicates that the default should
                be used.

    dwOutQueue - Specifies the recommended size of the provider's
                 internal transmit queue in bytes.  This value must be
                 even.  A value of -1 indicates that the default should
                 be used.

Return Value:

    The return value is TRUE if the function is successful or FALSE
    if an error occurs.

--*/

{

    NTSTATUS Status;

    HANDLE SyncEvent;
    IO_STATUS_BLOCK Iosb;
    SERIAL_QUEUE_SIZE NewSizes = {0};

    //
    // Make sure that the sizes are even.
    //

    if (dwOutQueue != ((DWORD)-1)) {

        if (((dwOutQueue/2)*2) != dwOutQueue) {

            SetLastError(ERROR_INVALID_DATA);
            return FALSE;

        }

    }

    if (dwInQueue != ((DWORD)-1)) {

        if (((dwInQueue/2)*2) != dwInQueue) {

            SetLastError(ERROR_INVALID_DATA);
            return FALSE;

        }

    }

    NewSizes.InSize = dwInQueue;
    NewSizes.OutSize = dwOutQueue;


    if (!(SyncEvent = CreateEvent(
                      NULL,
                      TRUE,
                      FALSE,
                      NULL
                      ))) {

        return FALSE;

    }

    Status = NtDeviceIoControlFile(
                 hFile,
                 SyncEvent,
                 NULL,
                 NULL,
                 &Iosb,
                 IOCTL_SERIAL_SET_QUEUE_SIZE,
                 &NewSizes,
                 sizeof(SERIAL_QUEUE_SIZE),
                 NULL,
                 0
                 );

    if ( Status == STATUS_PENDING) {

        // Operation must complete before return & IoStatusBlock destroyed

        Status = NtWaitForSingleObject( SyncEvent, FALSE, NULL );
        if ( NT_SUCCESS(Status)) {

            Status = Iosb.Status;

        }
    }

    if (NT_ERROR(Status)) {

        CloseHandle(SyncEvent);
        BaseSetLastNTError(Status);
        return FALSE;

    }

    CloseHandle(SyncEvent);
    return TRUE;

}

BOOL
EscapeCommFunction(
    HANDLE hFile,
    DWORD dwFunc
    )

/*++

Routine Description:

    This function directs the communication-device specified by the
    hFile parameter to carry out the extended function specified by
    the dwFunc parameter.

Arguments:

    hFile - Specifies the communication device to receive the settings.
            The CreateFile function returns this value.

    dwFunc - Specifies the function code of the extended function.

Return Value:

    The return value is TRUE if the function is successful or FALSE
    if an error occurs.

--*/

{

    NTSTATUS Status;
    IO_STATUS_BLOCK Iosb;
    ULONG ControlCode;
    HANDLE Event;

    switch (dwFunc) {

        case SETXOFF: {
            ControlCode = IOCTL_SERIAL_SET_XOFF;
            break;
        }

        case SETXON: {
            ControlCode = IOCTL_SERIAL_SET_XON;
            break;
        }

        case SETRTS: {
            ControlCode = IOCTL_SERIAL_SET_RTS;
            break;
        }

        case CLRRTS: {
            ControlCode = IOCTL_SERIAL_CLR_RTS;
            break;
        }

        case SETDTR: {
            ControlCode = IOCTL_SERIAL_SET_DTR;
            break;
        }

        case CLRDTR: {
            ControlCode = IOCTL_SERIAL_CLR_DTR;
            break;
        }

        case RESETDEV: {
            ControlCode = IOCTL_SERIAL_RESET_DEVICE;
            break;
        }

        case SETBREAK: {
            ControlCode = IOCTL_SERIAL_SET_BREAK_ON;
            break;
        }

        case CLRBREAK: {
            ControlCode = IOCTL_SERIAL_SET_BREAK_OFF;
            break;
        }
        default: {

            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;


        }
    }


    if (!(Event = CreateEvent(
                      NULL,
                      TRUE,
                      FALSE,
                      NULL
                      ))) {

        return FALSE;

    }

    Status = NtDeviceIoControlFile(
                 hFile,
                 Event,
                 NULL,
                 NULL,
                 &Iosb,
                 ControlCode,
                 NULL,
                 0,
                 NULL,
                 0
                 );

    if ( Status == STATUS_PENDING) {

        // Operation must complete before return & IoStatusBlock destroyed

        Status = NtWaitForSingleObject( Event, FALSE, NULL );
        if ( NT_SUCCESS(Status)) {

            Status = Iosb.Status;

        }
    }

    if (NT_ERROR(Status)) {

        CloseHandle(Event);
        BaseSetLastNTError(Status);
        return FALSE;

    }

    CloseHandle(Event);
    return TRUE;

}

BOOL
GetCommConfig(
    HANDLE hCommDev,
    LPCOMMCONFIG lpCC,
    LPDWORD lpdwSize
    )
{

    NTSTATUS Status;
    IO_STATUS_BLOCK Iosb;
    ULONG configLength;
    HANDLE Event;
    DWORD olddwSize = *lpdwSize;


    //
    // Ask the device how big the device config structure is.
    //

    if (!(Event = CreateEvent(
                      NULL,
                      TRUE,
                      FALSE,
                      NULL
                      ))) {

        return FALSE;

    }

    Status = NtDeviceIoControlFile(
                 hCommDev,
                 Event,
                 NULL,
                 NULL,
                 &Iosb,
                 IOCTL_SERIAL_CONFIG_SIZE,
                 NULL,
                 0,
                 &configLength,
                 sizeof(configLength)
                 );

    if ( Status == STATUS_PENDING) {

        // Operation must complete before return & IoStatusBlock destroyed

        Status = NtWaitForSingleObject( Event, FALSE, NULL );
        if ( NT_SUCCESS(Status)) {

            Status = Iosb.Status;

        }
    }

    if (NT_ERROR(Status)) {

        configLength = 0;

    }

    if (!configLength) {

        //
        // The size needed is simply the size of the comm config structure.
        //

        CloseHandle(Event);
        if (!ARGUMENT_PRESENT(lpdwSize)) {

            BaseSetLastNTError(STATUS_INVALID_PARAMETER);
            return FALSE;

        } else {

            *lpdwSize = sizeof(COMMCONFIG);

            if (ARGUMENT_PRESENT(lpCC)) {

                //
                // Fill in the random fields.
                //

                lpCC->dwSize = sizeof(COMMCONFIG);
                lpCC->wVersion = 1;
                lpCC->wReserved = 0;
                lpCC->dwProviderSubType = PST_RS232;
                lpCC->dwProviderOffset = 0;
                lpCC->dwProviderSize = 0;
                lpCC->wcProviderData[0] = 0;

                return GetCommState(
                           hCommDev,
                           &lpCC->dcb
                           );

            } else {

                return TRUE;

            }

        }

    } else {

        if (!ARGUMENT_PRESENT(lpdwSize)) {

            CloseHandle(Event);
            BaseSetLastNTError(STATUS_INVALID_PARAMETER);
            return FALSE;

        } else {

            if (*lpdwSize < sizeof(COMMCONFIG)) {

                CloseHandle(Event);
                BaseSetLastNTError(STATUS_INVALID_PARAMETER);
                *lpdwSize = configLength;
                return FALSE;

            } else {

                if (ARGUMENT_PRESENT(lpCC)) {

                    lpCC->wVersion = 1;
                    lpCC->dwProviderSubType = PST_MODEM;

                    if (*lpdwSize < configLength) {

                        lpCC->dwProviderOffset = 0;
                        lpCC->dwProviderSize = 0;
                        lpCC->wcProviderData[0] = 0;
                        *lpdwSize = sizeof(COMMCONFIG);
                        CloseHandle(Event);

                        return GetCommState(
                                   hCommDev,
                                   &lpCC->dcb
                                   );

                    } else {

                        *lpdwSize = configLength;

                        //
                        // Call down to the lower level serial provider
                        // if there is a passed comm config.  Assume
                        // that the buffer is as large as it needs to be.
                        // Parameter validation will insure that we
                        // can write to at least that much.
                        //

                        Status = NtDeviceIoControlFile(
                                     hCommDev,
                                     Event,
                                     NULL,
                                     NULL,
                                     &Iosb,
                                     IOCTL_SERIAL_GET_COMMCONFIG,
                                     NULL,
                                     0,
                                     lpCC,
                                     configLength
                                     );

                        if ( Status == STATUS_PENDING) {

                            // Operation must complete before return & IoStatusBlock destroyed

                            Status = NtWaitForSingleObject( Event, FALSE, NULL );
                            if ( NT_SUCCESS(Status)) {

                                Status = Iosb.Status;

                            }
                        }

                        if (NT_ERROR(Status)) {

                            CloseHandle(Event);
                            BaseSetLastNTError(Status);
                            return FALSE;

                        }

                        //
                        // Got the config stuff, get the comm state too.
                        //

                        CloseHandle(Event);
                        return GetCommState(
                                   hCommDev,
                                   &lpCC->dcb
                                   );

                    }

                } else {


                    *lpdwSize = configLength;
                    CloseHandle(Event);
                    return TRUE;

                }

            }

        }

    }

}

BOOL
GetCommMask(
    HANDLE hFile,
    LPDWORD lpEvtMask
    )

/*++

Routine Description:


    This function retrieves the value of the event mask for the handle
    hFile. The mask is not cleared

Arguments:

    hFile - Specifies the communication device to be examined.
            The CreateFile function returns this value.

    lpEvtMask - Points to a DWORD which is to receive the mask of events
                which are currently enabled.

Return Value:

    The return value is TRUE if the function is successful or FALSE
    if an error occurs.

--*/

{

    NTSTATUS Status;
    HANDLE SyncEvent;
    IO_STATUS_BLOCK Iosb;

    //
    // First we do an assert to make sure that the
    // values in the win header files are the same
    // as the nt serial interface, and that the
    // size of the win32 wait mask is the same size
    // as the nt wait mask.
    //

    ASSERT((SERIAL_EV_RXCHAR   == EV_RXCHAR  ) &&
           (SERIAL_EV_RXFLAG   == EV_RXFLAG  ) &&
           (SERIAL_EV_TXEMPTY  == EV_TXEMPTY ) &&
           (SERIAL_EV_CTS      == EV_CTS     ) &&
           (SERIAL_EV_DSR      == EV_DSR     ) &&
           (SERIAL_EV_RLSD     == EV_RLSD    ) &&
           (SERIAL_EV_BREAK    == EV_BREAK   ) &&
           (SERIAL_EV_ERR      == EV_ERR     ) &&
           (SERIAL_EV_RING     == EV_RING    ) &&
           (SERIAL_EV_PERR     == EV_PERR    ) &&
           (SERIAL_EV_RX80FULL == EV_RX80FULL) &&
           (SERIAL_EV_EVENT1   == EV_EVENT1  ) &&
           (SERIAL_EV_EVENT2   == EV_EVENT2  ) &&
           (sizeof(ULONG) == sizeof(DWORD)));

    //
    // All is well, get the mask from the driver.
    //

    if (!(SyncEvent = CreateEvent(
                          NULL,
                          TRUE,
                          FALSE,
                          NULL
                          ))) {

        return FALSE;

    }

    Status = NtDeviceIoControlFile(
                 hFile,
                 SyncEvent,
                 NULL,
                 NULL,
                 &Iosb,
                 IOCTL_SERIAL_GET_WAIT_MASK,
                 NULL,
                 0,
                 lpEvtMask,
                 sizeof(ULONG)
                 );

    if ( Status == STATUS_PENDING) {

        // Operation must complete before return & IoStatusBlock destroyed

        Status = NtWaitForSingleObject( SyncEvent, FALSE, NULL );
        if ( NT_SUCCESS(Status)) {

            Status = Iosb.Status;

        }
    }

    if (NT_ERROR(Status)) {

        CloseHandle(SyncEvent);
        BaseSetLastNTError(Status);
        return FALSE;

    }

    CloseHandle(SyncEvent);
    return TRUE;

}

BOOL
GetCommModemStatus(
    HANDLE hFile,
    LPDWORD lpModemStat
    )

/*++

Routine Description:

    This routine returns the most current value of the modem
    status registers non-delta values.


Arguments:

    hFile - Specifies the communication device to be examined.
            The CreateFile function returns this value.

    lpEvtMask - Points to a DWORD which is to receive the mask of
                non-delta values in the modem status register.

Return Value:

    The return value is TRUE if the function is successful or FALSE
    if an error occurs.

--*/

{

    NTSTATUS Status;
    HANDLE SyncEvent;
    IO_STATUS_BLOCK Iosb;

    if (!(SyncEvent = CreateEvent(
                          NULL,
                          TRUE,
                          FALSE,
                          NULL
                          ))) {

        return FALSE;

    }

    Status = NtDeviceIoControlFile(
                 hFile,
                 SyncEvent,
                 NULL,
                 NULL,
                 &Iosb,
                 IOCTL_SERIAL_GET_MODEMSTATUS,
                 NULL,
                 0,
                 lpModemStat,
                 sizeof(DWORD)
                 );

    if ( Status == STATUS_PENDING) {

        // Operation must complete before return & IoStatusBlock destroyed

        Status = NtWaitForSingleObject( SyncEvent, FALSE, NULL );
        if ( NT_SUCCESS(Status)) {

            Status = Iosb.Status;

        }
    }

    if (NT_ERROR(Status)) {

        CloseHandle(SyncEvent);
        BaseSetLastNTError(Status);
        return FALSE;

    }

    CloseHandle(SyncEvent);
    return TRUE;

}

BOOL
GetCommProperties(
    HANDLE hFile,
    LPCOMMPROP lpCommProp
    )

/*++

Routine Description:

    This function fills the ubffer pointed to by lpCommProp with the
    communications properties associated with the communications device
    specified by the hFile.

Arguments:

    hFile - Specifies the communication device to be examined.
            The CreateFile function returns this value.

    lpCommProp - Points to the COMMPROP data structure that is to
                 receive the communications properties structure.  This
                 structure defines certain properties of the communications
                 device.

Return Value:

    The return value is TRUE if the function is successful or FALSE
    if an error occurs.

--*/

{

    NTSTATUS Status;
    HANDLE SyncEvent;
    IO_STATUS_BLOCK Iosb;
    DWORD bufferLength;

    //
    // Make sure that the windows defines and the nt defines are
    // still in sync.
    //

    ASSERT((SERIAL_PCF_DTRDSR        == PCF_DTRDSR) &&
           (SERIAL_PCF_RTSCTS        == PCF_RTSCTS) &&
           (SERIAL_PCF_CD            == PCF_RLSD) &&
           (SERIAL_PCF_PARITY_CHECK  == PCF_PARITY_CHECK) &&
           (SERIAL_PCF_XONXOFF       == PCF_XONXOFF) &&
           (SERIAL_PCF_SETXCHAR      == PCF_SETXCHAR) &&
           (SERIAL_PCF_TOTALTIMEOUTS == PCF_TOTALTIMEOUTS) &&
           (SERIAL_PCF_INTTIMEOUTS   == PCF_INTTIMEOUTS) &&
           (SERIAL_PCF_SPECIALCHARS  == PCF_SPECIALCHARS) &&
           (SERIAL_PCF_16BITMODE     == PCF_16BITMODE) &&
           (SERIAL_SP_PARITY         == SP_PARITY) &&
           (SERIAL_SP_BAUD           == SP_BAUD) &&
           (SERIAL_SP_DATABITS       == SP_DATABITS) &&
           (SERIAL_SP_STOPBITS       == SP_STOPBITS) &&
           (SERIAL_SP_HANDSHAKING    == SP_HANDSHAKING) &&
           (SERIAL_SP_PARITY_CHECK   == SP_PARITY_CHECK) &&
           (SERIAL_SP_CARRIER_DETECT == SP_RLSD) &&
           (SERIAL_BAUD_075          == BAUD_075) &&
           (SERIAL_BAUD_110          == BAUD_110) &&
           (SERIAL_BAUD_134_5        == BAUD_134_5) &&
           (SERIAL_BAUD_150          == BAUD_150) &&
           (SERIAL_BAUD_300          == BAUD_300) &&
           (SERIAL_BAUD_600          == BAUD_600) &&
           (SERIAL_BAUD_1200         == BAUD_1200) &&
           (SERIAL_BAUD_1800         == BAUD_1800) &&
           (SERIAL_BAUD_2400         == BAUD_2400) &&
           (SERIAL_BAUD_4800         == BAUD_4800) &&
           (SERIAL_BAUD_7200         == BAUD_7200) &&
           (SERIAL_BAUD_9600         == BAUD_9600) &&
           (SERIAL_BAUD_14400        == BAUD_14400) &&
           (SERIAL_BAUD_19200        == BAUD_19200) &&
           (SERIAL_BAUD_38400        == BAUD_38400) &&
           (SERIAL_BAUD_56K          == BAUD_56K) &&
           (SERIAL_BAUD_57600        == BAUD_57600) &&
           (SERIAL_BAUD_115200       == BAUD_115200) &&
           (SERIAL_BAUD_USER         == BAUD_USER) &&
           (SERIAL_DATABITS_5        == DATABITS_5) &&
           (SERIAL_DATABITS_6        == DATABITS_6) &&
           (SERIAL_DATABITS_7        == DATABITS_7) &&
           (SERIAL_DATABITS_8        == DATABITS_8) &&
           (SERIAL_DATABITS_16       == DATABITS_16) &&
           (SERIAL_DATABITS_16X      == DATABITS_16X) &&
           (SERIAL_STOPBITS_10       == STOPBITS_10) &&
           (SERIAL_STOPBITS_15       == STOPBITS_15) &&
           (SERIAL_STOPBITS_20       == STOPBITS_20) &&
           (SERIAL_PARITY_NONE       == PARITY_NONE) &&
           (SERIAL_PARITY_ODD        == PARITY_ODD) &&
           (SERIAL_PARITY_EVEN       == PARITY_EVEN) &&
           (SERIAL_PARITY_MARK       == PARITY_MARK) &&
           (SERIAL_PARITY_SPACE      == PARITY_SPACE));
    ASSERT((SERIAL_SP_UNSPECIFIED    == PST_UNSPECIFIED) &&
           (SERIAL_SP_RS232          == PST_RS232) &&
           (SERIAL_SP_PARALLEL       == PST_PARALLELPORT) &&
           (SERIAL_SP_RS422          == PST_RS422) &&
           (SERIAL_SP_RS423          == PST_RS423) &&
           (SERIAL_SP_RS449          == PST_RS449) &&
           (SERIAL_SP_FAX            == PST_FAX) &&
           (SERIAL_SP_SCANNER        == PST_SCANNER) &&
           (SERIAL_SP_BRIDGE         == PST_NETWORK_BRIDGE) &&
           (SERIAL_SP_LAT            == PST_LAT) &&
           (SERIAL_SP_TELNET         == PST_TCPIP_TELNET) &&
           (SERIAL_SP_X25            == PST_X25));

    ASSERT(sizeof(SERIAL_COMMPROP) == sizeof(COMMPROP));
    //
    // Get the total length of what to pass down.  If the
    // application indicates that there is provider specific data
    // (by setting dwProvSpec1 to COMMPROP_INITIAILIZED) then
    // use what's at the start of the commprop.
    //

    bufferLength = sizeof(COMMPROP);

    if (lpCommProp->dwProvSpec1 == COMMPROP_INITIALIZED) {

        bufferLength = lpCommProp->wPacketLength;

    }

    //
    // Zero out the commprop.  This might create an access violation
    // if it isn't big enough.  Which is ok, since we would rather
    // get it before we create the sync event.
    //

    RtlZeroMemory(lpCommProp, bufferLength);

    if (!(SyncEvent = CreateEvent(
                          NULL,
                          TRUE,
                          FALSE,
                          NULL
                          ))) {

        return FALSE;

    }

    Status = NtDeviceIoControlFile(
                 hFile,
                 SyncEvent,
                 NULL,
                 NULL,
                 &Iosb,
                 IOCTL_SERIAL_GET_PROPERTIES,
                 NULL,
                 0,
                 lpCommProp,
                 bufferLength
                 );

    if ( Status == STATUS_PENDING) {

        // Operation must complete before return & IoStatusBlock destroyed

        Status = NtWaitForSingleObject( SyncEvent, FALSE, NULL );
        if ( NT_SUCCESS(Status)) {

            Status = Iosb.Status;

        }

    }

    if (NT_ERROR(Status)) {

        CloseHandle(SyncEvent);
        BaseSetLastNTError(Status);
        return FALSE;

    }

    CloseHandle(SyncEvent);
    return TRUE;

}

BOOL
GetCommState(
    HANDLE hFile,
    LPDCB lpDCB
    )

/*++

Routine Description:

    This function fills the buffer pointed to by the lpDCB parameter with
    the device control block of the communication device specified by hFile
    parameter.

Arguments:

    hFile - Specifies the communication device to be examined.
            The CreateFile function returns this value.

    lpDCB - Points to the DCB data structure that is to receive the current
            device control block.  The structure defines the control settings
            for the device.

Return Value:

    The return value is TRUE if the function is successful or FALSE
    if an error occurs.

--*/

{

    SERIAL_BAUD_RATE LocalBaud;
    SERIAL_LINE_CONTROL LineControl;
    SERIAL_CHARS Chars;
    SERIAL_HANDFLOW HandFlow;
    IO_STATUS_BLOCK Iosb;
    NTSTATUS Status;

    //
    // Given the possiblity that the app may be doing asynchronous
    // io we need an event to wait on.
    //
    // We need to make sure that any exit to this routine closes this
    // event handle.
    //
    HANDLE SyncEvent;

    //
    // Make sure the windows mapping is the same as the NT mapping.
    //

    ASSERT((ONESTOPBIT == STOP_BIT_1) &&
           (ONE5STOPBITS == STOP_BITS_1_5) &&
           (TWOSTOPBITS == STOP_BITS_2));

    ASSERT((NOPARITY == NO_PARITY) &&
           (ODDPARITY == ODD_PARITY) &&
           (EVENPARITY == EVEN_PARITY) &&
           (MARKPARITY == MARK_PARITY) &&
           (SPACEPARITY == SPACE_PARITY));

    //
    // Zero out the dcb.  This might create an access violation
    // if it isn't big enough.  Which is ok, since we would rather
    // get it before we create the sync event.
    //

    RtlZeroMemory(lpDCB, sizeof(DCB));

    lpDCB->DCBlength = sizeof(DCB);
    lpDCB->fBinary = TRUE;

    if (!(SyncEvent = CreateEvent(
                          NULL,
                          TRUE,
                          FALSE,
                          NULL
                          ))) {

        return FALSE;

    }

    Status = NtDeviceIoControlFile(
                 hFile,
                 SyncEvent,
                 NULL,
                 NULL,
                 &Iosb,
                 IOCTL_SERIAL_GET_BAUD_RATE,
                 NULL,
                 0,
                 &LocalBaud,
                 sizeof(LocalBaud)
                 );

    if ( Status == STATUS_PENDING) {

        // Operation must complete before return & IoStatusBlock destroyed

        Status = NtWaitForSingleObject( SyncEvent, FALSE, NULL );
        if ( NT_SUCCESS(Status)) {

            Status = Iosb.Status;

        }

    }

    if (NT_ERROR(Status)) {

        CloseHandle(SyncEvent);
        BaseSetLastNTError(Status);
        return FALSE;

    }

    lpDCB->BaudRate = LocalBaud.BaudRate;

    Status = NtDeviceIoControlFile(
                 hFile,
                 SyncEvent,
                 NULL,
                 NULL,
                 &Iosb,
                 IOCTL_SERIAL_GET_LINE_CONTROL,
                 NULL,
                 0,
                 &LineControl,
                 sizeof(LineControl)
                 );

    if ( Status == STATUS_PENDING) {

        // Operation must complete before return & IoStatusBlock destroyed

        Status = NtWaitForSingleObject( SyncEvent, FALSE, NULL );
        if ( NT_SUCCESS(Status)) {

            Status = Iosb.Status;

        }

    }

    if (NT_ERROR(Status)) {

        CloseHandle(SyncEvent);
        BaseSetLastNTError(Status);
        return FALSE;

    }

    lpDCB->Parity = LineControl.Parity;
    lpDCB->ByteSize = LineControl.WordLength;
    lpDCB->StopBits = LineControl.StopBits;

    Status = NtDeviceIoControlFile(
                 hFile,
                 SyncEvent,
                 NULL,
                 NULL,
                 &Iosb,
                 IOCTL_SERIAL_GET_CHARS,
                 NULL,
                 0,
                 &Chars,
                 sizeof(Chars)
                 );

    if ( Status == STATUS_PENDING) {

        // Operation must complete before return & IoStatusBlock destroyed

        Status = NtWaitForSingleObject( SyncEvent, FALSE, NULL );
        if ( NT_SUCCESS(Status)) {

            Status = Iosb.Status;

        }

    }

    if (NT_ERROR(Status)) {

        CloseHandle(SyncEvent);
        BaseSetLastNTError(Status);
        return FALSE;

    }

    lpDCB->XonChar = Chars.XonChar;
    lpDCB->XoffChar = Chars.XoffChar;
    lpDCB->ErrorChar = Chars.ErrorChar;
    lpDCB->EofChar = Chars.EofChar;
    lpDCB->EvtChar = Chars.EventChar;

    Status = NtDeviceIoControlFile(
                 hFile,
                 SyncEvent,
                 NULL,
                 NULL,
                 &Iosb,
                 IOCTL_SERIAL_GET_HANDFLOW,
                 NULL,
                 0,
                 &HandFlow,
                 sizeof(HandFlow)
                 );

    if ( Status == STATUS_PENDING) {

        // Operation must complete before return & IoStatusBlock destroyed

        Status = NtWaitForSingleObject( SyncEvent, FALSE, NULL );
        if ( NT_SUCCESS(Status)) {

            Status = Iosb.Status;

        }

    }

    if (NT_ERROR(Status)) {

        CloseHandle(SyncEvent);
        BaseSetLastNTError(Status);
        return FALSE;

    }

    if (HandFlow.ControlHandShake & SERIAL_CTS_HANDSHAKE) {

        lpDCB->fOutxCtsFlow = TRUE;

    }

    if (HandFlow.ControlHandShake & SERIAL_DSR_HANDSHAKE) {

        lpDCB->fOutxDsrFlow = TRUE;

    }

    if (HandFlow.FlowReplace & SERIAL_AUTO_TRANSMIT) {

        lpDCB->fOutX = TRUE;

    }

    if (HandFlow.FlowReplace & SERIAL_AUTO_RECEIVE) {

        lpDCB->fInX = TRUE;

    }

    if (HandFlow.FlowReplace & SERIAL_NULL_STRIPPING) {

        lpDCB->fNull = TRUE;

    }

    if (HandFlow.FlowReplace & SERIAL_ERROR_CHAR) {

        lpDCB->fErrorChar = TRUE;

    }

    if (HandFlow.FlowReplace & SERIAL_XOFF_CONTINUE) {

        lpDCB->fTXContinueOnXoff = TRUE;

    }

    if (HandFlow.ControlHandShake & SERIAL_ERROR_ABORT) {

        lpDCB->fAbortOnError = TRUE;

    }

    switch (HandFlow.FlowReplace & SERIAL_RTS_MASK) {
        case 0:
            lpDCB->fRtsControl = RTS_CONTROL_DISABLE;
            break;
        case SERIAL_RTS_CONTROL:
            lpDCB->fRtsControl = RTS_CONTROL_ENABLE;
            break;
        case SERIAL_RTS_HANDSHAKE:
            lpDCB->fRtsControl = RTS_CONTROL_HANDSHAKE;
            break;
        case SERIAL_TRANSMIT_TOGGLE:
            lpDCB->fRtsControl = RTS_CONTROL_TOGGLE;
            break;
    }

    switch (HandFlow.ControlHandShake & SERIAL_DTR_MASK) {
        case 0:
            lpDCB->fDtrControl = DTR_CONTROL_DISABLE;
            break;
        case SERIAL_DTR_CONTROL:
            lpDCB->fDtrControl = DTR_CONTROL_ENABLE;
            break;
        case SERIAL_DTR_HANDSHAKE:
            lpDCB->fDtrControl = DTR_CONTROL_HANDSHAKE;
            break;
    }

    lpDCB->fDsrSensitivity =
        (HandFlow.ControlHandShake & SERIAL_DSR_SENSITIVITY)?(TRUE):(FALSE);
    lpDCB->XonLim = (WORD)HandFlow.XonLimit;
    lpDCB->XoffLim = (WORD)HandFlow.XoffLimit;

    CloseHandle(SyncEvent);
    return TRUE;
}

BOOL
GetCommTimeouts(
    HANDLE hFile,
    LPCOMMTIMEOUTS lpCommTimeouts
    )

/*++

Routine Description:

    This function returns the timeout characteristics for all read and
    write operations on the handle specified by hFile.

Arguments:

    hFile - Specifies the communication device to be examined.
            The CreateFile function returns this value.

    lpCommTimeouts - Points to a structure which is to receive the
                     current communications timeouts.


Return Value:

    The return value is TRUE if the function is successful or FALSE
    if an error occurs.

--*/

{

    SERIAL_TIMEOUTS To;
    NTSTATUS Status;
    IO_STATUS_BLOCK Iosb;
    HANDLE Event;

    if (!(Event = CreateEvent(
                      NULL,
                      TRUE,
                      FALSE,
                      NULL
                      ))) {

        return FALSE;

    } else {


        Status = NtDeviceIoControlFile(
                     hFile,
                     Event,
                     NULL,
                     NULL,
                     &Iosb,
                     IOCTL_SERIAL_GET_TIMEOUTS,
                     NULL,
                     0,
                     &To,
                     sizeof(To)
                     );

        if ( Status == STATUS_PENDING) {

            // Operation must complete before return & IoStatusBlock destroyed

            Status = NtWaitForSingleObject( Event, FALSE, NULL );
            if ( NT_SUCCESS( Status )) {

                Status = Iosb.Status;

            }

        }

        if (NT_ERROR(Status)) {

            BaseSetLastNTError(Status);
            CloseHandle(Event);
            return FALSE;

        }

        CloseHandle(Event);

        //
        // Everything went ok.  Move the value from the Nt records
        // to the windows record.
        //

        lpCommTimeouts->ReadIntervalTimeout = To.ReadIntervalTimeout;
        lpCommTimeouts->ReadTotalTimeoutMultiplier = To.ReadTotalTimeoutMultiplier;
        lpCommTimeouts->ReadTotalTimeoutConstant = To.ReadTotalTimeoutConstant;
        lpCommTimeouts->WriteTotalTimeoutMultiplier = To.WriteTotalTimeoutMultiplier;
        lpCommTimeouts->WriteTotalTimeoutConstant = To.WriteTotalTimeoutConstant;

        return TRUE;

    }

}

BOOL
PurgeComm(
    HANDLE hFile,
    DWORD dwFlags
    )

/*++

Routine Description:

    This function is used to purge all characters from the transmit
    or receive queues of the communication device specified by the
    hFile parameter.  The dwFlags parameter specifies what function
    is to be performed.

Arguments:

    hFile - Specifies the communication device to be purged.
            The CreateFile function returns this value.

    dwFlags - Bit mask defining actions to be taken.

Return Value:

    The return value is TRUE if the function is successful or FALSE
    if an error occurs.

--*/

{
    HANDLE Event;
    NTSTATUS Status;
    IO_STATUS_BLOCK Iosb;

    if (!(Event = CreateEvent(
                      NULL,
                      TRUE,
                      FALSE,
                      NULL
                      ))) {

        return FALSE;

    } else {

        Status = NtDeviceIoControlFile(
                     hFile,
                     Event,
                     NULL,
                     NULL,
                     &Iosb,
                     IOCTL_SERIAL_PURGE,
                     &dwFlags,
                     sizeof(ULONG),
                     NULL,
                     0
                     );

        if ( Status == STATUS_PENDING) {

            // Operation must complete before return & IoStatusBlock destroyed

            Status = NtWaitForSingleObject( Event, FALSE, NULL );
            if ( NT_SUCCESS( Status )) {

                Status = Iosb.Status;

            }

        }

        if (NT_ERROR(Status)) {

            CloseHandle(Event);
            BaseSetLastNTError(Status);
            return FALSE;

        }

        CloseHandle(Event);
        return TRUE;

    }


}

BOOL
SetCommBreak(
    HANDLE hFile
    )

/*++

Routine Description:

    The function suspends character transmission and places the transmission
    line in a break state until the break condition is cleared..

Arguments:

    hFile - Specifies the communication device to be suspended.
            The CreateFile function returns this value.

Return Value:

    The return value is TRUE if the function is successful or FALSE
    if an error occurs.

--*/

{

    return EscapeCommFunction(hFile,SETBREAK);

}

BOOL
SetCommConfig(
    HANDLE hCommDev,
    LPCOMMCONFIG lpCC,
    DWORD dwSize
    )

{


    NTSTATUS Status;
    IO_STATUS_BLOCK Iosb;
    HANDLE Event;
    LPCOMMCONFIG comConf = (PVOID)lpCC;


    if (lpCC->dwProviderOffset) {

        if (!(Event = CreateEvent(
                          NULL,
                          TRUE,
                          FALSE,
                          NULL
                          ))) {

            return FALSE;

        }

        //
        //
        // Call the driver to set the config structure.
        //

        Status = NtDeviceIoControlFile(
                     hCommDev,
                     Event,
                     NULL,
                     NULL,
                     &Iosb,
                     IOCTL_SERIAL_SET_COMMCONFIG,
                     lpCC,
                     dwSize,
                     NULL,
                     0
                     );

        if ( Status == STATUS_PENDING) {

            // Operation must complete before return & IoStatusBlock destroyed

            Status = NtWaitForSingleObject( Event, FALSE, NULL );
            if ( NT_SUCCESS(Status)) {

                Status = Iosb.Status;

            }
        }

        if (NT_ERROR(Status)) {

            CloseHandle(Event);
            BaseSetLastNTError(Status);
            return FALSE;

        }

        CloseHandle(Event);

    }

    return SetCommState(
               hCommDev,
               &comConf->dcb
               );
}

BOOL
SetCommMask(
    HANDLE hFile,
    DWORD dwEvtMask
    )

/*++

Routine Description:

    The function enables the event mask of the communication device
    specified by the hFile parameter.  The bits of the nEvtMask parameter
    define which events are to be enabled.

Arguments:

    hFile - Specifies the communication device to receive the settings.
            The CreateFile function returns this value.

    dwEvtMask - Specifies which events are to enabled.

Return Value:

    The return value is TRUE if the function is successful or FALSE
    if an error occurs.

--*/

{

    NTSTATUS Status;
    IO_STATUS_BLOCK Iosb;
    HANDLE Event;

    //
    // First we do an assert to make sure that the
    // values in the win header files are the same
    // as the nt serial interface and the the size
    // mask that serial expects is the same as the
    // size that win32 expects.
    //

    ASSERT((SERIAL_EV_RXCHAR   == EV_RXCHAR  ) &&
           (SERIAL_EV_RXFLAG   == EV_RXFLAG  ) &&
           (SERIAL_EV_TXEMPTY  == EV_TXEMPTY ) &&
           (SERIAL_EV_CTS      == EV_CTS     ) &&
           (SERIAL_EV_DSR      == EV_DSR     ) &&
           (SERIAL_EV_RLSD     == EV_RLSD    ) &&
           (SERIAL_EV_BREAK    == EV_BREAK   ) &&
           (SERIAL_EV_ERR      == EV_ERR     ) &&
           (SERIAL_EV_RING     == EV_RING    ) &&
           (SERIAL_EV_PERR     == EV_PERR    ) &&
           (SERIAL_EV_RX80FULL == EV_RX80FULL) &&
           (SERIAL_EV_EVENT1   == EV_EVENT1  ) &&
           (SERIAL_EV_EVENT2   == EV_EVENT2  ) &&
           (sizeof(DWORD) == sizeof(ULONG)));


    //
    // Make sure that the users mask doesn't contain any values
    // we don't support.
    //

    if (dwEvtMask & (~(EV_RXCHAR   |
                       EV_RXFLAG   |
                       EV_TXEMPTY  |
                       EV_CTS      |
                       EV_DSR      |
                       EV_RLSD     |
                       EV_BREAK    |
                       EV_ERR      |
                       EV_RING     |
                       EV_PERR     |
                       EV_RX80FULL |
                       EV_EVENT1   |
                       EV_EVENT2))) {

        SetLastError(ERROR_INVALID_DATA);
        return FALSE;

    }


    if (!(Event = CreateEvent(
                      NULL,
                      TRUE,
                      FALSE,
                      NULL
                      ))) {

        return FALSE;

    } else {


        //
        // All is well, send the mask to the driver.
        //

        ULONG LocalMask = dwEvtMask;

        Status = NtDeviceIoControlFile(
                     hFile,
                     Event,
                     NULL,
                     NULL,
                     &Iosb,
                     IOCTL_SERIAL_SET_WAIT_MASK,
                     &LocalMask,
                     sizeof(ULONG),
                     NULL,
                     0
                     );

        if ( Status == STATUS_PENDING) {

            // Operation must complete before return & IoStatusBlock destroyed

            Status = NtWaitForSingleObject( Event, FALSE, NULL );
            if ( NT_SUCCESS( Status )) {

                Status = Iosb.Status;

            }

        }

        if (NT_ERROR(Status)) {

            CloseHandle(Event);
            BaseSetLastNTError(Status);
            return FALSE;

        }

        CloseHandle(Event);
        return TRUE;

    }

}

BOOL
SetCommState(
    HANDLE hFile,
    LPDCB lpDCB
    )

/*++

Routine Description:

    The SetCommState function sets a communication device to the state
    specified in the lpDCB parameter.  The device is identified by the
    hFile parameter.  This function reinitializes all hardwae and controls
    as specified byt the lpDCB, but does not empty the transmit or
    receive queues.

Arguments:

    hFile - Specifies the communication device to receive the settings.
            The CreateFile function returns this value.

    lpDCB - Points to a DCB structure that contains the desired
            communications setting for the device.

Return Value:

    The return value is TRUE if the function is successful or FALSE
    if an error occurs.

--*/

{

    SERIAL_BAUD_RATE LocalBaud;
    SERIAL_LINE_CONTROL LineControl;
    SERIAL_CHARS Chars;
    SERIAL_HANDFLOW HandFlow = {0};
    IO_STATUS_BLOCK Iosb;
    NTSTATUS Status;

    //
    // Keep a copy of what the DCB was like before we started
    // changing things.  If some error occurs we can use
    // it to restore the old setup.
    //
    DCB OldDcb;

    //
    // Given the possiblity that the app may be doing asynchronous
    // io we need an event to wait on.  While it would be very
    // strange to be setting the comm state while IO is active
    // we need to make sure we don't compound the problem by
    // returning before this API's IO is actually finished.  This
    // can happen because the file handle is set on the completion
    // of any IO.
    //
    // We need to make sure that any exit to this routine closes this
    // event handle.
    //
    HANDLE SyncEvent;

    if (GetCommState(
            hFile,
            &OldDcb
            )) {

        //
        // Try to set the baud rate.  If we fail here, we just return
        // because we never actually got to set anything.
        //

        if (!(SyncEvent = CreateEvent(
                              NULL,
                              TRUE,
                              FALSE,
                              NULL
                              ))) {

            return FALSE;

        }

        LocalBaud.BaudRate = lpDCB->BaudRate;
        Status = NtDeviceIoControlFile(
                     hFile,
                     SyncEvent,
                     NULL,
                     NULL,
                     &Iosb,
                     IOCTL_SERIAL_SET_BAUD_RATE,
                     &LocalBaud,
                     sizeof(LocalBaud),
                     NULL,
                     0
                     );

        if ( Status == STATUS_PENDING) {

            // Operation must complete before return & IoStatusBlock destroyed

            Status = NtWaitForSingleObject( SyncEvent, FALSE, NULL );
            if ( NT_SUCCESS(Status)) {

                Status = Iosb.Status;

            }

        }

        if (NT_ERROR(Status)) {

            CloseHandle(SyncEvent);
            BaseSetLastNTError(Status);
            return FALSE;

        }

        LineControl.StopBits = lpDCB->StopBits;
        LineControl.Parity = lpDCB->Parity;
        LineControl.WordLength = lpDCB->ByteSize;
        LocalBaud.BaudRate = lpDCB->BaudRate;
        Chars.XonChar   = lpDCB->XonChar;
        Chars.XoffChar  = lpDCB->XoffChar;
        Chars.ErrorChar = lpDCB->ErrorChar;
        Chars.BreakChar = lpDCB->ErrorChar;
        Chars.EofChar   = lpDCB->EofChar;
        Chars.EventChar = lpDCB->EvtChar;

        HandFlow.FlowReplace &= ~SERIAL_RTS_MASK;
        switch (lpDCB->fRtsControl) {
            case RTS_CONTROL_DISABLE:
                break;
            case RTS_CONTROL_ENABLE:
                HandFlow.FlowReplace |= SERIAL_RTS_CONTROL;
                break;
            case RTS_CONTROL_HANDSHAKE:
                HandFlow.FlowReplace |= SERIAL_RTS_HANDSHAKE;
                break;
            case RTS_CONTROL_TOGGLE:
                HandFlow.FlowReplace |= SERIAL_TRANSMIT_TOGGLE;
                break;
            default:
                SetCommState(
                    hFile,
                    &OldDcb
                    );
                CloseHandle(SyncEvent);
                BaseSetLastNTError(STATUS_INVALID_PARAMETER);
                return FALSE;
        }

        HandFlow.ControlHandShake &= ~SERIAL_DTR_MASK;
        switch (lpDCB->fDtrControl) {
            case DTR_CONTROL_DISABLE:
                break;
            case DTR_CONTROL_ENABLE:
                HandFlow.ControlHandShake |= SERIAL_DTR_CONTROL;
                break;
            case DTR_CONTROL_HANDSHAKE:
                HandFlow.ControlHandShake |= SERIAL_DTR_HANDSHAKE;
                break;
            default:
                SetCommState(
                    hFile,
                    &OldDcb
                    );
                CloseHandle(SyncEvent);
                BaseSetLastNTError(STATUS_INVALID_PARAMETER);
                return FALSE;
        }

        if (lpDCB->fDsrSensitivity) {

            HandFlow.ControlHandShake |= SERIAL_DSR_SENSITIVITY;

        }

        if (lpDCB->fOutxCtsFlow) {

            HandFlow.ControlHandShake |= SERIAL_CTS_HANDSHAKE;

        }

        if (lpDCB->fOutxDsrFlow) {

            HandFlow.ControlHandShake |= SERIAL_DSR_HANDSHAKE;

        }

        if (lpDCB->fOutX) {

            HandFlow.FlowReplace |= SERIAL_AUTO_TRANSMIT;

        }

        if (lpDCB->fInX) {

            HandFlow.FlowReplace |= SERIAL_AUTO_RECEIVE;

        }

        if (lpDCB->fNull) {

            HandFlow.FlowReplace |= SERIAL_NULL_STRIPPING;

        }

        if (lpDCB->fErrorChar) {

            HandFlow.FlowReplace |= SERIAL_ERROR_CHAR;
        }

        if (lpDCB->fTXContinueOnXoff) {

            HandFlow.FlowReplace |= SERIAL_XOFF_CONTINUE;

        }

        if (lpDCB->fAbortOnError) {

            HandFlow.ControlHandShake |= SERIAL_ERROR_ABORT;

        }

        //
        // For win95 compatiblity, if we are setting with
        // xxx_control_XXXXXXX then set the modem status line
        // to that state.
        //

        if (lpDCB->fRtsControl == RTS_CONTROL_ENABLE) {

            EscapeCommFunction(
                hFile,
                SETRTS
                );

        } else if (lpDCB->fRtsControl == RTS_CONTROL_DISABLE) {

            EscapeCommFunction(
                hFile,
                CLRRTS
                );

        }
        if (lpDCB->fDtrControl == DTR_CONTROL_ENABLE) {

            EscapeCommFunction(
                hFile,
                SETDTR
                );

        } else if (lpDCB->fDtrControl == DTR_CONTROL_DISABLE) {

            EscapeCommFunction(
                hFile,
                CLRDTR
                );

        }




        HandFlow.XonLimit = lpDCB->XonLim;
        HandFlow.XoffLimit = lpDCB->XoffLim;


        Status = NtDeviceIoControlFile(
                     hFile,
                     SyncEvent,
                     NULL,
                     NULL,
                     &Iosb,
                     IOCTL_SERIAL_SET_LINE_CONTROL,
                     &LineControl,
                     sizeof(LineControl),
                     NULL,
                     0
                     );

        if ( Status == STATUS_PENDING) {

            // Operation must complete before return & IoStatusBlock destroyed

            Status = NtWaitForSingleObject( SyncEvent, FALSE, NULL );
            if ( NT_SUCCESS(Status)) {

                Status = Iosb.Status;

            }

        }

        if (NT_ERROR(Status)) {

            CloseHandle(SyncEvent);
            SetCommState(
                hFile,
                &OldDcb
                );
            BaseSetLastNTError(Status);
            return FALSE;

        }

        Status = NtDeviceIoControlFile(
                     hFile,
                     SyncEvent,
                     NULL,
                     NULL,
                     &Iosb,
                     IOCTL_SERIAL_SET_CHARS,
                     &Chars,
                     sizeof(Chars),
                     NULL,
                     0
                     );

        if ( Status == STATUS_PENDING) {

            // Operation must complete before return & IoStatusBlock destroyed

            Status = NtWaitForSingleObject( SyncEvent, FALSE, NULL );
            if ( NT_SUCCESS(Status)) {

                Status = Iosb.Status;

            }

        }

        if (NT_ERROR(Status)) {

            CloseHandle(SyncEvent);
            SetCommState(
                hFile,
                &OldDcb
                );
            BaseSetLastNTError(Status);
            return FALSE;

        }

        Status = NtDeviceIoControlFile(
                     hFile,
                     SyncEvent,
                     NULL,
                     NULL,
                     &Iosb,
                     IOCTL_SERIAL_SET_HANDFLOW,
                     &HandFlow,
                     sizeof(HandFlow),
                     NULL,
                     0
                     );

        if ( Status == STATUS_PENDING) {

            // Operation must complete before return & IoStatusBlock destroyed

            Status = NtWaitForSingleObject( SyncEvent, FALSE, NULL );
            if ( NT_SUCCESS(Status)) {

                Status = Iosb.Status;

            }

        }

        if (NT_ERROR(Status)) {

            CloseHandle(SyncEvent);
            SetCommState(
                hFile,
                &OldDcb
                );
            BaseSetLastNTError(Status);
            return FALSE;

        }
        CloseHandle(SyncEvent);
        return TRUE;

    }

    return FALSE;

}

BOOL
SetCommTimeouts(
    HANDLE hFile,
    LPCOMMTIMEOUTS lpCommTimeouts
    )

/*++

Routine Description:

    This function establishes the timeout characteristics for all
    read and write operations on the handle specified by hFile.

Arguments:

    hFile - Specifies the communication device to receive the settings.
            The CreateFile function returns this value.

    lpCommTimeouts - Points to a structure containing timeout parameters.

Return Value:

    The return value is TRUE if the function is successful or FALSE
    if an error occurs.

--*/

{

    SERIAL_TIMEOUTS To;
    NTSTATUS Status;
    IO_STATUS_BLOCK Iosb;
    HANDLE Event;

    To.ReadIntervalTimeout = lpCommTimeouts->ReadIntervalTimeout;
    To.ReadTotalTimeoutMultiplier = lpCommTimeouts->ReadTotalTimeoutMultiplier;
    To.ReadTotalTimeoutConstant = lpCommTimeouts->ReadTotalTimeoutConstant;
    To.WriteTotalTimeoutMultiplier = lpCommTimeouts->WriteTotalTimeoutMultiplier;
    To.WriteTotalTimeoutConstant = lpCommTimeouts->WriteTotalTimeoutConstant;


    if (!(Event = CreateEvent(
                      NULL,
                      TRUE,
                      FALSE,
                      NULL
                      ))) {

        return FALSE;

    } else {

        Status = NtDeviceIoControlFile(
                     hFile,
                     Event,
                     NULL,
                     NULL,
                     &Iosb,
                     IOCTL_SERIAL_SET_TIMEOUTS,
                     &To,
                     sizeof(To),
                     NULL,
                     0
                     );

        if ( Status == STATUS_PENDING) {

            // Operation must complete before return & IoStatusBlock destroyed

            Status = NtWaitForSingleObject( Event, FALSE, NULL );
            if ( NT_SUCCESS(Status)) {

                Status = Iosb.Status;

            }

        }

        if (NT_ERROR(Status)) {

            CloseHandle(Event);
            BaseSetLastNTError(Status);
            return FALSE;

        }

        CloseHandle(Event);
        return TRUE;

    }

}

BOOL
TransmitCommChar(
    HANDLE hFile,
    char cChar
    )

/*++

Routine Description:

    The function marks the character specified by the cChar parameter
    for immediate transmission, by placing it at the head of the transmit
    queue.

Arguments:

    hFile - Specifies the communication device to send the character.
            The CreateFile function returns this value.

    cChar - Specifies the character to be placed in the recieve queue.

Return Value:

    The return value is TRUE if the function is successful or FALSE
    if an error occurs.

--*/

{

    NTSTATUS Status;
    IO_STATUS_BLOCK Iosb;
    HANDLE Event;

    if (!(Event = CreateEvent(
                      NULL,
                      TRUE,
                      FALSE,
                      NULL
                      ))) {

        return FALSE;

    } else {

        Status = NtDeviceIoControlFile(
                     hFile,
                     Event,
                     NULL,
                     NULL,
                     &Iosb,
                     IOCTL_SERIAL_IMMEDIATE_CHAR,
                     &cChar,
                     sizeof(UCHAR),
                     NULL,
                     0
                     );

        if ( Status == STATUS_PENDING) {

            // Operation must complete before return & IoStatusBlock destroyed

            Status = NtWaitForSingleObject( Event, FALSE, NULL );
            if ( NT_SUCCESS(Status)) {

                Status = Iosb.Status;

            }

        }

        if (NT_ERROR(Status)) {

            CloseHandle(Event);
            BaseSetLastNTError(Status);
            return FALSE;

        }

        CloseHandle(Event);
        return TRUE;

    }

}

BOOL
WaitCommEvent(
    HANDLE hFile,
    LPDWORD lpEvtMask,
    LPOVERLAPPED lpOverlapped
    )

/*++

Routine Description:

    This function will wait until any of the events occur that were
    provided as the nEvtMask parameter to SetcommMask.  If while waiting
    the event mask is changed (via another call to SetCommMask), the
    function will return immediately.  The function will fill the EvtMask
    pointed to by the lpEvtMask parameter with the reasons that the
    wait was satisfied.

Arguments:

    hFile - Specifies the communication device to be waited on.
            The CreateFile function returns this value.

    lpEvtMask - Points to a mask that will receive the reason that
                the wait was satisfied.

    lpOverLapped - An optional overlapped handle.

Return Value:

    The return value is TRUE if the function is successful or FALSE
    if an error occurs.

--*/

{

    NTSTATUS Status;

    if (ARGUMENT_PRESENT(lpOverlapped)) {
        lpOverlapped->Internal = (DWORD)STATUS_PENDING;

        Status = NtDeviceIoControlFile(
                     hFile,
                     lpOverlapped->hEvent,
                     NULL,
                     (ULONG_PTR)lpOverlapped->hEvent & 1 ? NULL : lpOverlapped,
                     (PIO_STATUS_BLOCK)&lpOverlapped->Internal,
                     IOCTL_SERIAL_WAIT_ON_MASK,
                     NULL,
                     0,
                     lpEvtMask,
                     sizeof(ULONG)
                     );

        if (!NT_ERROR(Status) && (Status != STATUS_PENDING)) {

            return TRUE;

        } else {

            BaseSetLastNTError(Status);
            return FALSE;

        }

    } else {

        IO_STATUS_BLOCK Iosb;
        HANDLE Event;

        if (!(Event = CreateEvent(
                          NULL,
                          TRUE,
                          FALSE,
                          NULL
                          ))) {

            return FALSE;

        } else {

            Status = NtDeviceIoControlFile(
                         hFile,
                         Event,
                         NULL,
                         NULL,
                         &Iosb,
                         IOCTL_SERIAL_WAIT_ON_MASK,
                         NULL,
                         0,
                         lpEvtMask,
                         sizeof(ULONG)
                         );

            if ( Status == STATUS_PENDING) {

                //
                // Operation must complete before return &
                // IoStatusBlock destroyed

                Status = NtWaitForSingleObject( Event, FALSE, NULL );
                if ( NT_SUCCESS(Status)) {

                    Status = Iosb.Status;

                }

            }

            CloseHandle(Event);

            if (NT_ERROR(Status)) {

                BaseSetLastNTError(Status);
                return FALSE;

            }

            return TRUE;

        }

    }

}
