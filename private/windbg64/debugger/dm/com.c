/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    api.c

Abstract:

    This module implements the all apis that simulate their
    WIN32 counterparts.

Author:

    Wesley Witt (wesw) 8-Mar-1992

Environment:

    NT 3.1

Revision History:

--*/
#include "precomp.h"
#pragma hdrstop

#ifdef  min
#undef  min
#undef  max
#endif

#define COM_PORT_NAME   "_NT_DEBUG_PORT"
#define COM_PORT_BAUD   "_NT_DEBUG_BAUD_RATE"
#define OUT_NORMAL      0
#define OUT_TERMINAL    1

ULONG   KdPollThreadMode = 0;

//
// Global Data
//
HANDLE DmKdComPort;

//
// This overlapped structure will be used for all serial read
// operations. We only need one structure since the code is
// designed so that no more than one serial read operation is
// outstanding at any one time.
//
OVERLAPPED ReadOverlapped;

//
// This overlapped structure will be used for all serial write
// operations. We only need one structure since the code is
// designed so that no more than one serial write operation is
// outstanding at any one time.
//
OVERLAPPED WriteOverlapped;

//
// This overlapped structure will be used for all event operations.
// We only need one structure since the code is designed so that no more
// than one serial event operation is outstanding at any one time.
//
OVERLAPPED EventOverlapped;


//
// Global to watch changes in event status. (used for carrier detection)
//
DWORD DmKdComEvent;


CRITICAL_SECTION csComPort;
CRITICAL_SECTION csPacket;


BOOL
DmKdInitComPort(
    BOOL KdModemControl
    )
{
    char   ComPortName[16];
    ULONG  Baud;
    DCB    LocalDcb;
    COMMTIMEOUTS To;
    DWORD  mask;


    Baud = (ULONG)KdOptions[KDO_BAUDRATE].value;
    sprintf( ComPortName, "\\\\.\\com%d", (ULONG)KdOptions[KDO_PORT].value );

    //
    // Open the device
    //
    DmKdComPort = CreateFile(
                        (PSZ)ComPortName,
                        GENERIC_READ | GENERIC_WRITE,
                        0,
                        NULL,
                        OPEN_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                        NULL
                        );

    if ( DmKdComPort == (HANDLE)-1 ) {
        return FALSE;
    }

    SetupComm(DmKdComPort,(DWORD)4096,(DWORD)4096);

    //
    // Create the events used by the overlapped structures for the
    // read and write.
    //

    ReadOverlapped.hEvent = CreateEvent(
                                NULL,
                                TRUE,
                                FALSE,NULL
                                );

    if (!ReadOverlapped.hEvent) {
        return FALSE;
    }

    WriteOverlapped.hEvent = CreateEvent(
                                 NULL,
                                 TRUE,
                                 FALSE,NULL
                                 );

    if (!WriteOverlapped.hEvent) {
        return FALSE;
    }

    ReadOverlapped.Offset = 0;
    ReadOverlapped.OffsetHigh = 0;

    WriteOverlapped.Offset = 0;
    WriteOverlapped.OffsetHigh = 0;

    //
    // Set up the Comm port....
    //

    if (!GetCommState(
             DmKdComPort,
             &LocalDcb
             )) {

        return FALSE;
    }

    LocalDcb.BaudRate = Baud;
    LocalDcb.ByteSize = 8;
    LocalDcb.Parity = NOPARITY;
    LocalDcb.StopBits = ONESTOPBIT;
    LocalDcb.fDtrControl = DTR_CONTROL_ENABLE;
    LocalDcb.fRtsControl = RTS_CONTROL_ENABLE;
    LocalDcb.fBinary = TRUE;
    LocalDcb.fOutxCtsFlow = FALSE;
    LocalDcb.fOutxDsrFlow = FALSE;
    LocalDcb.fOutX = FALSE;
    LocalDcb.fInX = FALSE;

    if (!SetCommState(
            DmKdComPort,
            &LocalDcb
            )) {

        return FALSE;
    }

    //
    // Set the normal read and write timeout time.
    // The symbols are 10 millisecond intervals.
    //

    To.ReadIntervalTimeout = 0;
    To.ReadTotalTimeoutMultiplier = 0;
    To.ReadTotalTimeoutConstant = 4 * 1000;
    To.WriteTotalTimeoutMultiplier = 0;
    To.WriteTotalTimeoutConstant = 4 * 1000;

    if (!SetCommTimeouts(
             DmKdComPort,
             &To
             )) {

        return FALSE;
    }

    DmKdComEvent = 0;
    if (KdModemControl) {

        //
        //  Debugger is being run over a modem.  Set event to watch
        //  carrier detect.
        //

        GetCommMask (DmKdComPort, &mask);
        mask = mask | EV_ERR; // | EV_RLSD;
        if (!SetCommMask (DmKdComPort, mask)) {
            return FALSE;
        }

        EventOverlapped.hEvent = CreateEvent(
                                    NULL,
                                    TRUE,
                                    FALSE,NULL
                                    );

        if (!EventOverlapped.hEvent) {
            return FALSE;
        }

        EventOverlapped.Offset = 0;
        EventOverlapped.OffsetHigh = 0;

        DmKdComEvent = 1;     // Fake an event, so modem status will be checked
    }

    InitializeCriticalSection(&csComPort);
    InitializeCriticalSection(&csPacket);

    return TRUE;
}

BOOL
DmKdWriteComPort(
    IN PUCHAR   Buffer,
    IN ULONG    SizeOfBuffer,
    IN PULONG   BytesWritten
    )
/*++

Routine Description:

    Writes the supplied bytes to the COM port.  Handles overlapped
    IO requirements and other common com port maintanance.

--*/
{
    BOOL rc;
    DWORD   TrashErr;
    COMSTAT TrashStat;


    EnterCriticalSection(&csComPort);

//  if (DmKdComEvent) {
//      DmKdCheckComStatus ( );
//  }

    rc = WriteFile(
             DmKdComPort,
             Buffer,
             SizeOfBuffer,
             BytesWritten,
             &WriteOverlapped
             );

    if (!rc) {

       if (GetLastError() == ERROR_IO_PENDING) {

           rc = GetOverlappedResult(
                    DmKdComPort,
                    &WriteOverlapped,
                    BytesWritten,
                    TRUE
                    );

        } else {

            //
            // Device could be locked up.  Clear it just in case.
            //
            ClearCommError( DmKdComPort, &TrashErr, &TrashStat );

        }
    }

    //
    // this is here for digiboards
    //
    FlushFileBuffers( DmKdComPort );

    LeaveCriticalSection(&csComPort);

    return rc;
}

BOOL
DmKdReadComPort(
    IN PUCHAR   Buffer,
    IN ULONG    SizeOfBuffer,
    IN PULONG   BytesRead
    )
/*++

Routine Description:

    Reads bytes from the COM port.  Handles overlapped
    IO requirements and other common com port maintanance.

--*/
{
    BOOL rc;
    DWORD   TrashErr;
    COMSTAT TrashStat;


    EnterCriticalSection(&csComPort);

//  if (DmKdComEvent) {
//      DmKdCheckComStatus ( );
//  }

    rc = ReadFile(
             DmKdComPort,
             Buffer,
             SizeOfBuffer,
             BytesRead,
             &ReadOverlapped
             );

    if (!rc) {

       if (GetLastError() == ERROR_IO_PENDING) {

           rc = GetOverlappedResult(
                    DmKdComPort,
                    &ReadOverlapped,
                    BytesRead,
                    TRUE
                    );

        } else {

            //
            // Device could be locked up.  Clear it just in case.
            //
            ClearCommError( DmKdComPort, &TrashErr, &TrashStat );

        }
    }

    LeaveCriticalSection(&csComPort);

    return rc;
}

VOID
DmKdCheckComStatus (
   )
/*++

Routine Description:

    Called when the com port status trigger signals a change.
    This function handles the change in status.

    Note: status is only monitored when being used over the modem.

--*/
{
    DWORD   status;
    DWORD   CommErr;
    COMSTAT CommStat;
#if 0
    BOOL rc;
    ULONG   br;
    UCHAR   buf[20];
#endif

    if (!DmKdComEvent) {
        //
        // Not triggered, just return
        //

        return ;
    }
    DmKdComEvent = 0;

    GetCommModemStatus (DmKdComPort, &status);
#if 0
    if (!(status & MS_RLSD_ON)) {
        DEBUG_PRINT ("KD: No carrier detect - in terminal mode\n");

        //
        // Send any keystrokes to the ComPort
        //

        KdPollThreadMode = OUT_TERMINAL;

        //
        // Loop and read any com input
        //

        while (!(status & 0x80)) {
            GetCommModemStatus (DmKdComPort, &status);
            rc = DmKdReadComPort(buf, sizeof buf, &br);
            if (rc != TRUE  ||  br == 0)
                continue;

            DMPrintShellMsg( "%s", buf );

        }

        KdPollThreadMode = OUT_NORMAL;
        DEBUG_PRINT ("KD: Carrier detect - returning to debugger\n");

        ClearCommError (
            DmKdComPort,
            &CommErr,
            &CommStat
            );

    } else {
#endif

        CommErr = 0;
        ClearCommError (
            DmKdComPort,
            &CommErr,
            &CommStat
            );

        if (CommErr) {
            DEBUG_PRINT( "Comm Error: " );
        }

        if (CommErr & CE_OVERRUN) {
            DEBUG_PRINT (" [overrun err] ");
        }

        if (CommErr & CE_RXOVER) {
            DEBUG_PRINT (" [overflow err] ");
        }

        if (CommErr & CE_RXPARITY) {
            DEBUG_PRINT (" [parify err] ");
        }

        if (CommErr & CE_BREAK) {
            DEBUG_PRINT (" [break err] ");
        }

        if (CommErr & CE_FRAME) {
            DEBUG_PRINT (" [frame err] ");
        }

        if (CommErr & CE_TXFULL) {
            DEBUG_PRINT (" [txfull err] ");
        }

        if (CommErr) {
            DEBUG_PRINT( "\n" );
        }
#if 0
    }
#endif


    //
    // Reset trigger
    //

    WaitCommEvent (DmKdComPort, &DmKdComEvent, &EventOverlapped);
}
