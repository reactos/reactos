/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    tenv.c

Abstract:

    Test program for the NT OS Runtime Library (RTL) Environment API Calls

Author:

    Steve Wood (stevewo) 30-Jan-1991

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stdio.h>

VOID
DumpEnvironment( PVOID env )
{
    PWCHAR s = env;

    while (*s) {
        printf( "%79.79ws\n", s );
        while (*s++) {
            }
        }
}

VOID
SetEnvironment(
    PVOID *env,
    PCHAR Name,
    PCHAR Value
    );

VOID
SetEnvironment(
    PVOID *env,
    PCHAR Name,
    PCHAR Value
    )
{
    NTSTATUS Status;
    STRING NameString, ValueString;
    UNICODE_STRING uNameString, uValueString;

    RtlInitString( &NameString, Name );
    Status = RtlAnsiStringToUnicodeString(&uNameString, &NameString, TRUE);
    if (!NT_SUCCESS( Status )) {
        printf( " - failed converting to Unicode, Status == %X\n", Status );
	DumpEnvironment(*env);
	printf( "\n" );
	return;
    }
    if (Value != NULL) {
        RtlInitString( &ValueString, Value );
	Status = RtlAnsiStringToUnicodeString(&uValueString, &ValueString, TRUE);
        printf( "TENV: set variable (%X) %Z=%Z\n", *env, &NameString, &ValueString );
        Status = RtlSetEnvironmentVariable( env, &uNameString, &uValueString );
        printf( "TENV: (%X)", *env);
	RtlFreeUnicodeString(&uNameString);
	RtlFreeUnicodeString(&uValueString);
        }
    else {
        printf( "TENV: delete variable (%X) %Z\n", *env, &NameString );
        Status = RtlSetEnvironmentVariable( env, &uNameString, NULL );
        printf( "TENV: (%X)", *env, &NameString, &ValueString );
	RtlFreeUnicodeString(&uNameString);
        }

    if (NT_SUCCESS( Status )) {
        printf( "\n" );
        }
    else {
        printf( " - failed, Status == %X\n", Status );
        }
    DumpEnvironment(*env);
    printf( "\n" );
}


int
_cdecl
main(
    int argc,
    char **argv,
    char **envp
    )
{
    int i;
    PVOID env;
    PVOID nenv;
    NTSTATUS Status;
    char bigbuf[4100];

    for (i=0; i<argc; i++) {
        printf( "argv[ %d ] = %s\n", i, argv[ i ] );
        }

    i = 0;
    while (envp[ i ]) {
        printf( "envp[ %d ] = %s\n", i, envp[ i ] );
        i++;
        }
    
    for (i=0 ; i<4099 ; i++)
	bigbuf[i] = (i%26) + (((i&1) == 0) ? 'a' : 'A');
    bigbuf[4099] = '\0';

    env = NtCurrentPeb()->ProcessParameters->Environment;
    Status = RtlCreateEnvironment(TRUE, &nenv);	// clone current
    if (!NT_SUCCESS( Status )) {
        printf( "Unable to create clone environment - %X\n", Status );
	return 1;
    }

    // First, check with process environment
    DumpEnvironment( &env);
    SetEnvironment( &env, "aaaa", "12345" );
    SetEnvironment( &env, "aaaa", "1234567890" );
    SetEnvironment( &env, "aaaa", "1" );
    SetEnvironment( &env, "aaaa", "" );
    SetEnvironment( &env, "aaaa", NULL );
    SetEnvironment( &env, "AAAA", "12345" );
    SetEnvironment( &env, "AAAA", "1234567890" );
    SetEnvironment( &env, "AAAA", "1" );
    SetEnvironment( &env, "AAAA", "" );
    SetEnvironment( &env, "AAAA", NULL );
    SetEnvironment( &env, "MMMM", "12345" );
    SetEnvironment( &env, "MMMM", "1234567890" );
    SetEnvironment( &env, "MMMM", "1" );
    SetEnvironment( &env, "MMMM", "" );
    SetEnvironment( &env, "MMMM", NULL );
    SetEnvironment( &env, "ZZZZ", "12345" );
    SetEnvironment( &env, "ZZZZ", "1234567890" );
    SetEnvironment( &env, "ZZZZ", "1" );
    SetEnvironment( &env, "ZZZZ", "" );
    SetEnvironment( &env, "ZZZZ", NULL );
    SetEnvironment( &env, "BIGBUF", bigbuf );
    SetEnvironment( &env, "BIGBUF", NULL );

    // Second, check with non-process environment
    DumpEnvironment(nenv);
    SetEnvironment( &nenv, "aaaa", "12345" );
    SetEnvironment( &nenv, "aaaa", "1234567890" );
    SetEnvironment( &nenv, "aaaa", "1" );
    SetEnvironment( &nenv, "aaaa", "" );
    SetEnvironment( &nenv, "aaaa", NULL );
    SetEnvironment( &nenv, "AAAA", "12345" );
    SetEnvironment( &nenv, "AAAA", "1234567890" );
    SetEnvironment( &nenv, "AAAA", "1" );
    SetEnvironment( &nenv, "AAAA", "" );
    SetEnvironment( &nenv, "AAAA", NULL );
    SetEnvironment( &nenv, "MMMM", "12345" );
    SetEnvironment( &nenv, "MMMM", "1234567890" );
    SetEnvironment( &nenv, "MMMM", "1" );
    SetEnvironment( &nenv, "MMMM", "" );
    SetEnvironment( &nenv, "MMMM", NULL );
    SetEnvironment( &nenv, "ZZZZ", "12345" );
    SetEnvironment( &nenv, "ZZZZ", "1234567890" );
    SetEnvironment( &nenv, "ZZZZ", "1" );
    SetEnvironment( &nenv, "ZZZZ", "" );
    SetEnvironment( &nenv, "ZZZZ", NULL );
    SetEnvironment( &nenv, "BIGBUF", bigbuf );
    SetEnvironment( &nenv, "BIGBUF", NULL );
    return( 0 );
}
