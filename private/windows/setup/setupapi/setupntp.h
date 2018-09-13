/*++

Copyright (c) 1995-1998 Microsoft Corporation

Module Name:

    setupntp.h

Abstract:

    Private top-level header file for Windows NT Setup
    services Dll.

Author:

    Ted Miller (tedm) 11-Jan-1995

Revision History:

    Jim Schmidt (jimschm)   16-Dec-1998 Log api init
    Jim Schmidt (jimschm)   28-Apr-1997 Added stub.h
    Jamie Hunter (jamiehun) 13-Jan-1997 Added backup.h

--*/


//
// System header files
//

#if DBG
#ifndef MEM_DBG
#define MEM_DBG 1
#endif
#else
#ifndef MEM_DBG
#define MEM_DBG 0
#endif
#endif

//
// NT Header Files
//
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>


#include <windows.h>
#include <windowsx.h>
#include <setupapi.h>
#include <diamondd.h>
#include <lzexpand.h>
#include <commdlg.h>
#include <commctrl.h>
#include <dlgs.h>
#include <regstr.h>
#include <infstr.h>
#include <cfgmgr32.h>
#include <spapip.h>
#include <objbase.h>
#include <devguid.h>
#include <wincrypt.h>
#include <mscat.h>
#include <softpub.h>
#include <wintrust.h>
#include <shlobj.h>
#include <cdm.h>
#include <userenv.h>
#include <userenvp.h>
#include <secedit.h>
#include <sfcapip.h>


//
// CRT header files
//
#include <process.h>
#include <malloc.h>
#include <wchar.h>
#include <stddef.h>
#include <stdio.h>
#include <fcntl.h>
#include <tchar.h>
#include <mbstring.h>

//
// Debug memory functions and wrappers to track allocations
//

#if MEM_DBG

VOID
SetTrackFileAndLine (
    PCSTR File,
    UINT Line
    );

VOID
ClrTrackFileAndLine (
    VOID
    );

#define TRACK_ARG_DECLARE       PCSTR __File, UINT __Line
#define TRACK_ARG_COMMA         ,
#define TRACK_ARG_CALL          __FILE__, __LINE__
#define TRACK_PUSH              SetTrackFileAndLine(__File, __Line);
#define TRACK_POP               ClrTrackFileAndLine();

#else

#define TRACK_ARG_DECLARE
#define TRACK_ARG_COMMA
#define TRACK_ARG_CALL
#define TRACK_PUSH
#define TRACK_POP

#endif

//
// Private header files
//
#include "locking.h"
#include "cntxtlog.h"
#include "inf.h"
#include "backup.h"
#include "fileq.h"
#include "devinst.h"
#include "devres.h"
#include "rc_ids.h"
#include "msg.h"
#include "stub.h"
#include "helpids.h"



//
// Private DNF_ flags (start at 0x10000000)
//
#define PDNF_MASK                   0xF0000000  // Mask for private PDNF_xxx flags
#define PDNF_CLEANUP_SOURCE_PATH    0x10000000  // Delete the source path when we destroy the driver node
                                                // used when drivers are downloaded from the Internet


//
// Module handle for this DLL. Filled in at process attach.
//
extern HANDLE MyDllModuleHandle;

//
// Module handle for security DLL. Initialized to NULL in at process attach. Filled in when SCE APIs have to be called
//
extern HINSTANCE SecurityDllHandle;

//
// OS Version Information structure filled in at process attach.
//
extern OSVERSIONINFO OSVersionInfo;

//
// Static strings we retreive once, at process attach.
//
extern PCTSTR WindowsDirectory,InfDirectory,SystemDirectory,ConfigDirectory,DriversDirectory,System16Directory;
extern PCTSTR SystemSourcePath,ServicePackSourcePath,DriverCacheSourcePath;
extern PCTSTR OsLoaderRelativePath;     // may be NULL
extern PCTSTR WindowsBackupDirectory;   // Directory to write uninstall backups to
extern PCTSTR ProcessFileName;          // Filename of app calling setupapi

//
// are we inside gui setup? determined at process attach
//
extern BOOL GuiSetupInProgress;

//
// various other global flags
//
extern DWORD GlobalSetupFlags;

//
// global window message for cancelling autoplay.
//
extern UINT g_uQueryCancelAutoPlay;

//
// Static multi-sz list of directories to be searched for INFs.
//
extern PCTSTR InfSearchPaths;

//
// Name of platform (mips, alpha, x86, ppc, etc)
//
extern PCTSTR PlatformName;

//
// Debug memory functions and wrappers to track allocations
//
extern HANDLE g_hHeap;

#if MEM_DBG

PVOID
MyDebugMalloc(
    IN DWORD Size,
    IN PCSTR Filename,
    IN DWORD Line
    );

#define MyMalloc(sz) MyDebugMalloc( sz , __FILE__ , __LINE__ )

//
// Macros and wrappers are needed for externally exposed functions
//

DWORD
TrackedQueryRegistryValue(
    IN          TRACK_ARG_DECLARE,
    IN  HKEY    KeyHandle,
    IN  PCTSTR  ValueName,
    OUT PTSTR  *Value,
    OUT PDWORD  DataType,
    OUT PDWORD  DataSizeBytes
    );

#define QueryRegistryValue(a,b,c,d,e)   TrackedQueryRegistryValue(TRACK_ARG_CALL,a,b,c,d,e)

PTSTR
TrackedDuplicateString(
    IN TRACK_ARG_DECLARE,
    IN PCTSTR String
    );

#define DuplicateString(x)              TrackedDuplicateString(TRACK_ARG_CALL,x)

//
// Macros needed for internal routines are at the bottom of this file
//

#endif


//
// Resource/string retrieval routines in resource.c
//

VOID
SetDlgText(
    IN HWND hwndDlg,
    IN INT  iControl,
    IN UINT nStartString,
    IN UINT nEndString
    );

#define SDT_MAX_TEXT    1000        // Max SetDlgText() combined text size

PTSTR
MyLoadString(
    IN UINT StringId
    );

PTSTR
FormatStringMessage(
    IN UINT FormatStringId,
    ...
    );

PTSTR
FormatStringMessageV(
    IN UINT     FormatStringId,
    IN va_list *ArgumentList
    );

PTSTR
FormatStringMessageFromString(
    IN PTSTR FormatString,
    ...
    );

PTSTR
FormatStringMessageFromStringV(
    IN PTSTR    FormatString,
    IN va_list *ArgumentList
    );

PTSTR
RetreiveAndFormatMessage(
    IN UINT MessageId,
    ...
    );

PTSTR
RetreiveAndFormatMessageV(
    IN UINT     MessageId,
    IN va_list *ArgumentList
    );

INT
FormatMessageBox(
    IN HANDLE hinst,
    IN HWND   hwndParent,
    IN UINT   TextMessageId,
    IN PCTSTR Title,
    IN UINT   Style,
    ...
    );

//
// This is in shell32.dll and in windows\inc16\shlsemip.h but
// that file cannot be #include'd here as it has macros that clash
// with our own, etc.
//
int
RestartDialog(
    IN HWND hwnd,
    IN PCTSTR Prompt,
    IN DWORD Return
    );


//
// Decompression/filename manupilation routines in decomp.c.
//
PTSTR
SetupGenerateCompressedName(
    IN PCTSTR Filename
    );

DWORD
SetupInternalGetFileCompressionInfo(
    IN  PCTSTR            SourceFileName,
    OUT PTSTR            *ActualSourceFileName,
    OUT PWIN32_FIND_DATA  SourceFindData,
    OUT PDWORD            TargetFileSize,
    OUT PUINT             CompressionType
    );

DWORD
SetupDetermineSourceFileName(
    IN  PCTSTR            FileName,
    OUT PBOOL             UsedCompressedName,
    OUT PTSTR            *FileNameLocated,
    OUT PWIN32_FIND_DATA  FindData
    );

BOOL
pSetupDoesFileMatch(
    IN  PCTSTR            InputName,
    IN  PCTSTR            CompareName,
    OUT PBOOL             UsedCompressedName,
    OUT PTSTR            *FileNameLocated
    );

//
// Diamond functions. The Process and Thread Attach routines are called
// by the DLL entry point routine and should not be called by anyone else.
//
BOOL
DiamondProcessAttach(
    IN BOOL Attach
    );

VOID
DiamondThreadAttach(
    IN BOOL Attach
    );

BOOL
DiamondIsCabinet(
    IN PCTSTR FileName
    );

DWORD
DiamondProcessCabinet(
    IN PCTSTR CabinetFile,
    IN DWORD  Flags,
    IN PVOID  MsgHandler,
    IN PVOID  Context,
    IN BOOL   IsUnicodeMsgHandler
    );

//
// Misc routines
//
VOID
DiskPromptGetDriveType(
    IN  PCTSTR PathToSource,
    OUT PUINT  DriveType,
    OUT PBOOL  IsRemovable
    );

BOOL
SetTruncatedDlgItemText(
    HWND hdlg,
    UINT CtlId,
    PCTSTR TextIn
    );

LPTSTR
CompactFileName(
    LPCTSTR FileNameIn,
    DWORD CharsToRemove
    );

DWORD
ExtraChars(
    HWND hwnd,
    LPCTSTR TextBuffer
    );

VOID
pSetupInitPlatformPathOverrideSupport(
    IN BOOL Init
    );

VOID
pSetupInitSourceListSupport(
    IN BOOL Init
    );

DWORD
pSetupDecompressOrCopyFile(
    IN  PCTSTR SourceFileName,
    IN  PCTSTR TargetFileName,
    IN  PUINT  CompressionType, OPTIONAL
    IN  BOOL   AllowMove,
    OUT PBOOL  Moved            OPTIONAL
    );

BOOL
_SetupInstallFileEx(
    IN  PSP_FILE_QUEUE      Queue,             OPTIONAL
    IN  PSP_FILE_QUEUE_NODE QueueNode,         OPTIONAL
    IN  HINF                InfHandle,         OPTIONAL
    IN  PINFCONTEXT         InfContext,        OPTIONAL
    IN  PCTSTR              SourceFile,        OPTIONAL
    IN  PCTSTR              SourcePathRoot,    OPTIONAL
    IN  PCTSTR              DestinationName,   OPTIONAL
    IN  DWORD               CopyStyle,
    IN  PVOID               CopyMsgHandler,    OPTIONAL
    IN  PVOID               Context,           OPTIONAL
    OUT PBOOL               FileWasInUse,
    IN  BOOL                IsMsgHandlerNativeCharWidth,
    OUT PBOOL               SignatureVerifyFailed
    );

//
// Define flags for _SetupCopyOEMInf
//
#define SCOI_NO_UI_ON_SIGFAIL                 1
#define SCOI_NO_ERRLOG_ON_MISSING_CATALOG     2
#define SCOI_NO_ERRLOG_IF_INF_ALREADY_PRESENT 4
#define SCOI_KEEP_INF_AND_CAT_ORIGINAL_NAMES  8 // for exception INFs

BOOL
_SetupCopyOEMInf(
    IN  PCTSTR               SourceInfFileName,
    IN  PCTSTR               OEMSourceMediaLocation,          OPTIONAL
    IN  DWORD                OEMSourceMediaType,
    IN  DWORD                CopyStyle,
    OUT PTSTR                DestinationInfFileName,          OPTIONAL
    IN  DWORD                DestinationInfFileNameSize,
    OUT PDWORD               RequiredSize,                    OPTIONAL
    OUT PTSTR               *DestinationInfFileNameComponent, OPTIONAL
    IN  PCTSTR               SourceInfOriginalName,
    IN  PCTSTR               SourceInfCatalogName,            OPTIONAL
    IN  HWND                 Owner,
    IN  PCTSTR               DeviceDesc,                      OPTIONAL
    IN  DWORD                DriverSigningPolicy,
    IN  DWORD                Flags,
    IN  PCTSTR               AltCatalogFile,                  OPTIONAL
    IN  PSP_ALTPLATFORM_INFO AltPlatformInfo,                 OPTIONAL
    OUT PDWORD               DriverSigningError,              OPTIONAL
    OUT PTSTR                CatalogFilenameOnSystem,
    IN  PSETUP_LOG_CONTEXT   LogContext
    );

PTSTR
AllocAndReturnDriverSearchList(
    IN DWORD SearchControl
    );

pSetupGetSecurityInfo(
    IN HINF Inf,
    IN PCTSTR SectionName,
    OUT PCTSTR *SecDesc );

BOOL
pSetupGetDriverDate(
    IN  HINF        InfHandle,
    IN  PCTSTR      Section,
    IN OUT PFILETIME   pFileTime
    );

BOOL
pSetupGetDriverVersion(
    IN  HINF        InfHandle,
    IN  PCTSTR      Section,
    OUT DWORDLONG   *Version
    );

PTSTR
GetMultiSzFromInf(
    IN  HINF    InfHandle,
    IN  PCTSTR  SectionName,
    IN  PCTSTR  Key,
    OUT PBOOL   OutOfMemory
    );

VOID
pSetupInitNetConnectionList(
    IN BOOL Init
    );

BOOL
_SetupGetSourceFileSize(
    IN  HINF        InfHandle,
    IN  PINFCONTEXT InfContext,     OPTIONAL
    IN  PCTSTR      FileName,       OPTIONAL
    IN  PCTSTR      Section,        OPTIONAL
    OUT PDWORD      FileSize,
    IN  UINT        RoundingFactor  OPTIONAL
    );

BOOL
_SetupGetSourceFileLocation(
    IN  HINF        InfHandle,
    IN  PINFCONTEXT InfContext,         OPTIONAL
    IN  PCTSTR      FileName,           OPTIONAL
    OUT PUINT       SourceId,           OPTIONAL
    OUT PTSTR       ReturnBuffer,       OPTIONAL
    IN  DWORD       ReturnBufferSize,
    OUT PDWORD      RequiredSize,       OPTIONAL
    OUT PINFCONTEXT LineContext         OPTIONAL
    );

DWORD
pSetupLogSectionError(
    IN HINF             InfHandle,
    IN HDEVINFO         DeviceInfoSet,      OPTIONAL
    IN PSP_DEVINFO_DATA DeviceInfoData,     OPTIONAL
    IN PCTSTR           SectionName,
    IN DWORD            MsgID,
    IN DWORD            Err,
    IN PCTSTR           KeyName             OPTIONAL
);

//
// Routine to call out to a PSP_FILE_CALLBACK, handles
// Unicode<-->ANSI issues
//
UINT
pSetupCallMsgHandler(
    IN PVOID MsgHandler,
    IN BOOL  MsgHandlerIsNativeCharWidth,
    IN PVOID Context,
    IN UINT  Notification,
    IN UINT_PTR Param1,
    IN UINT_PTR Param2
    );

UINT
pSetupCallDefaultMsgHandler(
    IN PVOID Context,
    IN UINT  Notification,
    IN UINT_PTR Param1,
    IN UINT_PTR Param2
    );

//
// Internal routine to get MRU list.
//
DWORD
pSetupGetList(
    IN  DWORD    Flags,
    OUT PCTSTR **List,
    OUT PUINT    Count,
    OUT PBOOL    NoBrowse
    );

#define  SRCPATH_USEPNFINFORMATION  0x00000001
#define  SRCPATH_USEINFLOCATION     0x00000002

#define SRCINFO_SVCPACK_SOURCE  1


#define PSP_COPY_USE_DRIVERCACHE     0x80000000
#define PSP_COPY_CHK_DRIVERCACHE     0x40000000


PTSTR
pSetupGetDefaultSourcePath(
    IN  HINF   InfHandle,
    IN  DWORD  Flags,
    OUT PDWORD InfSourceMediaType
    );

VOID
InfSourcePathFromFileName(
    IN  PCTSTR  InfFileName,
    OUT PTSTR  *SourcePath,  OPTIONAL
    OUT PBOOL   TryPnf
    );

BOOL
pSetupGetSourceInfo(
    IN  HINF        InfHandle,         OPTIONAL
    IN  PINFCONTEXT LayoutLineContext, OPTIONAL
    IN  UINT        SourceId,
    IN  UINT        InfoDesired,
    OUT PTSTR       ReturnBuffer,      OPTIONAL
    IN  DWORD       ReturnBufferSize,
    OUT PDWORD      RequiredSize       OPTIONAL
    );

//
// Define an additional private flag for the pStringTable APIs.
// Private flags are added from MSB down; public flags are added
// from LSB up.
//
#define STRTAB_ALREADY_LOWERCASE 0x80000000

//
// Private string table functions that don't do locking.  These are
// to be used for optimization purposes by components that already have
// a locking mechanism (e.g., HINF, HDEVINFO).
//
LONG
pStringTableLookUpString(
    IN     PVOID   StringTable,
    IN OUT PTSTR   String,
    OUT    PDWORD  StringLength,
    OUT    PDWORD  HashValue,           OPTIONAL
    OUT    PVOID  *FindContext,         OPTIONAL
    IN     DWORD   Flags,
    OUT    PVOID   ExtraData,           OPTIONAL
    IN     UINT    ExtraDataBufferSize  OPTIONAL
    );

LONG
pStringTableAddString(
    IN     PVOID StringTable,
    IN OUT PTSTR String,
    IN     DWORD Flags,
    IN     PVOID ExtraData,     OPTIONAL
    IN     UINT  ExtraDataSize  OPTIONAL
    );

BOOL
pStringTableGetExtraData(
    IN  PVOID StringTable,
    IN  LONG  StringId,
    OUT PVOID ExtraData,
    IN  UINT  ExtraDataBufferSize
    );

BOOL
pStringTableSetExtraData(
    IN PVOID StringTable,
    IN LONG  StringId,
    IN PVOID ExtraData,
    IN UINT  ExtraDataSize
    );

BOOL
pStringTableEnum(
    IN  PVOID                StringTable,
    OUT PVOID                ExtraDataBuffer,     OPTIONAL
    IN  UINT                 ExtraDataBufferSize, OPTIONAL
    IN  PSTRTAB_ENUM_ROUTINE Callback,
    IN  LPARAM               lParam               OPTIONAL
    );

PTSTR
pStringTableStringFromId(
    IN PVOID StringTable,
    IN LONG  StringId
    );

PVOID
pStringTableDuplicate(
    IN PVOID StringTable
    );

VOID
pStringTableDestroy(
    IN PVOID StringTable
    );

VOID
pStringTableTrim(
    IN PVOID StringTable
    );

PVOID
pStringTableInitialize(
    IN UINT ExtraDataSize   OPTIONAL
    );

DWORD
pStringTableGetDataBlock(
    IN  PVOID  StringTable,
    OUT PVOID *StringTableBlock
    );


//
// PNF String table routines
//
PVOID
InitializeStringTableFromPNF(
    IN PPNF_HEADER PnfHeader,
    IN LCID Locale
    );


//
// Routines for creating/destroying global mini-icon list.
//
BOOL
CreateMiniIcons(
    VOID
    );

VOID
DestroyMiniIcons(
    VOID
    );


//
// Global log init/terminate
//

VOID
InitLogApi (
    VOID
    );

VOID
TerminateLogApi (
    VOID
    );



//
// DIRID mapping routines.
//
PCTSTR
pSetupVolatileDirIdToPath(
    IN PCTSTR      DirectoryId,    OPTIONAL
    IN UINT        DirectoryIdInt, OPTIONAL
    IN PCTSTR      SubDirectory,   OPTIONAL
    IN PLOADED_INF Inf
    );

DWORD
ApplyNewVolatileDirIdsToInfs(
    IN PLOADED_INF MasterInf,
    IN PLOADED_INF Inf        OPTIONAL
    );

PCTSTR
pSetupDirectoryIdToPathEx(
    IN     PCTSTR  DirectoryId,        OPTIONAL
    IN OUT PUINT   DirectoryIdInt,     OPTIONAL
    IN     PCTSTR  SubDirectory,       OPTIONAL
    IN     PCTSTR  InfSourcePath,      OPTIONAL
    IN OUT PCTSTR *OsLoaderPath,       OPTIONAL
    OUT    PBOOL   VolatileSystemDirId OPTIONAL
    );

PCTSTR
pGetPathFromDirId(
    IN     PCTSTR  DirectoryId,        OPTIONAL
    IN     PCTSTR  SubDirectory,       OPTIONAL
    IN     HINF    Inf
    );


//
// Routines for inter-thread communication.
//
BOOL
WaitForPostedThreadMessage(
    OUT LPMSG MessageData,
    IN  UINT  Message
    );

//
// Macro to make ansi vs unicode string handling
// a little easier
//
#ifdef UNICODE
#define NewAnsiString(x)        UnicodeToAnsi(x)
#define NewPortableString(x)    AnsiToUnicode(x)
#else
#define NewAnsiString(x)        DuplicateString(x)
#define NewPortableString(x)    DuplicateString(x)
#endif

//
// Internal file-handling routines in fileutil.c
//
DWORD
MapFileForRead(
    IN  HANDLE   FileHandle,
    OUT PDWORD   FileSize,
    OUT PHANDLE  MappingHandle,
    OUT PVOID   *BaseAddress
    );

BOOL
DoMove(
    IN PCTSTR CurrentName,
    IN PCTSTR NewName
    );

BOOL
DelayedMove(
    IN PCTSTR CurrentName,
    IN PCTSTR NewName       OPTIONAL
    );

extern GUID DriverVerifyGuid;

DWORD
VerifySourceFile(
    IN  PSETUP_LOG_CONTEXT     LogContext,
    IN  PSP_FILE_QUEUE         Queue,                      OPTIONAL
    IN  PSP_FILE_QUEUE_NODE    QueueNode,                  OPTIONAL
    IN  PCTSTR                 Key,
    IN  PCTSTR                 FileToVerifyFullPath,
    IN  PCTSTR                 OriginalSourceFileFullPath, OPTIONAL
    IN  PSP_ALTPLATFORM_INFO   AltPlatformInfo,            OPTIONAL
    IN  BOOL                   UseOemCatalogs,
    OUT SetupapiVerifyProblem *Problem,
    OUT LPTSTR                 ProblemFile
    );

BOOL
Verify3rdPartyInfFile(
    IN  PSETUP_LOG_CONTEXT  LogContext,
    IN  LPCTSTR             CurrentInfName,
    IN  PLOADED_INF         pInf
    );

BOOL
IsInfForDeviceInstall(
    IN  PSETUP_LOG_CONTEXT  LogContext,        OPTIONAL
    IN  PLOADED_INF         LoadedInf,         OPTIONAL
    OUT PTSTR              *DeviceDesc,        OPTIONAL
    OUT PDWORD              PolicyToUse,       OPTIONAL
    OUT PBOOL               UseOriginalInfName OPTIONAL
    );

DWORD
GetCodeSigningPolicyForInf(
    IN  PSETUP_LOG_CONTEXT LogContext,        OPTIONAL
    IN  HINF               InfHandle,
    OUT PBOOL              UseOriginalInfName OPTIONAL
    );

BOOL
IsFileProtected(
    IN  LPCTSTR            FileFullPath,
    IN  PSETUP_LOG_CONTEXT LogContext,   OPTIONAL
    OUT PHANDLE            phSfp         OPTIONAL
    );


//
// Utils
//

PCTSTR
GetSystemSourcePath(
    TRACK_ARG_DECLARE
    );

PCTSTR
GetServicePackSourcePath(
    TRACK_ARG_DECLARE
    );


//
// Log stuff.
//
typedef enum {
    SetupapiFatalError,
    SetupapiError,
    SetupapiWarning,
    SetupapiInformation,
    SetupapiDetailedInformation
} SetupapiLogLevel;

PVOID
CreateSetupapiLog(
    IN LPCTSTR          FileName,    OPTIONAL
    IN SetupapiLogLevel LogLevel,
    IN BOOL             Append
    );

VOID
CloseSetupapiLog(
    IN PVOID LogFile
    );

BOOL
WriteTextToSetupapiLog(
    IN PVOID            LogFile,
    IN SetupapiLogLevel Level,
    IN LPCTSTR          Text
    );

BOOL
WriteToSetupapiLog(
    IN PVOID            LogFile,
    IN SetupapiLogLevel Level,
    IN UINT             MessageId,
    ...
    );



#if DBG

#define MYTRACE  DebugPrint /*(...)*/

#else

#define MYTRACE  1?0: /*(...)*/

#endif

#ifdef _X86_
BOOL
IsNEC98(
    VOID
    );

#endif

//
// Stubs to allow ANSI build to run on Win9x
//

#ifdef DBGHEAP_CHECK

    #ifdef ANSI_SETUPAPI

        #define ASSERT_HEAP_IS_VALID()

    #else

        NTSYSAPI
        BOOLEAN
        NTAPI
        RtlValidateProcessHeaps (
            VOID
            );

        #define ASSERT_HEAP_IS_VALID()   MYASSERT(RtlValidateHeap(g_hHeap,0,NULL))

    #endif // ANSI_SETUPAPI

#else

    #define ASSERT_HEAP_IS_VALID()

#endif // DBGHEAP_CHECK



#if MEM_DBG

//
// Macros for internal memory allocation tracking
//

#define GetSystemSourcePath()           GetSystemSourcePath(TRACK_ARG_CALL)
#define GetServicePackSourcePath()      GetServicePackSourcePath(TRACK_ARG_CALL)
#define InheritLogContext(a,b)          InheritLogContext(TRACK_ARG_CALL,a,b)

#endif
