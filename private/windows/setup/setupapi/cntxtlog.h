/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    cntxtlog.h

Abstract:

    This module implements more logging for setupapi

Author:

    Gabe Schaffer (t-gabes) 7-Jul-1998

Revision History:

    Jamie Hunter (jamiehun) 26-Aug-1998

--*/

/*
    There are two log levels. One is set in the registry, which determines
    what is logged and how. The other is passed as a parameter, and indicates
    under what conditions the entry should be logged. For example, the registry
    would say that all errors and warnings should be logged, while the call to
    WriteLogEntry would specify in the LogLevel parameter that some string
    should be logged as a warning.

    The 24 least-significant bits specify flags which indicate the type of
    message when passed as a parameter, or the type of messages to logged
    when set in the registry.

    The 4 most-significant bits are flags:

    SETUP_LOG_DEBUGOUT - when specified in the registry, indicates that all
        log output should be sent to the debugger as well as the logfile.
        This flag is currently not supported as a parameter to WriteLogEntry.

    SETUP_LOG_SIMPLE - when specified in the registry, indicates that all
        logfile entries should be appended to the logfile in chronological
        order, rather than grouped by section name. This flag is not
        currently supported as a parameter.

    SETUP_LOG_BUFFER - when specified as a parameter, indicates that the
        message to be logged will be buffered until such time as a call to
        WriteLogEntry with the same LogContext but this flag is not specified.
        When an entry is to be logged without this flag set, the latest
        message will be added to the buffer, and the buffer will be flushed to
        the logfile.

        This allows you to build up a string to log without having to do
        buffer management everywhere.

        This flag does not make sense in the registry.

        NOTE: for proper functioning, do not mix log levels while buffering
        output.

        NOTE: if this flag is NOT specified, the string to be logged MUST
        end in a newline, otherwise bad stuff will happen if another log
        context writes to the logfile immediately afterward.

    SETUP_LOG_IS_CONTEXT - when specified in the registry, indicates that
        all context messages should be immediately logged to the logfile,
        rather than buffered in the LogContext.

        This flag should never appear in a file other than cntxtlog.*;
        that is, as a parameter it only makes sense if added to a multiple
        of SETUP_FIRST_LOG_CONTEXT.

        NOTE: the context buffering mechanism is explained below where
        ContextInfo is defined in SETUP_LOG_CONTEXT.
*/

//
// BUGBUG!!! (jamiehun)
// Registry/filename strings that should belong elsewhere
//
#define SP_REGKEY_LOGLEVEL      TEXT("LogLevel")
#define SP_REGKEY_LOGPATH       TEXT("LogPath")
#define SP_REGKEY_APPLOGLEVEL   TEXT("AppLogLevels")
#define SP_LOG_FILENAME         TEXT("setupapi.log")


//
// these are for general setup log entries
//
#define SETUP_LOG_LEVELMASK     0x000000FF
#define SETUP_LOG_NOLOG         0x00000001 // indicates no-logging (remember 0 is default)
#define SETUP_LOG_ERROR         0x00000010 // 10-1f is varying levels of errors
#define SETUP_LOG_WARNING       0x00000020 // 20-2f is varying levels of warnings
#define SETUP_LOG_INFO          0x00000030 // 30-3f is varying levels of info
#define SETUP_LOG_VERBOSE       0x00000040 // 40-4f is varying levels of verbose
#define SETUP_LOG_TIME          0x00000050 // 50+ allow logging of time-stamped enties
#define SETUP_LOG_TIMEALL       0x00000060 // 60+ turns on time-stamping of all entries
#define SETUP_LOG_DEFAULT       0x00000020

//
// these are for driver-only log entries
//
#define DRIVER_LOG_LEVELMASK    0x0000FF00
#define DRIVER_LOG_NOLOG        0x00000100 // indicates no-logging (remember 0 is default)
#define DRIVER_LOG_ERROR        0x00001000
#define DRIVER_LOG_WARNING      0x00002000
#define DRIVER_LOG_INFO         0x00003000
#define DRIVER_LOG_INFO1        0x00003100
#define DRIVER_LOG_VERBOSE      0x00004000
#define DRIVER_LOG_VERBOSE1     0x00004100
#define DRIVER_LOG_TIME         0x00005000
#define DRIVER_LOG_TIMEALL      0x00006000
#define DRIVER_LOG_DEFAULT      0x00003000

//
// Calling AllocLogInfoSlot will return an index with SETUP_LOG_IS_CONTEXT set
// this index represents a nested stack entry for logging information
// that will get dumped if an actual log entry is dumped
// thus providing more information
// note that lower 16 bits are used for index, re-using above Log level bits
//
#define SETUP_LOG_IS_CONTEXT    0x10000000
#define SETUP_LOG_CONTEXTMASK   0x0000ffff

//
// set this bit in loglevel to have output sent to debugger
//
#define SETUP_LOG_DEBUGOUT  0x80000000
//
// set this bit in loglevel to have entries simply appended to the log
//
#define SETUP_LOG_SIMPLE    0x40000000
//
// pass this flag to WriteLogEntry to have the entry stored in a buffer,
// to be spit out the next time the flag is *not* specified
//
#define SETUP_LOG_BUFFER    0x20000000
//
// pass this flag to registry to indicate we want to log SETUP_LOG_ISCONTEXT
//
#define SETUP_LOG_ALL_CONTEXT (SETUP_LOG_IS_CONTEXT)
//
// for validating registry log value
//
#define SETUP_LOG_VALIDREGBITS (SETUP_LOG_SIMPLE|SETUP_LOG_DEBUGOUT|SETUP_LOG_ALL_CONTEXT|DRIVER_LOG_LEVELMASK|SETUP_LOG_LEVELMASK)
//
// for validating non-context log value
//
#define SETUP_LOG_VALIDLOGBITS (SETUP_LOG_DEBUGOUT|SETUP_LOG_BUFFER|DRIVER_LOG_LEVELMASK|SETUP_LOG_LEVELMASK)
//
// for validating context log value
//
#define SETUP_LOG_VALIDCONTEXTBITS (SETUP_LOG_IS_CONTEXT | SETUP_LOG_CONTEXTMASK)


//
// This is the structure that holds all of the data required to persist logging
// information. It is not to be confused with the SETUPLOG_CONTEXT struct that
// is used elsewhere in setup.
//
typedef struct _SETUP_LOG_CONTEXT {
    //
    // Pointer to allocated name of section to be used.
    // If NULL, a section name will be generated on first use.
    //
    PTSTR       SectionName;

    //
    // Pointer to allocated name of file to log to. NULL
    // indicates no logging to file.
    //
    PTSTR       FileName;

    //
    // Flag indicating whether to send log output to debugger.
    //
    BOOL        DebuggerOutput;

    //
    // Flag indicating whether entries should simply be stuck
    // onto the end of the log, rather than put in the proper section.
    //
    BOOL        SimpleAppend;

    //
    // Multiple structures may simultaneously have pointers to
    // this struct, so a ref count is needed. CreateLogContext()
    // will set this to 1, and DeleteLogContext() will decrement
    // this until it reaches 0 (at which point the structure is
    // actually freed).
    //
    UINT        RefCount;

    //
    // This is the number of entries that have been logged in this
    // context. If timestamp is used for the section name, this will
    // allow us to use the time of the first error, rather than the
    // time the log context was created.
    //
    UINT        LoggedEntries;

    //
    // This specifies what items are logged, and what are thrown
    // away. It is a bitmap, with various bits indicating various
    // things. See SETUP_LOG_* and DRIVER_LOG_* at the beginning
    // of the file.
    //
    DWORD       LogLevel;

    //
    // These fields are for implementation of
    // AllocLogSlot and ReleaseLogSlot functions
    // ContextInfo is a list of strings indexable via SETUP_LOG_CONTEXTMASK
    // bits of a context slot returned by AllocLogSlot (ie, the slot)
    // it is also enumeratable via the link list headed by ContextFirstUsed
    // ContextFirstUsed points to first slot currently in use (bottom of stack)
    // ContextIndexes[ContextFirstUsed] points to next and so on
    // ContextLastUnused points to last slot that was released
    // -1 is used as end of list value
    // An entry may disappear in the middle of the stack if the context is used in
    // more than one thread
    //
    PTSTR       *ContextInfo;       // pointer to array of strings
    int         *ContextIndexes;    // by mark, is either index to ContextInfo, or to next unused mark
    int         ContextLastUnused;  // LIFO linked list of unused marks
    int         ContextBufferSize;  // items allocated for
    int         ContextFirstUsed;   // FIFO linked list of used marks
    int         ContextFirstAuto;   // FIFO linked list of auto-release used marks

    //
    // Sometimes multiple strings need to be logged as one entry, which
    // requires making multiple calls to WriteLogEntry. If SETUP_LOG_BUFFER
    // is specified, the text will be accumulated in the buffer until such
    // time as SETUP_LOG_BUFFER is not specified, in which case the
    // contents of Buffer is output together with the current string.
    //
    PTSTR       Buffer;

    //
    // In case multiple threads access this struct simultaneously,
    // access to ContextInfo must be serialized. Also, we
    // don't want this to be deleted while another thread is using it.
    //
    MYLOCK      Lock;

} SETUP_LOG_CONTEXT, *PSETUP_LOG_CONTEXT;

VOID
DebugPrint(
    PCTSTR format,
    ...                                 OPTIONAL
    );

DWORD
CreateLogContext(
    IN PCTSTR SectionName,              OPTIONAL
    OUT PSETUP_LOG_CONTEXT *LogContext
    );

VOID
DeleteLogContext(
    IN PSETUP_LOG_CONTEXT LogContext
    );

DWORD
RefLogContext(  // increment reference count
    IN PSETUP_LOG_CONTEXT LogContext
    );

DWORD
WriteLogEntry(
    IN PSETUP_LOG_CONTEXT LogContext,   OPTIONAL
    IN DWORD Level,
    IN DWORD MessageId,
    IN PCTSTR MessageStr,               OPTIONAL
    ...                                 OPTIONAL
    );

VOID
SetLogSectionName(
    IN PSETUP_LOG_CONTEXT LogContext,
    IN PCTSTR SectionName
    );

DWORD
InheritLogContext(
    IN TRACK_ARG_DECLARE TRACK_ARG_COMMA
    IN PSETUP_LOG_CONTEXT Source,
    OUT PSETUP_LOG_CONTEXT *Dest
    );

VOID
WriteLogError(
    IN PSETUP_LOG_CONTEXT LogContext,   OPTIONAL
    IN DWORD Level,
    IN DWORD Error
    );

DWORD
MakeUniqueName(
    IN  PCTSTR Component,        OPTIONAL
    OUT PTSTR * UniqueString
    );

DWORD
AllocLogInfoSlot(
    IN PSETUP_LOG_CONTEXT LogContext,
    IN BOOL               AutoRelease
    );

DWORD
AllocLogInfoSlotOrLevel(
    IN PSETUP_LOG_CONTEXT LogContext,
    IN DWORD              Level,
    IN BOOL               AutoRelease
    );

VOID
ReleaseLogInfoSlot(
    IN PSETUP_LOG_CONTEXT LogContext,
    DWORD Slot
    );

VOID
ReleaseLogInfoList(
    IN     PSETUP_LOG_CONTEXT LogContext,
    IN OUT PINT               ListStart
    );

