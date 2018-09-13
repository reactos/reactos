/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    os2ssmig.c

Abstract:

    This module contains the code necessary to initialize the
    OS/2 SS entries in the registry.  It migrates information
    from os/2's config.sys file to the NT registry.

Author:

    Ofer Porat (oferp) 30-Mar-1993

Environment:

    User Mode only

Revision History:

    The code was originally in the os/2 subsystem, but was moved
    to winlogon so the registry initialization can be done during
    boot time.  This way the registry info will be correct for the
    first os/2 program to run.

--*/

#include "precomp.h"
#pragma hdrstop

#define IF_OS2_DEBUG(x)                 // permanently enables KdPrint()s
static PVOID Os2Heap;                   // private heap used for allocs

#define PATHLIST_MAX        1024        // max length of pathlists such as Os2LibPath (in characters)
#define MAX_CONSYS_SIZE     16384       // max size of config.sys buffers (in bytes)
#define DOS_DEV_LEN         12          // length of "\\DosDevices\\"

//
// The following constants define the meaning of the Flags value in
// the config sys key entry:
// bit CONFSYSON_DISK -- 1 = os/2 config.sys on drive c, 0 = no os/2 config.sys on drive c
// bit DISABLEMIGTRATION -- if 1, migration from os/2 config.sys to NT registry is disabled.
//

#define FLAGS_CONFIGSYSONDISK   0x1L
#define FLAGS_DISABLEMIGRATION  0x2L

static WCHAR Os2SoftwareDirectory[]  = L"\\REGISTRY\\MACHINE\\SOFTWARE\\Microsoft";
static WCHAR Os2ProductDirectory[]   = L"OS/2 Subsystem for NT";
static WCHAR Os2VersionDirectory[]   = L"1.0";
static WCHAR Os2IniName[]            = L"os2.ini";
static WCHAR Os2ConfigSysName[]      = L"config.sys";
static WCHAR Os2ConfigFlagName[]     = L"Flags";
static WCHAR Os2ConfigStampName[]     = L"FileTimeSizeStamp";
static WCHAR Os2Class[]              = L"OS2SS";

static CHAR  Os2ConfigSysDefaultValue_NEC_PC9800[] =
//
// The '\a' in the following strings will be replaced by the SystemDirectory Value
// The '\b' in the following strings will be replaced by the SystemRoot Value
//
"NTREM Here is a summary of what is allowed to appear in this registry entry:\0"
"NTREM   Comments starting with REM will be visible to the user when s/he opens\0"
"NTREM     \b:\\config.sys.\0"
"NTREM   Comments starting with NTREM are only visible by direct access to the\0"
"NTREM     registry.\0"
"NTREM   The following OS/2 configuration commands are significant:\0"
"NTREM     COUNTRY=\0"
"NTREM     CODEPAGE=\0"
"NTREM     DEVINFO=KBD,\0"
"NTREM   Any other commands apart from the exceptions listed below will be\0"
"NTREM   visible to an OS/2 program that opens \b:\\config.sys, however they are\0"
"NTREM   not used internally by the NT OS/2 SubSystem.\0"
"NTREM   Exceptions:\0"
"NTREM     The following commands are completely ignored.  Their true values\0"
"NTREM     appear in the system environment and should be modified using the\0"
"NTREM     Control Panel System applet.  Note that LIBPATH is called Os2LibPath\0"
"NTREM     in the NT system environment.\0"
"SET PATH=<ignored>\0"
"LIBPATH=<ignored>\0"
"NTREM     In addition, any \"SET=\" commands (except COMSPEC) will be\0"
"NTREM     completely ignored.  You should set OS/2 environment variables just\0"
"NTREM     like any other Windows NT environment variables by using the Control\0"
"NTREM     Panel System applet.\0"
"NTREM   If you have an OS/2 editor available, it is highly recommended that you\0"
"NTREM   modify NT OS/2 config.sys configuration by editing \b:\\config.sys with\0"
"NTREM   this editor.  This is the documented way to make such modification, and\0"
"NTREM   is therefore less error-prone.\0"
"NTREM   Now comes the actual text.\0"
"REM\0"
"REM This is a fake OS/2 config.sys file used by the NT OS/2 SubSystem.\0"
"REM The following information resides in the Registry and NOT in a disk file.\0"
"REM OS/2 Apps that access \b:\\config.sys actually manipulate this information.\0"
"REM\0"
"PROTSHELL=\b:\\os2\\pmshell.exe \b:\\os2\\os2.ini \b:\\os2\\os2sys.ini \a\\cmd.exe\0"
"SET COMSPEC=\a\\cmd.exe\0"
;


static CHAR  Os2ConfigSysDefaultValue[] =
//
// The '\a' in the following strings will be replaced by the SystemDirectory Value
//
"NTREM Here is a summary of what is allowed to appear in this registry entry:\0"
"NTREM   Comments starting with REM will be visible to the user when s/he opens\0"
"NTREM     c:\\config.sys.\0"
"NTREM   Comments starting with NTREM are only visible by direct access to the\0"
"NTREM     registry.\0"
"NTREM   The following OS/2 configuration commands are significant:\0"
"NTREM     COUNTRY=\0"
"NTREM     CODEPAGE=\0"
"NTREM     DEVINFO=KBD,\0"
"NTREM   Any other commands apart from the exceptions listed below will be\0"
"NTREM   visible to an OS/2 program that opens c:\\config.sys, however they are\0"
"NTREM   not used internally by the NT OS/2 SubSystem.\0"
"NTREM   Exceptions:\0"
"NTREM     The following commands are completely ignored.  Their true values\0"
"NTREM     appear in the system environment and should be modified using the\0"
"NTREM     Control Panel System applet.  Note that LIBPATH is called Os2LibPath\0"
"NTREM     in the NT system environment.\0"
"SET PATH=<ignored>\0"
"LIBPATH=<ignored>\0"
"NTREM     In addition, any \"SET=\" commands (except COMSPEC) will be\0"
"NTREM     completely ignored.  You should set OS/2 environment variables just\0"
"NTREM     like any other Windows NT environment variables by using the Control\0"
"NTREM     Panel System applet.\0"
"NTREM   If you have an OS/2 editor available, it is highly recommended that you\0"
"NTREM   modify NT OS/2 config.sys configuration by editing c:\\config.sys with\0"
"NTREM   this editor.  This is the documented way to make such modification, and\0"
"NTREM   is therefore less error-prone.\0"
"NTREM   Now comes the actual text.\0"
"REM\0"
"REM This is a fake OS/2 config.sys file used by the NT OS/2 SubSystem.\0"
"REM The following information resides in the Registry and NOT in a disk file.\0"
"REM OS/2 Apps that access c:\\config.sys actually manipulate this information.\0"
"REM\0"
"PROTSHELL=c:\\os2\\pmshell.exe c:\\os2\\os2.ini c:\\os2\\os2sys.ini \a\\cmd.exe\0"
"SET COMSPEC=\a\\cmd.exe\0"
;

static WCHAR Os2ConfigSysKeyName[] =
L"\\REGISTRY\\MACHINE\\SOFTWARE\\Microsoft\\OS/2 Subsystem for NT\\1.0\\config.sys";
static WCHAR Os2EnvironmentDirectory[] =
L"\\REGISTRY\\MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment";
static HANDLE Os2EnvironmentKeyHandle = NULL;

static WCHAR Os2OriginalCanonicalConfigSys_NEC_PC9800[] = L"\\DosDevices\\A:\\CONFIG.SYS";
static WCHAR Os2OriginalCanonicalConfigSys[] = L"\\DosDevices\\C:\\CONFIG.SYS";

static BOOLEAN Os2LibPathFound = FALSE;
static WCHAR Os2LibPathValueName[] = L"Os2LibPath";
static UNICODE_STRING Os2LibPathValueData_U;

static PWSTR Os2PathString = NULL;

static PWSTR pOs2ConfigSys = NULL;
static ULONG Os2SizeOfConfigSys = 0;
static PWSTR pOs2UpperCaseConfigSys = NULL;

static WCHAR Os2SystemDirectory[MAX_PATH];

BOOLEAN GetSystemRootDrive(IN LPTCH lpDrive);


VOID
Os2RegSetFlags(
    IN HANDLE ConfigSysKeyHandle,
    IN ULONG FlagsValue
    )

/*++

Routine Description:

    Write the "Flags" value into the config.sys registry key.

Arguments:

    ConfigSysKeyHandle -- supplies read/write handle to config.sys reg key
    FlagsValue -- supplies the value to write

Return Value:

    None.

Note:

    Errors are ignored

--*/

{
    UNICODE_STRING String_U;
    NTSTATUS Status;

    RtlInitUnicodeString(&String_U, Os2ConfigFlagName);

    Status = NtSetValueKey(
                ConfigSysKeyHandle,
                &String_U,
                0L,
                REG_DWORD,
                &FlagsValue,
                sizeof(DWORD)
                );

    if (!NT_SUCCESS(Status)) {
#if DBG
        KdPrint(("Os2RegSetFlags: NetSetValueKey failed, Status = %lx\n", Status));
#endif
    }

    // ignore errors
}


VOID
Os2RegSetStamp(
    IN HANDLE ConfigSysKeyHandle,
    IN PLARGE_INTEGER pTimeStamp,
    IN PLARGE_INTEGER pSizeStamp
    )

/*++

Routine Description:

    Write the "FileTimeSizeStamp" value into the config.sys registry key.

Arguments:

    ConfigSysKeyHandle -- supplies read/write handle to config.sys reg key
    pTimeStamp, pSizeStamp -- supplies the value to write

Return Value:

    None.

Note:

    Errors are ignored

--*/

{
    UNICODE_STRING String_U;
    ULONG Buffer[4];
    NTSTATUS Status;

    Buffer[0] = pTimeStamp->LowPart;
    Buffer[1] = (ULONG) pTimeStamp->HighPart;
    Buffer[2] = pSizeStamp->LowPart;
    Buffer[3] = (ULONG) pSizeStamp->HighPart;

    RtlInitUnicodeString(&String_U, Os2ConfigStampName);

    Status = NtSetValueKey(
                ConfigSysKeyHandle,
                &String_U,
                0L,
                REG_BINARY,
                (PVOID) Buffer,
                sizeof(Buffer)
                );

    if (!NT_SUCCESS(Status)) {
#if DBG
        KdPrint(("Os2RegSetFlags: NetSetValueKey failed, Status = %lx\n", Status));
#endif
    }

    // ignore errors
}


BOOLEAN
Os2InitOriginalConfigSysProcessing(
    IN HANDLE ConfigSysKeyHandle,
    IN ULONG FlagsValue,
    IN HANDLE ConfigSysFileHandle,
    IN PLARGE_INTEGER pTimeStamp,
    IN PLARGE_INTEGER pSizeStamp
    )

/*++

Routine Description:

    This function checks if there is an OS/2 config.sys configuration file on the
    disk.  If so, it opens it and reads it in.  The file is converted to UNICODE,
    and an upper case copy of it is made.

Arguments:

    ConfigSysKeyHandle -- supplies a read/write handle to the config.sys reg key
        this is used to update the flags/stamp values
    FlagsValue -- The previously existing "Flags" value that was read in from the
        registry.  If this is -1, then no previous Flags value existed.
    ConfigSysFileHandle -- An optional handle to an already open config.sys file
    pTimeStamp, pSizeStamp -- info about the open config.sys file.

    Note:  The parameters ConfigSysFileHandle + pTimeStamp + pSizeStamp are optional,
        and come as a packet.  If the config.sys file was previously opened, the handle
        will be non-null, and all 3 params must be valid.  If the config.sys file was
        not previously opened (successfully), the handle should be NULL, and the other
        2 params need not be valid.

Return Value:

    TRUE if there's an OS/2 config.sys, and all the operations needed to prepare
    it have succeeded.  FALSE otherwise.

Notes:
    On success Sets global variables as follows:
        pOs2ConfigSys - a null-terminated UNICODE copy of the OS/2 config.sys.
        pOs2UpperCaseConfigSys - an upper case copy of pOs2ConfigSys.
        Os2SizeOfConfigSys - the number of characters in the above strings.

    ConfigSysFileHandle is closed before the function returns.

    The flags/stamp values in the config.sys reg key are updated if necessary.

--*/

{
    UNICODE_STRING CanonicalConfigDotSys_U;
    UNICODE_STRING Tmp_U;
    ANSI_STRING Tmp_MB;
    OBJECT_ATTRIBUTES Obja;
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatus;
    PSZ pTempOs2ConfigSys;
    LARGE_INTEGER TimeStamp, SizeStamp;

    if (FlagsValue != (ULONG)-1) {

        if (FlagsValue & FLAGS_DISABLEMIGRATION) {

            //
            // ConfigSysFileHandle cannot be non-NULL at this point
            // so there's no need to close it.  Also, no need to
            // update flags/stamp.
            //

            return(FALSE);
        }

        FlagsValue &= ~FLAGS_CONFIGSYSONDISK;

    } else {
        FlagsValue = 0;

        //
        // 1st time migration will always take place (after system setup)
        // if you you want to disable post 1st-time migration, set FlagsValue
        // to a default of FLAGS_DISABLEMIGRATION.  If you don't any migration
        // at all (not even 1st-time), then set the FlagsValue similarly, and
        // also do a { Os2RegSetFlags(..), return FALSE }
        //
    }

    if (ConfigSysFileHandle == NULL) {

        // Try opening OS/2's config.sys file

        if (IsNEC_98) {
            TCHAR chTemp;
            if (GetSystemRootDrive(&chTemp)) {
                wsprintf(Os2OriginalCanonicalConfigSys_NEC_PC9800,
                        L"\\DosDevices\\%c:\\CONFIG.SYS",
                        chTemp);
            }
            RtlInitUnicodeString(&CanonicalConfigDotSys_U, Os2OriginalCanonicalConfigSys_NEC_PC9800);
        } else 
            RtlInitUnicodeString(&CanonicalConfigDotSys_U, Os2OriginalCanonicalConfigSys);

        InitializeObjectAttributes(&Obja,
                       &CanonicalConfigDotSys_U,
                       OBJ_CASE_INSENSITIVE,
                       NULL,
                       NULL);

        Status = NtOpenFile(&ConfigSysFileHandle,
                            FILE_GENERIC_READ,
                            &Obja,
                            &IoStatus,
                            FILE_SHARE_READ,
                            FILE_SYNCHRONOUS_IO_NONALERT
                           );

        if (!NT_SUCCESS(Status)) {

            //
            // Debug print removed, because this is a valid situation (no config.sys file)
            //
#if 0
#if DBG
            KdPrint(("Os2InitOriginalConfigSysProcessing: FAILED - NtOpenFile-1 %lx\n", Status));
#endif
#endif
            Os2RegSetFlags(ConfigSysKeyHandle, FlagsValue);
            return (FALSE);
        }

        Status = Or2GetFileStamps(
                        ConfigSysFileHandle,
                        &TimeStamp,
                        &SizeStamp
                        );

        if (!NT_SUCCESS(Status)) {
#if DBG
            KdPrint(("Os2InitOriginalConfigSysProcessing: Can't get config.sys time/size stamp, Status = %lx\n", Status));
#endif

            NtClose(ConfigSysFileHandle);
            Os2RegSetFlags(ConfigSysKeyHandle, FlagsValue);
            return(FALSE);
        }

        pTimeStamp = &TimeStamp;
        pSizeStamp = &SizeStamp;
    }

    // Get the file length

    Os2SizeOfConfigSys = pSizeStamp->LowPart;

    if ( Os2SizeOfConfigSys > ( 0x7ffe / sizeof( WCHAR ) ) )
    {
        KdPrint(("Os2InitOriginalConfigSysProcessing:  File too big!\n" ));

        NtClose(ConfigSysFileHandle);

        Os2RegSetFlags(ConfigSysKeyHandle, FlagsValue);

        return(FALSE);
    }

    // Allocate space for reading it in

    // the + 1 in following parameter is for inserting the NUL character

    pTempOs2ConfigSys = (PSZ) RtlAllocateHeap(Os2Heap, 0, Os2SizeOfConfigSys + 1);

    if (pTempOs2ConfigSys == NULL) {
#if DBG
        KdPrint(("Os2InitOriginalConfigSysProcessing: FAILED - RtlAllocateHeap pTempOs2ConfigSys\n"));
#endif
        Os2SizeOfConfigSys = 0;
        NtClose(ConfigSysFileHandle);
        Os2RegSetFlags(ConfigSysKeyHandle, FlagsValue);
        return (FALSE);
    }

    pOs2ConfigSys = (PWSTR) RtlAllocateHeap(Os2Heap, 0, (Os2SizeOfConfigSys + 1) * sizeof(WCHAR));

    if (pOs2ConfigSys == NULL) {
#if DBG
        KdPrint(("Os2InitOriginalConfigSysProcessing: FAILED - RtlAllocateHeap pOs2ConfigSys\n"));
#endif
        Os2SizeOfConfigSys = 0;
        RtlFreeHeap(Os2Heap, 0, pTempOs2ConfigSys);
        NtClose(ConfigSysFileHandle);
        Os2RegSetFlags(ConfigSysKeyHandle, FlagsValue);
        return (FALSE);
    }

    // Read it in

    Status = NtReadFile(ConfigSysFileHandle,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatus,
                         (PVOID)pTempOs2ConfigSys,
                         Os2SizeOfConfigSys,
                         NULL,
                         NULL
                        );
    NtClose(ConfigSysFileHandle);
    if (!NT_SUCCESS(Status)) {
#if DBG
        KdPrint(("Os2InitOriginalConfigSysProcessing: FAILED - NtReadFile %lx\n", Status));
#endif
        Os2SizeOfConfigSys = 0;
        RtlFreeHeap(Os2Heap, 0, pTempOs2ConfigSys);
        RtlFreeHeap(Os2Heap, 0, pOs2ConfigSys);
        pOs2ConfigSys = NULL;
        Os2RegSetFlags(ConfigSysKeyHandle, FlagsValue);
        return (FALSE);
    }

    pTempOs2ConfigSys[Os2SizeOfConfigSys] = '\0';

    // Convert to UNICODE

    RtlInitAnsiString(&Tmp_MB, pTempOs2ConfigSys);

    Tmp_U.Buffer = pOs2ConfigSys;
    Tmp_U.MaximumLength = (USHORT) ((Os2SizeOfConfigSys + 1) * sizeof(WCHAR));

    RtlOemStringToUnicodeString(&Tmp_U, (POEM_STRING)&Tmp_MB ,FALSE);

    Os2SizeOfConfigSys = Tmp_U.Length / sizeof(WCHAR);

    pOs2ConfigSys[Os2SizeOfConfigSys] = UNICODE_NULL;

    RtlFreeHeap(Os2Heap, 0, pTempOs2ConfigSys);

    // Prepare the upper case copy

    pOs2UpperCaseConfigSys = (PWSTR) RtlAllocateHeap(Os2Heap, 0, (Os2SizeOfConfigSys + 1) * sizeof(WCHAR));
    if (pOs2UpperCaseConfigSys == NULL) {
#if DBG
        KdPrint(("Os2InitOriginalConfigSysProcessing: FAILED - RtlAllocateHeap pOs2UpperConfigSys\n"));
#endif
        Os2SizeOfConfigSys = 0;
        RtlFreeHeap(Os2Heap, 0, pOs2ConfigSys);
        pOs2ConfigSys = NULL;
        Os2RegSetFlags(ConfigSysKeyHandle, FlagsValue);
        return (FALSE);
    }

    wcscpy(pOs2UpperCaseConfigSys, pOs2ConfigSys);
    Or2UnicodeStrupr(pOs2UpperCaseConfigSys);

    //
    // Verify that the CONFIG.SYS file is really an OS/2 file and not a
    // DOS file.
    // Look for certain strings that MUST appear in an OS/2 CONFIG.SYS
    // file and don't appear in DOS CONFIG.SYS files
    //

    if ((wcsstr(pOs2UpperCaseConfigSys, L"LIBPATH") == NULL) ||
        (wcsstr(pOs2UpperCaseConfigSys, L"PROTSHELL") == NULL) ||
        (wcsstr(pOs2UpperCaseConfigSys, L"PROTECTONLY") == NULL)
       ) {
        Os2SizeOfConfigSys = 0;
        RtlFreeHeap(Os2Heap, 0, pOs2ConfigSys);
        RtlFreeHeap(Os2Heap, 0, pOs2UpperCaseConfigSys);
        pOs2UpperCaseConfigSys = pOs2ConfigSys = NULL;
        Os2RegSetFlags(ConfigSysKeyHandle, FlagsValue);
        return (FALSE);
    }

    FlagsValue |= FLAGS_CONFIGSYSONDISK;

    Os2RegSetFlags(ConfigSysKeyHandle, FlagsValue);
    Os2RegSetStamp(ConfigSysKeyHandle, pTimeStamp, pSizeStamp);

    return (TRUE);
}


VOID
Os2SetDirectiveProcessingDispatchFunction(
    IN ULONG DispatchTableIndex,
    IN PVOID UserParameter,
    IN PWSTR Name,
    IN ULONG NameLen,
    IN PWSTR Value,
    IN ULONG ValueLen
    )

/*++

Routine Description:

    This is a Dispatch Routine that is used to process SET directives in OS/2's config.sys
    file.  The directives are entered into the system environment.

Arguments:

    Standard arguments passed to a Dispatch Function, see the description of
    Or2IterateEnvironment in os2ssrtl.c

    UserParameter - points to a pointer which indicates the current position in the
        config.sys registry entry we're building.

Return Value:

    None.

--*/

{

    UNICODE_STRING  VarName_U;      // for setting up variable name
    UNICODE_STRING  VarValue_U;     // for setting up variable value
    PWSTR           Dest = *(PWSTR *) UserParameter;
    NTSTATUS        Status;
//  ULONG           ResultLength;
    WCHAR           wch;
//  KEY_VALUE_PARTIAL_INFORMATION KeyValueInfo;

    // Set up variable name

    VarName_U.Buffer = Value;
    VarName_U.Length = 0;

    while ((ValueLen > 0) && (*Value != L'=')) {
        Value++;
        VarName_U.Length += sizeof(WCHAR);
        ValueLen--;
    }

    if (ValueLen == 0 ||            // End of line reached without finding '='
        VarName_U.Length == 0) {    // Empty name
        return;
    }

    VarName_U.MaximumLength = VarName_U.Length;

    // Following SET directives are ignored

    if (Or2UnicodeEqualCI(VarName_U.Buffer, L"COMSPEC=", 8) ||
//      Or2UnicodeEqualCI(VarName_U.Buffer, L"PATH=", 5) ||
        Or2UnicodeEqualCI(VarName_U.Buffer, L"VIDEO_DEVICES=", 14) ||
        Or2UnicodeEqualCI(VarName_U.Buffer, L"VIO_IBMVGA=", 11) ||
        Or2UnicodeEqualCI(VarName_U.Buffer, L"VIO_VGA=", 8) ||
        Or2UnicodeEqualCI(VarName_U.Buffer, L"PROMPT=", 7)
       ) {
        return;
    }

    // Here, we have a valid name, followed by '='
    Value++;    // Skip the '='
    ValueLen--;

    // Set up variable value

    VarValue_U.Buffer = Value;
    VarValue_U.Length = (USHORT) (ValueLen * sizeof(WCHAR));  // what's left of the line
    VarValue_U.MaximumLength = VarValue_U.Length;

    //
    // Update the information in the registry with the info
    // in the file
    //

#if 0
    // not in effect anymore
    //
    // The KEYS variable is handled in a special way.
    // It's put in the registry config.sys in order to prevent possible
    // conflict.
    //

    if (Or2UnicodeEqualCI(VarName_U.Buffer, L"KEYS=", 5)) {
        RtlMoveMemory(Dest, L"SET ", 8);
        Dest += 4;
        RtlMoveMemory(Dest, VarName_U.Buffer, VarName_U.Length);
        Dest += VarName_U.Length / sizeof(WCHAR);
        *Dest++ = L'=';
        RtlMoveMemory(Dest, VarValue_U.Buffer, VarValue_U.Length);
        Dest += VarValue_U.Length / sizeof(WCHAR);
        *Dest++ = UNICODE_NULL;
        *(PWSTR *) UserParameter = Dest;
        return;
    }
#endif

    //
    // If Os2EnvironmentKeyHandle is NULL, we don't have
    // write access to the system environment, and we skip
    // setting the variable.
    //

    if (Os2EnvironmentKeyHandle == NULL) {
        return;
    }

    //
    // Special handling for PATH -- retained in the global variable
    // Os2PathString.  Only the last occurence of PATH is retained.
    // This is compatible with OS/2 which uses the last occurence.
    //

    if (Or2UnicodeEqualCI(VarName_U.Buffer, L"PATH=", 5)) {

        if (Os2PathString != NULL) {            // already found a path?

            RtlFreeHeap(Os2Heap, 0, Os2PathString);     // get rid of it
            Os2PathString = NULL;
        }

        Os2PathString = (PWSTR) RtlAllocateHeap(Os2Heap, 0, VarValue_U.Length + sizeof(WCHAR));

        if (Os2PathString == NULL) {
#if DBG
            KdPrint(("Os2SetDirectiveProcessingDispatchFunction: Failed to allocate heap space for PATH\n"));
#endif

            return;             // no space for storing the path -- skip it
        }

        // switch to the upper case copy...

        VarValue_U.Buffer = pOs2UpperCaseConfigSys + (VarValue_U.Buffer - pOs2ConfigSys);

        RtlMoveMemory(Os2PathString, VarValue_U.Buffer, VarValue_U.Length);
        Os2PathString[VarValue_U.Length/sizeof(WCHAR)] = UNICODE_NULL;

        return;
    }

    //
    // Otherwise, it's stored in the system environment
    //

    //
    // If a value key of the same name already exists,
    // don't replace it. This is done in order to prevent
    // the OS/2 SS from overriding possible NT definitions
    //

    //
    // !!! Modification 5/30/93 -- We decided to be a little more brave about this.
    // On one hand there doesn't seem to be any conflict between any possible OS/2
    // definitions, and NT definitions.  On the other hand, since we now do re-migration
    // on every config.sys change, if we don't update existing variables, the user's
    // important OS/2 variables (e.g. DPATH) will not get remigrated.  Therefore we
    // always update. -- oferp
    //

#if 0

    Status = NtQueryValueKey(Os2EnvironmentKeyHandle,
                             &VarName_U,
                             KeyValuePartialInformation,
                             &KeyValueInfo,
                             sizeof(KeyValueInfo),
                             &ResultLength
                            );
#else

    Status = STATUS_OBJECT_NAME_NOT_FOUND;

#endif


    if (Status == STATUS_OBJECT_NAME_NOT_FOUND) {

        //
        // Set the system wide variable to the value specified
        // in the original OS/2 config.sys
        //

        wch = Value[ValueLen];
        Value[ValueLen] = UNICODE_NULL;

        Status = NtSetValueKey(Os2EnvironmentKeyHandle,
                               &VarName_U,
                               (ULONG)0,
                               REG_EXPAND_SZ,
                               VarValue_U.Buffer,
                               VarValue_U.Length + sizeof(WCHAR)
                              );

        Value[ValueLen] = wch;

        if (!NT_SUCCESS(Status))
        {
#if DBG
            IF_OS2_DEBUG( INIT ) {
                KdPrint(("Os2SetDirectiveProcessingDispatchFunction: Unable to NtSetValueKey() system env, rc = %X\n",
                           Status));
            }
#endif
            return;
        }

    } else {
#if DBG
        if (Status != STATUS_BUFFER_OVERFLOW) {
            IF_OS2_DEBUG( INIT ) {
                KdPrint(("Os2SetDirectiveProcessingDispatchFunction: Unable to NtQueryValueKey() system env, rc = %X\n",
                           Status));
            }
        }
#endif
        return;
    }
}


VOID
Os2CommaProcessingDispatchFunction(
    IN ULONG DispatchTableIndex,
    IN PVOID UserParameter,
    IN PWSTR Name,
    IN ULONG NameLen,
    IN PWSTR Value,
    IN ULONG ValueLen
    )

/*++

Routine Description:

    This is a Dispatch Routine that is used to process certain directives in OS/2's config.sys
    file.  The directives are copied into the config.sys registry entry we're building.
    Some directives are truncated after a certain number of commas.

    !!! Modification 5/30/93 -- Migration of the three NLS related statements has been disabled.
        Reason:  The intended default behavior is this:  The registry config.sys entry has no
        NLS entries, and the OS/2 subsystem picks up these values from NT's default values.
        Supporting the registry config.sys values is only meant to be used if the user actually
        wants to have separate OS/2 and NT NLS parameters.  This is almost never the case.  The
        reason for allowing dual-NLS mode is because some NT languages may not be supported on
        OS/2, and so the user may want to specify a different (existing) NLS setting for the
        OS/2 subsystem.  The usual setting is therefore to have no NLS directives in the registry
        config.sys.  That's why we shouldn't migrate. -- oferp

Arguments:

    Standard arguments passed to a Dispatch Function, see the description of
    Or2IterateEnvironment in os2ssrtl.c

    UserParameter - points to a pointer which indicates the current position in the
        config.sys registry entry we're building.

Return Value:

    None.

--*/

{
//
// Lines passed to this dispatch function need to be copied to the output as are
// except they should be truncated after a certain number of commas.  The
// following table lists the number of commas.  0 means no truncation.
//

#if 0
    ULONG ItemTable[] = { 1,     // COUNTRY
                          0,     // CODEPAGE
                          2      // DEVINFO (only with KBD)
                        };
    PWSTR Dest = *(PWSTR *) UserParameter;
    ULONG CommaCtr;

    if (DispatchTableIndex== 2 && !Or2UnicodeEqualCI(Value, L"KBD,", 4)) {
        return;
    }

    // First, copy the Name

    RtlMoveMemory(Dest, Name, NameLen * sizeof(WCHAR));
    Dest += NameLen;
    *Dest++ = L'=';

    // Now, copy the value for the right number of commas

    CommaCtr = 0;

    while (ValueLen > 0) {
        if (ItemTable[DispatchTableIndex] != 0 && *Value == L',') {
            CommaCtr++;
            if (CommaCtr == ItemTable[DispatchTableIndex]) {
                break;
            }
        }

        *Dest++ = *Value++;
        ValueLen--;
    }
    *Dest++ = UNICODE_NULL;
    *(PWSTR *) UserParameter = Dest;

#else

    return;                 // do nothing!

#endif
}


VOID
Os2ProcessOriginalConfigSys(
    IN OUT PWSTR *DestPtr
    )

/*++

Routine Description:

    This function processes OS/2's config.sys file after it has been properly initialized
    by Os2InitOriginalConfigSysProcessing.

Arguments:

    DestPtr - points to a pointer which indicates the current position in the
        config.sys registry entry we're building.

Return Value:

    None.

--*/

{
    static ENVIRONMENT_DISPATCH_TABLE_ENTRY DispatchTable[] =
        {
            { L"COUNTRY", L"=", Os2CommaProcessingDispatchFunction, NULL },
            { L"CODEPAGE", L"=", Os2CommaProcessingDispatchFunction, NULL },
            { L"DEVINFO", L"=", Os2CommaProcessingDispatchFunction, NULL },
            { L"LIBPATH", L"=", Or2FillInSearchRecordDispatchFunction, NULL },
            { L"SET", L" \t", Os2SetDirectiveProcessingDispatchFunction, NULL }
        };
    ENVIRONMENT_SEARCH_RECORD LibPathRecord;
    ULONG i;
    PWSTR p;
    WCHAR ch;

    //
    // Most of the job is done by Or2IterateEnvironment which processes the file
    // according to a dispatch table.
    //
    // LibPath needs to be handled a little differently.  We need to process only
    // the *last* occurence of LIBPATH= in the file (this is the same as OS/2).
    // Therefore we only record the position of the LIBPATH= statments as we run
    // into them.  After we're finished we'll have the position of the last one,
    // and we can process that line.
    //

    for (i = 0; i < 5; i++) {
        DispatchTable[i].UserParameter = (PVOID) DestPtr;
    }

    DispatchTable[3].UserParameter = (PVOID)&LibPathRecord;
    LibPathRecord.DispatchTableIndex = (ULONG)-1;

    Or2IterateEnvironment(pOs2ConfigSys,
                          DispatchTable,
                          5,
                          CRLF_DELIM);


    if (Os2LibPathFound && LibPathRecord.DispatchTableIndex != (ULONG)-1) {

        // handle LIBPATH if there was one

        // get a pointer to the upper case version, so we can append it

        p = pOs2UpperCaseConfigSys + (LibPathRecord.Value - pOs2ConfigSys);

        ch = p[LibPathRecord.ValueLen];
        p[LibPathRecord.ValueLen] = UNICODE_NULL;

        Or2AppendPathToPath(Os2Heap,
                            p,
                            &Os2LibPathValueData_U,
                            TRUE);
        p[LibPathRecord.ValueLen] = ch;
    }
}


VOID
Os2TerminateOriginalConfigSysProcessing(
    VOID
    )

/*++

Routine Description:

    Cleans up processing of OS/2's config.sys.  Releases the storage used to store the
    file.

Arguments:

    None.

Return Value:

    None.

--*/

{
    RtlFreeHeap(Os2Heap, 0, pOs2ConfigSys);
    RtlFreeHeap(Os2Heap, 0, pOs2UpperCaseConfigSys);
    pOs2UpperCaseConfigSys = pOs2ConfigSys = NULL;
    Os2SizeOfConfigSys = 0;
}


NTSTATUS
Os2SbBuildSD(
    PSECURITY_DESCRIPTOR SecurityDescriptor,
    PACL *pDacl
    )

/*++

Routine Description:

    Builds a security descriptor for creating our registry keys.

    Administrators -- all access
    everyone -- read access

Arguments:

    SecurityDescriptor -- supplies a pointer to a preallocated SD that will be built
    pDacl -- returns a pointer to a dacl that should be released from Os2Heap after
             we're finished using the security descriptor.

Return Value:

    NT error code.

--*/

{
    PACL Dacl;
    PACE_HEADER Pace;
    PSID AdminAliasSid;
    ULONG DaclSize;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY WorldSidAuthority = SECURITY_WORLD_SID_AUTHORITY;
    PSID WorldSid;
    NTSTATUS Status;

    //
    // Create the SIDs for local admin and World.
    //

    Status = RtlAllocateAndInitializeSid(
                 &NtAuthority,
                 2,
                 SECURITY_BUILTIN_DOMAIN_RID,
                 DOMAIN_ALIAS_RID_ADMINS,
                 0, 0, 0, 0, 0, 0,
                 &AdminAliasSid
                 );

    if (!NT_SUCCESS(Status)) {
#if DBG
        IF_OS2_DEBUG( INIT ) {
            KdPrint(("Os2SbBuildSD: RtlAllocateAndInitializeSid(Admin), Status = %lx\n", Status));
        }
#endif
        return (Status);
    }

    Status = RtlAllocateAndInitializeSid(
                 &WorldSidAuthority,
                 1,
                 SECURITY_WORLD_RID,
                 0, 0, 0, 0, 0, 0, 0,
                 &WorldSid
                 );

    if (!NT_SUCCESS(Status)) {
#if DBG
        IF_OS2_DEBUG( INIT ) {
            KdPrint(("Os2SbBuildSD: RtlAllocateAndInitializeSid(World), Status = %lx\n", Status));
        }
#endif
        RtlFreeSid( AdminAliasSid );
        return (Status);
    }

    Status = RtlCreateSecurityDescriptor( SecurityDescriptor,
                                          SECURITY_DESCRIPTOR_REVISION );

    if (!NT_SUCCESS(Status)) {
#if DBG
        IF_OS2_DEBUG( INIT ) {
            KdPrint(("Os2SbBuildSD: RtlCreateSecurityDescriptor, Status = %lx\n", Status));
        }
#endif
        RtlFreeSid( AdminAliasSid );
        RtlFreeSid( WorldSid );
        return (Status);
    }


    //
    // Compute the size of the buffer needed for the
    // DACL.
    //

    DaclSize = sizeof( ACL ) +
               2 * (sizeof( ACCESS_ALLOWED_ACE ) - sizeof( ULONG ))
               +
               RtlLengthSid( AdminAliasSid ) +
               RtlLengthSid( WorldSid );

    Dacl = (PACL) RtlAllocateHeap(Os2Heap, 0, DaclSize);

    if (Dacl == NULL) {
#if DBG
        IF_OS2_DEBUG( INIT ) {
            KdPrint(("Os2SbBuildSD: RtlAllocateHeap failed\n"));
        }
#endif
        RtlFreeSid( AdminAliasSid );
        RtlFreeSid( WorldSid );
        return(STATUS_NO_MEMORY);
    }

    //
    // Build the ACL
    //

    Status = RtlCreateAcl ( Dacl, DaclSize, ACL_REVISION2 );

    if (!NT_SUCCESS(Status)) {
#if DBG
        IF_OS2_DEBUG( INIT ) {
            KdPrint(("Os2SbBuildSD: RtlCreateAcl, Status = %lx\n", Status));
        }
#endif
        RtlFreeHeap(Os2Heap, 0, Dacl);
        RtlFreeSid( AdminAliasSid );
        RtlFreeSid( WorldSid );
        return (Status);
    }

    Status = RtlAddAccessAllowedAce (
                 Dacl,
                 ACL_REVISION2,
                 GENERIC_ALL,
                 AdminAliasSid
                 );

    if (!NT_SUCCESS(Status)) {
#if DBG
        IF_OS2_DEBUG( INIT ) {
            KdPrint(("Os2SbBuildSD: RtlAddAccessAllowedAce(Admin), Status = %lx\n", Status));
        }
#endif
        RtlFreeHeap(Os2Heap, 0, Dacl);
        RtlFreeSid( AdminAliasSid );
        RtlFreeSid( WorldSid );
        return (Status);
    }

    Status = RtlAddAccessAllowedAce (
                 Dacl,
                 ACL_REVISION2,
                 GENERIC_READ,
                 WorldSid
                 );

    if (!NT_SUCCESS(Status)) {
#if DBG
        IF_OS2_DEBUG( INIT ) {
            KdPrint(("Os2SbBuildSD: RtlAddAccessAllowedAce(World), Status = %lx\n", Status));
        }
#endif
        RtlFreeHeap(Os2Heap, 0, Dacl);
        RtlFreeSid( AdminAliasSid );
        RtlFreeSid( WorldSid );
        return (Status);
    }

    Status = RtlGetAce(
                Dacl,
                0L,
                &Pace
                );

    if (!NT_SUCCESS(Status)) {
#if DBG
        IF_OS2_DEBUG( INIT ) {
            KdPrint(("Os2SbBuildSD: RtlGetAce(Admin), Status = %lx\n", Status));
        }
#endif
        RtlFreeHeap(Os2Heap, 0, Dacl);
        RtlFreeSid( AdminAliasSid );
        RtlFreeSid( WorldSid );
        return (Status);
    }

    Pace->AceFlags |= CONTAINER_INHERIT_ACE;

    Status = RtlGetAce(
                Dacl,
                1L,
                &Pace
                );

    if (!NT_SUCCESS(Status)) {
#if DBG
        IF_OS2_DEBUG( INIT ) {
            KdPrint(("Os2SbBuildSD: RtlGetAce(World), Status = %lx\n", Status));
        }
#endif
        RtlFreeHeap(Os2Heap, 0, Dacl);
        RtlFreeSid( AdminAliasSid );
        RtlFreeSid( WorldSid );
        return (Status);
    }

    Pace->AceFlags |= CONTAINER_INHERIT_ACE;

    //
    // Add the ACL to the security descriptor
    //

    Status = RtlSetDaclSecurityDescriptor(
                 SecurityDescriptor,
                 TRUE,
                 Dacl,              // put (PACL) NULL to allow everyone all access
                 FALSE );

    if (!NT_SUCCESS(Status)) {
#if DBG
        IF_OS2_DEBUG( INIT ) {
            KdPrint(("Os2SbBuildSD: RtlSetDaclSecurityDescriptor, Status = %lx\n", Status));
        }
#endif
        RtlFreeHeap(Os2Heap, 0, Dacl);
        RtlFreeSid( AdminAliasSid );
        RtlFreeSid( WorldSid );
        return (Status);
    }

    //
    // These have been copied into the security descriptor, so
    // we can free them.
    //

    RtlFreeSid( AdminAliasSid );

    RtlFreeSid( WorldSid );

    *pDacl = Dacl;

    return(STATUS_SUCCESS);
}


NTSTATUS
Os2SbInitializeRegistryKeys(
    OUT PHANDLE phConfigSysKeyHandle,
    OUT PHANDLE phEnvironmentKeyHandle
    )

/*++

Routine Description:

    This routine creates the OS/2 subsystem key hierarchy in the registry.  It also opens
    the config.sys key, and opens the system environment key.

Arguments:

    phConfigSysKeyHandle - Returns a READ/WRITE handle to the config.sys key in the registry.

    phEnvironmentKeyHandle - Returns a READ/WRITE handle to the system environment key.
        If this key can't be opened due to access denied, NULL is returned and the
        return value will be STATUS_SUCCESS.

    Note --- If the return value is not a success return value, none of the above return
        variables can be considered to be valid.

Return Value:

    The value is an NTSTATUS type that is returned when some failure occurs.  It may
    indicate any of several errors that occur during the APIs called in this function.
    The return value should be tested with NT_SUCCESS().

--*/

{
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING Class_U;
    UNICODE_STRING SoftwareDirectory_U;
    UNICODE_STRING ProductDirectory_U;
    UNICODE_STRING VersionDirectory_U;
    UNICODE_STRING Os2IniName_U;
    UNICODE_STRING ConfigSysName_U;
    UNICODE_STRING EnvRegDir_U;
    HANDLE SoftwareKeyHandle;
    HANDLE ProductKeyHandle;
    HANDLE VersionKeyHandle;
    HANDLE Os2IniKeyHandle;
    HANDLE ConfigSysKeyHandle;
    HANDLE EnvironmentKeyHandle;
    ULONG  Disposition;
    NTSTATUS Status;
    SECURITY_DESCRIPTOR localSecurityDescriptor;
    PSECURITY_DESCRIPTOR securityDescriptor;
    PACL Dacl = NULL;


    *phConfigSysKeyHandle = NULL;
    *phEnvironmentKeyHandle = NULL;

    // We start off by creating/opening the registry key hierarchy for our subsystem

    securityDescriptor = &localSecurityDescriptor;

    Status = Os2SbBuildSD(securityDescriptor,
                          &Dacl);

    if (!NT_SUCCESS(Status)) {
#if DBG
        IF_OS2_DEBUG( INIT ) {
            KdPrint(("Os2SbInitializeRegistryKeys: Os2SbBuildSD failed, Status = %lx\n", Status));
        }
#endif
        return (Status);
    }

    RtlInitUnicodeString(&Class_U, Os2Class);

    RtlInitUnicodeString(&SoftwareDirectory_U, Os2SoftwareDirectory);
    InitializeObjectAttributes(&Obja,
                               &SoftwareDirectory_U,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenKey(&SoftwareKeyHandle,
                       KEY_CREATE_SUB_KEY,
                       &Obja
                      );
    if (!NT_SUCCESS(Status)) {
#if DBG
        IF_OS2_DEBUG( INIT ) {
            KdPrint(("Os2SbInitializeRegistryKeys: Can't open software key, rc = %lx\n", Status));
        }
#endif
        if (Dacl != NULL) {
            RtlFreeHeap(Os2Heap, 0, Dacl);
        }
        return (Status);
    }

    RtlInitUnicodeString(&ProductDirectory_U, Os2ProductDirectory);
    InitializeObjectAttributes(&Obja,
                               &ProductDirectory_U,
                               OBJ_CASE_INSENSITIVE,
                               SoftwareKeyHandle,
                               securityDescriptor);

    Status = NtCreateKey(&ProductKeyHandle,
                         KEY_CREATE_SUB_KEY,
                         &Obja,
                         0,
                         &Class_U,
                         REG_OPTION_NON_VOLATILE,
                         &Disposition
                        );
    NtClose(SoftwareKeyHandle);
    if (!NT_SUCCESS(Status)) {
#if DBG
        IF_OS2_DEBUG( INIT ) {
            KdPrint(("Os2SbInitializeRegistryKeys: Can't create product key, rc = %lx\n", Status));
        }
#endif
        if (Dacl != NULL) {
            RtlFreeHeap(Os2Heap, 0, Dacl);
        }
        return (Status);
    }

    RtlInitUnicodeString(&VersionDirectory_U, Os2VersionDirectory);
    InitializeObjectAttributes(&Obja,
                               &VersionDirectory_U,
                               OBJ_CASE_INSENSITIVE,
                               ProductKeyHandle,
                               securityDescriptor);

    Status = NtCreateKey(&VersionKeyHandle,
                         KEY_CREATE_SUB_KEY,
                         &Obja,
                         0,
                         &Class_U,
                         REG_OPTION_NON_VOLATILE,
                         &Disposition
                        );
    NtClose(ProductKeyHandle);
    if (!NT_SUCCESS(Status))
    {
#if DBG
        IF_OS2_DEBUG( INIT ) {
            KdPrint(("Os2SbInitializeRegistryKeys: Can't create version key, rc = %lx\n", Status));
        }
#endif
        if (Dacl != NULL) {
            RtlFreeHeap(Os2Heap, 0, Dacl);
        }
        return (Status);
    }

    RtlInitUnicodeString(&Os2IniName_U, Os2IniName);
    InitializeObjectAttributes(&Obja,
                               &Os2IniName_U,
                               OBJ_CASE_INSENSITIVE,
                               VersionKeyHandle,
                               securityDescriptor);

    Status = NtCreateKey(&Os2IniKeyHandle,
                         KEY_CREATE_SUB_KEY,
                         &Obja,
                         0,
                         &Class_U,
                         REG_OPTION_NON_VOLATILE,
                         &Disposition
                        );
    if (!NT_SUCCESS(Status))
    {
#if DBG
        IF_OS2_DEBUG( INIT ) {
            KdPrint(("Os2SbInitializeRegistryKeys: Can't create os2ini key, rc = %lx\n", Status));
        }
#endif
        NtClose(VersionKeyHandle);
        if (Dacl != NULL) {
            RtlFreeHeap(Os2Heap, 0, Dacl);
        }
        return (Status);
    }
    NtClose(Os2IniKeyHandle);

    RtlInitUnicodeString(&ConfigSysName_U, Os2ConfigSysName);
    InitializeObjectAttributes(&Obja,
                               &ConfigSysName_U,
                               OBJ_CASE_INSENSITIVE,
                               VersionKeyHandle,
                               securityDescriptor);

    Status = NtCreateKey(&ConfigSysKeyHandle,
                         KEY_READ | KEY_WRITE,
                         &Obja,
                         0,
                         &Class_U,
                         REG_OPTION_NON_VOLATILE,
                         &Disposition
                        );

    if (Dacl != NULL) {
        RtlFreeHeap(Os2Heap, 0, Dacl);
    }
    NtClose(VersionKeyHandle);

    if (!NT_SUCCESS(Status)) {
#if DBG
        IF_OS2_DEBUG( INIT ) {
            KdPrint(("Os2SbInitializeRegistryKeys: Can't create/open config.sys key, rc = %lx\n",
                      Status));
        }
#endif
        return(Status);
    }

    // Open the environment.

    RtlInitUnicodeString(&EnvRegDir_U, Os2EnvironmentDirectory);
    InitializeObjectAttributes(&Obja,
                               &EnvRegDir_U,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenKey(&EnvironmentKeyHandle,
                       KEY_READ | KEY_WRITE,
                       &Obja
                      );

    if (!NT_SUCCESS(Status)) {
#if DBG
        IF_OS2_DEBUG( INIT ) {
            KdPrint(("Os2SbInitializeRegistryKeys: Unable to NtOpenKey() the system environment, rc = %lx\n",
                      Status));
        }
#endif
        if (Status != STATUS_ACCESS_DENIED) {
            NtClose(ConfigSysKeyHandle);
            return(Status);
        }

        //
        // on denied access, return with no error so we can at least create the
        // config.sys value entry
        // The caller will know this occured because the environment handle is null.
        //

    } else {
        *phEnvironmentKeyHandle = EnvironmentKeyHandle;
    }

    *phConfigSysKeyHandle = ConfigSysKeyHandle;
    return (STATUS_SUCCESS);
}


NTSTATUS
Os2SbInitializeRegistry(
    IN ULONG FlagsValue,
    IN HANDLE ConfigSysFileHandle,
    IN PLARGE_INTEGER pTimeStamp,
    IN PLARGE_INTEGER pSizeStamp
    )

/*++

Routine Description:

    This function is responsible for initializing the entire Registry component of the
    Subsystem.  It generates the key hierarchy in the registry.

    In the generation of the key hierarchy, it also generates a config.sys entry.
    The information in this entry is taken from the following sources:

      -- some default strings we put in (see Os2ConfigSysDefaultValue).
      -- Information from OS/2's config.sys file is added as follows:

        > SET commands are put in the system environment.
          Some SET commands are ignored (see Os2SetDirectiveProcessingDispatchFunction)
        > LIBPATH commands are merged to the Os2LibPath variable in the
          system environment.  A terminating semicolon is added if there
          is only one path in the path list.
        > SET PATH= is ignored, and the system path remains the same.

Arguments:

    FlagsValue -- The previously existing "Flags" value that was read in from the
        registry.  If this is -1, then no previous Flags value existed.
    ConfigSysFileHandle -- An optional handle to an already open config.sys file
    pTimeStamp, pSizeStamp -- info about the open config.sys file.

    Note:  The parameters ConfigSysFileHandle + pTimeStamp + pSizeStamp are optional,
        and come as a packet.  If the config.sys file was previously opened, the handle
        will be non-null, and all 3 params must be valid.  If the config.sys file was
        not previously opened (successfully), the handle should be NULL, and the other
        2 params need not be valid.

Return Value:

    The value is an NTSTATUS type that is returned when some failure occurs.  It may
    indicate any of several errors that occur during the APIs called in this function.
    The return value should be tested with NT_SUCCESS().  If an unsuccessful value
    is returned, it means the registry component was not properly initialized.

--*/

{
    UNICODE_STRING Os2LibPathValueName_U;
    HANDLE ConfigSysKeyHandle;
    NTSTATUS Status;
    UNICODE_STRING ConfigSysName_U;
    PWCHAR pInfo;
    PUCHAR Src;
    PWCHAR Src1;
    PWCHAR Dest;
    WCHAR ch, ch1;
    TCHAR chTemp;

    // Create the key hierarchy

    Status = Os2SbInitializeRegistryKeys(&ConfigSysKeyHandle,
                                       &Os2EnvironmentKeyHandle);

    if (!NT_SUCCESS(Status)) {
#if DBG
        IF_OS2_DEBUG( INIT ) {
            KdPrint(("Os2SbInitializeRegistry: failed Os2SbInitializeRegistryKeys, Status = %lx\n", Status));
        }
#endif
        return(Status);
    }

    //
    // Create the config.sys entry
    // and migrate information from os/2's config.sys file.
    //

    Os2LibPathFound = FALSE;
    Os2LibPathValueData_U.Buffer = NULL;

    if (Os2EnvironmentKeyHandle == NULL) {          // we have no write access to the environment
        goto Os2NoEnvAccess;                        // skip over the Os2LibPath stuff
    }

    // Get Os2LibPath from sys env so we can update it.

    if (!Or2GetEnvPath(&Os2LibPathValueData_U,
                       Os2Heap,
                       PATHLIST_MAX,
                       Os2EnvironmentKeyHandle,
                       Os2LibPathValueName,
                       FALSE)) {
#if DBG
        IF_OS2_DEBUG( INIT ) {
            KdPrint(("Os2SbInitializeRegistry: Unable to fetch Os2LibPath from sys env\n"));
        }
#endif
    } else {

        // make sure there's at lease one semicolon

        Or2CheckSemicolon(&Os2LibPathValueData_U);

        Os2LibPathFound = TRUE;
    }

    if (Os2LibPathFound && Os2LibPathValueData_U.Length == 0) {     // it's empty (or nonexistent)
                                                                    // set up a default
        UNICODE_STRING tmp;

        RtlInitUnicodeString(&tmp, Os2SystemDirectory);

        RtlCopyUnicodeString(&Os2LibPathValueData_U, &tmp);

        RtlAppendUnicodeToString(&Os2LibPathValueData_U, L"\\os2\\dll;");

        Os2LibPathValueData_U.Buffer[Os2LibPathValueData_U.Length/sizeof(WCHAR)] = UNICODE_NULL;
    }

Os2NoEnvAccess:

    // Now set up the initial regisry config.sys entry.

    // Allocate a buffer to build the config.sys entry in.  (it's a multi-string)

    pInfo = (PWCHAR) RtlAllocateHeap(Os2Heap, 0, MAX_CONSYS_SIZE);
    if (pInfo == NULL)
    {
#if DBG
        IF_OS2_DEBUG( INIT ) {
            KdPrint(("Os2SbInitializeRegistry: Unable to RtlAllocateHeap() space for config.sys value buffer\n"));
        }
#endif
        if (Os2LibPathValueData_U.Buffer != NULL) {
            RtlFreeHeap(Os2Heap, 0, Os2LibPathValueData_U.Buffer);
            Os2LibPathValueData_U.Buffer = NULL;
            Os2LibPathFound = FALSE;
        }
        if (Os2EnvironmentKeyHandle != NULL) {
            NtClose(Os2EnvironmentKeyHandle);
            Os2EnvironmentKeyHandle = NULL;
        }
        NtClose(ConfigSysKeyHandle);

        return (STATUS_NO_MEMORY);
    }

    // Initially, copy our default value into the entry.

    if (IsNEC_98) {
        Src = (PUCHAR) Os2ConfigSysDefaultValue_NEC_PC9800;
        if (!GetSystemRootDrive(&chTemp)) {
            chTemp = L'a';
        }
    }
    else
        Src = (PUCHAR) Os2ConfigSysDefaultValue;

    Dest = pInfo;
    do
    {
        ch = (WCHAR) *Src++;
        if (ch == L'\a')
        {
            Src1 = Os2SystemDirectory;
            ch1 = *Src1++;
            while (ch1 != UNICODE_NULL)
            {
                *Dest++ = ch1;
                ch1 = *Src1++;
            }
        }
        else if (IsNEC_98 && ch == L'\b')
        {
            *Dest++ = chTemp;
        }
        else
        {
            *Dest++ = ch;
        }
    } while (!((ch == UNICODE_NULL) && (*Src == '\0')));


    // check if the an OS/2 CONFIG.SYS file already exists on
    // the drive. Its name will be C:\CONFIG.SYS
    // If it exists, process it for additional information
    // The system env will also be updated

    if (Os2InitOriginalConfigSysProcessing(
                    ConfigSysKeyHandle,
                    FlagsValue,
                    ConfigSysFileHandle,
                    pTimeStamp,
                    pSizeStamp
                    )) {

        if (Os2EnvironmentKeyHandle == NULL) {          // we have no write access to the environment

            //
            // We couldn't open a write key to the sys env for some reason, so we
            // won't be able to migrate SET variables to NT from config.sys.
            // Perhaps a popup should be generated to notify the user of this at
            // this point.
            // Anyway, we go on and only write the config.sys entry, skipping
            // variable migration.
            //
#if DBG
            IF_OS2_DEBUG( INIT ) {
                KdPrint(("Os2SbInitializeRegistry: Initializing registry without writing system env\n"));
            }
#endif
        }

        Os2ProcessOriginalConfigSys(&Dest);
        Os2TerminateOriginalConfigSysProcessing();
    }
#if 0

    //
    // The code below is disabled.  We'll add COUNTRY= when creating a fictive
    // config.sys on the fly.  If the code is reenabled in the future -- it
    // should be fixed.  It doesn't make sure there are at least three characters
    // (with leading 0s) in the country code.
    //

    else {

        WCHAR CountryCode[7];
        ULONG CountryLength;
        //
        // add a default "COUNTRY=" line
        //

        CountryLength = GetLocaleInfoW(
                            GetSystemDefaultLCID(),
                            LOCALE_ICOUNTRY,
                            CountryCode,
                            6);

        if (CountryLength == 0) {
#if DBG
            KdPrint(("Os2SbInitializeRegistry: Cannot get locale country info, LastError = %lx\n",
                    GetLastError()));
#endif
        } else {

            RtlMoveMemory(Dest, L"COUNTRY=", 16);
            Dest += 8;

            RtlMoveMemory(Dest, CountryCode, CountryLength * sizeof(WCHAR));
            Dest += CountryLength;

            *Dest++ = UNICODE_NULL;
        }
    }
#endif

    // Write the new Os2LibPath

    if (!IsNEC_98) {

        if (Os2LibPathFound) {

            RtlInitUnicodeString(&Os2LibPathValueName_U, Os2LibPathValueName);
            Status = NtSetValueKey(Os2EnvironmentKeyHandle,
                                   &Os2LibPathValueName_U,
                                   (ULONG)0,
                                   REG_EXPAND_SZ,
                                   Os2LibPathValueData_U.Buffer,
                                   Os2LibPathValueData_U.Length + sizeof(WCHAR)
                                  );

#if DBG
            if (!NT_SUCCESS(Status))
            {
                IF_OS2_DEBUG( INIT ) {
                    KdPrint(("Os2SbInitializeRegistry: Unable to NtSetValueKey() system environment, rc = %X\n",
                              Status));
                }
            }
#endif

            Os2LibPathFound = FALSE;
            RtlFreeHeap(Os2Heap, 0, Os2LibPathValueData_U.Buffer);
            Os2LibPathValueData_U.Buffer = NULL;
        }

        // handle a possible PATH value setting

        if (Os2PathString != NULL) {

            UNICODE_STRING Os2PathValueName_U;
            UNICODE_STRING Os2PathValueData_U;

            //
            // Os2EnvironmentKeyHandle can't be NULL at this point because
            // we wouldn't have set Os2PathString...
            //

            if (!Or2GetEnvPath(&Os2PathValueData_U,
                               Os2Heap,
                               PATHLIST_MAX,
                               Os2EnvironmentKeyHandle,
                               L"Path",
                               FALSE)) {
#if DBG
                KdPrint(("Os2SbInitializeRegistry: Unable to fetch Path from sys env\n"));
#endif
                //
                // Can't get value, skip processing
                //

            } else {

                if (Os2PathValueData_U.Length == 0) {

                    //
                    // If we couldn't get the value, or it's empty, don't process it
                    //

#if DBG
                    KdPrint(("Os2SbInitializeRegistry: Path in sys env empty or not found\n"));
#endif
                } else {

                    //
                    // Append our string...
                    //

                    Or2AppendPathToPath(Os2Heap,
                                        Os2PathString,
                                        &Os2PathValueData_U,
                                        TRUE);

                    //
                    // Put it back into the registry...
                    //

                    RtlInitUnicodeString(&Os2PathValueName_U, L"Path");
                    Status = NtSetValueKey(Os2EnvironmentKeyHandle,
                                           &Os2PathValueName_U,
                                           (ULONG)0,
                                           REG_EXPAND_SZ,
                                           Os2PathValueData_U.Buffer,
                                           Os2PathValueData_U.Length + sizeof(WCHAR)
                                          );

#if DBG
                    if (!NT_SUCCESS(Status)) {
                        KdPrint(("Os2SbInitializeRegistry: Unable to NtSetValueKey() system environment (2), rc = %X\n",
                                  Status));
                    }
#endif
                     // ignore error from setvaluekey
                 }

                 RtlFreeHeap(Os2Heap, 0, Os2PathValueData_U.Buffer);
             }

             RtlFreeHeap(Os2Heap, 0, Os2PathString);
             Os2PathString = NULL;
        }
    }

    if (Os2EnvironmentKeyHandle != NULL) {
        NtClose(Os2EnvironmentKeyHandle);
        Os2EnvironmentKeyHandle = NULL;
    }

    // set the REG_MULTI_SZ terminating NUL

    *Dest++ = UNICODE_NULL;

    // Finally, write the config.sys multi-string entry into the registry.

    RtlInitUnicodeString(&ConfigSysName_U, Os2ConfigSysName);
    Status = NtSetValueKey(ConfigSysKeyHandle,
                           &ConfigSysName_U,
                           (ULONG)0,
                           REG_MULTI_SZ,
                           (PVOID)pInfo,
                           (PBYTE)Dest - (PBYTE)pInfo
                          );
    RtlFreeHeap(Os2Heap, 0, pInfo);
    if (!NT_SUCCESS(Status))
    {
#if DBG
        IF_OS2_DEBUG( INIT ) {
            KdPrint(("Os2SbInitializeRegistry: Unable to NtSetValueKey() registry config.sys, rc = %X\n",
                      Status));
        }
#endif
        NtClose(ConfigSysKeyHandle);
        return (Status);
    }

    NtClose(ConfigSysKeyHandle);
    return (STATUS_SUCCESS);
}


VOID
Os2MigrationProcedure(
    VOID
    )

/*++

Routine Description:

    This routine oversees the os/2 config.sys migration procedure.  It's called from winlogon during
    boot.  It checks to see if the os/2ss config.sys-related registy entries are intact.  If they
    aren't, it rebuilds them.  If they are intact, it checks if config.sys migration is enabled,
    and if the os/2 config.sys file exists and has changed (size/lastwritetime) since the last migration.
    If so, it remigrates the config.sys info.  Note that SET variables are only added, never
    removed.  The same goes for LIBPATH.  PATH is not migrated from os/2 config.sys to NT.

Arguments:

    None.

Return Value:

    None.

--*/

{
    BYTE RegBuffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + 4 * sizeof(ULONG)];
    OBJECT_ATTRIBUTES Obja;
    IO_STATUS_BLOCK IoStatus;
    UNICODE_STRING String_U;
    LARGE_INTEGER TimeStamp;
    LARGE_INTEGER SizeStamp;
    PKEY_VALUE_PARTIAL_INFORMATION pInfo = (PKEY_VALUE_PARTIAL_INFORMATION) RegBuffer;
    PULONG RegVal;
    HANDLE ConfigSysKeyHandle;
    HANDLE ConfigSysFileHandle = NULL;
    ULONG FlagsValue = (ULONG)-1;
    ULONG ResultLength;
    NTSTATUS Status;

    RtlInitUnicodeString(&String_U, Os2ConfigSysKeyName);
    InitializeObjectAttributes(&Obja,
                               &String_U,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenKey(&ConfigSysKeyHandle,
                       KEY_READ,
                       &Obja
                      );

    if (!NT_SUCCESS(Status)) {

        if (Status != STATUS_OBJECT_PATH_NOT_FOUND &&
            Status != STATUS_OBJECT_NAME_NOT_FOUND &&
            Status != STATUS_OBJECT_PATH_INVALID) {
#if DBG
            KdPrint(("Os2MigrationProcedure: Can't Open config.sys registry key, Status = %lx\n", Status));
#endif
            return;
        }

        goto BuildRegistry;
    }

    RtlInitUnicodeString(&String_U, Os2ConfigFlagName);
    Status = NtQueryValueKey(ConfigSysKeyHandle,
                             &String_U,
                             KeyValuePartialInformation,
                             pInfo,
                             sizeof(RegBuffer),
                             &ResultLength
                            );

    if (!NT_SUCCESS(Status)) {

        NtClose(ConfigSysKeyHandle);

        if (Status != STATUS_OBJECT_PATH_NOT_FOUND &&
            Status != STATUS_OBJECT_NAME_NOT_FOUND &&
            Status != STATUS_OBJECT_PATH_INVALID) {
#if DBG
            KdPrint(("Os2MigrationProcedure: Can't Read config.sys flag value key, Status = %lx\n", Status));
#endif
            return;
        }

        goto BuildRegistry;
    }

    if (pInfo->Type != REG_DWORD ||
        pInfo->DataLength != sizeof(DWORD)) {
        NtClose(ConfigSysKeyHandle);
        goto BuildRegistry;
    }

    FlagsValue = *(PULONG) (pInfo->Data);

    RtlInitUnicodeString(&String_U, Os2ConfigSysName);
    Status = NtQueryValueKey(ConfigSysKeyHandle,
                             &String_U,
                             KeyValuePartialInformation,
                             NULL,
                             0,
                             &ResultLength
                            );

    //
    // The 2 expected status results are:
    //
    // STATUS_OBJECT_NAME_NOT_FOUND - config.sys not yet defined
    // STATUS_BUFFER_TOO_SMALL      - config.sys already defined
    //

    if (Status == STATUS_OBJECT_NAME_NOT_FOUND ||
        Status == STATUS_OBJECT_PATH_NOT_FOUND ||
        Status == STATUS_OBJECT_PATH_INVALID) {
        NtClose(ConfigSysKeyHandle);
        goto BuildRegistry;
    }

    if (Status != STATUS_BUFFER_TOO_SMALL) {
#if DBG
        KdPrint(("Os2MigrationProcedure: Can't Read config.sys registry value key, Status = %lx\n", Status));
#endif
        NtClose(ConfigSysKeyHandle);
        return;
    }

    if ((FlagsValue & FLAGS_DISABLEMIGRATION) ||
        !(FlagsValue & FLAGS_CONFIGSYSONDISK)) {

        //
        // os/2 config.sys migration disabled
        // or no os/2 config.sys on disk
        //

        NtClose(ConfigSysKeyHandle);
        return;
    }

    RtlInitUnicodeString(&String_U, Os2ConfigStampName);
    Status = NtQueryValueKey(ConfigSysKeyHandle,
                             &String_U,
                             KeyValuePartialInformation,
                             pInfo,
                             sizeof(RegBuffer),
                             &ResultLength
                            );

    NtClose(ConfigSysKeyHandle);

    if (!NT_SUCCESS(Status)) {

        if (Status != STATUS_OBJECT_PATH_NOT_FOUND &&
            Status != STATUS_OBJECT_NAME_NOT_FOUND &&
            Status != STATUS_OBJECT_PATH_INVALID) {
#if DBG
            KdPrint(("Os2MigrationProcedure: Can't Read config.sys stamp value key, Status = %lx\n", Status));
#endif
            return;
        }

        goto BuildRegistry;
    }

    if (pInfo->Type != REG_BINARY ||
        pInfo->DataLength != 4 * sizeof(DWORD)) {
        goto BuildRegistry;
    }

    RegVal = (PULONG) (pInfo->Data);

    if (IsNEC_98)
        RtlInitUnicodeString(&String_U, Os2OriginalCanonicalConfigSys_NEC_PC9800);
    else
        RtlInitUnicodeString(&String_U, Os2OriginalCanonicalConfigSys);

    InitializeObjectAttributes(&Obja,
                   &String_U,
                   OBJ_CASE_INSENSITIVE,
                   NULL,
                   NULL);

    Status = NtOpenFile(&ConfigSysFileHandle,
                        FILE_GENERIC_READ,
                        &Obja,
                        &IoStatus,
                        FILE_SHARE_READ,
                        FILE_SYNCHRONOUS_IO_NONALERT
                       );

    if (!NT_SUCCESS(Status)) {
        goto BuildRegistry;
    }

    Status = Or2GetFileStamps(
                    ConfigSysFileHandle,
                    &TimeStamp,
                    &SizeStamp
                    );

    if (!NT_SUCCESS(Status)) {
#if DBG
        KdPrint(("Os2MigrationProcedure: Can't get config.sys time/size stamp, Status = %lx\n", Status));
#endif

        NtClose(ConfigSysFileHandle);
        ConfigSysFileHandle = NULL;
        goto BuildRegistry;
    }

    if (TimeStamp.LowPart == RegVal[0] &&
        (ULONG) TimeStamp.HighPart == RegVal[1] &&
        SizeStamp.LowPart == RegVal[2] &&
        (ULONG) SizeStamp.HighPart == RegVal[3]) {

        //
        // os/2 config.sys hasn't changed, skip processing
        //

        NtClose(ConfigSysFileHandle);
        return;
    }

    //
    // We need to install the registry stuff
    //

BuildRegistry:

    if (GetSystemDirectoryW(Os2SystemDirectory, MAX_PATH) == 0) {
#if DBG
        KdPrint(("Os2MigrationProcedure: Cannot obtain name of system directory, LastError = %lx\n",
                   GetLastError()));
#endif
        if (ConfigSysFileHandle != NULL) {
            NtClose(ConfigSysFileHandle);
        }
        return;
    }

    //
    // Create a heap to use for dynamic memory allocation.
    //

    Os2Heap = RtlCreateHeap( HEAP_GROWABLE,
                             NULL,
                             0x10000L,  // Initial size of heap is 64K
                             0x1000L,   // Commit an initial page
                             NULL,
                             NULL       // Reserved
                           );
    if (Os2Heap == NULL) {
#if DBG
        KdPrint(("Os2MigrationProcedure: Error at RtlCreateHeap of Os2Heap\n"));
#endif
        if (ConfigSysFileHandle != NULL) {
            NtClose(ConfigSysFileHandle);
        }
        return;
    }

    Status = Os2SbInitializeRegistry(
                    FlagsValue,
                    ConfigSysFileHandle,
                    &TimeStamp,
                    &SizeStamp
                    );

    if (!NT_SUCCESS(Status)) {
#if DBG
        KdPrint(("Os2MigrationProcedure: Os2SbInitializeRegistry() failed, Status = %lx\n", Status));
#endif
    }

    //
    // ConfigSysFileHandle is closed by Os2SbInitializeRegistry
    //

    RtlDestroyHeap(Os2Heap);
}

BOOLEAN
GetSystemRootDrive(
    IN LPTCH lpDrive
    )

/*++

Routine Description:
    Get drive letter from %Systemroot% for PC98 configuration.

--*/

{
    TCHAR szTemp[MAX_PATH];
    DWORD  dwType = REG_SZ;
    DWORD  cbData = sizeof(szTemp);
    LONG lRet;
    HKEY hSubKey;
    static TCHAR szDriveSave = TEXT('\0');

    if (szDriveSave) {
        *lpDrive = szDriveSave;
        return TRUE;
    }

    lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                        TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion"),
                        0,
                        KEY_QUERY_VALUE,
                        &hSubKey);

    if (ERROR_SUCCESS == lRet) {
        lRet = RegQueryValueEx(hSubKey,
                               TEXT("SystemRoot"),
                               NULL,
                               &dwType,
                               (LPBYTE)szTemp,
                               &cbData);
    } else {
        return FALSE;
    }

    RegCloseKey(hSubKey);

    if (ERROR_SUCCESS != lRet) {
        return FALSE;
    }
    *lpDrive = szDriveSave = szTemp[0];
    return TRUE;
}
