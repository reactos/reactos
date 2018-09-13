/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    priv.c

Abstract:

    This module provides a command capability to enable and disable
    privileges.  This command is expected to be an internal cmd.exe
    command, but is expected to be passed parameters as if it were
    an external command.


    THIS IS A TEMPORARY COMMAND.  IF IT IS DESIRED TO MAKE THIS A
    PERMANENT COMMAND, THEN THIS FILE NEEDS TO BE GONE THROUGH WITH
    A FINE-TOOTH COMB TO ROBUSTLY HANDLE ALL ERROR SITUATIONS AND TO
    PROVIDE APPROPRIATE ERROR MESSAGES.

Author:

    Jim Kelly  1-Apr-1991.

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>

//#include <sys\types.h>
//#include <sys\stat.h>
//#include <malloc.h>
//#include <stdlib.h>
//#include <ctype.h>
#include <stdio.h>
#include <string.h>

//#include <tools.h>

//
// command qualifier flag values
//

BOOLEAN SwitchEnable  = FALSE;
BOOLEAN SwitchDisable = FALSE;
BOOLEAN SwitchReset   = FALSE;
BOOLEAN SwitchAll     = FALSE;

#ifndef SHIFT
#define SHIFT(c,v)      {c--; v++;}
#endif //SHIFT




//
// Function definitions...
//


VOID
Usage ( VOID );

BOOLEAN
OpenAppropriateToken(
    OUT PHANDLE Token
    );

VOID
EnableAllPrivileges( VOID );

VOID
ResetAllPrivileges( VOID );

VOID
DisableAllPrivileges( VOID );

int
PrivMain (
    IN int c,
    IN PCHAR v[]
    );




VOID
Usage (
    VOID
    )
/*++


Routine Description:

    This routine prints the "Usage:" message.

Arguments:

    None.

Return Value:

    None.

--*/
{

    printf( "\n");
    printf( "\n");

    printf( "Usage: priv [/EDRA] {PrivilegeName}\n");
    printf( "           /E - Enable Privilege(s)\n");
    printf( "           /D - Disable Privilege(s)\n");
    printf( "           /R - Reset to default setting(s)\n");
    printf( "           /A - Apply To All Privileges\n");
    printf( "\n");

    printf( "    The qualifiers /E and /D are mutually exclusive and can not\n");
    printf( "    be used in the same command.\n");
    printf( "    If /A is specified, then the PrivilegeName is ignored.\n");
    printf( "\n");
    printf( "\n");
    printf( "Examples:\n");
    printf( "\n");
    printf( "    priv /ae\n");
    printf( "    (enables all held privileges.\n");
    printf( "\n");
    printf( "    priv /ad\n");
    printf( "    disables all held privileges.\n");
    printf( "\n");
    printf( "    priv /ar\n");
    printf( "    (returns all privileges to their default setting.\n");
    printf( "\n");
    printf( "    priv /e SeSetTimePrivilege\n");
    printf( "    (enables the privileges called: SeSetTimePrivilege\n");
    printf( "\n");
    printf( "\n");

    return;
}


BOOLEAN
OpenAppropriateToken(
    OUT PHANDLE Token
    )
/*++


Routine Description:

    This routine opens the appropriate TOKEN object.  For an internal
    command, this is the current process's token.  If this command is
    ever made external, then it will be the parent process's token.

    If the token can't be openned, then a messages is printed indicating
    a problem has been encountered.

    The caller is expected to close this token when no longer needed.


Arguments:

    Token - Receives the handle value of the openned token.


Return Value:

    TRUE - Indicates the token was successfully openned.

    FALSE - Indicates the token was NOT successfully openned.

--*/

{
    NTSTATUS Status, IgnoreStatus;
    OBJECT_ATTRIBUTES ProcessAttributes;
    HANDLE Process;
    PTEB CurrentTeb;

    CurrentTeb = NtCurrentTeb();
    InitializeObjectAttributes(&ProcessAttributes, NULL, 0, NULL, NULL);
    Status = NtOpenProcess(
                 &Process,                   // TargetHandle
                 PROCESS_QUERY_INFORMATION,  // DesiredAccess
                 &ProcessAttributes,         // ObjectAttributes
                 &CurrentTeb->ClientId       // ClientId
                 );

    if (NT_SUCCESS(Status)) {

        Status = NtOpenProcessToken(
                     Process,
                     TOKEN_ADJUST_PRIVILEGES |
                     TOKEN_QUERY,
                     Token
                     );

         IgnoreStatus = NtClose( Process );

         if ( NT_SUCCESS(Status) ) {

             return TRUE;

         }

    }

    printf( "\n");
    printf( "\n");
    printf( "You are not allowed to change your own privilege settings.\n");
    printf( "Operation failed.\n");

    return FALSE;

}



VOID
EnableAllPrivileges(
    VOID
    )
/*++


Routine Description:

    This routine enables all privileges in the token.

Arguments:

    None.

Return Value:

    None.

--*/
{
    NTSTATUS Status;
    HANDLE Token;
    ULONG ReturnLength, Index;
    PTOKEN_PRIVILEGES  NewState;


    if ( !OpenAppropriateToken(&Token) ) {
        return;
    }

    //
    // Get the size needed to query current privilege settings...
    //

    Status = NtQueryInformationToken(
                 Token,                      // TokenHandle
                 TokenPrivileges,            // TokenInformationClass
                 NewState,                   // TokenInformation
                 0,                          // TokenInformationLength
                 &ReturnLength               // ReturnLength
                 );
    ASSERT( Status == STATUS_BUFFER_TOO_SMALL );

    NewState = RtlAllocateHeap( RtlProcessHeap(), 0, ReturnLength );
    ASSERT( NewState != NULL );


    Status = NtQueryInformationToken(
                 Token,                      // TokenHandle
                 TokenPrivileges,            // TokenInformationClass
                 NewState,                   // TokenInformation
                 ReturnLength,               // TokenInformationLength
                 &ReturnLength               // ReturnLength
                 );
    ASSERT( NT_SUCCESS(Status) || NT_INFORMATION(Status) );


    //
    // Set the state settings so that all privileges are enabled...
    //

    if (NewState->PrivilegeCount > 0) {
        Index = NewState->PrivilegeCount;

        while (Index < NewState->PrivilegeCount) {
            NewState->Privileges[Index].Attributes = SE_PRIVILEGE_ENABLED;
            Index += 1;
        }
    }


    //
    // Change the settings in the token...
    //

    Status = NtAdjustPrivilegesToken(
                 Token,                            // TokenHandle
                 FALSE,                            // DisableAllPrivileges
                 NewState,                         // NewState (OPTIONAL)
                 ReturnLength,                     // BufferLength
                 NULL,                             // PreviousState (OPTIONAL)
                 &ReturnLength                     // ReturnLength
                 );
    ASSERT( NT_SUCCESS(Status) || NT_INFORMATION(Status) );



    RtlFreeHeap( RtlProcessHeap(), 0, NewState );
    Status = NtClose( Token );

    return;

}



VOID
ResetAllPrivileges(
    VOID
    )
/*++


Routine Description:

    This routine resets all privileges in the token to their default state.

Arguments:

    None.

Return Value:

    None.

--*/
{
    NTSTATUS Status;
    HANDLE Token;
    ULONG ReturnLength, Index;
    PTOKEN_PRIVILEGES  NewState;


    if ( !OpenAppropriateToken(&Token) ) {
        printf( "\n");
        printf( "\n");
        printf( "You are not allowed to change your own privilege settings.\n");
        printf( "Operation failed.\n");
        return;
    }

    //
    // Get the size needed to query current privilege settings...
    //

    Status = NtQueryInformationToken(
                 Token,                      // TokenHandle
                 TokenPrivileges,            // TokenInformationClass
                 NewState,                   // TokenInformation
                 0,                          // TokenInformationLength
                 &ReturnLength               // ReturnLength
                 );
    ASSERT( STATUS_BUFFER_TOO_SMALL );

    NewState = RtlAllocateHeap( RtlProcessHeap(), 0, ReturnLength );
    ASSERT( NewState != NULL );


    Status = NtQueryInformationToken(
                 Token,                      // TokenHandle
                 TokenPrivileges,            // TokenInformationClass
                 NewState,                   // TokenInformation
                 ReturnLength,               // TokenInformationLength
                 &ReturnLength               // ReturnLength
                 );
    ASSERT( NT_SUCCESS(Status) || NT_INFORMATION(Status) );


    //
    // Set the state settings so that all privileges are reset to
    // their default settings...
    //

    if (NewState->PrivilegeCount > 0) {
        Index = NewState->PrivilegeCount;

        while (Index < NewState->PrivilegeCount) {
            if (NewState->Privileges[Index].Attributes ==
                SE_PRIVILEGE_ENABLED_BY_DEFAULT) {
                NewState->Privileges[Index].Attributes = SE_PRIVILEGE_ENABLED;
            }
            else {
                NewState->Privileges[Index].Attributes = 0;
            }

            Index += 1;
        }
    }


    //
    // Change the settings in the token...
    //

    Status = NtAdjustPrivilegesToken(
                 Token,                            // TokenHandle
                 FALSE,                            // DisableAllPrivileges
                 NewState,                         // NewState (OPTIONAL)
                 ReturnLength,                     // BufferLength
                 NULL,                             // PreviousState (OPTIONAL)
                 &ReturnLength                     // ReturnLength
                 );
    ASSERT( NT_SUCCESS(Status) || NT_INFORMATION(Status) );



    RtlFreeHeap( RtlProcessHeap(), 0, NewState );
    Status = NtClose( Token );

    return;

}




VOID
DisableAllPrivileges(
    VOID
    )
/*++


Routine Description:

    This routine disables all privileges in the token.

Arguments:

    None.

Return Value:

    None.

--*/
{
    ULONG IgnoredReturnLength;
    HANDLE Token;
    NTSTATUS Status;

    if ( !OpenAppropriateToken(&Token) ) {
        printf( "\n");
        printf( "\n");
        printf( "You are not allowed to change your own privilege settings.\n");
        printf( "Operation failed.\n");
        return;
    }

    //
    // Disable all the privileges.
    //


    Status = NtAdjustPrivilegesToken(
                 Token,                            // TokenHandle
                 TRUE,                             // DisableAllPrivileges
                 NULL,                             // NewState (OPTIONAL)
                 0,                                // BufferLength
                 NULL,                             // PreviousState (OPTIONAL)
                 &IgnoredReturnLength              // ReturnLength
                 );
    ASSERT( NT_SUCCESS(Status) || NT_INFORMATION(Status) );

    Status = NtClose( Token );
    return;

}


int
PrivMain (
    IN int c,
    IN PCHAR v[]
    )
/*++


Routine Description:

    This routine is the main entry routine for the "priv" command.

Arguments:

    TBS

Return Value:

    TBS

--*/
{
    PCHAR p;
    CHAR ch;
    ULONG DispositionDirectives;


    try {
        DispositionDirectives = 0;
        SHIFT (c,v);
        while ((c > 0) && ((ch = *v[0]))) {
            p = *v;
            if (ch == '/') {
                while (*++p != '\0') {
                    if (*p == 'E') {
                        SwitchEnable = TRUE;
                        DispositionDirectives += 1;
                    }
                    if (*p == 'D') {
                        SwitchDisable = TRUE;
                        DispositionDirectives += 1;
                    }
                    if (*p == 'R') {
                        SwitchReset = TRUE;
                        DispositionDirectives += 1;
                    }
                    else if (*p == 'A') {
                        SwitchAll  = TRUE;
                    }
                    else {
                        Usage();
                    }
                }
            SHIFT(c,v);
            }
        }

        //
        // Make sure we don't have conflicting parameters
        //
        // Rules:
        //
        //      If /A isn't specified, then a privilege name must be.
        //      Exactly one of /E, /D, and /R must be specified.
        //
        //


        if (!SwitchAll && (c == 0)) {
            printf( "\n");
            printf( "\n");
            printf( "You must provide privilege name or use the /A switch.\n");
            Usage();
            return ( 0 );
        }

        if (DispositionDirectives != 1) {
            printf( "\n");
            printf( "\n");
            printf( "You must provide one and only one of the following");
            printf( "switches: /E, /D, /R\n");
            Usage();
            return ( 0 );

        }


        //
        // Everything appears legitimate
        //

        if (SwitchAll) {

            //
            // /A switch specified
            //

            if (SwitchEnable) {
                EnableAllPrivileges();
            }
            else if (SwitchDisable) {
                DisableAllPrivileges();
            }
            else {
                ResetAllPrivileges();
            }
        }

        //
        // privilege name specified...
        //

        else {
            printf( "\n");
            printf( "I'm sorry, but due to the lack of time and interest,\n");
            printf( "individual privilege selection is not yet supported.\n");
            printf( "Please use the /A qualifier for the time being.\n");
            printf( "\n");
        }

    } finally {
        return ( 0 );
    }

    return( 0 );

}
