/*++

Copyright (c) 1991-92  Microsoft Corporation

Module Name:

    Alert.c

Abstract:

    This file contains NetAlertRaise().
    for the NetAlert API.

Author:

    John Rogers (JohnRo) 03-Apr-1992

Environment:

    User Mode - Win32

Revision History:

    04-Apr-1992 JohnRo
        Created NetAlertRaise() API from RitaW's AlTest (alerter svc test).
    06-Apr-1992 JohnRo
        Added/improved error checking.
    08-May-1992 JohnRo
        Quiet normal debug output.
    08-May-1992 JohnRo
        Use <prefix.h> equates.

--*/


// These must be included first:

#include <windows.h>    // DWORD, CreateFile(), etc.
#include <lmcons.h>     // IN, NET_API_STATUS, etc.

// These may be included in any order:

#include <lmalert.h>    // My prototype, ALERTER_MAILSLOT, LPSTD_ALERT, etc.
#include <lmerr.h>      // NO_ERROR, NERR_NoRoom, etc.
#include <netdebug.h>   // NetpKdPrint(()), FORMAT_ equates, etc.
#include <prefix.h>     // PREFIX_ equates.
#include <string.h>     // memcpy().
#include <strucinf.h>   // NetpAlertStructureInfo().
#include <timelib.h>    // time_now().
#include <tstr.h>       // TCHAR_EOS.


#if DBG
// BUGBUG: Put a bit for this somewhere!
//#define IF_DEBUG( anything )  if (TRUE)
#define IF_DEBUG( anything )  if (FALSE)
#else
#define IF_DEBUG( anything )  if (FALSE)
#endif


NET_API_STATUS NET_API_FUNCTION
NetAlertRaise(
    IN LPCWSTR AlertType,
    IN LPVOID  Buffer,
    IN DWORD   BufferSize
    )
/*++

Routine Description:

    This routine raises an alert to notify the Alerter service by writing to
    the Alerter service mailslot.

Arguments:

    AlertType - Supplies the name of the alert event which could be one
        of the three the Alerter service supports: ADMIN, USER, or PRINTING.
        The ALERT_xxx_EVENT equates are used to provide these strings.

    Buffer - Supplies the data to be written to the alert mailslot.
        This must begin with a STD_ALERT structure.

    BufferSize - Supplies the size in number of bytes of Buffer.

Return Value:

    NET_API_STATUS - NO_ERROR or reason for failure.

--*/
{
    NET_API_STATUS ApiStatus;
    HANDLE FileHandle;
    DWORD MaxTotalSize;
    DWORD MaxVariableSize;
    DWORD NumberOfBytesWritten;
    DWORD RequiredFixedSize;

    //
    // Check for caller errors.
    // BUGBUG: We could make sure Buffer has AlertType in it correctly.
    //
    if (AlertType == NULL) {
        return (ERROR_INVALID_PARAMETER);
    } else if ( (*AlertType) == TCHAR_EOS ) {
        return (ERROR_INVALID_PARAMETER);
    } else if (Buffer == NULL) {
        return (ERROR_INVALID_PARAMETER);
    }

    ApiStatus = NetpAlertStructureInfo(
            (LPWSTR)AlertType,
            & MaxTotalSize,
            & RequiredFixedSize,
            & MaxVariableSize );
    if (ApiStatus != NO_ERROR) {
        return (ApiStatus);
    }
    if (BufferSize < ( sizeof(STD_ALERT) + RequiredFixedSize) ) {
        return (ERROR_INVALID_PARAMETER);
    } else if (BufferSize > MaxTotalSize) {
        return (ERROR_INVALID_PARAMETER);
    }

    //
    // Open the Alerter mailslot to write to it.
    //
    FileHandle = CreateFile(
            ALERTER_MAILSLOT,
            GENERIC_WRITE,
            FILE_SHARE_WRITE | FILE_SHARE_READ,
            (LPSECURITY_ATTRIBUTES) NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL );                      // no template file.
    if (FileHandle == (HANDLE) (-1)) {

        ApiStatus = (NET_API_STATUS) GetLastError();
        IF_DEBUG( ALERT ) {
            NetpKdPrint(( PREFIX_NETAPI
                "NetAlertRaise: Problem with opening mailslot "
                FORMAT_API_STATUS "\n", ApiStatus ));
        }
        return (ApiStatus);
    }

    IF_DEBUG( ALERT ) {
        NetpKdPrint(( PREFIX_NETAPI "NetAlertRaise: "
                "Successfully opened the mailslot.  Message (partial) is:\n"));
        NetpDbgHexDump( Buffer, NetpDbgReasonable(BufferSize) );
    }

    //
    // Write alert notification to mailslot to be read by Alerter service.
    //
    if (WriteFile(
            FileHandle,
            Buffer,
            BufferSize,
            &NumberOfBytesWritten,
            NULL                      // no overlapped structure.
            ) == FALSE) {

        ApiStatus = (NET_API_STATUS) GetLastError();
        NetpKdPrint(( PREFIX_NETAPI "NetAlertRaise: Error " FORMAT_API_STATUS
                " writing to mailslot.\n", ApiStatus ));
    } else {

        NetpAssert( NumberOfBytesWritten == BufferSize );
        IF_DEBUG(ALERT) {
            NetpKdPrint(( PREFIX_NETAPI "NetAlertRaise: "
                    "Successful in writing to mailslot; length "
                    FORMAT_DWORD ", bytes written " FORMAT_DWORD "\n",
                    BufferSize, NumberOfBytesWritten));
        }
    }

    (VOID) CloseHandle(FileHandle);
    return (NO_ERROR);

} // NetAlertRaise



NET_API_STATUS NET_API_FUNCTION
NetAlertRaiseEx(
    IN LPCWSTR AlertType,
    IN LPVOID  VariableInfo,
    IN DWORD   VariableInfoSize,
    IN LPCWSTR ServiceName
    )

/*++

Routine Description:

    This routine raises an alert to notify the Alerter service by writing to
    the Alerter service mailslot.

Arguments:

    AlertType - Supplies the name of the alert event which could be one
        of the three the Alerter service supports: ADMIN, USER, or PRINTING.
        The ALERT_xxx_EVENT equates are used to provide these strings.

    VariableInfo - Supplies the variable length portion of the alert
        notification.

    VariableInfoSize - Supplies the size in number of bytes of the variable
        portion of the notification.

    ServiceName - Supplies the name of the service which raised the alert.

Return Value:

    NET_API_STATUS - NO_ERROR or reason for failure.

--*/
{

#define TEMP_VARIABLE_SIZE (512-sizeof(STD_ALERT))       // BUGBUG: arbitrary?

    BYTE AlertMailslotBuffer[TEMP_VARIABLE_SIZE + sizeof(STD_ALERT)];
    LPSTD_ALERT Alert = (LPSTD_ALERT) AlertMailslotBuffer;
    NET_API_STATUS ApiStatus;
    DWORD DataSize = VariableInfoSize + sizeof(STD_ALERT);

    //
    // Check for caller errors.
    //
    if (AlertType == NULL) {
        return (ERROR_INVALID_PARAMETER);
    } else if ( (*AlertType) == TCHAR_EOS ) {
        return (ERROR_INVALID_PARAMETER);
    } else if (VariableInfo == NULL) {
        return (ERROR_INVALID_PARAMETER);

#if 0
    } else if (VariableInfoSize < sizeof(STD_ALERT)) {
        return (ERROR_INVALID_PARAMETER);
#endif // 0

    } else if (VariableInfoSize > TEMP_VARIABLE_SIZE) {
        return (NERR_NoRoom);  // BUGBUG: Better error code?
    } else if (ServiceName == NULL) {
        return (ERROR_INVALID_PARAMETER);
    } else if ( (*ServiceName) == TCHAR_EOS ) {
        return (ERROR_INVALID_PARAMETER);
    }

    //
    // Copy variable portion to end of our buffer.
    //
    (VOID) memcpy(ALERT_OTHER_INFO(Alert), VariableInfo, VariableInfoSize);

    //
    // Store current time in seconds since 1970.
    //
    Alert->alrt_timestamp = (DWORD) time_now();

    //
    // Put alert event name into AlertMailslotBuffer
    //
    (VOID) STRCPY(Alert->alrt_eventname, AlertType);

    //
    // Put service name into AlertMailslotBuffer
    //
    (VOID) STRCPY(Alert->alrt_servicename, ServiceName);

    //
    // Write alert notification to mailslot to be read by Alerter service
    //
    ApiStatus = NetAlertRaise(
            AlertType,
            Alert,                   // buffer
            DataSize );              // buffer size

    return (ApiStatus);

} // NetAlertRaiseEx
