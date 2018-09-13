
//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//                Global Definitions                                        //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

#define DevPrint
//#define DevPrint DbgPrint

#define Error(N,S) { DbgPrint(#N); DbgPrint(" Error %08lx\n", S); }

#define Delay(SECONDS) {                                               \
    LARGE_INTEGER Time;                                                \
    Time.QuadPart = -10 * 1000 * 1000, ((LONGLONG)SECONDS);            \
    NtDelayExecution(TRUE,(PLARGE_INTEGER)&Time);                               \
}


//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//                Global Variables                                          //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    STRING EventName;
    UNICODE_STRING UnicodeEventName;
    HANDLE EventHandle;
    STRING PortName;
    UNICODE_STRING UnicodePortName;
    STRING RelativePortName;
    UNICODE_STRING UnicodeRelativePortName;
    HANDLE EarPort;
    HANDLE TalkPort;
    SECURITY_QUALITY_OF_SERVICE SecurityQos;
    ULONG RequestCount;
    HANDLE ClientToken;
    TOKEN_STATISTICS ClientTokenStatistics;
    ULONG IgnoreLength;

    HANDLE SepServerThread;




//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//                Test Routine Definitions                                  //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////
BOOLEAN
SepClientTestStatic(VOID);

BOOLEAN
SepClientTestDynamic(VOID);

BOOLEAN
SepClientTestEffectiveOnly(
    BOOLEAN StaticTest
    );

BOOLEAN
SepClientTestNotEffectiveOnly(
    BOOLEAN StaticTest
    );

BOOLEAN
SepClientTestAnonymous(
    BOOLEAN StaticTest,
    BOOLEAN EffectiveOnly
    );

BOOLEAN
SepClientTestIdentification(
    BOOLEAN StaticTest,
    BOOLEAN EffectiveOnly
    );

BOOLEAN
SepClientTestImpersonation(
    BOOLEAN StaticTest,
    BOOLEAN EffectiveOnly
    );

VOID
SepClientConnect(
    SECURITY_IMPERSONATION_LEVEL ImpersonationLevel,
    SECURITY_CONTEXT_TRACKING_MODE TrackingMode,
    BOOLEAN EffectiveOnly
    );

VOID
SepClientMakeRemoteCall( VOID );

VOID
SepClientDropConnection( VOID );

BOOLEAN
SepClientTest(VOID);

NTSTATUS
SepClientInitialize(
  );






BOOLEAN
SepServerTestStatic(VOID);

BOOLEAN
SepServerTestDynamic(VOID);

BOOLEAN
SepServerTestEffectiveOnly(
    BOOLEAN StaticTest
    );

BOOLEAN
SepServerTestNotEffectiveOnly(
    BOOLEAN StaticTest
    );

BOOLEAN
SepServerTestAnonymous(
    BOOLEAN StaticTest,
    BOOLEAN EffectiveOnly
    );

BOOLEAN
SepServerTestIdentification(
    BOOLEAN StaticTest,
    BOOLEAN EffectiveOnly
    );

BOOLEAN
SepServerTestImpersonation(
    BOOLEAN StaticTest,
    BOOLEAN EffectiveOnly
    );

VOID
SepServerWaitForNextConnect( VOID );

VOID
SepServerGetNextMessage( VOID );

VOID
SepServerCompleteMessage( VOID );

VOID
SepServerDropConnection( VOID );



BOOLEAN
SepServerTest(VOID);

NTSTATUS
SepServerInitialize(
  );

VOID
SepServerSpawnClientProcess(VOID);




VOID
SepWritePipe( PSZ String );

VOID
SepReadPipe(VOID);

VOID
SepTransceivePipe( PSZ String );




HANDLE
SepServerCreatePipe(VOID);

VOID
SepServerListenPipe(VOID);

VOID
SepServerImpersonatePipe(VOID);

VOID
SepServerDisconnectPipe(VOID);




HANDLE
SepClientOpenPipe( VOID );




BOOLEAN
CtLnpQos (VOID);


//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//                Client-Side Test Routines                                 //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////


VOID
SepClientConnect(
    SECURITY_IMPERSONATION_LEVEL ImpersonationLevel,
    SECURITY_CONTEXT_TRACKING_MODE TrackingMode,
    BOOLEAN EffectiveOnly
    )

{

    SecurityQos.ImpersonationLevel = ImpersonationLevel;
    SecurityQos.ContextTrackingMode = TrackingMode;
    SecurityQos.EffectiveOnly = EffectiveOnly;

    DevPrint("\nClient: ");
    TalkPort = SepClientOpenPipe();

    return;
}


VOID
SepClientMakeRemoteCall( VOID )

{

    DevPrint("\nClient: ");
    SepTransceivePipe( "Make Client Call\n" );

    RequestCount += 1;

    return;
}


VOID
SepClientDropConnection( VOID )

{

    Status = NtClose( TalkPort );  SEASSERT_SUCCESS(Status);

    return;

}


BOOLEAN
SepClientTestStatic(VOID)

{

    BOOLEAN CompletionStatus;

    //
    //  Static Context Tracking ... Suite
    //

    CompletionStatus = SepClientTestEffectiveOnly( TRUE );


    if (CompletionStatus == TRUE) {

        CompletionStatus = SepClientTestNotEffectiveOnly( TRUE );
    }

    return CompletionStatus;

}


BOOLEAN
SepClientTestDynamic(VOID)

{
    BOOLEAN CompletionStatus;

    //
    // Dynamic Context Tracking ... Suite
    //

    CompletionStatus = SepClientTestEffectiveOnly( FALSE );


    if (CompletionStatus == TRUE) {

        CompletionStatus = SepClientTestNotEffectiveOnly( FALSE );
    }

    return CompletionStatus;

}


BOOLEAN
SepClientTestEffectiveOnly(
    BOOLEAN StaticTest
    )


{

    BOOLEAN CompletionStatus;

    //
    // Effective Only ... Test
    //

    CompletionStatus = SepClientTestAnonymous( StaticTest, TRUE );
    if (CompletionStatus == TRUE) {
        CompletionStatus = SepClientTestIdentification( StaticTest, TRUE );
    }
    if (CompletionStatus == TRUE) {
        CompletionStatus = SepClientTestImpersonation( StaticTest, TRUE );
    }

    return CompletionStatus;

}


BOOLEAN
SepClientTestNotEffectiveOnly(
    BOOLEAN StaticTest
    )

{
    BOOLEAN CompletionStatus;

    //
    // Not Effective Only ... Test
    //

    CompletionStatus = SepClientTestAnonymous( StaticTest, FALSE );
    if (CompletionStatus == TRUE) {
        CompletionStatus = SepClientTestIdentification( StaticTest, FALSE );
    }
    if (CompletionStatus == TRUE) {
        CompletionStatus = SepClientTestImpersonation( StaticTest, FALSE );
    }

    return CompletionStatus;

}


BOOLEAN
SepClientTestAnonymous(
    BOOLEAN StaticTest,
    BOOLEAN EffectiveOnly
    )

{

    //////////////////////////////////////////////////////////////////////////
    //                                                                      //
    //        Anonymous Use Test                                            //
    //                                                                      //
    //////////////////////////////////////////////////////////////////////////

    SECURITY_CONTEXT_TRACKING_MODE TrackingMode;

    if (StaticTest) {
        TrackingMode = SECURITY_STATIC_TRACKING;
    } else {
        TrackingMode = SECURITY_DYNAMIC_TRACKING;
    }

    if (!StaticTest) {
        //
        // No action for dynamic test
        //
        return TRUE;
    }

    //
    // Anonymous Use ... Test
    //


    SepClientConnect(
        SecurityAnonymous,
        TrackingMode,
        EffectiveOnly
        );

    SepClientMakeRemoteCall();

    SepClientDropConnection();


    return TRUE;
}


BOOLEAN
SepClientTestIdentification(
    BOOLEAN StaticTest,
    BOOLEAN EffectiveOnly
    )

{

    //////////////////////////////////////////////////////////////////////////
    //                                                                      //
    //        Identification Use Test                                       //
    //                                                                      //
    //////////////////////////////////////////////////////////////////////////

    SECURITY_CONTEXT_TRACKING_MODE TrackingMode;

    if (StaticTest) {
        TrackingMode = SECURITY_STATIC_TRACKING;
    } else {
        TrackingMode = SECURITY_DYNAMIC_TRACKING;
    }

    //
    // Identification Use ... Test
    //


    SepClientConnect(
        SecurityIdentification,
        TrackingMode,
        EffectiveOnly
        );

    SepClientMakeRemoteCall();

    SepClientDropConnection();


    return TRUE;

}


BOOLEAN
SepClientTestImpersonation(
    BOOLEAN StaticTest,
    BOOLEAN EffectiveOnly
    )

{

    //////////////////////////////////////////////////////////////////////////
    //                                                                      //
    //        Impersonation Use Test                                        //
    //                                                                      //
    //////////////////////////////////////////////////////////////////////////

    SECURITY_CONTEXT_TRACKING_MODE TrackingMode;

    if (StaticTest) {
        TrackingMode = SECURITY_STATIC_TRACKING;
    } else {
        TrackingMode = SECURITY_DYNAMIC_TRACKING;
    }


    //
    // Impersonation Use ... Test
    //


    SepClientConnect(
        SecurityImpersonation,
        TrackingMode,
        EffectiveOnly
        );

    SepClientMakeRemoteCall();

    SepClientDropConnection();



    return TRUE;

}




BOOLEAN
SepClientTest(VOID)
//
// Tests:
//
//      Static Context Tracking Tests
//          Effective Only
//              Anonymous
//              Identification
//              Impersonation
//          Not Effective Only
//              Anonymous
//              Identification
//              Impersonation
//
//      Dynamic Context Tracking Tests
//          Effective Only
//              Identification
//              Impersonation
//          Not Effective Only
//              Identification
//              Impersonation
//
{

    BOOLEAN CompletionStatus;




    //
    // Run the static test suite...
    //

    CompletionStatus = SepClientTestStatic();

    //
    // Run the dynamic test suite...
    //

    if (CompletionStatus == TRUE) {
        CompletionStatus = SepClientTestDynamic();
    }

    DbgPrint("Se: Client Test Complete.\n");


    return CompletionStatus;
}


NTSTATUS
SepClientInitialize(
  )

{



    DbgPrint("Se: Client Initializing ...\n");


    RequestCount = 0;


    //
    // Signal the named event to start the test
    //

    DbgPrint("Se: Client Starting Test ...\n");
    Status = NtSetEvent( EventHandle, NULL ); SEASSERT_SUCCESS(Status);

    Status = NtClose( EventHandle ); SEASSERT_SUCCESS(Status);


    return STATUS_SUCCESS;
}


//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//                Server-Side Test Routines                                 //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////


VOID
SepServerWaitForNextConnect( VOID )
{

    DevPrint("\nServer: ");
    SepServerListenPipe();

    Status = NtDuplicateObject(
                 NtCurrentProcess(),     // SourceProcessHandle
                 EarPort,                // SourceHandle
                 NtCurrentProcess(),     // TargetProcessHandle
                 &TalkPort,              // TargetHandle
                 0,                      // DesiredAccess (over-ridden by option)
                 0,                      // HandleAttributes
                 DUPLICATE_SAME_ACCESS   // Options
                 );
    ASSERT(NT_SUCCESS(Status));


    return;

}

VOID
SepServerGetNextMessage( VOID )

{



    DevPrint("\nServer: ");
    SepReadPipe();

    RequestCount += 1;

    return;
}

VOID
SepServerCompleteMessage( VOID )

{

    DevPrint("\nServer: ");
    SepWritePipe("Return From Server\n");
    return;
}

VOID
SepServerImpersonateClient( VOID )

{

    DevPrint("\nServer: ");
    SepServerImpersonatePipe( );

}


VOID
SepServerRevertToSelf( VOID )

{
    NTSTATUS TmpStatus;
    HANDLE NullHandle;

    NullHandle = NULL;
    TmpStatus = NtSetInformationThread(
                    SepServerThread,
                    ThreadImpersonationToken,
                    (PVOID)&NullHandle,
                    (ULONG)sizeof(HANDLE)
                    );   SEASSERT_SUCCESS(TmpStatus);

}


VOID
SepServerDropConnection( VOID )

{
    DevPrint("\nServer: ");
    SepServerDisconnectPipe();

    return;
}

BOOLEAN
SepServerTestStatic(VOID)

{
    BOOLEAN CompletionStatus;

    DbgPrint("Se:    Static Context Tracking ...                           Suite\n");

    CompletionStatus = SepServerTestEffectiveOnly( TRUE );


    if (CompletionStatus == TRUE) {

        CompletionStatus = SepServerTestNotEffectiveOnly( TRUE );
    }

    return CompletionStatus;

}


BOOLEAN
SepServerTestDynamic(VOID)

{
    BOOLEAN CompletionStatus;

    DbgPrint("Se:    Dynamic Context Tracking ...                          Suite\n");

    CompletionStatus = SepServerTestEffectiveOnly( FALSE );


    if (CompletionStatus == TRUE) {

        CompletionStatus = SepServerTestNotEffectiveOnly( FALSE );
    }

    return CompletionStatus;

}


BOOLEAN
SepServerTestEffectiveOnly(
    BOOLEAN StaticTest
    )

{

    BOOLEAN CompletionStatus;

    DbgPrint("Se:      Effective Only ...                                    Test\n");

    CompletionStatus = SepServerTestAnonymous( StaticTest, TRUE );
    if (CompletionStatus == TRUE) {
        CompletionStatus = SepServerTestIdentification( StaticTest, TRUE );
    }
    if (CompletionStatus == TRUE) {
        CompletionStatus = SepServerTestImpersonation( StaticTest, TRUE );
    }

    return CompletionStatus;

}


BOOLEAN
SepServerTestNotEffectiveOnly(
    BOOLEAN StaticTest
    )

{

    BOOLEAN CompletionStatus;

    DbgPrint("Se:      Not Effective Only ...                                Test\n");

    CompletionStatus = SepServerTestAnonymous( StaticTest, FALSE );
    if (CompletionStatus == TRUE) {
        CompletionStatus = SepServerTestIdentification( StaticTest, FALSE );
    }
    if (CompletionStatus == TRUE) {
        CompletionStatus = SepServerTestImpersonation( StaticTest, FALSE );
    }

    return CompletionStatus;

}


BOOLEAN
SepServerTestAnonymous(
    BOOLEAN StaticTest,
    BOOLEAN EffectiveOnly
    )

{
    BOOLEAN CompletionStatus = TRUE;

    //////////////////////////////////////////////////////////////////////////
    //                                                                      //
    //        Anonymous Use Test                                            //
    //                                                                      //
    //////////////////////////////////////////////////////////////////////////


    if (!StaticTest) {
        //
        // No action for dynamic test
        //

        return TRUE;
    }

    DbgPrint("Se:        Anonymous Use ...                                     ");

    SepServerWaitForNextConnect();

    SepServerGetNextMessage();


    SepServerImpersonateClient();
    Status = NtOpenThreadToken(
                 SepServerThread,
                 TOKEN_ALL_ACCESS,
                 TRUE,
                 &ClientToken
                 );
    SepServerRevertToSelf();
    if (Status == STATUS_CANT_OPEN_ANONYMOUS) {

        DbgPrint(" Succeeded\n");

    } else {
        DbgPrint("* ! FAILED (srvr) ! *\n");
        DbgPrint("Status is: 0x%lx \n", Status );
        CompletionStatus = FALSE;
    }


    SepServerCompleteMessage();

    SepServerDropConnection();

    //
    // Appease the compiler Gods..
    //

    if (EffectiveOnly) {;}


    return CompletionStatus;

}


BOOLEAN
SepServerTestIdentification(
    BOOLEAN StaticTest,
    BOOLEAN EffectiveOnly
    )

{

    BOOLEAN CompletionStatus = TRUE;
    //////////////////////////////////////////////////////////////////////////
    //                                                                      //
    //        Identification Use Test                                       //
    //                                                                      //
    //////////////////////////////////////////////////////////////////////////

    DbgPrint("Se:        Identification Use ...                                ");

    SepServerWaitForNextConnect();

    SepServerGetNextMessage();

    SepServerImpersonateClient();
    Status = NtOpenThreadToken(
                 SepServerThread,
                 TOKEN_ALL_ACCESS,
                 TRUE,
                 &ClientToken
                 );  SEASSERT_SUCCESS(Status);
    SepServerRevertToSelf();
    Status = NtQueryInformationToken(
                 ClientToken,
                 TokenStatistics,
                 &ClientTokenStatistics,
                 (ULONG)sizeof(TOKEN_STATISTICS),
                 &IgnoreLength
                 );  SEASSERT_SUCCESS(Status);

    if ( (ClientTokenStatistics.TokenType == TokenImpersonation) &&
         (ClientTokenStatistics.ImpersonationLevel == SecurityIdentification)
       ) {
        DbgPrint(" Succeeded\n");

    } else {
        DbgPrint("* ! FAILED (srvr) ! *\n");
        CompletionStatus = FALSE;
    }


    SepServerCompleteMessage();

    SepServerDropConnection();

    //
    // Appease the compiler Gods..
    //
    if (StaticTest) {;}
    if (EffectiveOnly) {;}

    return CompletionStatus;
}


BOOLEAN
SepServerTestImpersonation(
    BOOLEAN StaticTest,
    BOOLEAN EffectiveOnly
    )

{
    BOOLEAN CompletionStatus = TRUE;

    //////////////////////////////////////////////////////////////////////////
    //                                                                      //
    //        Impersonation Use Test                                        //
    //                                                                      //
    //////////////////////////////////////////////////////////////////////////

    DbgPrint("Se:        Impersonation Use ...                                 ");


    SepServerWaitForNextConnect();

    SepServerGetNextMessage();



    SepServerImpersonateClient();
    Status = NtOpenThreadToken(
                 SepServerThread,
                 TOKEN_ALL_ACCESS,
                 TRUE,
                 &ClientToken
                 );  SEASSERT_SUCCESS(Status);
    SepServerRevertToSelf();
    Status = NtQueryInformationToken(
                 ClientToken,
                 TokenStatistics,
                 &ClientTokenStatistics,
                 (ULONG)sizeof(TOKEN_STATISTICS),
                 &IgnoreLength
                 );  SEASSERT_SUCCESS(Status);

    if ( (ClientTokenStatistics.TokenType == TokenImpersonation) &&
         (ClientTokenStatistics.ImpersonationLevel == SecurityImpersonation)
       ) {
        DbgPrint(" Succeeded\n");

    } else {
        DbgPrint("* ! FAILED (srvr) ! *\n");
        CompletionStatus = FALSE;
    }




    SepServerCompleteMessage();

    SepServerDropConnection();

    //
    // Appease the compiler gods
    //
    if (StaticTest) {;}
    if (EffectiveOnly) {;}

    return CompletionStatus;
}


BOOLEAN
SepServerTest(VOID)
//
// Tests:
//
//      Static Context Tracking Tests
//          Effective Only
//              Anonymous
//              Identification
//              Impersonation
//          Not Effective Only
//              Anonymous
//              Identification
//              Impersonation
//
//      Dynamic Context Tracking Tests
//          Effective Only
//              Identification
//              Impersonation
//          Not Effective Only
//              Identification
//              Impersonation
//
{

    BOOLEAN CompletionStatus;


    DbgPrint("Se: Server Starting Test ...\n");

    //
    // Run the static test suite...
    //

    CompletionStatus = SepServerTestStatic();

    //
    // Run the dynamic test suite...
    //

    if (CompletionStatus == TRUE) {
        CompletionStatus = SepServerTestDynamic();
    }

    DbgPrint("Se: Server Test Complete.\n");

    //
    // Print test results
    //

    DbgPrint("\n");
    DbgPrint("\n");
    DbgPrint("**********************\n");
    DbgPrint("**                  **\n");

    if (CompletionStatus == TRUE) {
        DbgPrint("**  Test Succeeded  **\n");
    } else {
        DbgPrint("**  Test Failed !!  **\n");
    }

    DbgPrint("**                  **\n");
    DbgPrint("**********************\n");

    return CompletionStatus;

}

NTSTATUS
SepServerInitialize(
  )

{

    OBJECT_ATTRIBUTES ThreadAttributes;
    PTEB CurrentTeb;


    DbgPrint("Se: Server Initializing ...\n");

    //
    // Initialize global variables
    //

    RequestCount = 0;

    //
    // Get a handle to our thread to so that we can access our thread
    // even when impersonating an anonymous client (which we can't do
    // using NtCurrentThread()).
    //

    CurrentTeb = NtCurrentTeb();
    InitializeObjectAttributes(&ThreadAttributes, NULL, 0, NULL, NULL);
    Status = NtOpenThread(
                 &SepServerThread,           // TargetHandle
                 THREAD_ALL_ACCESS,          // DesiredAccess
                 &ThreadAttributes,          // ObjectAttributes
                 &CurrentTeb->ClientId       // ClientId
                 );
    ASSERT( NT_SUCCESS(Status) );


    //
    // Create the server's port
    //

    EarPort = SepServerCreatePipe();



    //
    // Spawn a copy of ourselves...
    //

    DbgPrint("Se: Server Spawning client process ...\n");
    SepServerSpawnClientProcess();


    DbgPrint("Se: Server waiting for start of test signal ...\n");

    Status = NtWaitForSingleObject(
                 EventHandle,
                 TRUE,
                 NULL
                 ); SEASSERT_SUCCESS(Status);

    Status = NtClose( EventHandle );  SEASSERT_SUCCESS(Status);


    return STATUS_SUCCESS;
}

VOID
SepServerSpawnClientProcess(VOID)

{

    RTL_USER_PROCESS_INFORMATION ProcessInformation;
    STRING ProgramName;
    UNICODE_STRING UnicodeProgramName;
    STRING ImagePathName;
    UNICODE_STRING UnicodeImagePathName;
    PRTL_USER_PROCESS_PARAMETERS ProcessParameters;

    RtlInitString( &ProgramName, "\\SystemRoot\\Bin\\utlnpqos.exe" );
    Status = RtlAnsiStringToUnicodeString(
                 &UnicodeProgramName,
                 &ProgramName,
                 TRUE );  SEASSERT_SUCCESS( NT_SUCCESS(Status) );
    RtlInitString( &ImagePathName, "utlnpqos.exe");
    Status = RtlAnsiStringToUnicodeString(
                 &UnicodeImagePathName,
                 &ImagePathName,
                 TRUE );  SEASSERT_SUCCESS( NT_SUCCESS(Status) );


    Status = RtlCreateProcessParameters(
                 &ProcessParameters,
                 &ImagePathName,        // FIX, FIX &UnicodeImagePathName, (when converted to unicode)
                 NULL,
                 NULL,
                 NULL,
                 NULL,
                 NULL,
                 NULL,
                 NULL,
                 NULL
                 );

    SEASSERT_SUCCESS(Status);
    RtlFreeUnicodeString( &UnicodeImagePathName );


    Status = RtlCreateUserProcess(
                 &ProgramName,                   // FIX, FIX &UnicodeProgramName (when converted to unicode)
                 ProcessParameters,              // ProcessParameters
                 NULL,                           // ProcessSecurityDescriptor
                 NULL,                           // ThreadSecurityDescriptor
                 NtCurrentProcess(),             // ParentProcess
                 FALSE,                          // InheritHandles
                 NULL,                           // DebugPort
                 NULL,                           // ExceptionPort
                 &ProcessInformation             // ProcessInformation
                 ); SEASSERT_SUCCESS(Status);
    RtlFreeUnicodeString( &UnicodeProgramName );

    Status = NtResumeThread(
                  ProcessInformation.Thread,
                  NULL
                  ); SEASSERT_SUCCESS(Status);

    RtlDestroyProcessParameters( ProcessParameters );

}




//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//                Main Program Entry Routine                                //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

BOOLEAN
CtLnpQos (VOID)
{

    BOOLEAN Result = TRUE;

    RtlInitString( &PortName, "\\Device\\NamedPipe\\TestLnpQosServerPort" );
    Status = RtlAnsiStringToUnicodeString(
                 &UnicodePortName,
                 &PortName,
                 TRUE );  SEASSERT_SUCCESS( NT_SUCCESS(Status) );

    RtlInitString( &RelativePortName, "TestLnpQosServerPort" );
    Status = RtlAnsiStringToUnicodeString(
                 &UnicodeRelativePortName,
                 &RelativePortName,
                 TRUE );  SEASSERT_SUCCESS( NT_SUCCESS(Status) );


    //
    // Determine whether we are the client or server side of the test.
    // This is done by creating or opening a named event object.  If the
    // event does not yet exist, then we are the client, and must create
    // the server process.  Otherwise, we are the server and the client
    // is waiting for us to signal the event.
    //

    RtlInitString( &EventName, "\\TestLnpQosEvent" );
    Status = RtlAnsiStringToUnicodeString(
                 &UnicodeEventName,
                 &EventName,
                 TRUE );  SEASSERT_SUCCESS( NT_SUCCESS(Status) );

    InitializeObjectAttributes(
        &ObjectAttributes,
        &UnicodeEventName,
        OBJ_OPENIF,
        NULL,
        NULL
        );
    Status = NtCreateEvent(
                 &EventHandle,
                 EVENT_ALL_ACCESS,
                 &ObjectAttributes,
                 SynchronizationEvent,
                 FALSE
                 );
    RtlFreeUnicodeString( &UnicodeEventName );

    if (Status == STATUS_OBJECT_NAME_EXISTS) {

        //
        // Server is already running, therefore, this process gets to be
        // the client.
        //

        Status = SepClientInitialize(); SEASSERT_SUCCESS(Status);
        Result = SepClientTest();

    } else {

        SEASSERT_SUCCESS(Status);

        //
        // Event wasn't yet there, so we must be the server.
        //

    DbgPrint("Se: Starting Local Named Pipe Impersonation Test.\n");

        Status = SepServerInitialize(); SEASSERT_SUCCESS(Status);
        Result = SepServerTest();

    DbgPrint("Se: End Test.\n");

        }



    Status = NtTerminateThread( NtCurrentThread(), STATUS_SUCCESS);
    SEASSERT_SUCCESS(Status);

    return Result;

}


//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//   Named Pipe Common Operations                                           //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////


VOID
SepReadPipe(
    )
{
    IO_STATUS_BLOCK Iosb;
    UCHAR Buffer[512];

    DevPrint("ReadPipe...\n", 0);

    if (!NT_SUCCESS(Status = NtReadFile( TalkPort,
                                      (HANDLE)NULL,
                                      (PIO_APC_ROUTINE)NULL,
                                      (PVOID)NULL,
                                      &Iosb,
                                      Buffer,
                                      512,
                                      (PLARGE_INTEGER)NULL,
                                      (PULONG) NULL ))) {
        Error( NtReadFile, Status );
    }

    if (!NT_SUCCESS(Status = NtWaitForSingleObject( TalkPort, TRUE, NULL ))) {

        Error( NtWaitForSingleObject, Status );
    }

    if (!NT_SUCCESS(Iosb.Status)) {

        Error( NtReadFileFinalStatus, Iosb.Status );
    }

    return;
}


VOID
SepWritePipe(
    PSZ String
    )
{
    NTSTATUS Status;
    IO_STATUS_BLOCK Iosb;


    DevPrint("WritePipe...\n", 0);

    if (!NT_SUCCESS(Status = NtWriteFile( TalkPort,
                                       (HANDLE)NULL,
                                       (PIO_APC_ROUTINE)NULL,
                                       (PVOID)NULL,
                                       &Iosb,
                                       String,
                                       strlen( String ),
                                       (PLARGE_INTEGER)NULL,
                                       (PULONG)NULL ))) {
        Error( NtWriteFile, Status );
    }

    if (!NT_SUCCESS(Status = NtWaitForSingleObject( TalkPort, TRUE, NULL ))) {

        Error( NtWaitForSingleObject, Status );
    }

    if (!NT_SUCCESS(Iosb.Status)) {

        Error( NtWriteFileFinalStatus, Iosb.Status );
    }

    return;
}


VOID
SepTransceivePipe(
    PSZ String
    )
{
    NTSTATUS Status;
    IO_STATUS_BLOCK Iosb;
    UCHAR Buffer[512];


    DevPrint("TransceivePipe...\n", 0);

    if (!NT_SUCCESS(Status = NtFsControlFile(
                                TalkPort,
                                NULL,   // Event
                                NULL,   // ApcRoutine
                                NULL,   // ApcContext
                                &Iosb,
                                FSCTL_PIPE_TRANSCEIVE,
                                String,
                                strlen( String ),
                                Buffer,
                                511
                                ))) {
        Error( NtTransceiveFile, Status );
    }

    if (!NT_SUCCESS(Status = NtWaitForSingleObject( TalkPort, TRUE, NULL ))) {

        Error( NtWaitForSingleObject, Status );
    }

    if (!NT_SUCCESS(Iosb.Status)) {

        Error( NtTransceiveFileFinalStatus, Iosb.Status );
    }

    return;
}


//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//   Named Pipe Server Operations                                           //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

HANDLE
SepServerCreatePipe(
    VOID
    )
{
    HANDLE PipeHandle;
    NTSTATUS Status;
    IO_STATUS_BLOCK Iosb;
    LARGE_INTEGER Timeout;
    READ_MODE Mode;
    ULONG Share;
    NAMED_PIPE_CONFIGURATION Config = FILE_PIPE_FULL_DUPLEX;
    NAMED_PIPE_TYPE PipeType        = FILE_PIPE_MESSAGE_TYPE;
    COMPLETION_MODE CompletionMode  = FILE_PIPE_QUEUE_OPERATION;
    ULONG MaximumInstances          = 4;


    //
    //  Set the default timeout to 60 seconds, and initalize the attributes
    //

    Timeout.QuadPart = -10 * 1000 * 1000 * 60;

    InitializeObjectAttributes(
        &ObjectAttributes,
        &UnicodePortName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    //
    //  Calculate the readmode and share access
    //

    Mode = (PipeType == FILE_PIPE_MESSAGE_TYPE ? FILE_PIPE_MESSAGE_MODE :
                                                 FILE_PIPE_BYTE_STREAM_MODE);

    Share = (Config == FILE_PIPE_INBOUND  ? FILE_SHARE_WRITE :
            (Config == FILE_PIPE_OUTBOUND ? FILE_SHARE_READ :
                                            FILE_SHARE_READ | FILE_SHARE_WRITE));

    if (!NT_SUCCESS(Status = NtCreateNamedPipeFile(
                                &PipeHandle,
                                GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                                &ObjectAttributes,
                                &Iosb,
                                Share,
                                FILE_CREATE,
                                0,
                                PipeType,
                                Mode,
                                CompletionMode,
                                MaximumInstances,
                                1024,
                                1024,
                                (PLARGE_INTEGER)&Timeout ))) {

        Error( CreatePipe, Status );
    }
    RtlFreeUnicodeString( &UnicodePortName );

    return PipeHandle;
}


VOID
SepServerListenPipe(
    )
{
    NTSTATUS Status;
    IO_STATUS_BLOCK Iosb;

    DevPrint("ListenPipe...\n", 0);

    if (!NT_SUCCESS(Status = NtFsControlFile(
                                EarPort,
                                NULL,   // Event
                                NULL,   // ApcRoutine
                                NULL,   // ApcContext
                                &Iosb,
                                FSCTL_PIPE_LISTEN,
                                NULL,   // InputBuffer
                                0,      // InputBufferLength,
                                NULL,   // OutputBuffer
                                0       // OutputBufferLength
                                ))) {

        Error( ListenPipe, Status );
    }
    if (!NT_SUCCESS(Status = NtWaitForSingleObject( EarPort, TRUE, NULL ))) {

        Error( NtWaitForSingleObject, Status );
    }

    if (!NT_SUCCESS(Iosb.Status)) {

        Error( ListenPipeFinalStatus, Iosb.Status );
    }


    return;
}


VOID
SepServerImpersonatePipe(
    )
{
    NTSTATUS Status;
    IO_STATUS_BLOCK Iosb;

    DevPrint("ImpersonatePipe...\n", 0);

    if (!NT_SUCCESS(Status = NtFsControlFile(
                                TalkPort,
                                NULL,   // Event
                                NULL,   // ApcRoutine
                                NULL,   // ApcContext
                                &Iosb,
                                FSCTL_PIPE_IMPERSONATE,
                                NULL,   // InputBuffer
                                0,      // InputBufferLength,
                                NULL,   // OutputBuffer
                                0       // OutputBufferLength
                                ))) {

        Error( ImpersonatePipe, Status );
    }
    if (!NT_SUCCESS(Status = NtWaitForSingleObject( TalkPort, TRUE, NULL ))) {

        Error( NtWaitForSingleObject, Status );
    }

    if (!NT_SUCCESS(Iosb.Status)) {

        Error( ImpersonatePipeFinalStatus, Iosb.Status );
    }

    return;
}


VOID
SepServerDisconnectPipe(
    )
{
    NTSTATUS Status;
    IO_STATUS_BLOCK Iosb;

    DevPrint("DisconnectPipe...\n", 0);
    DevPrint("        (Flush)...\n", 0);

    if (!NT_SUCCESS(Status = NtFlushBuffersFile(
                                TalkPort,
                                &Iosb
                                ))) {
        Error( DisconnectPipe, Status );
    }

    if (!NT_SUCCESS(Iosb.Status)) {

        Error( FlushPipeFinalStatus, Iosb.Status );
    }


    DevPrint("        (Close Talk Port)...\n", 0);
    Status = NtClose( TalkPort ); SEASSERT_SUCCESS(Status);

    DevPrint("        (Disconnect)...\n", 0);
    if (!NT_SUCCESS(Status = NtFsControlFile(
                                EarPort,
                                NULL,   // Event
                                NULL,   // ApcRoutine
                                NULL,   // ApcContext
                                &Iosb,
                                FSCTL_PIPE_DISCONNECT,
                                NULL,   // InputBuffer
                                0,      // InputBufferLength,
                                NULL,   // OutputBuffer
                                0       // OutputBufferLength
                                ))) {

        Error( DisconnectPipe, Status );
    }
    if (!NT_SUCCESS(Status = NtWaitForSingleObject( EarPort, TRUE, NULL ))) {

        Error( NtWaitForSingleObject, Status );
    }

    if (!NT_SUCCESS(Iosb.Status)) {

        Error( DisconnectPipeFinalStatus, Iosb.Status );
    }

    return;
}


//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//   Named Pipe Client Operations                                           //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

HANDLE
SepClientOpenPipe(
    VOID
    )
{
    HANDLE PipeHandle, NpfsHandle;
    NTSTATUS Status;
    IO_STATUS_BLOCK Iosb;
    ULONG Share;
    STRING Npfs;
    UNICODE_STRING UnicodeNpfs;
    PFILE_PIPE_WAIT_FOR_BUFFER WaitPipe;
    ULONG WaitPipeLength;
    NAMED_PIPE_CONFIGURATION Config = FILE_PIPE_FULL_DUPLEX;
    READ_MODE ReadMode              = FILE_PIPE_MESSAGE_MODE;
    COMPLETION_MODE CompletionMode  = FILE_PIPE_QUEUE_OPERATION;


//#ifdef NOT_YET_WORKING
    //
    // Wait for the server's pipe to reach a listen state...
    //

    RtlInitString( &Npfs, "\\Device\\NamedPipe\\");
    Status = RtlAnsiStringToUnicodeString(
                 &UnicodeNpfs,
                 &Npfs,
                 TRUE );  SEASSERT_SUCCESS( NT_SUCCESS(Status) );

    InitializeObjectAttributes(
        &ObjectAttributes,
        &UnicodeNpfs,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL);

    if (!NT_SUCCESS(Status = NtOpenFile(
                                &NpfsHandle,
                                GENERIC_READ | SYNCHRONIZE,
                                &ObjectAttributes,
                                &Iosb,
                                FILE_SHARE_READ,
                                0 ))) {

        Error( OpenNpfs, Status );
    }
    RtlFreeUnicodeString( &UnicodeNpfs );

    WaitPipeLength =
        FIELD_OFFSET(FILE_PIPE_WAIT_FOR_BUFFER, Name[0]) +
        RelativePortName.MaximumLength;                 //UNICODEFIX UnicodeRelativePortName.MaximumLength;
    WaitPipe = RtlAllocateHeap(RtlProcessHeap(), 0, WaitPipeLength);
    WaitPipe->TimeoutSpecified = FALSE;

    WaitPipe->NameLength = RelativePortName.Length;     //UNICODEFIX UnicodeRelativePortName.Length;
    strcpy(WaitPipe->Name, RelativePortName.Buffer);    //UNICODEFIX UnicodePortName.Buffer;

    if (!NT_SUCCESS(Status = NtFsControlFile(
                                NpfsHandle,
                                NULL,        // Event
                                NULL,        // ApcRoutine
                                NULL,        // ApcContext
                                &Iosb,
                                FSCTL_PIPE_WAIT,
                                WaitPipe,       // Buffer for data to the FS
                                WaitPipeLength,
                                NULL,        // OutputBuffer
                                0            // OutputBufferLength
                                ))) {

        Error( ClientWaitPipe, Status );
    }
    if (Status == STATUS_PENDING) {
        if (!NT_SUCCESS(Status = NtWaitForSingleObject( NpfsHandle, TRUE, NULL ))) {

            Error( NtWaitForSingleObject, Status );
        }
    }

    if (!NT_SUCCESS(Iosb.Status)) {

        Error( ClientWaitPipeFinalStatus, Iosb.Status );
    }

    Status = NtClose( NpfsHandle );
    ASSERT(NT_SUCCESS(Status));
//#endif  // NOT_YET_WORKING
//    Delay(1);


    //
    //  Initialize the attributes
    //

    InitializeObjectAttributes(
        &ObjectAttributes,
        &UnicodePortName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );
    ObjectAttributes.SecurityQualityOfService = (PVOID)(&SecurityQos);

    //
    //  Calculate the share access
    //

    Share = (Config == FILE_PIPE_INBOUND  ? FILE_SHARE_WRITE :
            (Config == FILE_PIPE_OUTBOUND ? FILE_SHARE_READ :
                       FILE_SHARE_READ | FILE_SHARE_WRITE));



    //
    // And now open it...
    //

    if (!NT_SUCCESS(Status = NtOpenFile(
                                &PipeHandle,
                                GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                                &ObjectAttributes,
                                &Iosb,
                                Share,
                                0 ))) {

        Error( OpenPipe, Status );
    }

    if ((ReadMode != FILE_PIPE_BYTE_STREAM_MODE) ||
        (CompletionMode != FILE_PIPE_QUEUE_OPERATION)) {

        FILE_PIPE_INFORMATION Buffer;

        Buffer.ReadMode = ReadMode;
        Buffer.CompletionMode = CompletionMode;

        if (!NT_SUCCESS(Status = NtSetInformationFile(
                                PipeHandle,
                                &Iosb,
                                &Buffer,
                                sizeof(FILE_PIPE_INFORMATION),
                                FilePipeInformation ))) {

            Error( NtSetInformationFile, Status );
        }
    }

    return PipeHandle;
}
