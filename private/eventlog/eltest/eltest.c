/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    ELTEST.C

Abstract:

    Test Routines for the EventLog.

THINGS I WANT THIS TO DO...
    AddReg <ServerName> <logname> <EntryName> <EventMessageFile>
        <CategoryMessageFile> <CategoryCount> <ParameterMessageFile>
        <TypesSupported>   - Creates A Registry Entry.

        eltest addreg application mytest mf= eltest.dll cat=

    CreateMessageFile <??? Is this possible ???>

    WriteEvent <ServerName> <EventSource> <Type> <Category> <EventId> <UserSid?>
        <NumStrings> <Strings> <RawData>

    ReadLog <Server> <LogFile> <ReadFlags> <RecordOffset> <bufSize>
        If LogFile isn't one of the popular ones, then it could be a backup
        logfile.

    GetNumEvents <Server> <LogFile>

    GetOldest <Server> <LogFile>

    Clear <Server> <LogFile>

    Backup <Server> <LogFile> <BackupFile>


    LOOPTESTS....
    I should be able to run this test like mprtest such that it doesn't leave
    the test process until told.  This way we can register an event source,
    then if we call WriteEvent without a specified EventSource, it will use
    the stored source.  Calling RegisterEventSource twice without calling
    DeRegisterSource would be an error.  (Or better yet, I could keep a table
    of sources and handles).

    RegisterEventSource <EventSource>
    DeRegisterSource <EventSource>




PROTOTYPES FOR FUNCTION....

BOOL
CloseEventLog (
    HANDLE hEventLog
    )
BOOL
DeregisterEventSource (
    HANDLE hEventLog
    )

BOOL
NotifyChangeEventLog(
    HANDLE  hEventLog,
    HANDLE  hEvent
    )
BOOL
GetNumberOfEventLogRecords (
    HANDLE hEventLog,
    PDWORD NumberOfRecords
    )
BOOL
GetOldestEventLogRecord (
    HANDLE hEventLog,
    PDWORD OldestRecord
    )
BOOL
ClearEventLogW (
    HANDLE hEventLog,
    LPCWSTR BackupFileName
    )
BOOL
BackupEventLogW (
    HANDLE hEventLog,
    LPCWSTR BackupFileName
    )
HANDLE
OpenEventLogW (
    LPCWSTR  UNCServerName,
    LPCWSTR  ModuleName
    )
HANDLE
RegisterEventSourceW (
    LPCWSTR  UNCServerName,
    LPCWSTR  ModuleName
    )
HANDLE
OpenBackupEventLogW (
    LPCWSTR  UNCServerName,
    LPCWSTR  FileName
    )
BOOL
ReadEventLogW (
    HANDLE      hEventLog,
    DWORD       dwReadFlags,
    DWORD       dwRecordOffset,
    LPVOID      lpBuffer,
    DWORD       nNumberOfBytesToRead,
    DWORD       *pnBytesRead,
    DWORD       *pnMinNumberOfBytesNeeded
    )
BOOL
ReportEventW (
    HANDLE      hEventLog,
    WORD        wType,
    WORD        wCategory       OPTIONAL,
    DWORD       dwEventID,
    PSID        lpUserSid       OPTIONAL,
    WORD        wNumStrings,
    DWORD       dwDataSize,
    LPCWSTR     *lpStrings      OPTIONAL,
    LPVOID      lpRawData       OPTIONAL
    )




Author:

    Dan Lafferty    (danl)  09-March-1994

Environment:

    User Mode - Win32

Revision History:

    09-Mar-1994     danl
        created

--*/

//
// INCLUDES
//
#define UNICODE 1
#include <nt.h>         // DbgPrint prototype
#include <ntrtl.h>      // DbgPrint prototype
#include <nturtl.h>     // needed for winbase.h


#include <stdlib.h>     // atoi
#include <stdio.h>      // printf
#include <conio.h>      // getch
#include <string.h>     // strcmp
#include <windows.h>    // win32 typedefs
#include <tstr.h>       // Unicode
#include <debugfmt.h>   // FORMAT_LPTSTR

//------------------
// DEFINES
//------------------
#define APPLICATION_LOG     "Application"
#define SYSTEM_LOG          "System"
#define SECURITY_LOG        "Security"

#define REG_APPLICATION_KEY "SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\"
#define REG_SYSTEM_KEY      "SYSTEM\\CurrentControlSet\\Services\\EventLog\\System\\"
#define REG_SECURITY_KEY    "SYSTEM\\CurrentControlSet\\Services\\EventLog\\Security\\"

#define EVENT_SOURCE_NAME       "tevent"
#define MSG_DLL                 "%SystemRoot%\\System32\\tevent.dll"

#define VALUE_EVENT_MF          TEXT("EventMessageFile")
#define VALUE_CATEGORY_MF       TEXT("CategoryMessageFile")
#define VALUE_PARAMETER_MF      TEXT("ParameterMessageFile")
#define VALUE_TYPES_SUPPORTED   TEXT("TypesSupported")
#define VALUE_CATEGORY_COUNT    TEXT("CategoryCount")

#define TYPES_SUPPORTED        (EVENTLOG_ERROR_TYPE     |   \
                                EVENTLOG_WARNING_TYPE   |   \
                                EVENTLOG_INFORMATION_TYPE)
//----------------------
// GLOBALS
//----------------------
    LPTSTR  ApplLogRegName=TEXT(REG_APPLICATION_KEY);
    LPTSTR  SysLogRegName =TEXT(REG_SYSTEM_KEY);
    LPTSTR  SecLogRegName =TEXT(REG_SECURITY_KEY);
    LPTSTR  ApplLogName   = TEXT(APPLICATION_LOG);
    LPTSTR  SysLogName    = TEXT(SYSTEM_LOG);
    LPTSTR  SecLogName    = TEXT(SECURITY_LOG);

//----------------------
// FUNCTION PROTOTYPES
//----------------------

VOID
AddRegUsage(VOID);

DWORD
AddSourceToRegistry(
    IN  LPTSTR  ServerName,
    IN  LPTSTR  LogName,
    IN  LPTSTR  EventSourceName,
    IN  LPTSTR  *argv,
    IN  DWORD   argc
    );

BOOL
ConvertToUnicode(
    OUT LPWSTR  *UnicodeOut,
    IN  LPSTR   AnsiIn
    );

DWORD
DelSourceInRegistry(
    IN  LPTSTR  ServerName,
    IN  LPTSTR  LogName,
    IN  LPTSTR  EventSourceName
    );

VOID
DisplayStatus (
    IN  LPTSTR              ServiceName,
    IN  LPTSTR              DisplayName,
    IN  LPSERVICE_STATUS    ServiceStatus
    );

BOOL
MakeArgsUnicode (
    DWORD           argc,
    PCHAR           argv[]
    );

BOOL
ProcessArgs (
    LPTSTR      ServerName,
    DWORD       argc,
    LPTSTR      argv[]
    );

VOID
Usage(
    VOID);

VOID
ConfigUsage(VOID);

VOID
CreateUsage(VOID);

VOID
QueryUsage(VOID);

LONG
wtol(
    IN LPWSTR string
    );

VOID
UserInputLoop(
    LPTSTR  ServerName
    );
DWORD
ReadLogFile(
    LPTSTR  ServerName,
    LPTSTR  LogName,
    IN  LPTSTR  *argv,
    IN  DWORD   argc
    );
VOID
ReadLogUsage(VOID);

VOID
DisplayRecord(
    PEVENTLOGRECORD     pElRecord,
    BOOL                PrintTheHeader
    );

/****************************************************************************/
VOID __cdecl
main (
    DWORD           argc,
    PCHAR           argvAnsi[]
    )

/*++

Routine Description:

    Allows manual testing of the EVENTLOG API.

        eltest



Arguments:



Return Value:



--*/
{
    UCHAR   i;
    DWORD   j;
    DWORD   argIndex;
    LPTSTR  pServerName=NULL;
    LPTSTR  *argv;

    if (argc <2) {
        Usage();
        return;
    }

    //
    // Make the arguments unicode if necessary.
    //
#ifdef UNICODE
    if (!MakeArgsUnicode(argc, argvAnsi)) {
        return;
    }
#endif

    argv = (LPTSTR *)argvAnsi;

    argIndex = 1;
    if (STRNCMP (argv[1], TEXT("\\\\"), 2) == 0) {
        pServerName = argv[1];
        argIndex = 2;               // skip over servername.
    }

    //
    // Check to see if we are to run in Loop Mode, or in single function
    // mode.  In Loop Mode, we go into a loop, and ask the user for
    // input until the user decides to quit.
    //
    // Process Arguments:
    //
    // INDEX   0       1            2              3
    //         EL <ServerName> <Function> <FunctionOptions...>
    //

    if (STRICMP (argv[argIndex], TEXT("Loop")) == 0) {
        UserInputLoop(pServerName);
    }
    else {
        ProcessArgs(pServerName, argc-argIndex, &(argv[argIndex]));
    }


#ifdef UNICODE
    //
    // Free up the unicode strings if there are any
    //
    for(j=0; j<argc; j++) {
        LocalFree(argv[j]);
    }
#endif

    return;
}

VOID
UserInputLoop(
    LPTSTR  ServerName
    )

/*++

Routine Description:

    This function sits in a loop, gathering input from the user, and
    processing that input until the user indicates that it should stop.
    The following user commands indicate that we should stop:
        done
        exit
        stop
        quit

Arguments:


Return Value:


--*/
{
    UCHAR   i;
    DWORD   j;
    LPTSTR  *argv;
    UCHAR   buffer[255];
    LPSTR   argvA[20];
    DWORD   argc=0;
    BOOL    KeepGoing;

    do {
        //------------------------------
        // Get input from the user
        //------------------------------
        buffer[0] = 90-2;

        printf("\nwaiting for instructions... \n");
        cgets(buffer);

        if (buffer[1] > 0) {
            //--------------------------------------
            // put the string in argv/argc format.
            //--------------------------------------
            buffer[1]+=2;       // make this an end offset
            argc=0;
            for (i=2,j=0; i<buffer[1]; i++,j++) {
                argc++;
                argvA[j] = &(buffer[i]);
                while ((buffer[i] != ' ') && (buffer[i] != '\0')) {
                    i++;
                }
                buffer[i] = '\0';
            }

            //------------------------------------------
            // Make the arguments unicode if necessary.
            //------------------------------------------
#ifdef UNICODE

            if (!MakeArgsUnicode(argc, argvA)) {
                return;
            }

#endif
            //-----------------------------------------------
            // If the first argument doesn't indicate that
            // we should stop, then process the arguments.
            //-----------------------------------------------
            argv = (LPTSTR *)argvA;

            if((STRICMP (argv[0], TEXT("done")) == 0) ||
               (STRICMP (argv[0], TEXT("stop")) == 0) ||
               (STRICMP (argv[0], TEXT("exit")) == 0) ||
               (STRICMP (argv[0], TEXT("quit")) == 0)) {
                KeepGoing  = FALSE;
            }
            else {
                KeepGoing = ProcessArgs(ServerName, argc, argv);
            }

#ifdef UNICODE
            //-----------------------------------------------
            // Free up the unicode strings if there are any
            //-----------------------------------------------
            for(j=0; j<argc; j++) {
                LocalFree(argv[j]);
            }
#endif
        }
    } while (KeepGoing);

    return;

}

/****************************************************************************/
BOOL
ProcessArgs (
    LPTSTR      ServerName,
    DWORD       argc,
    LPTSTR      argv[]
    )

/*++

Routine Description:


Arguments:



Return Value:



--*/

{
    DWORD           status;
    DWORD           specialFlag = FALSE;
    DWORD           argIndex;       // index to unchecked portion of arglist.


    argIndex = 0;

    //
    // If we are adding a registry entry, the get a handle to it.
    // Otherwise, get a handle to the LogFile.
    //
    //-----------------------
    // AddSourceToRegistry
    //-----------------------
    if (STRICMP (argv[argIndex], TEXT("AddReg")) == 0 ) {

        //
        // Must have at least "AddReg logname EntryName"
        //
        if (argc < (argIndex + 3)) {
            AddRegUsage();
            goto CleanExit;
        }

        status = AddSourceToRegistry(
                    ServerName,
                    argv[argIndex+1],    // LogName
                    argv[argIndex+2],    // SourceName
                    &argv[argIndex+1],
                    argc-(argIndex+2)
                    );
    }
    //-----------------------
    // DeleteFromRegistry
    //-----------------------
    else if (STRICMP (argv[argIndex], TEXT("DelReg")) == 0) {
        //
        // Must have at least "DelReg logname EntryName"
        //
        if (argc < (argIndex + 3)) {
            goto CleanExit;
        }

        status = DelSourceInRegistry(
                    ServerName,
                    argv[argIndex+1],    // LogName
                    argv[argIndex+2]     // SourceName
                    );

    }
    //-----------------------
    // WriteEvent
    //-----------------------
    else if (STRICMP (argv[argIndex], TEXT("WriteEvent")) == 0) {
        printf("In WriteEvent\n");
        if (ServerName != NULL) {
            printf("ServerName = "FORMAT_LPTSTR"\n",ServerName);
        }
    }
    //-----------------------
    // ReadLog
    //-----------------------
    else if (STRICMP (argv[argIndex], TEXT("ReadLog")) == 0) {
        printf("In ReadLog\n");
        if (ServerName != NULL) {
            printf("ServerName = "FORMAT_LPTSTR"\n",ServerName);
        }
        //
        // Must have at least "ReadLog logname"
        //
        if (argc < (argIndex + 2)) {
            ReadLogUsage();
            goto CleanExit;
        }

        status = ReadLogFile(
                    ServerName,         // ServerName
                    argv[argIndex+1],   // LogName
                    &argv[argIndex+1],  // argv
                    argc-(argIndex+1)); // argc
    }
    //-----------------------
    // GetNumEvents
    //-----------------------
    else if (STRICMP (argv[argIndex], TEXT("GetNumEvents")) == 0) {
        printf("in GetNumEvents\n");
        if (ServerName != NULL) {
            printf("ServerName = "FORMAT_LPTSTR"\n",ServerName);
        }
    }
    //-----------------------
    // GetOldest
    //-----------------------
    else if (STRICMP (argv[argIndex], TEXT("GetOldest")) == 0) {
        printf("in GetOldest\n");
        if (ServerName != NULL) {
            printf("ServerName = "FORMAT_LPTSTR"\n",ServerName);
        }
    }
    //-----------------------
    // ClearLog
    //-----------------------
    else if (STRICMP (argv[argIndex], TEXT("ClearLog")) == 0) {
        printf("in ClearLog\n");
        if (ServerName != NULL) {
            printf("ServerName = "FORMAT_LPTSTR"\n",ServerName);
        }
    }
    //-----------------------
    // Backup
    //-----------------------
    else if (STRICMP (argv[argIndex], TEXT("Backup")) == 0) {
        printf("in Backup\n");
        if (ServerName != NULL) {
            printf("ServerName = "FORMAT_LPTSTR"\n",ServerName);
        }
    }
    //-----------------------
    // RegisterSource
    //-----------------------
    else if (STRICMP (argv[argIndex], TEXT("RegisterSource")) == 0) {
        printf("in RegisterSource\n");
        if (ServerName != NULL) {
            printf("ServerName = "FORMAT_LPTSTR"\n",ServerName);
        }
    }
    //-----------------------
    // DeRegisterSource
    //-----------------------
    else if (STRICMP (argv[argIndex], TEXT("DeRegisterSource")) == 0) {
        printf("in DeRegisterSource\n");
        if (ServerName != NULL) {
            printf("ServerName = "FORMAT_LPTSTR"\n",ServerName);
        }
    }
    //****************
    // Exit Program
    //****************
    else if (STRICMP (argv[0], TEXT("Exit")) == 0) {
        //
        // THIS SHOULD CLOSE HANDLES.
        //
        return(FALSE);
    }
    else {
        printf("Bad argument\n");
        Usage();
    }

CleanExit:


    return(TRUE);
}



BOOL
MakeArgsUnicode (
    DWORD           argc,
    PCHAR           argv[]
    )


/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    DWORD   i;

    //
    // ScConvertToUnicode allocates storage for each string.
    // We will rely on process termination to free the memory.
    //
    for(i=0; i<argc; i++) {

        if(!ConvertToUnicode( (LPWSTR *)&(argv[i]), argv[i])) {
            printf("Couldn't convert argv[%d] to unicode\n",i);
            return(FALSE);
        }


    }
    return(TRUE);
}

BOOL
ConvertToUnicode(
    OUT LPWSTR  *UnicodeOut,
    IN  LPSTR   AnsiIn
    )

/*++

Routine Description:

    This function translates an AnsiString into a Unicode string.
    A new string buffer is created by this function.  If the call to
    this function is successful, the caller must take responsibility for
    the unicode string buffer that was allocated by this function.
    The allocated buffer should be free'd with a call to LocalFree.

    NOTE:  This function allocates memory for the Unicode String.

    BUGBUG:  This should be changed to return either
        ERROR_NOT_ENOUGH_MEMORY or ERROR_INVALID_PARAMETER

Arguments:

    AnsiIn - This is a pointer to an ansi string that is to be converted.

    UnicodeOut - This is a pointer to a location where the pointer to the
        unicode string is to be placed.

Return Value:

    TRUE - The conversion was successful.

    FALSE - The conversion was unsuccessful.  In this case a buffer for
        the unicode string was not allocated.

--*/
{

    NTSTATUS        ntStatus;
    DWORD           bufSize;
    UNICODE_STRING  unicodeString;
    ANSI_STRING     ansiString;

    //
    // Allocate a buffer for the unicode string.
    //

    bufSize = (strlen(AnsiIn)+1) * sizeof(WCHAR);

    *UnicodeOut = (LPWSTR)LocalAlloc(LMEM_ZEROINIT, (UINT)bufSize);

    if (*UnicodeOut == NULL) {
        printf("ScConvertToUnicode:LocalAlloc Failure %ld\n",GetLastError());
        return(FALSE);
    }

    //
    // Initialize the string structures
    //
    RtlInitAnsiString( &ansiString, AnsiIn);

    unicodeString.Buffer = *UnicodeOut;
    unicodeString.MaximumLength = (USHORT)bufSize;
    unicodeString.Length = 0;

    //
    // Call the conversion function.
    //
    ntStatus = RtlAnsiStringToUnicodeString (
                &unicodeString,     // Destination
                &ansiString,        // Source
                (BOOLEAN)FALSE);    // Allocate the destination

    if (!NT_SUCCESS(ntStatus)) {

        printf("ScConvertToUnicode:RtlAnsiStringToUnicodeString Failure %lx\n",
        ntStatus);

        return(FALSE);
    }

    //
    // Fill in the pointer location with the unicode string buffer pointer.
    //
    *UnicodeOut = unicodeString.Buffer;

    return(TRUE);

}

/****************************************************************************/
VOID
DisplayStatus (
    IN  LPTSTR              ServiceName,
    IN  LPTSTR              DisplayName,
    IN  LPSERVICE_STATUS    ServiceStatus
    )

/*++

Routine Description:

    Displays the service name and  the service status.

    |
    |SERVICE_NAME: messenger
    |DISPLAY_NAME: messenger
    |        TYPE       : WIN32
    |        STATE      : ACTIVE,STOPPABLE, PAUSABLE, ACCEPTS_SHUTDOWN
    |        EXIT_CODE  : 0xC002001
    |        CHECKPOINT : 0x00000001
    |        WAIT_HINT  : 0x00003f21
    |

Arguments:

    ServiceName - This is a pointer to a string containing the name of
        the service.

    DisplayName - This is a pointer to a string containing the display
        name for the service.

    ServiceStatus - This is a pointer to a SERVICE_STATUS structure from
        which information is to be displayed.

Return Value:

    none.

--*/
{

    printf("\nSERVICE_NAME: "FORMAT_LPTSTR"\n", ServiceName);
    if (DisplayName != NULL) {
        printf("DISPLAY_NAME: "FORMAT_LPTSTR"\n", DisplayName);
    }

    printf("        TYPE               : %lx  ", ServiceStatus->dwServiceType);

    switch(ServiceStatus->dwServiceType){
    case SERVICE_WIN32_OWN_PROCESS:
        printf("WIN32_OWN_PROCESS \n");
        break;
    case SERVICE_WIN32_SHARE_PROCESS:
        printf("WIN32_SHARE_PROCESS \n");
        break;
    case SERVICE_WIN32:
        printf("WIN32 \n");
        break;
    case SERVICE_ADAPTER:
        printf("ADAPTER \n");
        break;
    case SERVICE_KERNEL_DRIVER:
        printf("KERNEL_DRIVER \n");
        break;
    case SERVICE_FILE_SYSTEM_DRIVER:
        printf("FILE_SYSTEM_DRIVER \n");
        break;
    case SERVICE_DRIVER:
        printf("DRIVER \n");
        break;
    default:
        printf(" ERROR \n");
    }

    printf("        STATE              : %lx  ", ServiceStatus->dwCurrentState);

    switch(ServiceStatus->dwCurrentState){
        case SERVICE_STOPPED:
            printf("STOPPED ");
            break;
        case SERVICE_START_PENDING:
            printf("START_PENDING ");
            break;
        case SERVICE_STOP_PENDING:
            printf("STOP_PENDING ");
            break;
        case SERVICE_RUNNING:
            printf("RUNNING ");
            break;
        case SERVICE_CONTINUE_PENDING:
            printf("CONTINUE_PENDING ");
            break;
        case SERVICE_PAUSE_PENDING:
            printf("PAUSE_PENDING ");
            break;
        case SERVICE_PAUSED:
            printf("PAUSED ");
            break;
        default:
            printf(" ERROR ");
    }

    //
    // Print Controls Accepted Information
    //

    if (ServiceStatus->dwControlsAccepted & SERVICE_ACCEPT_STOP) {
        printf("\n                                (STOPPABLE,");
    }
    else {
        printf("\n                                (NOT_STOPPABLE,");
    }

    if (ServiceStatus->dwControlsAccepted & SERVICE_ACCEPT_PAUSE_CONTINUE) {
        printf("PAUSABLE,");
    }
    else {
        printf("NOT_PAUSABLE,");
    }

    if (ServiceStatus->dwControlsAccepted & SERVICE_ACCEPT_SHUTDOWN) {
        printf("ACCEPTS_SHUTDOWN)\n");
    }
    else {
        printf("IGNORES_SHUTDOWN)\n");
    }

    //
    // Print Exit Code
    //
    printf("        WIN32_EXIT_CODE    : %d\t(0x%lx)\n",
        ServiceStatus->dwWin32ExitCode,
        ServiceStatus->dwWin32ExitCode);
    printf("        SERVICE_EXIT_CODE  : %d\t(0x%lx)\n",
        ServiceStatus->dwServiceSpecificExitCode,
        ServiceStatus->dwServiceSpecificExitCode  );

    //
    // Print CheckPoint & WaitHint Information
    //

    printf("        CHECKPOINT         : 0x%lx\n", ServiceStatus->dwCheckPoint);
    printf("        WAIT_HINT          : 0x%lx\n", ServiceStatus->dwWaitHint  );

    return;
}

VOID
Usage(
    VOID)
{
    printf("DESCRIPTION:\n");
    printf("\tEL is a command line program used for testing the eventlog \n");
    printf("USAGE:\n");
    printf("\tEL <ServerName> [Function] <FunctionOptions...> \n\n");
    printf("\tThe option <server> has the form \"\\\\ServerName\"\n");
    printf("\tFurther help on Functions can be obtained by typing: \"el [Function]\"\n");
    printf("\tFunctions:\n"
           "\t  AddReg-----------Creates a registry entry for an event source.\n"
           "\t  DelReg-----------Deletes a registry entry.\n"
           "\t  WriteEvent-------Writes an event.\n"
           "\t  ReadLog----------Reads from the logfile.\n"
           "\t  GetNumEvents-----Gets the number of events in the specified log.\n"
           "\t  GetOldest--------Gets the record number for the oldest record"
           "\t                   in the log\n"
           "\t  ClearLog---------Clears the specified Log.\n"
           "\t  Backup-----------Copies the specified log to a new file.\n"
           "\t  RegisterSource---Registers a name for the event source.\n"
           "\t                   The handle is stored internally.\n"
           "\t  DeRegisterSource-Closes handle opened with RegSource.\n"
           "\t  NotifyChange-----A thread is created which gets notified of EL changes.\n");

    printf("\n");
}

VOID
AddRegUsage(VOID)
{

    printf("\nAdds a subkey under one of the logfiles listed in the registry.\n");
    printf("SYNTAX: \n  eltest addreg <ServerName> logfile <SubKeyName> <option1> <option2>...\n");
    printf("ADDREG OPTIONS:\n");
    printf("NOTE: The option name includes the equal sign.\n");

    printf("    MsgFile=    Name of Event Message File\n"
           "    CatFile=    Name of Category Message File\n"
           "    ParamFile=  Name of Parameter Message File\n"
           "    CatCount=   Category Count\n"
           "    Type=       <error|warning|information|AuditSuccess|AuditFailure|All>\n");
    printf("EXAMPLE:\n   eltest addreg application myapp MsgFile= MyMsgs.dll"
           " Type= error Type= warning\n");

}

VOID
ConfigUsage(VOID)
{
    printf("Modifies a service entry in the registry and Service Database.\n");
    printf("SYNTAX: \nsc config <service> <option1> <option2>...\n");
    printf("CONFIG OPTIONS:\n");
    printf("NOTE: The option name includes the equal sign.\n"
        " type= <own|share|kernel|filesys|rec|adapt|error>\n"
        " start= <boot|system|auto|demand|disabled|error>\n"
        " error= <normal|severe|critical|error|ignore>\n"
        " binPath= <BinaryPathName>\n"
        " group= <LoadOrderGroup>\n"
        " tag= <yes|no>\n"
        " depend= <Dependencies(space seperated)>\n"
        " obj= <AccountName|ObjectName>\n"
        " DisplayName= <display name>\n"
        " password= <password> \n");
}
VOID
CreateUsage(VOID)
{
    printf("Creates a service entry in the registry and Service Database.\n");
    printf("SYNTAX: \nsc create <service> <option1> <option2>...\n");
    printf("CREATE OPTIONS:\n");
    printf("NOTE: The option name includes the equal sign.\n"
        " type= <own|share|kernel|filesys|rec|error>\n"
        " start= <boot|system|auto|demand|disabled|error>\n"
        " error= <normal|severe|critical|error|ignore>\n"
        " binPath= <BinaryPathName>\n"
        " group= <LoadOrderGroup>\n"
        " tag= <yes|no>\n"
        " depend= <Dependencies(space seperated)>\n"
        " obj= <AccountName|ObjectName>\n"
        " DisplayName= <display name>\n"
        " password= <password> \n");
}

VOID
ReadLogUsage(VOID)
{

    printf("\nReads a logfile and dumps the contents.\n");
    printf("SYNTAX: \n  eltest readlog <ServerName> logfile <option1> <option2>...\n");
    printf("READLOG OPTIONS:\n");
    printf("NOTE: The option name includes the equal sign.\n");

    printf("    ReadFlag=  <fwd|back|seek|seq> (default = fwd) \n"
           "    RecordNum=  record number where read should start (default=0)\n"
           "    BufSize=    size of the buffer (default = 10000)\n");
    printf("EXAMPLE:\n   eltest addreg application myapp MsgFile= MyMsgs.dll"
           " Type= error Type= warning\n");
}

DWORD
AddSourceToRegistry(
    IN  LPTSTR  ServerName,
    IN  LPTSTR  LogName,
    IN  LPTSTR  EventSourceName,
    IN  LPTSTR  *argv,
    IN  DWORD   argc
    )

/*++

Routine Description:

    This function writes to the registry all the information to register
    this application as an event source.

Arguments:


Return Value:


--*/
{
    TCHAR   tempName[MAX_PATH];
    HKEY    hKey;
    DWORD   dwStatus=NO_ERROR;
    HKEY    hRegistry=HKEY_LOCAL_MACHINE;

    LPTSTR  EventMessageFile=NULL;
    LPTSTR  CategoryMessageFile=NULL;
    LPTSTR  ParameterMessageFile=NULL;
    DWORD   dwTypes=0;
    DWORD   dwCategoryCount=0;
    DWORD   i;

    //
    // Look at the LogName, and generate the appropriate registry key
    // path for that log.
    //
    if (STRICMP(LogName, ApplLogName) == 0) {
        STRCPY(tempName, ApplLogRegName);
    }
    else if (STRICMP(LogName, SysLogName) == 0) {
        STRCPY(tempName, SysLogRegName);
    }
    else if (STRICMP(LogName, SecLogName) == 0) {
        STRCPY(tempName, SecLogRegName);
    }
    else {
        printf("AddSourceToRegistry: Invalid LogName\n");
        return(ERROR_INVALID_PARAMETER);
    }
    STRCAT(tempName, EventSourceName);


    //
    // Get Variable Arguments
    //
    for (i=0; i<argc ;i++ ) {
        if (STRICMP(argv[i], TEXT("EventMsgFile=")) == 0) {
            EventMessageFile = argv[i+1];
            i++;
        }
        if (STRICMP(argv[i], TEXT("CategoryMsgFile=")) == 0) {
            CategoryMessageFile = argv[i+1];
            i++;
        }
        if (STRICMP(argv[i], TEXT("ParameterMsgFile=")) == 0) {
            ParameterMessageFile = argv[i+1];
            i++;
        }
        if (STRICMP(argv[i], TEXT("Type=")) == 0) {
            //--------------------------------------------------------
            // We want to allow for several arguments of type= in the
            // same line.  These should cause the different arguments
            // to be or'd together.
            //--------------------------------------------------------
            if (STRICMP(argv[i+1],TEXT("error")) == 0) {
                dwTypes |= EVENTLOG_ERROR_TYPE;
            }
            if (STRICMP(argv[i+1],TEXT("warning")) == 0) {
                dwTypes |= EVENTLOG_WARNING_TYPE;
            }
            if (STRICMP(argv[i+1],TEXT("information")) == 0) {
                dwTypes |= EVENTLOG_INFORMATION_TYPE;
            }
            if (STRICMP(argv[i+1],TEXT("AuditSuccess")) == 0) {
                dwTypes |= EVENTLOG_AUDIT_SUCCESS;
            }
            if (STRICMP(argv[i+1],TEXT("AuditFailure")) == 0) {
                dwTypes |= EVENTLOG_AUDIT_FAILURE;
            }
            if (STRICMP(argv[i+1],TEXT("All")) == 0) {
                dwTypes |= (EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE |
                            EVENTLOG_INFORMATION_TYPE | EVENTLOG_AUDIT_SUCCESS |
                            EVENTLOG_AUDIT_FAILURE);
            }
            else {
                printf("Invalid Type\n");
                AddRegUsage();
                return(ERROR_INVALID_PARAMETER);
            }
            i++;
        }
        if (STRICMP(argv[i], TEXT("CategoryCount=")) == 0) {
            dwCategoryCount = ATOL(argv[i+1]);
            i++;
        }

    }

    //
    // Connect to the registry on the correct machine.
    //
    printf("Connect to Registry\n");
    dwStatus = RegConnectRegistry(ServerName, HKEY_LOCAL_MACHINE, &hRegistry);
    if (dwStatus != NO_ERROR) {
        printf("RegConnectRegistry Failed %d\n",GetLastError());
        return(dwStatus);
    }

    //
    // Create the new key for this source
    //
    printf("Create Key\n");
    dwStatus = RegCreateKey(hRegistry, tempName, &hKey);
    if (dwStatus != ERROR_SUCCESS) {
        printf("Couldn't create Source Key in registry %d\n",dwStatus);
        return(dwStatus);
    }
    if (EventMessageFile != NULL) {
        printf("Set EventMessageFile\n");
        dwStatus = RegSetValueEx(
                hKey,
                VALUE_EVENT_MF,
                0,
                REG_EXPAND_SZ,
                (LPBYTE)EventMessageFile,
                STRLEN(EventMessageFile) + sizeof(TCHAR));

        if (dwStatus != ERROR_SUCCESS) {
            printf("RegSetValue (messageFile) failed %d\n",GetLastError());
            goto CleanExit;
        }
    }
    //
    // Set the Category Message File
    //
    if (CategoryMessageFile != NULL) {
        printf("Set Category Message File\n");
        dwStatus = RegSetValueEx(
                hKey,
                VALUE_CATEGORY_MF,
                0,
                REG_EXPAND_SZ,
                (LPBYTE)CategoryMessageFile,
                STRLEN(CategoryMessageFile) + sizeof(TCHAR));

        if (dwStatus != ERROR_SUCCESS) {
            printf("RegSetValue (category mf) failed %d\n",GetLastError());
            goto CleanExit;
        }
    }

    //
    // Set the Parameter Message File
    //
    if (ParameterMessageFile != NULL) {
        printf("Set Parameter Message File\n");
        dwStatus = RegSetValueEx(
                hKey,
                VALUE_PARAMETER_MF,
                0,
                REG_EXPAND_SZ,
                (LPBYTE)ParameterMessageFile,
                STRLEN(ParameterMessageFile) + sizeof(TCHAR));

        if (dwStatus != ERROR_SUCCESS) {
            printf("RegSetValue (Parameter mf) failed %d\n",GetLastError());
            goto CleanExit;
        }
    }

    //
    // Set the Types Supported
    //
    if (dwTypes != 0) {
        printf("Set Types Supported\n");
        dwStatus = RegSetValueEx(
                hKey,
                VALUE_TYPES_SUPPORTED,
                0,
                REG_DWORD,
                (LPBYTE) &dwTypes,
                sizeof(DWORD));

        if (dwStatus != ERROR_SUCCESS) {
            printf("RegSetValue (TypesSupported) failed %d\n",GetLastError());
            goto CleanExit;
        }

    }

    //
    // Set the Category Count
    //
    if (dwCategoryCount != 0) {
        printf("Set CategoryCount\n");
        dwStatus = RegSetValueEx(
                hKey,
                VALUE_CATEGORY_COUNT,
                0,
                REG_DWORD,
                (LPBYTE) &dwCategoryCount,
                sizeof(DWORD));

        if (dwStatus != ERROR_SUCCESS) {
            printf("RegSetValue (CategoryCount) failed %d\n",GetLastError());
            goto CleanExit;
        }
    }
    dwStatus = NO_ERROR;
CleanExit:
    RegCloseKey(hKey);
    RegCloseKey(hRegistry);
    return(dwStatus);
}

DWORD
DelSourceInRegistry(
    IN  LPTSTR  ServerName,
    IN  LPTSTR  LogName,
    IN  LPTSTR  EventSourceName
    )

/*++

Routine Description:

    This function writes to the registry all the information to register
    this application as an event source.

Arguments:


Return Value:


--*/
{
    LPTSTR  tempName;
    HKEY    hParentKey;
    BOOL    status=FALSE;
    DWORD   dwStatus;
    HKEY    hRegistry=HKEY_LOCAL_MACHINE;


    //
    // Look at the LogName, and generate the appropriate registry key
    // path for that log.
    //
    if (STRICMP(LogName, ApplLogName) == 0) {
        tempName = ApplLogRegName;
    }
    else if (STRICMP(LogName, SysLogName) == 0) {
        tempName = SysLogRegName;
    }
    else if (STRICMP(LogName, SecLogName) == 0) {
        tempName = SecLogRegName;
    }
    else {
        printf("AddSourceToRegistry: Invalid LogName\n");
        return(ERROR_INVALID_PARAMETER);
    }

    //
    // Connect to the registry on the correct machine.
    //
    dwStatus = RegConnectRegistry(ServerName, HKEY_LOCAL_MACHINE, &hRegistry);
    if (dwStatus != NO_ERROR) {
        printf("RegConnectRegistry Failed %d\n",GetLastError());
        return(status);
    }

    //
    // Open the Parent Key of the key we want to delete.
    //
    dwStatus = RegOpenKeyEx(
                hRegistry,
                tempName,
                0,
                KEY_ALL_ACCESS,
                &hParentKey);

    if (dwStatus != ERROR_SUCCESS) {
        printf("Couldn't open Parent of key to be deleted. %d\n",dwStatus);
        goto CleanExit;
    }
    //
    // Delete the subkey.
    //
    dwStatus = RegDeleteKey(hParentKey, EventSourceName);
    if (dwStatus != ERROR_SUCCESS) {
        printf("Couldn't delete "FORMAT_LPTSTR" key from registry %d\n",
            EventSourceName, dwStatus);
    }

    RegCloseKey(hParentKey);
CleanExit:
    RegCloseKey(hRegistry);
    return(status);
}

DWORD
ReadLogFile(
    LPTSTR  ServerName,
    LPTSTR  LogName,
    IN  LPTSTR  *argv,
    IN  DWORD   argc
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    DWORD   dwReadFlag = EVENTLOG_FORWARDS_READ;
    DWORD   dwRecordNum = 0;
    DWORD   BufSize = 10000;
    DWORD   numBytesRead;
    DWORD   numBytesReqd;
    LPVOID  pElBuffer = NULL;
    PEVENTLOGRECORD    pElRecord;
    BOOL    PrintTheHeader;
    DWORD   i;
    HANDLE  hEventLog=NULL;

    //
    // Get Variable Arguments
    //
    for (i=0; i<argc ;i++ ) {
        if (STRICMP(argv[i], TEXT("ReadFlag=")) == 0) {
            if (STRICMP(argv[i+1],TEXT("fwd")) == 0) {
                dwReadFlag |= EVENTLOG_FORWARDS_READ;
            }
            if (STRICMP(argv[i+1],TEXT("back")) == 0) {
                dwReadFlag |= EVENTLOG_BACKWARDS_READ;
            }
            if (STRICMP(argv[i+1],TEXT("seek")) == 0) {
                dwReadFlag |= EVENTLOG_SEEK_READ;
            }
            if (STRICMP(argv[i+1],TEXT("seq")) == 0) {
                dwReadFlag |= EVENTLOG_SEQUENTIAL_READ;
            }
            i++;
        }
        if (STRICMP(argv[i], TEXT("RecordNum=")) == 0) {
            dwRecordNum = ATOL(argv[i+1]);
            i++;
        }
        if (STRICMP(argv[i], TEXT("BufSize=")) == 0) {
            BufSize = ATOL(argv[i+1]);
            i++;
        }
        hEventLog = OpenEventLog(ServerName,LogName);
        if (hEventLog == NULL) {
            printf("OpenEventLog failed %d\n",GetLastError());
            return(0);
        }
        pElBuffer = LocalAlloc(LPTR, BufSize);
        if (pElBuffer == NULL) {
            printf("ReadLogFile: LocalAlloc Failed %d\n",GetLastError());
            goto CleanExit;
        }

        //---------------------------------------------------------
        // Read and Display the contents of the eventlog
        //---------------------------------------------------------
        PrintTheHeader = TRUE;
TryAgain:
        while(ReadEventLog(
                hEventLog,
                dwReadFlag,
                dwRecordNum,
                pElBuffer,
                BufSize,
                &numBytesRead,
                &numBytesReqd)) {

            pElRecord = (PEVENTLOGRECORD) pElBuffer;
            while ((PBYTE) pElRecord < (PBYTE) pElBuffer + numBytesRead) {
                //
                // Print the record to the display
                //
                DisplayRecord(pElRecord,PrintTheHeader);
                PrintTheHeader = FALSE;
                //
                // Move to the next event in the buffer
                //
                pElRecord = (PEVENTLOGRECORD)((PBYTE) pElRecord +
                    pElRecord->Length);
            }
        }
        switch(GetLastError()) {
        case ERROR_INSUFFICIENT_BUFFER:
            //
            // Increase the size of the buffer and try again
            //
            if (numBytesReqd > BufSize) {
                LocalFree(pElBuffer);
                BufSize = numBytesReqd;
                pElBuffer = LocalAlloc(LPTR, BufSize);
                if (!pElBuffer) {
                    printf("ReadLogFile: LocalAlloc Failed %d\n",GetLastError());
                }
                goto TryAgain;
            }
            else {
                printf("ReadLogFile #1: THIS SHOULD NEVER HAPPEN\n");
            }
            break;
        case ERROR_EVENTLOG_FILE_CHANGED:
            //
            // The current read position for this handle has been overwritten.
            // Reopen the file and print a message to the effect that some
            // records may have been missed.
            //
            printf("ReadLogFile: Current Read position has been overwritten\n");

            hEventLog = OpenEventLog(ServerName,LogName);
            if (hEventLog == NULL) {
                printf("OpenEventLog failed %d\n",GetLastError());
                goto CleanExit;
            }
            goto TryAgain;
        case ERROR_HANDLE_EOF:
            printf("EOF\n");
            break;
        default:
            printf("UnknownError: %d\n",GetLastError());
            break;
        }
    }
CleanExit:
    if (pElBuffer != NULL) {
        LocalFree(pElBuffer);
    }
    if (hEventLog != NULL) {
        CloseEventLog(hEventLog);
    }
    return(0);
}

VOID
DisplayRecord(
    PEVENTLOGRECORD     pElRecord,
    BOOL                PrintTheHeader
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    if (PrintTheHeader) {
        printf("RecNum/tTimeGen/tWriteTime/tEventID/tType/tNumStr/tCat/n");
    }
    printf("%d/t%d/t%d/t%d/t%d/t%d/t%d\n",
        pElRecord->RecordNumber,
        pElRecord->TimeGenerated,
        pElRecord->TimeWritten,
        pElRecord->EventID,
        pElRecord->EventType,
        pElRecord->NumStrings,
        pElRecord->EventCategory);
}

LONG
wtol(
    IN LPWSTR string
    )
{
    LONG value = 0;

    while((*string != L'\0')  &&
            (*string >= L'0') &&
            ( *string <= L'9')) {
        value = value * 10 + (*string - L'0');
        string++;
    }

    return(value);
}

