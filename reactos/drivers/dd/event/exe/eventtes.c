/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    EventTest.c

Abstract:

    Simple console test app demonstrating how a Win32 app can share 
    an event object with a kernel-mode driver. For more information 
    on using Event Objects at the application level see the Win32 SDK.

Author:

    Jeff Midkiff    (jeffmi)    23-Jul-96

Enviroment:

    User Mode

Revision History:

--*/


//
// INCLUDES
//
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <winioctl.h>
#include <conio.h>

#include "event.h"


//
// MAIN
//
void __cdecl 
main(
    int argc, 
    char ** argv
    )
{
    BOOL    bStatus;
    HANDLE  hDevice;
    ULONG   ulReturnedLength;

    SET_EVENT setEvent;
    FLOAT fDelay;
    

    if ( (argc < 2) || (argv[1] == NULL) ) {
        printf("event <delay>\n");
        printf("\twhere <delay> = time to delay the event signal in seconds.\n");
        exit(0);
    }
    sscanf( argv[1], "%f", &fDelay );

    //
    // open the device
    //
    hDevice = CreateFile(
                "\\\\.\\EVENT",                     // lpFileName
                GENERIC_READ | GENERIC_WRITE,       // dwDesiredAccess
                FILE_SHARE_READ | FILE_SHARE_WRITE, // dwShareMode
                NULL,                               // lpSecurityAttributes
                OPEN_EXISTING,                      // dwCreationDistribution
                0,                                  // dwFlagsAndAttributes
                NULL                                // hTemplateFile
                );
                
    if (hDevice == INVALID_HANDLE_VALUE) {
        printf("CreateFile error = %d\n", GetLastError() );
        exit(0);
    }


    //
    // set the event signal delay
    //
    setEvent.DueTime.QuadPart = -((LONGLONG)(fDelay * 10.0E6));// use relative time for this sample


    //
    // test the driver for bad event handles
    //
    setEvent.hEvent = NULL;
    bStatus = DeviceIoControl(
                        hDevice,                // Handle to device
                        IOCTL_SET_EVENT,        // IO Control code
                        &setEvent,              // Input Buffer to driver.
                        SIZEOF_SETEVENT,        // Length of input buffer in bytes.
                        NULL,                   // Output Buffer from driver.
                        0,                      // Length of output buffer in bytes.
                        &ulReturnedLength,      // Bytes placed in buffer.
                        NULL                    // synchronous call
                        );
    if ( !bStatus ) {
        printf("Bad handle TEST returned code %d.\n\n", GetLastError() );
    } else {
        printf("we should never get here\n");
        exit(0);
    }


    //
    // 
    //
    setEvent.hEvent = CreateEvent( 
                            NULL,   // lpEventAttributes
                            TRUE,   // bManualReset
                            FALSE,  // bInitialState
                            NULL    // lpName
                            );


    if ( !setEvent.hEvent ) {
        printf("CreateEvent error = %d\n", GetLastError() );
    } else {

        printf("Event HANDLE = 0x%x\n",  setEvent.hEvent );
        printf("Press any key to exit.\n");
        while( !_kbhit() ) {
            bStatus = DeviceIoControl(
                            hDevice,                // Handle to device
                            IOCTL_SET_EVENT,        // IO Control code
                            &setEvent,              // Input Buffer to driver.
                            SIZEOF_SETEVENT,        // Length of input buffer in bytes.
                            NULL,                   // Output Buffer from driver.
                            0,                      // Length of output buffer in bytes.
                            &ulReturnedLength,      // Bytes placed in buffer.
                            NULL                    // synchronous call
                            );

            if ( !bStatus ) {
                printf("Ioctl failed with code %d\n", GetLastError() );
                break;
            } else {
                printf("Waiting for Event...\n");

                WaitForSingleObject(setEvent.hEvent,
                                    INFINITE );

                printf("Event signalled.\n\n");

                ResetEvent( setEvent.hEvent);
                //printf("Event reset.\n");
            }
        }
    }

    //
    // close the driver
    //
    if ( !CloseHandle(hDevice) )     {
        printf("Failed to close device.\n");
    }

    exit(0);
}


// EOF
