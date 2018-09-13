/*++

Copyright (c) 1995-1998 Microsoft Corporation

Module Name:

    fileq.h

Abstract:

    Private header file for setup file queue routines.
    A setup file queue is a list of pending rename, delete,
    and copy operations.

Author:

    Ted Miller (tedm) 15-Feb-1995

Revision History:

    Jamie Hunter (jamiehun) 13-Jan-1998
        Added backup & un-windable copying
    Gabe Schaffer (t-gabes) 19-Jul-1998
        Added LogCotext to SP_FILE_QUEUE

--*/

//
// Declare this forward reference here so structures below can use it
// before it's defined.
//
struct _SP_FILE_QUEUE;
struct _SP_FILE_QUEUE_NODE;

//
// Define structure that describes a source media in use
// in a particular file queue.
//
typedef struct _SOURCE_MEDIA_INFO {

    struct _SOURCE_MEDIA_INFO *Next;

    //
    // String IDs for description and tagfile.
    //
    LONG Description;
    LONG DescriptionDisplayName; // case-sensitive form for display.

    LONG Tagfile;

    //
    // String ID for source root path
    //
    LONG SourceRootPath;

    //
    // Copy queue for this media.
    //
    struct _SP_FILE_QUEUE_NODE *CopyQueue;
    UINT CopyNodeCount;

    //
    // Flags for this source media descriptor
    //
    DWORD Flags;

} SOURCE_MEDIA_INFO, *PSOURCE_MEDIA_INFO;

//
// Define valid flags for SOURCE_MEDIA_INFO.Flags
//
#define SMI_FLAG_NO_SOURCE_ROOT_PATH            0x1
#define SMI_FLAG_USE_SVCPACK_SOURCE_ROOT_PATH   0x2
#define SMI_FLAG_USE_LOCAL_SOURCE_CAB           0x4

//
// Define structure that describes a catalog, used for signing
// and file verification.
//
typedef struct _SPQ_CATALOG_INFO {

    struct _SPQ_CATALOG_INFO *Next;

    //
    // String ID for original filename of the catalog file,
    // such as specified in CatalogFile= in the [Version] section
    // of an inf file.
    //
    // This field may be -1, which indicates no CatalogFile= line
    // was specified in the INF.
    //
    LONG CatalogFileFromInf;

    //
    // String ID for original filename of the catalog file specified by the
    // INF for an alternate platform (the alternate platform having been setup
    // by a call to SetupSetFileQueueAlternatePlatform).  This field is only
    // valid when the containing file queue has the FQF_USE_ALT_PLATFORM flag
    // set.
    //
    // This field may be -1, which indicates that no CatalogFile= line was
    // specified in the INF (or at least not one that can be used given the
    // currently active alternate platform parameters).
    //
    LONG AltCatalogFileFromInf;
    //
    // Also, maintain a temporary storage for the new alternate catalog string
    // ID to be used while we're processing the catalog list, retrieving the
    // platform-specific entries associated with each INF.  This is done so that
    // if we encounter an error part-way through (e.g, out-of-memory or couldn't
    // load INF), then we don't have to maintain a separate list in order to do
    // a rollback.
    //
    LONG AltCatalogFileFromInfPending;

    //
    // String ID for the full (source) path of the INF.
    //
    LONG InfFullPath;

    //
    // String ID for the source INF's original (simple) name (may be -1 if the
    // source INF's original name is the same as its current name.
    //
    LONG InfOriginalName;

    //
    // String ID for the INF's final resting place (i.e., its name in the INF
    // directory, unless it's been part of an alternate catalog install, in
    // which case it will be the same as InfFullPath).  This value will be -1
    // until the catalog node has been processed by _SetupVerifyQueuedCatalogs.
    // After that, its value will be equal to InfFullPath if the INF was in the
    // Inf directory in the first place, or was part of an alternate catalog
    // installation.  Otherwise, it'll be the string ID for the unique name we
    // used when copying the INF into the Inf directory.
    //
    LONG InfFinalPath;

#if 0
    //
    // Pointer to media descriptor for first file that caused this
    // catalog node to be enqueued. This gives a pretty good indicator
    // of which media we expect the catalog file to be on.
    //
    PSOURCE_MEDIA_INFO SourceMediaInfo;
#endif

    //
    // Error code indicating the cause of failure to validate the catalog.
    //
    DWORD VerificationFailureError;

    //
    // CATINFO_FLAG flags containing information about this catalog node such
    // as whether it is the 'primary device INF' for a device installation.
    //
    DWORD Flags;

    //
    // Full filepath of catalog file. This is the catalog file as
    // it's been installed on the system.
    //
    TCHAR CatalogFilenameOnSystem[MAX_PATH];

} SPQ_CATALOG_INFO, *PSPQ_CATALOG_INFO;

//
// Catalog node flags.
//
#define CATINFO_FLAG_PRIMARY_DEVICE_INF  0x00000001 // primary device INF for a
                                                    // device installation queue

#define CATINFO_FLAG_NEWLY_COPIED        0x00000002 // indicates whether INF/CAT
                                                    // were newly copied when
                                                    // this catalog node was
                                                    // verified.

//
// Define structure that describes a node in a file queue.
//
typedef struct _SP_FILE_QUEUE_NODE {

    struct _SP_FILE_QUEUE_NODE *Next;

    //
    // Operation: copy, delete, rename
    //
    UINT Operation;

    //
    // Copy:
    //
    // String ID for source root path
    // (such as F:\ or \\SERVER\SHARE\SUBDIR).
    //
    // Delete: unused
    // Rename: unused
    //
    LONG SourceRootPath;

    //
    // Copy:
    //
    // String ID for rest of the path (between the root and the filename).
    // Generally this is the directory specified for the source media
    // in [SourceDisksNames].
    //
    // Not always specified (-1 if not specified).
    //
    // Delete: unused
    //
    // Rename: source path of file to be renamed
    //
    LONG SourcePath;

    //
    // Copy: String ID for source filename (filename only, no path).
    // Delete: unused
    // Rename: source filename of file to be renamed. If not specified
    //         SourcePath contains complete full path of file.
    //
    LONG SourceFilename;

    //
    // Copy: String ID for the target directory (no filename).
    // Delete: part 1 of the full path of the file to delete (ie, path part)
    // Rename: Target directory for file (ie, rename is actually a move).
    //         If not specified rename is a rename only (TargetFilename
    //         contains the new filename).
    //
    LONG TargetDirectory;

    //
    // Copy: String ID for the target filename (filename only, no path),
    // Delete: part 2 of the full path of the file to delete (ie, file part)
    //         If not specified then TargetDirectory contains complete full path.
    // Rename: supplies new filename for rename/move operation. Filename part only.
    //
    LONG TargetFilename;

    //
    //  Copy  : String ID for Security Descriptor information
    //  Delete: Unused
    //  Rename: Unused
    LONG SecurityDesc;


    //
    // Copy: Information about the source media on which this file can be found.
    // Delete: unused
    // Rename: unused
    //
    PSOURCE_MEDIA_INFO SourceMediaInfo;

    //
    // Style flags for file operation
    //
    DWORD StyleFlags;

    //
    // Internal-use flags: In-use disposition, etc.
    //
    UINT InternalFlags;

    //
    // Pointer to catalog info for this file, used for file signing.
    // May be NULL.
    //
    PSPQ_CATALOG_INFO CatalogInfo;

} SP_FILE_QUEUE_NODE, *PSP_FILE_QUEUE_NODE;

//
// Internal flags.
//
#define INUSE_IN_USE            0x00000001  // file was in use
#define INUSE_INF_WANTS_REBOOT  0x00000002  // file was in use and inf file
                                            // want reboot if this file was in use
#define IQF_PROCESSED           0x00000004  // queue node was already processed
#define IQF_DELAYED_DELETE_OK   0x00000008  // Use delayed delete if delete fails
#define IQF_MATCH               0x00000010  // Node matches current file in cabinet
#define IQF_LAST_MATCH          0x00000020  // Node is last in chain of matches
#define IQF_FROM_BAD_OEM_INF    0x00000040  // Copynode from invalid (w.r.t. codesigning) OEM INF
#define IQF_ALLOW_UNSIGNED      0x00000080  // node is unsigned but allow installation 
                                            //   (w.r.t. system file protection)
#define IQF_TARGET_PROTECTED    0x00000100  // node is replacing a system file

#define SCE_DLL L"scecli.dll"
#define LD_WIN95 2
#define ST_SCE_SET 0
#define ST_SCE_DELETE 1
#define ST_SCE_RENAME 2
#define ST_SCE_UNWIND 3
#define ST_SCE_SERVICES 4


//
// Define structure describing a setup file operation queue.
//
typedef struct _SP_FILE_QUEUE {
    //
    // We'll maintain separate lists internally for each type
    // of queued operation. Each source media has its own copy queue.
    //
    //
    PSP_FILE_QUEUE_NODE BackupQueue;
    PSP_FILE_QUEUE_NODE DeleteQueue;
    PSP_FILE_QUEUE_NODE RenameQueue;

    //
    // Number of nodes in the various queues.
    //
    UINT CopyNodeCount;
    UINT DeleteNodeCount;
    UINT RenameNodeCount;
    UINT BackupNodeCount;

    //
    // Pointer to first source media descriptor.
    //
    PSOURCE_MEDIA_INFO SourceMediaList;

    //
    // Number of source media descriptors.
    //
    UINT SourceMediaCount;

    //
    // Pointer to head of linked list of catalog descriptor structures.
    // There will be one item in this list for each catalog file
    // referenced in any file's (copy) queue node.
    //
    PSPQ_CATALOG_INFO CatalogList;

    //
    // Specifies what driver signing policy was in effect when this file queue
    // was created.  This will have been retrieved from the registry, or from
    // the DS, if applicable.  This field can take one of three values:
    //
    //   DRIVERSIGN_NONE    -  silently succeed installation of unsigned/
    //                         incorrectly-signed files.  A PSS log entry will
    //                         be generated, however (as it will for all 3 types)
    //   DRIVERSIGN_WARNING -  warn the user, but let them choose whether or not
    //                         they still want to install the problematic file
    //   DRIVERSIGN_BLOCKING - do not allow the file to be installed
    //
    // Note:  the use of the term "file" above refers generically to both
    // individual files and packages (i.e., INF/CAT/driver file combinations)
    //
    DWORD DriverSigningPolicy;

    //
    // Specifies the window handle that owns any UI dealing with driver signing.
    // This is filled in based on the Owner argument passed into
    // _SetupVerifyQueuedCatalogs.
    //
    HWND hWndDriverSigningUi;

    //
    // If this queue has been marked as a device install queue, store the
    // description of the device being installed in case we need to popup a
    // digital signature verification failure dialog.
    //
    // (This value may be -1)
    //
    LONG DeviceDescStringId;

    //
    // Structure that contains alternate platform information that was
    // associated with the queue via SetupSetFileQueueAlternatePlatform.  This
    // embedded structure is only valid if the FQF_USE_ALT_PLATFORM flag is set.
    //
    SP_ALTPLATFORM_INFO AltPlatformInfo;

    //
    // String ID of override catalog file to use (typically, goes hand-in-hand
    // with an AltPlatformInfo).  If no catalog override is in effect, this
    // string ID will be -1.
    //
    LONG AltCatalogFile;

    //
    // String table that all data structures associated with
    // this queue make use of.
    //
    // (NOTE: Since there is no locking mechanism on the enclosing
    // SP_FILE_QUEUE structure, this StringTable must handle its own
    // synchronization.  Therefore, this string table contains 'live'
    // locks, and must be accessed with the public versions (in spapip.h)
    // of the StringTable* APIs.)
    //
    PVOID StringTable;

    //
    // Maintain a lock refcount for user-supplied queues contained in device
    // information elements.  This ensures that the queue can't be deleted as
    // long as its being referenced in at least one device installation parameter
    // block.
    //
    DWORD LockRefCount;

    //
    // Queue flags.
    //
    DWORD Flags;

    //
    // SIS-related fields.
    //
    HANDLE SisSourceHandle;
    PCTSTR SisSourceDirectory;

    //
    // Backup and unwind fields
    //
    LONG BackupInfID;               // stringID (relative to StringTable) of Inf file associated with backup
    PVOID TargetLookupTable;        // all entries here have associated data
    PSP_UNWIND_NODE UnwindQueue;    // order of restore and file info
    PSP_DELAYMOVE_NODE DelayMoveQueue;    // order of delayed renames
    PSP_DELAYMOVE_NODE DelayMoveQueueTail; // last of delayed renames

    //
    // Signature used for a primitive form of validation.
    //
    DWORD Signature;

    //
    // Pointer to log context for error logging
    //
    PSETUP_LOG_CONTEXT LogContext;

} SP_FILE_QUEUE, *PSP_FILE_QUEUE;



#define SP_FILE_QUEUE_SIG   0xc78e1098

//
// Internal-use queue commit routine.
//
BOOL
_SetupCommitFileQueue(
    IN HWND     Owner,         OPTIONAL
    IN HSPFILEQ QueueHandle,
    IN PVOID    MsgHandler,
    IN PVOID    Context,
    IN BOOL     IsMsgHandlerNativeCharWidth
    );
//
// Internal-use, add a single copy to the queue
//
BOOL
pSetupQueueSingleCopy(
    IN HSPFILEQ QueueHandle,
    IN HINF     InfHandle,
    IN HINF     ListInfHandle,  OPTIONAL
    IN PCTSTR   SectionName,    OPTIONAL
    IN PCTSTR   SourceRootPath,
    IN PCTSTR   SourceFilename,
    IN PCTSTR   TargetFilename,
    IN DWORD    CopyStyle,
    IN PCTSTR   SecurityDescriptor,
    IN PCTSTR   CacheName
    );

//
// Internal-use
//

PTSTR
pSetupFormFullPath(
    IN PVOID  StringTable,
    IN LONG   PathPart1,
    IN LONG   PathPart2,    OPTIONAL
    IN LONG   PathPart3     OPTIONAL
    );

DWORD
pGetInfOriginalNameAndCatalogFile(
    IN  PLOADED_INF          Inf,                     OPTIONAL
    IN  LPCTSTR              CurrentName,             OPTIONAL
    OUT PBOOL                DifferentName,           OPTIONAL
    OUT LPTSTR               OriginalName,            OPTIONAL
    IN  DWORD                OriginalNameSize,
    OUT LPTSTR               OriginalCatalogName,     OPTIONAL
    IN  DWORD                OriginalCatalogNameSize,
    IN  PSP_ALTPLATFORM_INFO AltPlatformInfo          OPTIONAL
    );


DWORD
_SetupVerifyQueuedCatalogs(
    IN  HWND           Owner,
    IN  PSP_FILE_QUEUE Queue,
    IN  DWORD          Flags,
    OUT PTSTR          DeviceInfFinalName,  OPTIONAL
    OUT PBOOL          DeviceInfNewlyCopied OPTIONAL
    );

DWORD
LoadNtOnlyDll(
    IN  PCTSTR DllName,
    OUT HINSTANCE *Dll_Handle
    );

BOOL
pSetupProtectedRenamesFlag(    
    BOOL bSet    
    );


#ifdef UNICODE

DWORD
pSetupCallSCE(
    IN DWORD Operation,
    IN PCWSTR FullName,
    IN PSP_FILE_QUEUE Queue,
    IN PCWSTR String1,
    IN DWORD Index1,
    IN PSECURITY_DESCRIPTOR SecDesc  OPTIONAL
    );

#endif





#define VERCAT_INSTALL_INF_AND_CAT          0x00000001
#define VERCAT_NO_PROMPT_ON_ERROR           0x00000002
#define VERCAT_PRIMARY_DEVICE_INF_FROM_INET 0x00000004

