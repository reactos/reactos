#ifndef SNMPELEA_H
#define SNMPELEA_H

//
// NTSTATUS
//

typedef LONG NTSTATUS;
/*lint -e624 */  // Don't complain about different typedefs.   // winnt
typedef NTSTATUS *PNTSTATUS;
/*lint +e624 */  // Resume checking for different typedefs.    // winnt

//
//  Status values are 32 bit values layed out as follows:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +---+-+-------------------------+-------------------------------+
//  |Sev|C|       Facility          |               Code            |
//  +---+-+-------------------------+-------------------------------+
//
//  where
//
//      Sev - is the severity code
//
//          00 - Success
//          01 - Informational
//          10 - Warning
//          11 - Error
//
//      C - is the Customer code flag
//
//      Facility - is the facility code
//
//      Code - is the facility's status code
//

//
// Generic test for success on any status value (non-negative numbers
// indicate success).
//

#define NT_SUCCESS(Status) ((NTSTATUS)(Status) >= 0)

//
// Generic test for information on any status value.
//

#define NT_INFORMATION(Status) ((ULONG)(Status) >> 30 == 1)

//
// Generic test for warning on any status value.
//

#define NT_WARNING(Status) ((ULONG)(Status) >> 30 == 2)

//
// Generic test for error on any status value.
//

#define NT_ERROR(Status) ((ULONG)(Status) >> 30 == 3)

// begin_winnt
#define APPLICATION_ERROR_MASK       0x20000000
#define ERROR_SEVERITY_SUCCESS       0x00000000
#define ERROR_SEVERITY_INFORMATIONAL 0x40000000
#define ERROR_SEVERITY_WARNING       0x80000000
#define ERROR_SEVERITY_ERROR         0xC0000000
// end_winnt

typedef	HMODULE	*PHMODULE;

#define	HANDLESIZE		sizeof(HANDLE)
#define	EVENTRECSIZE	sizeof(EVENTLOGRECORD)
#define	LOG_BUF_SIZE	4096
#define	EVENTIDSIZE		4
#define MAX_QUEUE_SIZE	20

#define	HALFMAXDWORD	0x80000000

#define	SERVICE_ROOT						TEXT("SYSTEM\\CurrentControlSet\\Services\\")
#define	EXTENSION_ROOT						TEXT("SOFTWARE\\Microsoft\\SNMP_EVENTS\\EventLog\\")
#define	EXTENSION_SOURCES					TEXT("SOFTWARE\\Microsoft\\SNMP_EVENTS\\EventLog\\Sources\\")

#define EVNTAGNT_NAME                       TEXT("EvntAgnt")
#define EVENTLOG_BASE		SERVICE_ROOT	TEXT("EventLog\\")
#define	EVENTLOG_ROOT		EVENTLOG_BASE	TEXT("Application\\")
#define	EVENTLOG_SERVICE	EVENTLOG_ROOT	EVNTAGNT_NAME

#define	EXTENSION_PARM		EXTENSION_ROOT	TEXT("Parameters")

#define	EXTENSION_MSG_MODULE				TEXT("EventMessageFile")
#define	EXTENSION_PARM_MODULE				TEXT("ParameterMessageFile")
#define	EXTENSION_PRIM_MODULE				TEXT("PrimaryModule")

#define	EXTENSION_TRACE_FILE				TEXT("TraceFileName")
#define	EXTENSION_TRACE_LEVEL				TEXT("TraceLevel")
#define	EXTENSION_BASE_OID					TEXT("BaseEnterpriseOID")
#define	EXTENSION_SUPPORTED_VIEW			TEXT("SupportedView")
#define	EXTENSION_TRIM						TEXT("TrimMessage")
#define EXTENSION_MAX_TRAP_SIZE				TEXT("MaxTrapSize")
#define EXTENSION_TRIM_FLAG					TEXT("TrimFlag")

#define	EXTENSION_ENTERPRISE_OID			TEXT("EnterpriseOID")
#define	EXTENSION_APPEND					TEXT("Append")
#define	EXTENSION_COUNT						TEXT("Count")
#define	EXTENSION_TIME						TEXT("Time")
#define	EXTENSION_THRESHOLD_FLAG			TEXT("Threshold")
#define	EXTENSION_THRESHOLD_ENABLED			TEXT("ThresholdEnabled")
#define	EXTENSION_THRESHOLD_COUNT			TEXT("ThresholdCount")
#define	EXTENSION_THRESHOLD_TIME			TEXT("ThresholdTime")
#define	EXTENSION_LASTBOOT_TIME				TEXT("LastBootTime")

#define MUTEX_NAME							TEXT("SnmpEventLogMutex")	// Mutex name

typedef	struct	_VarBindQueue {
			DWORD				dwEventID;				// event ID
			DWORD				dwEventTime;			// time this event occurred
			BOOL				fProcessed;				// buffer processed flag
			AsnObjectIdentifier	*enterprise;			// enterprise OID
			RFC1157VarBindList	*lpVariableBindings;	// variable bindings list
	struct	_VarBindQueue		*lpNextQueueEntry;		// pointer to next buffer structure
} VarBindQueue, *PVarBindQueue;

typedef struct    _SourceHandleList {
         HINSTANCE   handle;
         TCHAR       sourcename[MAX_PATH+1];
   struct _SourceHandleList   *Next;
} SourceHandleList, *PSourceHandleList;

const 	UINT	MAX_TRAP_SIZE=4096;			// a guess provided by Microsoft
const	UINT	BASE_PDU_SIZE=300;			// a guess provided by Microsoft

const	UINT	THRESHOLD_COUNT=500;		// default performance threshold count
const	UINT	THRESHOLD_TIME=300;			// default performance threshold time in seconds

#endif					// end of snmpelea.h definitions
