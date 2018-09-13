
//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//                Global Definitions                                        //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////




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
    UNICODE_STRING PortName;
    HANDLE EarPort;
    HANDLE TalkPort;
    PORT_MESSAGE RequestMessage;
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




BOOLEAN
CtLpcQos (VOID);


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

    Status = NtConnectPort(
                 &TalkPort,
                 &PortName,
                 &SecurityQos,
                 0L,
                 NULL,
                 NULL,
                 NULL,
                 NULL,
                 NULL
                 );  SEASSERT_SUCCESS(Status);

    return;
}


VOID
SepClientMakeRemoteCall( VOID )

{
    PORT_MESSAGE ReplyMessage;

    Status = NtRequestWaitReplyPort(
                 TalkPort,
                 &RequestMessage,
                 &ReplyMessage
                 );

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

    //
    // Initialize global variables
    //

    RequestMessage.u1.s1.DataLength = 0;
    RequestMessage.u1.s1.TotalLength = (CSHORT)sizeof(PORT_MESSAGE);
    RequestMessage.u2.ZeroInit = 0;

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

    CONNECTION_REQUEST ConnectionRequest;

    ConnectionRequest.Length = (ULONG)sizeof(CONNECTION_REQUEST);

    //
    // Wait for the client to connect to the port
    //

    Status = NtListenPort(
                 EarPort,
                 &ConnectionRequest,
                 NULL,
                 0L
                 ); SEASSERT_SUCCESS(Status);

    Status = NtAcceptConnectPort(
                 &TalkPort,
                 NULL,
                 &ConnectionRequest,
                 TRUE,
                 NULL,
                 NULL,
                 NULL,
                 0L
                 ); SEASSERT_SUCCESS(Status);

    Status = NtCompleteConnectPort( TalkPort ); SEASSERT_SUCCESS(Status);

    return;

}

VOID
SepServerGetNextMessage( VOID )

{

    //
    // Wait for the next message to come in...
    //

    Status = NtReplyWaitReceivePort(
                 EarPort,
                 NULL,
                 NULL,
                 &RequestMessage
                 ); SEASSERT_SUCCESS(Status);

    RequestCount += 1;

    return;
}

VOID
SepServerCompleteMessage( VOID )

{
    PORT_MESSAGE ReplyMessage;

    ReplyMessage.u1.s1.DataLength = 0;
    ReplyMessage.u1.s1.TotalLength = (CSHORT)sizeof(PORT_MESSAGE);
    ReplyMessage.u2.ZeroInit = 0;
    ReplyMessage.ClientId = RequestMessage.ClientId;
    ReplyMessage.MessageId = RequestMessage.MessageId;

    //
    // Send the response message
    //

    Status = NtReplyPort(
                 EarPort,
                 &ReplyMessage
                 ); SEASSERT_SUCCESS(Status);

    return;
}

VOID
SepServerImpersonateClient( VOID )

{

    Status = NtImpersonateClientOfPort(
                 TalkPort,
                 &RequestMessage
                 );   SEASSERT_SUCCESS(Status);

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
    Status = NtClose( TalkPort ); SEASSERT_SUCCESS(Status);

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

    NTSTATUS Status;
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

    InitializeObjectAttributes(
        &ObjectAttributes,
        &PortName,
        0,
        NULL,
        NULL );

    Status = NtCreatePort(
                 &EarPort,
                 &ObjectAttributes,
                 0,
                 4,
                 4 * 256
                 ); SEASSERT_SUCCESS(Status);



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
    STRING ImagePathName, ProgramName;
    UNICODE_STRING UnicodeImagePathName, UnicodeProgramName;
    PRTL_USER_PROCESS_PARAMETERS ProcessParameters;

    RtlInitString( &ProgramName, "\\SystemRoot\\Bin\\utlpcqos.exe" );
    Status = RtlAnsiStringToUnicodeString(
                 &UnicodeProgramName,
                 &ProgramName,
                 TRUE );  SEASSERT_SUCCESS( NT_SUCCESS(Status) );
    RtlInitString( &ImagePathName, "utlpcqos.exe");
    Status = RtlAnsiStringToUnicodeString(
                 &UnicodeImagePathName,
                 &ImagePathName,
                 TRUE );  SEASSERT_SUCCESS( NT_SUCCESS(Status) );

    Status = RtlCreateProcessParameters(
                 &ProcessParameters,
                 &ImagePathName,        //UNICODEFIX &UnicodeImagePathName,
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


    Status = RtlCreateUserProcess(
                 &ProgramName,                   // UNICODEFIX &UnicodeProgramName,
                 ProcessParameters,              // ProcessParameters
                 NULL,                           // ProcessSecurityDescriptor
                 NULL,                           // ThreadSecurityDescriptor
                 NtCurrentProcess(),             // ParentProcess
                 FALSE,                          // InheritHandles
                 NULL,                           // DebugPort
                 NULL,                           // ExceptionPort
                 &ProcessInformation             // ProcessInformation
                 ); SEASSERT_SUCCESS(Status);

    Status = NtResumeThread(
                  ProcessInformation.Thread,
                  NULL
                  ); SEASSERT_SUCCESS(Status);

    RtlDestroyProcessParameters( ProcessParameters );
    RtlFreeUnicodeString( &UnicodeProgramName );
    RtlFreeUnicodeString( &UnicodeImagePathName );

}




//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//                Main Program Entry Routine                                //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

BOOLEAN
CtLpcQos (VOID)
{

    BOOLEAN Result = TRUE;

    RtlInitUnicodeString( &PortName, L"\\TestLpcQosServerPort" );

    //
    // Determine whether we are the client or server side of the test.
    // This is done by creating or opening a named event object.  If the
    // event does not yet exist, then we are the client, and must create
    // the server process.  Otherwise, we are the server and the client
    // is waiting for us to signal the event.
    //

    RtlInitString( &EventName, "\\TestLpcQosEvent" );
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

    DbgPrint("Se: Starting LPC Impersonation Test.\n");

        Status = SepServerInitialize(); SEASSERT_SUCCESS(Status);
        Result = SepServerTest();

    DbgPrint("Se: End Test.\n");

        }



    Status = NtTerminateThread( NtCurrentThread(), STATUS_SUCCESS);
    SEASSERT_SUCCESS(Status);

    return Result;

}
