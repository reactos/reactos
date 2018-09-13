/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    Tshutwnd.c

Abstract:

    This module contains the function test for the System Shutdown APIs

Author:

    Dave Chalmers (davidc) 30-Apr-1992

Environment:

    Windows, Crt - User Mode

Notes:

    Since this is a test program it relies on assertions for error checking
    rather than a more robust mechanism.

--*/

#define MAX_STRING_LENGTH   80

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#ifdef UNICODE
#error This module was designed to be built as ansi only
#endif


VOID
main(
    INT     argc,
    PCHAR   argv[ ]
    )

{
    LPTSTR  MachineName = NULL;
    WCHAR   UnicodeMachineName[MAX_STRING_LENGTH];
    PWCHAR  pUnicodeMachineName = NULL;
    BOOL    Result;
    BOOL    Failed = FALSE;
    DWORD   Error;

    //
    // Initialize options based on the command line.
    //

    while( *++argv ) {

        MachineName = *argv;
    }

    //
    // Get the machine name in unicode
    //

    if (MachineName != NULL) {

        MultiByteToWideChar(0,
                            MachineName, -1,
                            UnicodeMachineName, sizeof(UnicodeMachineName),
                            MB_PRECOMPOSED);

        pUnicodeMachineName = UnicodeMachineName;

        printf("Machine Name(a) = <%s>\n", MachineName);
        printf("Machine Name(u) = <%ws>\n", UnicodeMachineName);

    }


    //
    // Start the test
    //

    printf("Running test again machine <%s>\n\n", MachineName);




    //
    // InitiateSystemShutdown (Ansi)
    //



    printf("Test InitiateSystemShutdown (Ansi)...");


    Result = InitiateSystemShutdownA(
                    MachineName,
                    NULL,           // No message
                    0,              // Timeout
                    FALSE,          // Force
                    FALSE           // Reboot
                    );

    if (Result == FALSE) {

        Error = GetLastError();

        if (Error != ERROR_CALL_NOT_IMPLEMENTED) {

            printf("Failed.\n");
            printf("Call failed as expected but last error is incorrect\n");
            printf("LastError() returned %d, expected %d\n", Error, ERROR_CALL_NOT_IMPLEMENTED);
            Failed = TRUE;
        }

    } else {
        printf("Failed.\n");
        printf("Call succeeded, expected it to fail.\n");
        Failed = TRUE;
    }


    Result = InitiateSystemShutdownA(
                    MachineName,
                    "A shutdown message",
                    0,              // Timeout
                    FALSE,          // Force
                    FALSE           // Reboot
                    );

    if (Result == FALSE) {

        Error = GetLastError();

        if (Error != ERROR_CALL_NOT_IMPLEMENTED) {

            printf("Failed.\n");
            printf("Call failed as expected but last error is incorrect\n");
            printf("LastError() returned %d, expected %d\n", Error, ERROR_CALL_NOT_IMPLEMENTED);
            Failed = TRUE;
        }

    } else {
        printf("Failed.\n");
        printf("Call succeeded, expected it to fail.\n");
        Failed = TRUE;
    }

    if (Failed) {
        return;
    }

    printf("Succeeded.\n");





    //
    // InitiateSystemShutdown (Unicode)
    //



    printf("Test InitiateSystemShutdown (Unicode)...");


    Result = InitiateSystemShutdownW(
                    pUnicodeMachineName,
                    NULL,           // No message
                    0,              // Timeout
                    FALSE,          // Force
                    FALSE           // Reboot
                    );

    if (Result == FALSE) {

        Error = GetLastError();

        if (Error != ERROR_CALL_NOT_IMPLEMENTED) {

            printf("Failed.\n");
            printf("Call failed as expected but last error is incorrect\n");
            printf("LastError() returned %d, expected %d\n", Error, ERROR_CALL_NOT_IMPLEMENTED);
            Failed = TRUE;
        }

    } else {
        printf("Failed.\n");
        printf("Call succeeded, expected it to fail.\n");
        Failed = TRUE;
    }


    Result = InitiateSystemShutdownW(
                    pUnicodeMachineName,
                    L"A shutdown message",
                    0,              // Timeout
                    FALSE,          // Force
                    FALSE           // Reboot
                    );

    if (Result == FALSE) {

        Error = GetLastError();

        if (Error != ERROR_CALL_NOT_IMPLEMENTED) {

            printf("Failed.\n");
            printf("Call failed as expected but last error is incorrect\n");
            printf("LastError() returned %d, expected %d\n", Error, ERROR_CALL_NOT_IMPLEMENTED);
            Failed = TRUE;
        }

    } else {
        printf("Failed.\n");
        printf("Call succeeded, expected it to fail.\n");
        Failed = TRUE;
    }

    if (Failed) {
        return;
    }

    printf("Succeeded.\n");





    //
    // AbortSystemShutdown (Ansi)
    //



    printf("Test AbortSystemShutdown (Ansi)...");


    Result = AbortSystemShutdownA(
                    MachineName
                    );

    if (Result == FALSE) {

        Error = GetLastError();

        if (Error != ERROR_CALL_NOT_IMPLEMENTED) {

            printf("Failed.\n");
            printf("Call failed as expected but last error is incorrect\n");
            printf("LastError() returned %d, expected %d\n", Error, ERROR_CALL_NOT_IMPLEMENTED);
            Failed = TRUE;
        }

    } else {
        printf("Failed.\n");
        printf("Call succeeded, expected it to fail.\n");
        Failed = TRUE;
    }

    if (Failed) {
        return;
    }

    printf("Succeeded.\n");


    //
    // AbortSystemShutdown (Unicode)
    //



    printf("Test AbortSystemShutdown (Unicode)...");


    Result = AbortSystemShutdownW(
                    pUnicodeMachineName
                    );

    if (Result == FALSE) {

        Error = GetLastError();

        if (Error != ERROR_CALL_NOT_IMPLEMENTED) {

            printf("Failed.\n");
            printf("Call failed as expected but last error is incorrect\n");
            printf("LastError() returned %d, expected %d\n", Error, ERROR_CALL_NOT_IMPLEMENTED);
            Failed = TRUE;
        }

    } else {
        printf("Failed.\n");
        printf("Call succeeded, expected it to fail.\n");
        Failed = TRUE;
    }

    if (Failed) {
        return;
    }

    printf("Succeeded.\n");

    return;


    UNREFERENCED_PARAMETER(argc);
}
