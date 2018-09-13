/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    elfdef.h

Abstract:

    This file contains defines for the eventlog service.

Author:

    Rajen Shah (rajens) 1-Jul-1991

Revision History:

--*/

#ifndef _EVENTDEF_
#define _EVENTDEF_

//
// Logfile object specific access type
//
#define ELF_LOGFILE_READ             0x0001
#define ELF_LOGFILE_WRITE            0x0002
#define ELF_LOGFILE_CLEAR            0x0004
#define ELF_LOGFILE_START            0x0008
#define ELF_LOGFILE_STOP             0x000C
#define ELF_LOGFILE_CONFIGURE        0x0010
#define ELF_LOGFILE_BACKUP           0x0020     // Set iff a backup operator
                                                // opens the security log -
                                                // this overrides all other
                                                // flags.

#define ELF_LOGFILE_ALL_ACCESS       (STANDARD_RIGHTS_REQUIRED       | \
                                         ELF_LOGFILE_READ            | \
                                         ELF_LOGFILE_WRITE           | \
                                         ELF_LOGFILE_CLEAR           | \
                                         ELF_LOGFILE_START           | \
                                         ELF_LOGFILE_STOP            | \
                                         ELF_LOGFILE_CONFIGURE)

//
// Three types of logfiles are defined from a security perspective:
//
//    ELF_LOGFILE_SECURITY    - Only Admins/LocalSystem can RW these files
//    ELF_LOGFILE_SYSTEM      - Only Admins/LocalSystem can W these files
//    ELF_LOGFILE_APPLICATION - World can R/W these files
//
// System and Security will be SECURE, Application will be NON_SECURE
//

#define ELF_LOGFILE_SECURITY      0x0000
#define ELF_LOGFILE_SYSTEM        0x0001
#define ELF_LOGFILE_APPLICATION   0x0002

//
// Macro to convert a given file size into one that is "acceptable" for
// eventlogging. It basically truncates it to a 64K boundary making sure
// that it is as least 64K
//

#define     ELFFILESIZE(x) ((x & 0xFFFF0000) ? (x & 0xFFFF0000) : 65536)

//
// Macro for debug prints
//

#if DBG
#define     ElfDbgPrint(x)      if (ElfDebug) DbgPrint x
#define     ElfDbgPrintNC(x)    DbgPrint x
#else
#define     ElfDbgPrint(x)
#define     ElfDbgPrintNC(x)
#endif

//
// The largest possible buffer we would need to hold an admin alert
// information. This primarily depends on the number and length of the
// replacement strings that would be passed with the message ID.
//

#define     ELF_ADMIN_ALERT_BUFFER_SIZE     256

//
// Timeout defines.
//

#define     INFINITE_WAIT_TIME          -1      // Wait time for events
#define     ELF_GLOBAL_RESOURCE_WAIT    2000    // 2-second timeout for global resource

//
// Signature placed before each event record in a file. Is used to
// validate where we are in a file.
//

#define     ELF_RECORD_SIGNATURE    0x654c6652 // ASCII for eLfR

//
// Size by which to grow a log file until it reaches the max size
//

#define     ELF_DEFAULT_LOG_SIZE 65536

//
// Bits for whether to take the global resource exclusively or shared.
//

#define     ELF_GLOBAL_SHARED       0x0001
#define     ELF_GLOBAL_EXCLUSIVE    0x0002

//
// Flag bits to keep track of what resources have been allocated at INIT time
//

#define     ELF_INIT_LOGHANDLE_CRIT_SEC       0x0001
#define     ELF_INIT_GLOBAL_RESOURCE          0x0002
#define     ELF_STARTED_LPC_THREAD            0x0004
#define     ELF_STARTED_REGISTRY_MONITOR      0x0008
#define     ELF_STARTED_RPC_SERVER            0x0010
#define     ELF_INIT_LOGFILE_CRIT_SEC         0x0020
#define     ELF_INIT_WELL_KNOWN_SIDS          0x0040
#define     ELF_INIT_QUEUED_EVENT_CRIT_SEC    0x0080
#define     ELF_INIT_QUEUED_MESSAGE_CRIT_SEC  0x0100
#define     ELF_INIT_CLUS_CRIT_SEC            0x0200

//
// Enumeration and macro to keep track of the "log full" popup per log
//

#define     IS_WORKSTATION()        (USER_SHARED_DATA->NtProductType == NtProductWinNt)

typedef enum
{
    LOGPOPUP_NEVER_SHOW = 0,         // Never show it for this log (e.g., Security)
    LOGPOPUP_CLEARED,                // Show it when this log fills up
    LOGPOPUP_ALREADY_SHOWN           // Don't show it again until this log is cleared
}
LOGPOPUP, *PLOGPOPUP;


//
// Structure containing information on each log file
//
// ActualMaxFileSize and ConfigMaxFileSize are stored in BYTEs.
// ActualMaxFileSize is the actual size of the file on the disk.
// ConfigMaxFileSize is the configured size of the file, which may not
// be the same as the actual size of the file.
//
// CurrentRecordNumber is the next absolute record number to write
//
// OldestRecordNumber is the next one to get overwritten
//
// Retention time is stored as the number of seconds.
//
// BaseAddress points to the physical beginning of the file.
//
// ViewSize is ALWAYS the size of the file in bytes.
//
// For the Flag bits, see the ELF_LOGFILE_HEADER_xxxx bits defined below.
//

typedef struct _LOGFILE {
    LIST_ENTRY      FileList;
    LIST_ENTRY      Notifiees;          // List of ChangeNotify recipients
    PUNICODE_STRING LogFileName;        // Full path name of log file
    PUNICODE_STRING LogModuleName;      // Name of default module for this log
    ULONG           RefCount;           // Number of modules using this file
    ULONG           Flags;              // Autowrap, dirty, etc. - See bits below
    ULONG           ConfigMaxFileSize;  // Max it can be
    ULONG           ActualMaxFileSize;  // How big it is now
    ULONG           NextClearMaxFileSize; // Can't be shrunk on the fly
    ULONG           CurrentRecordNumber;// The next one to be created
    ULONG           OldestRecordNumber; // The next one to overwrite
    ULONG           SessionStartRecordNumber; //the first record number logged in this session
    ULONG           Retention;          // Max. Retention time
    ULONG           NextClearRetention; // they shrank the file when they set this
    HANDLE          FileHandle;         // Handle to open file
    HANDLE          SectionHandle;      // Memory mapped section handle
    PVOID           BaseAddress;        // Map view base address
    ULONG           ViewSize;           // Mapped view size
    ULONG           BeginRecord;        // Offset of first log record
    ULONG           EndRecord;          // Offset of byte after last log record
    ULONG           ulLastPulseTime;    // Time this log was last notified of a change
    LOGPOPUP        logpLogPopup;       // "Log full" policy for this log
    PSECURITY_DESCRIPTOR Sd;            // User security object for this log
    RTL_RESOURCE    Resource;
} LOGFILE, *PLOGFILE;

//
// Structure containing information on each module that is registered to
// log events.
//

typedef struct _LOGMODULE {
    LIST_ENTRY  ModuleList;
    PWSTR       ModuleName;         // Name of module
    ATOM        ModuleAtom;         // Atom identifying this module
    PLOGFILE    LogFile;            // Log file for this module
} LOGMODULE, *PLOGMODULE;

//
// Command codes put in the request packets.
//

#define     ELF_COMMAND_READ         1
#define     ELF_COMMAND_WRITE        2
#define     ELF_COMMAND_CLEAR        3
#define     ELF_COMMAND_BACKUP       4
#define     ELF_COMMAND_WRITE_QUEUED 5

//
// Structures that contain the operation-specific information.
//

typedef struct _WRITE_PKT {
    DWORD       Datasize;           // Size of data in the buffer
    PVOID       Buffer;             // Contains filled event log record
} WRITE_PKT, *PWRITE_PKT;


//
// The following flag bits are used in the READ_PKT Flag field.
//

#define     ELF_IREAD_UNICODE       0x0001
#define     ELF_IREAD_ANSI          0x0002
#define     ELF_LAST_READ_FORWARD   0x0004

typedef struct _READ_PKT {
    ULONG       Flags;              // UNICODE or ANSI
    ULONG       BufferSize;         // Bytes to read
    PVOID       Buffer;             // User's buffer
    ULONG       ReadFlags;          // Sequential? Forwards? Random-access? Backwards?
    ULONG       RecordNumber;       // Where to start the READ
    ULONG       MinimumBytesNeeded; // For return info if buffer too small
    ULONG       LastSeekPos;        // Last seek position in terms of bytes
    ULONG       LastSeekRecord;     // Last seek position in terms of records
    ULONG       BytesRead;          // Bytes read - for return to caller
    ULONG       RecordsRead;
} READ_PKT, *PREAD_PKT;

typedef struct _CLEAR_PKT {
    PUNICODE_STRING         BackupFileName; // File to back up current log file (or NULL)
} CLEAR_PKT, *PCLEAR_PKT;

typedef struct _BACKUP_PKT {
    PUNICODE_STRING         BackupFileName; // File to back up current log file (or NULL)
} BACKUP_PKT, *PBACKUP_PKT;

//
// Flags used in the ELF_REQUEST_RECORD
//

#define ELF_FORCE_OVERWRITE    0x01  // Ignore retention period for this write

//
// Structure for the packet that contains all the information needed
// to perform the request.
//

typedef struct _ELF_REQUEST_RECORD {
    USHORT      Flags;
    NTSTATUS    Status;             // To return status of operation
    PLOGFILE    LogFile;            // File on which to operate
    PLOGMODULE  Module;             // Information on module
    USHORT      Command;            // Operation to be performed
    union {
        PWRITE_PKT      WritePkt;
        PREAD_PKT       ReadPkt;
        PCLEAR_PKT      ClearPkt;
        PBACKUP_PKT     BackupPkt;
    } Pkt;
} ELF_REQUEST_RECORD, *PELF_REQUEST_RECORD;

typedef 
#ifdef _WIN64
__declspec(align(8))
#endif
struct _ELF_ALERT_RECORD {
    DWORD    TimeOut;
    DWORD    MessageId;
    DWORD    NumberOfStrings;
    // array of UNICODE_STRINGs NumberOfStringsLong
    // each string
} ELF_ALERT_RECORD, * PELF_ALERT_RECORD;

typedef struct _ELF_MESSAGE_RECORD {
    DWORD    MessageId;
    DWORD    NumberOfStrings;
    // UNICODE null terminated strings
} ELF_MESSAGE_RECORD, * PELF_MESSAGE_RECORD;

//
// Record for the linked list of deferred events (these are raised by the
// eventlog service itself for writing once the current operation is complete
//

typedef struct _ELF_QUEUED_EVENT {
    LIST_ENTRY  Next;
    enum _ELF_QUEUED_EVENT_TYPE {
        Event,
        Alert,
        Message
    } Type;
    union _ELF_QUEUED_EVENT_DATA {
        ELF_REQUEST_RECORD Request;
        ELF_ALERT_RECORD Alert;
        ELF_MESSAGE_RECORD Message;
    } Event;
} ELF_QUEUED_EVENT, *PELF_QUEUED_EVENT;

//
// Structure containing information on callers of ElfChangeNotify
//

typedef struct _NOTIFIEE {
    LIST_ENTRY      Next;
    IELF_HANDLE     Handle;
    HANDLE          Event;
} NOTIFIEE, *PNOTIFIEE;


//
// Structure that describes the header that is at the beginning of the
// log files.
//
// To see if there are any records in the file, one must subtract the
// EndOffset from the StartOffset (allowing for the file having wrapped
// around) and check for a difference of greater than 1.
//
// The header size is stored at the beginning and end so that it looks
// just like any other event log record (the lengths do at any rate).
//

typedef struct _ELF_LOGFILE_HEADER {
    ULONG       HeaderSize;             // Size of this header
    ULONG       Signature;              // Signature field
    ULONG       MajorVersion;
    ULONG       MinorVersion;
    ULONG       StartOffset;            // Where the first record is located
    ULONG       EndOffset;              // The end of the last record + 1
    ULONG       CurrentRecordNumber;    // The next record to create
    ULONG       OldestRecordNumber;     // The next record to overwrite
    ULONG       MaxSize;                // Max. size when file was created
    ULONG       Flags;                  // DIRTY, etc.
    ULONG       Retention;              // Last Retention period.
    ULONG       EndHeaderSize;          // Size of this header
} ELF_LOGFILE_HEADER, *PELF_LOGFILE_HEADER;

#define     FILEHEADERBUFSIZE       sizeof(ELF_LOGFILE_HEADER)
#define     ELF_LOG_FILE_SIGNATURE  0x654c664c  // ASCII for eLfL

//
// The following flag bits are used in ELF_LOGFILE_HEADER and in the
// LOGFILE structures' Flag fields.
//

#define     ELF_LOGFILE_HEADER_DIRTY    0x0001  // File has been written to
#define     ELF_LOGFILE_HEADER_WRAP     0x0002  // The file has wrapped
#define     ELF_LOGFILE_LOGFULL_WRITTEN 0x0004  // Written logfull record
#define     ELF_LOGFILE_ARCHIVE_SET     0x0008  // Archive bit flag


//
// Structure that defines the record that identifies the end of the
// circular log file.
// This record is used to identify where the last record in the circular
// buffer is located.
//
// NOTE: It is *essential* that this record is of a size that a "normal"
//       event log record can never have. There is code that relies on
//       this fact to detect an "EOF" record.
//
//       Care must be taken to not disturb the first part of this record.  It
//       is used to identify an EOF record.  ELFEOFUNIQUEPART must be the
//       number of bytes that are constant.
//

typedef struct _ELF_EOF_RECORD {
    ULONG       RecordSizeBeginning;
    ULONG       One;
    ULONG       Two;
    ULONG       Three;
    ULONG       Four;
    ULONG       BeginRecord;
    ULONG       EndRecord;
    ULONG       CurrentRecordNumber;
    ULONG       OldestRecordNumber;
    ULONG       RecordSizeEnd;
} ELF_EOF_RECORD, *PELF_EOF_RECORD;

#define     ELFEOFRECORDSIZE        sizeof (ELF_EOF_RECORD)

//
// The following constant is how much of the EOF record is constant, and can
// be used to identify an EOF record
//

#define     ELFEOFUNIQUEPART        5 * sizeof(ULONG)

//
// This is used to fill the end of a log record so that the fixed portion
// of a log record doesn't split the end of the file.  It must be less than
// the minimum size of any valid record
//

#define ELF_SKIP_DWORD sizeof(ELF_EOF_RECORD) - 1


//
// Time for the sender of a start or stop request to the Eventlog
// service to wait (in milliseconds) before checking on the
// Eventlog service again to see if it is done
//

#define ELF_WAIT_HINT_TIME            20000     // 20 seconds


//
// Flags used by ElfpCloseLogFile
//

#define ELF_LOG_CLOSE_NORMAL                    0x0000
#define ELF_LOG_CLOSE_FORCE                     0x0001
#define ELF_LOG_CLOSE_BACKUP                    0x0002

//
// Structure used to store information read from the registry
//

typedef struct _LOG_FILE_INFO {
    PUNICODE_STRING LogFileName;
    ULONG           MaxFileSize;
    ULONG           Retention;
    ULONG           GuestAccessRestriction;
    LOGPOPUP        logpLogPopup;
} LOG_FILE_INFO, *PLOG_FILE_INFO;

//
// DEBUG stuff.
//

//
// This signature is placed in the context handle for debug purposes only,
// to track down a bug in freeing the structures.
//

#define     ELF_CONTEXTHANDLE_SIGN          0x654c6648  // ASCII for eLfH

//
// The different file open (or create) options are based on the type of file.
// The types, and their meanings are:
//
//      ElfNormalLog        Normal log file, opened for cached io
//      ElfSecurityLog      Audit logs, opened for write-thru
//      ElfBackupLog        Not an active log file, opened read only, cached
//

typedef enum _ELF_LOG_TYPE {
    ElfNormalLog,
    ElfSecurityLog,
    ElfBackupLog
} ELF_LOG_TYPE, *PELF_LOG_TYPE;


//
// Eventlog States (used as return codes)
//

#define UPDATE_ONLY         0   // no change in state - just send current status.
#define STARTING            1   // the messenger is initializing.
#define RUNNING             2   // initialization completed normally - now running
#define STOPPING            3   // uninstall pending
#define STOPPED             4   // uninstalled
#define PAUSED              5   // Paused
#define PAUSING             6   // In the process of pausing
#define CONTINUING          7   // In the process of continuing

//
// Forced Shutdown PendingCodes
//
#define PENDING     TRUE
#define IMMEDIATE   FALSE

//
// defines for reliability logging
//
#define DEFAULT_INTERVAL 0

#define SHUTDOWN_UNPLANNED   0x80000000
#define SHUTDOWN_REASON_MASK 0xFFFF

typedef enum _TIMESTAMPEVENT{

    EVENT_Boot=0,
    EVENT_NormalShutdown,
    EVENT_AbNormalShutdown

} TIMESTAMPEVENT, *PTIMESTAMPEVENT;


//SS:Clustering specific extensions
typedef struct _PROPLOGFILEINFO{
    PLOGFILE    pLogFile;
    PVOID       pStartPosition;
    PVOID       pEndPosition;
    ULONG       ulTotalEventSize; 
    ULONG       ulNumRecords;
}PROPLOGFILEINFO, *PPROPLOGFILEINFO;

//structure for propagation is preallocated.
#define MAXSIZE_OF_EVENTSTOPROP (1 * 1024)

#endif // ifndef _EVENTDEF_
