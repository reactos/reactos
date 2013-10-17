#ifndef _WINSPOOL_
#define _WINSPOOL_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4820)
#endif

#define DI_CHANNEL        1
#define DI_CHANNEL_WRITE  2
#define DI_READ_SPOOL_JOB 3

#define FORM_BUILTIN      1

#define JOB_CONTROL_PAUSE   1
#define JOB_CONTROL_RESUME  2
#define JOB_CONTROL_CANCEL  3
#define JOB_CONTROL_RESTART 4
#define JOB_CONTROL_DELETE  5

#define JOB_STATUS_PAUSED            1
#define JOB_STATUS_ERROR             2
#define JOB_STATUS_DELETING          4
#define JOB_STATUS_SPOOLING          8
#define JOB_STATUS_PRINTING          16
#define JOB_STATUS_OFFLINE           32
#define JOB_STATUS_PAPEROUT          0x40
#define JOB_STATUS_PRINTED           0x80
#define JOB_STATUS_DELETED           0x100
#define JOB_STATUS_BLOCKED_DEVQ      0x200
#define JOB_STATUS_USER_INTERVENTION 0x400

#define JOB_POSITION_UNSPECIFIED     0

#define JOB_NOTIFY_TYPE              1

#define JOB_NOTIFY_FIELD_PRINTER_NAME        0
#define JOB_NOTIFY_FIELD_MACHINE_NAME        1
#define JOB_NOTIFY_FIELD_PORT_NAME           2
#define JOB_NOTIFY_FIELD_USER_NAME           3
#define JOB_NOTIFY_FIELD_NOTIFY_NAME         4
#define JOB_NOTIFY_FIELD_DATATYPE            5
#define JOB_NOTIFY_FIELD_PRINT_PROCESSOR     6
#define JOB_NOTIFY_FIELD_PARAMETERS          7
#define JOB_NOTIFY_FIELD_DRIVER_NAME         8
#define JOB_NOTIFY_FIELD_DEVMODE             9
#define JOB_NOTIFY_FIELD_STATUS              10
#define JOB_NOTIFY_FIELD_STATUS_STRING       11
#define JOB_NOTIFY_FIELD_SECURITY_DESCRIPTOR 12
#define JOB_NOTIFY_FIELD_DOCUMENT            13
#define JOB_NOTIFY_FIELD_PRIORITY            14
#define JOB_NOTIFY_FIELD_POSITION            15
#define JOB_NOTIFY_FIELD_SUBMITTED           16
#define JOB_NOTIFY_FIELD_START_TIME          17
#define JOB_NOTIFY_FIELD_UNTIL_TIME          18
#define JOB_NOTIFY_FIELD_TIME                19
#define JOB_NOTIFY_FIELD_TOTAL_PAGES         20
#define JOB_NOTIFY_FIELD_PAGES_PRINTED       21
#define JOB_NOTIFY_FIELD_TOTAL_BYTES         22
#define JOB_NOTIFY_FIELD_BYTES_PRINTED       23

#define JOB_ACCESS_ADMINISTER 16
#define JOB_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED|JOB_ACCESS_ADMINISTER)
#define JOB_READ (STANDARD_RIGHTS_READ|JOB_ACCESS_ADMINISTER)
#define JOB_WRITE (STANDARD_RIGHTS_WRITE|JOB_ACCESS_ADMINISTER)
#define JOB_EXECUTE (STANDARD_RIGHTS_EXECUTE|JOB_ACCESS_ADMINISTER)

#define PRINTER_NOTIFY_OPTIONS_REFRESH 1
#define PRINTER_ACCESS_ADMINISTER      4
#define PRINTER_ACCESS_USE             8

#define PRINTER_ERROR_INFORMATION 0x80000000
#define PRINTER_ERROR_WARNING     0x40000000
#define PRINTER_ERROR_SEVERE      0x20000000
#define PRINTER_ERROR_OUTOFPAPER  1
#define PRINTER_ERROR_JAM         2
#define PRINTER_ERROR_OUTOFTONER  4

#define PRINTER_CONTROL_PAUSE      1
#define PRINTER_CONTROL_RESUME     2
#define PRINTER_CONTROL_PURGE      3
#define PRINTER_CONTROL_SET_STATUS 4

#define PRINTER_STATUS_PAUSED            1
#define PRINTER_STATUS_ERROR             2
#define PRINTER_STATUS_PENDING_DELETION  4
#define PRINTER_STATUS_PAPER_JAM         8
#define PRINTER_STATUS_PAPER_OUT         0x10
#define PRINTER_STATUS_MANUAL_FEED       0x20
#define PRINTER_STATUS_PAPER_PROBLEM     0x40
#define PRINTER_STATUS_OFFLINE           0x80
#define PRINTER_STATUS_IO_ACTIVE         0x100
#define PRINTER_STATUS_BUSY              0x200
#define PRINTER_STATUS_PRINTING          0x400
#define PRINTER_STATUS_OUTPUT_BIN_FULL   0x800
#define PRINTER_STATUS_NOT_AVAILABLE     0x1000
#define PRINTER_STATUS_WAITING           0x2000
#define PRINTER_STATUS_PROCESSING        0x4000
#define PRINTER_STATUS_INITIALIZING      0x8000
#define PRINTER_STATUS_WARMING_UP        0x10000
#define PRINTER_STATUS_TONER_LOW         0x20000
#define PRINTER_STATUS_NO_TONER          0x40000
#define PRINTER_STATUS_PAGE_PUNT         0x80000
#define PRINTER_STATUS_USER_INTERVENTION 0x100000
#define PRINTER_STATUS_OUT_OF_MEMORY     0x200000
#define PRINTER_STATUS_DOOR_OPEN         0x400000
#define PRINTER_STATUS_SERVER_UNKNOWN    0x800000
#define PRINTER_STATUS_POWER_SAVE        0x1000000

#define PRINTER_ATTRIBUTE_QUEUED            1
#define PRINTER_ATTRIBUTE_DIRECT            2
#define PRINTER_ATTRIBUTE_DEFAULT           4
#define PRINTER_ATTRIBUTE_SHARED            8
#define PRINTER_ATTRIBUTE_NETWORK           0x10
#define PRINTER_ATTRIBUTE_HIDDEN            0x20
#define PRINTER_ATTRIBUTE_LOCAL             0x40
#define PRINTER_ATTRIBUTE_ENABLE_DEVQ       0x80
#define PRINTER_ATTRIBUTE_KEEPPRINTEDJOBS   0x100
#define PRINTER_ATTRIBUTE_DO_COMPLETE_FIRST 0x200
#define PRINTER_ATTRIBUTE_WORK_OFFLINE      0x400
#define PRINTER_ATTRIBUTE_ENABLE_BIDI       0x800
#define PRINTER_ATTRIBUTE_RAW_ONLY          0x1000
#define PRINTER_ATTRIBUTE_PUBLISHED         0x2000

#define PRINTER_ENUM_DEFAULT     1
#define PRINTER_ENUM_LOCAL       2
#define PRINTER_ENUM_CONNECTIONS 4
#define PRINTER_ENUM_FAVORITE    4
#define PRINTER_ENUM_NAME        8
#define PRINTER_ENUM_REMOTE      16
#define PRINTER_ENUM_SHARED      32
#define PRINTER_ENUM_NETWORK     0x40
#define PRINTER_ENUM_EXPAND      0x4000
#define PRINTER_ENUM_CONTAINER   0x8000
#define PRINTER_ENUM_ICONMASK    0xff0000
#define PRINTER_ENUM_ICON1       0x10000
#define PRINTER_ENUM_ICON2       0x20000
#define PRINTER_ENUM_ICON3       0x40000
#define PRINTER_ENUM_ICON4       0x80000
#define PRINTER_ENUM_ICON5       0x100000
#define PRINTER_ENUM_ICON6       0x200000
#define PRINTER_ENUM_ICON7       0x400000
#define PRINTER_ENUM_ICON8       0x800000

#define PRINTER_NOTIFY_TYPE                      0
#define PRINTER_NOTIFY_FIELD_SERVER_NAME         0
#define PRINTER_NOTIFY_FIELD_PRINTER_NAME        1
#define PRINTER_NOTIFY_FIELD_SHARE_NAME          2
#define PRINTER_NOTIFY_FIELD_PORT_NAME           3
#define PRINTER_NOTIFY_FIELD_DRIVER_NAME         4
#define PRINTER_NOTIFY_FIELD_COMMENT             5
#define PRINTER_NOTIFY_FIELD_LOCATION            6
#define PRINTER_NOTIFY_FIELD_DEVMODE             7
#define PRINTER_NOTIFY_FIELD_SEPFILE             8
#define PRINTER_NOTIFY_FIELD_PRINT_PROCESSOR     9
#define PRINTER_NOTIFY_FIELD_PARAMETERS          10
#define PRINTER_NOTIFY_FIELD_DATATYPE            11
#define PRINTER_NOTIFY_FIELD_SECURITY_DESCRIPTOR 12
#define PRINTER_NOTIFY_FIELD_ATTRIBUTES          13
#define PRINTER_NOTIFY_FIELD_PRIORITY            14
#define PRINTER_NOTIFY_FIELD_DEFAULT_PRIORITY    15
#define PRINTER_NOTIFY_FIELD_START_TIME          16
#define PRINTER_NOTIFY_FIELD_UNTIL_TIME          17
#define PRINTER_NOTIFY_FIELD_STATUS              18
#define PRINTER_NOTIFY_FIELD_STATUS_STRING       19
#define PRINTER_NOTIFY_FIELD_CJOBS               20
#define PRINTER_NOTIFY_FIELD_AVERAGE_PPM         21
#define PRINTER_NOTIFY_FIELD_TOTAL_PAGES         22
#define PRINTER_NOTIFY_FIELD_PAGES_PRINTED       23
#define PRINTER_NOTIFY_FIELD_TOTAL_BYTES         24
#define PRINTER_NOTIFY_FIELD_BYTES_PRINTED       25

#define PRINTER_CHANGE_ADD_PRINTER               1
#define PRINTER_CHANGE_SET_PRINTER               2
#define PRINTER_CHANGE_DELETE_PRINTER            4
#define PRINTER_CHANGE_FAILED_CONNECTION_PRINTER 8
#define PRINTER_CHANGE_PRINTER                   0xFF
#define PRINTER_CHANGE_ADD_JOB                   0x100
#define PRINTER_CHANGE_SET_JOB                   0x200
#define PRINTER_CHANGE_DELETE_JOB                0x400
#define PRINTER_CHANGE_WRITE_JOB                 0x800
#define PRINTER_CHANGE_JOB                       0xFF00
#define PRINTER_CHANGE_ADD_FORM                  0x10000
#define PRINTER_CHANGE_SET_FORM                  0x20000
#define PRINTER_CHANGE_DELETE_FORM               0x40000
#define PRINTER_CHANGE_FORM                      0x70000
#define PRINTER_CHANGE_ADD_PORT                  0x100000
#define PRINTER_CHANGE_CONFIGURE_PORT            0x200000
#define PRINTER_CHANGE_DELETE_PORT               0x400000
#define PRINTER_CHANGE_PORT                      0x700000
#define PRINTER_CHANGE_ADD_PRINT_PROCESSOR       0x1000000
#define PRINTER_CHANGE_DELETE_PRINT_PROCESSOR    0x4000000
#define PRINTER_CHANGE_PRINT_PROCESSOR           0x7000000
#define PRINTER_CHANGE_ADD_PRINTER_DRIVER        0x10000000
#define PRINTER_CHANGE_SET_PRINTER_DRIVER        0x20000000
#define PRINTER_CHANGE_DELETE_PRINTER_DRIVER     0x40000000
#define PRINTER_CHANGE_PRINTER_DRIVER            0x70000000
#define PRINTER_CHANGE_TIMEOUT                   0x80000000
#define PRINTER_CHANGE_ALL                       0x7777FFFF

#define PRINTER_NOTIFY_INFO_DISCARDED 1
#define PRINTER_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED|PRINTER_ACCESS_ADMINISTER|PRINTER_ACCESS_USE)
#define PRINTER_READ (STANDARD_RIGHTS_READ|PRINTER_ACCESS_USE)
#define PRINTER_WRITE (STANDARD_RIGHTS_WRITE|PRINTER_ACCESS_USE)
#define PRINTER_EXECUTE (STANDARD_RIGHTS_EXECUTE|PRINTER_ACCESS_USE)

#define NO_PRIORITY  0
#define MAX_PRIORITY 99
#define MIN_PRIORITY 1
#define DEF_PRIORITY 1

#define PORT_TYPE_WRITE        1
#define PORT_TYPE_READ         2
#define PORT_TYPE_REDIRECTED   4
#define PORT_TYPE_NET_ATTACHED 8

#define SERVER_ACCESS_ADMINISTER 1
#define SERVER_ACCESS_ENUMERATE  2
#define SERVER_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED|SERVER_ACCESS_ADMINISTER|SERVER_ACCESS_ENUMERATE)
#define SERVER_READ (STANDARD_RIGHTS_READ|SERVER_ACCESS_ENUMERATE)
#define SERVER_WRITE (STANDARD_RIGHTS_WRITE|SERVER_ACCESS_ADMINISTER|SERVER_ACCESS_ENUMERATE)
#define SERVER_EXECUTE (STANDARD_RIGHTS_EXECUTE|SERVER_ACCESS_ENUMERATE)

#define PORT_STATUS_TYPE_ERROR        1
#define PORT_STATUS_TYPE_WARNING      2
#define PORT_STATUS_TYPE_INFO         3
#define PORT_STATUS_OFFLINE           1
#define PORT_STATUS_PAPER_JAM         2
#define PORT_STATUS_PAPER_OUT         3
#define PORT_STATUS_OUTPUT_BIN_FULL   4
#define PORT_STATUS_PAPER_PROBLEM     5
#define PORT_STATUS_NO_TONER          6
#define PORT_STATUS_DOOR_OPEN         7
#define PORT_STATUS_USER_INTERVENTION 8
#define PORT_STATUS_OUT_OF_MEMORY     9
#define PORT_STATUS_TONER_LOW         10
#define PORT_STATUS_WARMING_UP        11
#define PORT_STATUS_POWER_SAVE        12

typedef struct _PRINTER_NOTIFY_OPTIONS_TYPE {
  WORD Type;
  WORD Reserved0;
  DWORD Reserved1;
  DWORD Reserved2;
  DWORD Count;
  PWORD pFields;
} PRINTER_NOTIFY_OPTIONS_TYPE, *PPRINTER_NOTIFY_OPTIONS_TYPE, *LPPRINTER_NOTIFY_OPTIONS_TYPE;

typedef struct _PRINTER_NOTIFY_OPTIONS {
  DWORD Version;
  DWORD Flags;
  DWORD Count;
  PPRINTER_NOTIFY_OPTIONS_TYPE pTypes;
} PRINTER_NOTIFY_OPTIONS, *PPRINTER_NOTIFY_OPTIONS, *LPPRINTER_NOTIFY_OPTIONS;

#ifndef RC_INVOKED

typedef struct _ADDJOB_INFO_1A {
  LPSTR Path;
  DWORD JobId;
} ADDJOB_INFO_1A, *PADDJOB_INFO_1A, *LPADDJOB_INFO_1A;

typedef struct _ADDJOB_INFO_1W {
  LPWSTR Path;
  DWORD JobId;
} ADDJOB_INFO_1W, *PADDJOB_INFO_1W, *LPADDJOB_INFO_1W;

typedef struct _DATATYPES_INFO_1A {
  LPSTR pName;
} DATATYPES_INFO_1A, *PDATATYPES_INFO_1A, *LPDATATYPES_INFO_1A;

typedef struct _DATATYPES_INFO_1W {
  LPWSTR pName;
} DATATYPES_INFO_1W, *PDATATYPES_INFO_1W, *LPDATATYPES_INFO_1W;

typedef struct _JOB_INFO_1A {
  DWORD JobId;
  LPSTR pPrinterName;
  LPSTR pMachineName;
  LPSTR pUserName;
  LPSTR pDocument;
  LPSTR pDatatype;
  LPSTR pStatus;
  DWORD Status;
  DWORD Priority;
  DWORD Position;
  DWORD TotalPages;
  DWORD PagesPrinted;
  SYSTEMTIME Submitted;
} JOB_INFO_1A, *PJOB_INFO_1A, *LPJOB_INFO_1A;

typedef struct _JOB_INFO_1W {
  DWORD JobId;
  LPWSTR pPrinterName;
  LPWSTR pMachineName;
  LPWSTR pUserName;
  LPWSTR pDocument;
  LPWSTR pDatatype;
  LPWSTR pStatus;
  DWORD Status;
  DWORD Priority;
  DWORD Position;
  DWORD TotalPages;
  DWORD PagesPrinted;
  SYSTEMTIME Submitted;
} JOB_INFO_1W, *PJOB_INFO_1W, *LPJOB_INFO_1W;

typedef struct _JOB_INFO_2A {
  DWORD JobId;
  LPSTR pPrinterName;
  LPSTR pMachineName;
  LPSTR pUserName;
  LPSTR pDocument;
  LPSTR pNotifyName;
  LPSTR pDatatype;
  LPSTR pPrintProcessor;
  LPSTR pParameters;
  LPSTR pDriverName;
  LPDEVMODEA pDevMode;
  LPSTR pStatus;
  PSECURITY_DESCRIPTOR pSecurityDescriptor;
  DWORD Status;
  DWORD Priority;
  DWORD Position;
  DWORD StartTime;
  DWORD UntilTime;
  DWORD TotalPages;
  DWORD Size;
  SYSTEMTIME Submitted;
  DWORD Time;
  DWORD PagesPrinted;
} JOB_INFO_2A, *PJOB_INFO_2A, *LPJOB_INFO_2A;

typedef struct _JOB_INFO_2W {
  DWORD JobId;
  LPWSTR pPrinterName;
  LPWSTR pMachineName;
  LPWSTR pUserName;
  LPWSTR pDocument;
  LPWSTR pNotifyName;
  LPWSTR pDatatype;
  LPWSTR pPrintProcessor;
  LPWSTR pParameters;
  LPWSTR pDriverName;
  LPDEVMODEW pDevMode;
  LPWSTR pStatus;
  PSECURITY_DESCRIPTOR pSecurityDescriptor;
  DWORD Status;
  DWORD Priority;
  DWORD Position;
  DWORD StartTime;
  DWORD UntilTime;
  DWORD TotalPages;
  DWORD Size;
  SYSTEMTIME Submitted;
  DWORD Time;
  DWORD PagesPrinted;
} JOB_INFO_2W, *PJOB_INFO_2W, *LPJOB_INFO_2W;

typedef struct _JOB_INFO_3 {
  DWORD JobId;
  DWORD NextJobId;
  DWORD Reserved;
} JOB_INFO_3, *PJOB_INFO_3, *LPJOB_INFO_3;

typedef struct _JOB_INFO_4A {
  DWORD JobId;
  LPSTR pPrinterName;
  LPSTR pMachineName;
  LPSTR pUserName;
  LPSTR pDocument;
  LPSTR pNotifyName;
  LPSTR pDatatype;
  LPSTR pPrintProcessor;
  LPSTR pParameters;
  LPSTR pDriverName;
  LPDEVMODEA pDevMode;
  LPSTR pStatus;
  PSECURITY_DESCRIPTOR pSecurityDescriptor;
  DWORD Status;
  DWORD Priority;
  DWORD Position;
  DWORD StartTime;
  DWORD UntilTime;
  DWORD TotalPages;
  DWORD Size;
  SYSTEMTIME Submitted;
  DWORD Time;
  DWORD PagesPrinted;
  LONG SizeHigh;
} JOB_INFO_4A, *PJOB_INFO_4A, *LPJOB_INFO_4A;

typedef struct _JOB_INFO_4W {
  DWORD JobId;
  LPWSTR pPrinterName;
  LPWSTR pMachineName;
  LPWSTR pUserName;
  LPWSTR pDocument;
  LPWSTR pNotifyName;
  LPWSTR pDatatype;
  LPWSTR pPrintProcessor;
  LPWSTR pParameters;
  LPWSTR pDriverName;
  LPDEVMODEW pDevMode;
  LPWSTR pStatus;
  PSECURITY_DESCRIPTOR pSecurityDescriptor;
  DWORD Status;
  DWORD Priority;
  DWORD Position;
  DWORD StartTime;
  DWORD UntilTime;
  DWORD TotalPages;
  DWORD Size;
  SYSTEMTIME Submitted;
  DWORD Time;
  DWORD PagesPrinted;
  LONG SizeHigh;
} JOB_INFO_4W, *PJOB_INFO_4W, *LPJOB_INFO_4W;

typedef struct _DOC_INFO_1A {
  LPSTR pDocName;
  LPSTR pOutputFile;
  LPSTR pDatatype;
} DOC_INFO_1A, *PDOC_INFO_1A, *LPDOC_INFO_1A;

typedef struct _DOC_INFO_1W {
  LPWSTR pDocName;
  LPWSTR pOutputFile;
  LPWSTR pDatatype;
} DOC_INFO_1W, *PDOC_INFO_1W, *LPDOC_INFO_1W;

typedef struct _DOC_INFO_2A {
  LPSTR pDocName;
  LPSTR pOutputFile;
  LPSTR pDatatype;
  DWORD dwMode;
  DWORD JobId;
} DOC_INFO_2A, *PDOC_INFO_2A, *LPDOC_INFO_2A;

typedef struct _DOC_INFO_2W {
  LPWSTR pDocName;
  LPWSTR pOutputFile;
  LPWSTR pDatatype;
  DWORD dwMode;
  DWORD JobId;
} DOC_INFO_2W, *PDOC_INFO_2W, *LPDOC_INFO_2W;

typedef struct _DRIVER_INFO_1A {
  LPSTR pName;
} DRIVER_INFO_1A, *PDRIVER_INFO_1A, *LPDRIVER_INFO_1A;

typedef struct _DRIVER_INFO_1W {
  LPWSTR pName;
} DRIVER_INFO_1W, *PDRIVER_INFO_1W, *LPDRIVER_INFO_1W;

typedef struct _DRIVER_INFO_2A {
  DWORD cVersion;
  LPSTR pName;
  LPSTR pEnvironment;
  LPSTR pDriverPath;
  LPSTR pDataFile;
  LPSTR pConfigFile;
} DRIVER_INFO_2A, *PDRIVER_INFO_2A, *LPDRIVER_INFO_2A;

typedef struct _DRIVER_INFO_2W {
  DWORD cVersion;
  LPWSTR pName;
  LPWSTR pEnvironment;
  LPWSTR pDriverPath;
  LPWSTR pDataFile;
  LPWSTR pConfigFile;
} DRIVER_INFO_2W, *PDRIVER_INFO_2W, *LPDRIVER_INFO_2W;

typedef struct _DRIVER_INFO_3A {
  DWORD cVersion;
  LPSTR pName;
  LPSTR pEnvironment;
  LPSTR pDriverPath;
  LPSTR pDataFile;
  LPSTR pConfigFile;
  LPSTR pHelpFile;
  LPSTR pDependentFiles;
  LPSTR pMonitorName;
  LPSTR pDefaultDataType;
} DRIVER_INFO_3A, *PDRIVER_INFO_3A, *LPDRIVER_INFO_3A;

typedef struct _DRIVER_INFO_3W {
  DWORD cVersion;
  LPWSTR pName;
  LPWSTR pEnvironment;
  LPWSTR pDriverPath;
  LPWSTR pDataFile;
  LPWSTR pConfigFile;
  LPWSTR pHelpFile;
  LPWSTR pDependentFiles;
  LPWSTR pMonitorName;
  LPWSTR pDefaultDataType;
} DRIVER_INFO_3W, *PDRIVER_INFO_3W, *LPDRIVER_INFO_3W;

typedef struct _DRIVER_INFO_4A {
  DWORD cVersion;          // SDK examples:
  LPSTR pName;             // QMS 810
  LPSTR pEnvironment;      // Win32 x86
  LPSTR pDriverPath;       // c:\drivers\pscript.dll
  LPSTR pDataFile;         // c:\drivers\QMS810.PPD
  LPSTR pConfigFile;       // c:\drivers\PSCRPTUI.DLL
  LPSTR pHelpFile;         // c:\drivers\PSCRPTUI.HLP
  LPSTR pDependentFiles;   // PSCRIPT.DLL\0QMS810.PPD\0PSCRIPTUI.DLL\0PSCRIPTUI.HLP\0PSTEST.TXT\0\0
  LPSTR pMonitorName;      // "PJL monitor"
  LPSTR pDefaultDataType;  // "EMF"
  LPSTR pszzPreviousNames; // "OldName1\0OldName2\0\0
} DRIVER_INFO_4A, *PDRIVER_INFO_4A, *LPDRIVER_INFO_4A;

typedef struct _DRIVER_INFO_4W {
  DWORD cVersion;
  LPWSTR pName;
  LPWSTR pEnvironment;
  LPWSTR pDriverPath;
  LPWSTR pDataFile;
  LPWSTR pConfigFile;
  LPWSTR pHelpFile;
  LPWSTR pDependentFiles;
  LPWSTR pMonitorName;
  LPWSTR pDefaultDataType;
  LPWSTR pszzPreviousNames;
} DRIVER_INFO_4W, *PDRIVER_INFO_4W, *LPDRIVER_INFO_4W;

typedef struct _DRIVER_INFO_5A {
  DWORD cVersion;           // SDK examples:
  LPSTR pName;              // QMS 810
  LPSTR pEnvironment;       // Win32 x86
  LPSTR pDriverPath;        // c:\drivers\pscript.dll
  LPSTR pDataFile;          // c:\drivers\QMS810.PPD
  LPSTR pConfigFile;        // c:\drivers\PSCRPTUI.DLL
  DWORD dwDriverAttributes; // driver attributes (like UMPD/KMPD)
  DWORD dwConfigVersion;    // version number of the config file since reboot
  DWORD dwDriverVersion;    // version number of the driver file since reboot
} DRIVER_INFO_5A, *PDRIVER_INFO_5A, *LPDRIVER_INFO_5A;

typedef struct _DRIVER_INFO_5W {
  DWORD cVersion;
  LPWSTR pName;
  LPWSTR pEnvironment;
  LPWSTR pDriverPath;
  LPWSTR pDataFile;
  LPWSTR pConfigFile;
  DWORD dwDriverAttributes;
  DWORD dwConfigVersion;
  DWORD dwDriverVersion;
} DRIVER_INFO_5W, *PDRIVER_INFO_5W, *LPDRIVER_INFO_5W;

typedef struct _DRIVER_INFO_6A {
  DWORD cVersion;
  LPSTR pName;
  LPSTR pEnvironment;
  LPSTR pDriverPath;
  LPSTR pDataFile;
  LPSTR pConfigFile;
  LPSTR pHelpFile;
  LPSTR pDependentFiles;
  LPSTR pMonitorName;
  LPSTR pDefaultDataType;
  LPSTR pszzPreviousNames;
  FILETIME ftDriverDate;
  DWORDLONG dwlDriverVersion;
  LPSTR pszMfgName;
  LPSTR pszOEMUrl;
  LPSTR pszHardwareID;
  LPSTR pszProvider;
} DRIVER_INFO_6A, *PDRIVER_INFO_6A, *LPDRIVER_INFO_6A;

typedef struct _DRIVER_INFO_6W {
  DWORD cVersion;
  LPWSTR pName;
  LPWSTR pEnvironment;
  LPWSTR pDriverPath;
  LPWSTR pDataFile;
  LPWSTR pConfigFile;
  LPWSTR pHelpFile;
  LPWSTR pDependentFiles;
  LPWSTR pMonitorName;
  LPWSTR pDefaultDataType;
  LPWSTR pszzPreviousNames;
  FILETIME ftDriverDate;
  DWORDLONG dwlDriverVersion;
  LPWSTR pszMfgName;
  LPWSTR pszOEMUrl;
  LPWSTR pszHardwareID;
  LPWSTR pszProvider;
} DRIVER_INFO_6W, *PDRIVER_INFO_6W, *LPDRIVER_INFO_6W;

#define PRINTER_DRIVER_PACKAGE_AWARE 0x00000001

typedef struct _DRIVER_INFO_8A {
  DWORD cVersion;
  LPSTR pName;
  LPSTR pEnvironment;
  LPSTR pDriverPath;
  LPSTR pDataFile;
  LPSTR pConfigFile;
  LPSTR pHelpFile;
  LPSTR pDependentFiles;
  LPSTR pMonitorName;
  LPSTR pDefaultDataType;
  LPSTR pszzPreviousNames;
  FILETIME ftDriverDate;
  DWORDLONG dwlDriverVersion;
  LPSTR pszMfgName;
  LPSTR pszOEMUrl;
  LPSTR pszHardwareID;
  LPSTR pszProvider;
  LPSTR pszPrintProcessor;
  LPSTR pszVendorSetup;
  LPSTR pszzColorProfiles;
  LPSTR pszInfPath;
  DWORD dwPrinterDriverAttributes;
  LPSTR pszzCoreDriverDependencies;
  FILETIME ftMinInboxDriverVerDate;
  DWORDLONG dwlMinInboxDriverVerVersion;
} DRIVER_INFO_8A, *PDRIVER_INFO_8A, *LPDRIVER_INFO_8A;

typedef struct _DRIVER_INFO_8W {
  DWORD cVersion;
  LPWSTR pName;
  LPWSTR pEnvironment;
  LPWSTR pDriverPath;
  LPWSTR pDataFile;
  LPWSTR pConfigFile;
  LPWSTR pHelpFile;
  LPWSTR pDependentFiles;
  LPWSTR pMonitorName;
  LPWSTR pDefaultDataType;
  LPWSTR pszzPreviousNames;
  FILETIME ftDriverDate;
  DWORDLONG dwlDriverVersion;
  LPWSTR pszMfgName;
  LPWSTR pszOEMUrl;
  LPWSTR pszHardwareID;
  LPWSTR pszProvider;
  LPWSTR pszPrintProcessor;
  LPWSTR pszVendorSetup;
  LPWSTR pszzColorProfiles;
  LPWSTR pszInfPath;
  DWORD dwPrinterDriverAttributes;
  LPWSTR pszzCoreDriverDependencies;
  FILETIME ftMinInboxDriverVerDate;
  DWORDLONG dwlMinInboxDriverVerVersion;
} DRIVER_INFO_8W, *PDRIVER_INFO_8W, *LPDRIVER_INFO_8W;

// FLAGS for dwDriverAttributes
#define DRIVER_KERNELMODE 0x00000001
#define DRIVER_USERMODE   0x00000002

// FLAGS for DeletePrinterDriverEx.
#define DPD_DELETE_UNUSED_FILES     0x00000001
#define DPD_DELETE_SPECIFIC_VERSION 0x00000002
#define DPD_DELETE_ALL_FILES        0x00000004

// FLAGS for AddPrinterDriverEx.
#define APD_STRICT_UPGRADE      0x00000001
#define APD_STRICT_DOWNGRADE    0x00000002
#define APD_COPY_ALL_FILES      0x00000004
#define APD_COPY_NEW_FILES      0x00000008
#if(_WIN32_WINNT >= 0x0501)
#define APD_COPY_FROM_DIRECTORY 0x00000010
#endif

typedef struct _MONITOR_INFO_1A {
  LPSTR pName;
} MONITOR_INFO_1A, *PMONITOR_INFO_1A, *LPMONITOR_INFO_1A;

typedef struct _MONITOR_INFO_1W {
  LPWSTR pName;
} MONITOR_INFO_1W, *PMONITOR_INFO_1W, *LPMONITOR_INFO_1W;

typedef struct _PORT_INFO_1A {
  LPSTR pName;
} PORT_INFO_1A, *PPORT_INFO_1A, *LPPORT_INFO_1A;

typedef struct _PORT_INFO_1W {
  LPWSTR pName;
} PORT_INFO_1W, *PPORT_INFO_1W, *LPPORT_INFO_1W;

typedef struct _MONITOR_INFO_2A {
  LPSTR pName;
  LPSTR pEnvironment;
  LPSTR pDLLName;
} MONITOR_INFO_2A, *PMONITOR_INFO_2A, *LPMONITOR_INFO_2A;

typedef struct _MONITOR_INFO_2W {
  LPWSTR pName;
  LPWSTR pEnvironment;
  LPWSTR pDLLName;
} MONITOR_INFO_2W, *PMONITOR_INFO_2W, *LPMONITOR_INFO_2W;

typedef struct _PORT_INFO_2A {
  LPSTR pPortName;
  LPSTR pMonitorName;
  LPSTR pDescription;
  DWORD fPortType;
  DWORD Reserved;
} PORT_INFO_2A, *PPORT_INFO_2A, *LPPORT_INFO_2A;

typedef struct _PORT_INFO_2W {
  LPWSTR pPortName;
  LPWSTR pMonitorName;
  LPWSTR pDescription;
  DWORD fPortType;
  DWORD Reserved;
} PORT_INFO_2W, *PPORT_INFO_2W, *LPPORT_INFO_2W;

typedef struct _PORT_INFO_3A {
  DWORD dwStatus;
  LPSTR pszStatus;
  DWORD dwSeverity;
} PORT_INFO_3A, *PPORT_INFO_3A, *LPPORT_INFO_3A;

typedef struct _PORT_INFO_3W {
  DWORD dwStatus;
  LPWSTR pszStatus;
  DWORD dwSeverity;
} PORT_INFO_3W, *PPORT_INFO_3W, *LPPORT_INFO_3W;

typedef struct _PRINTER_INFO_1A {
  DWORD Flags;
  LPSTR pDescription;
  LPSTR pName;
  LPSTR pComment;
} PRINTER_INFO_1A, *PPRINTER_INFO_1A, *LPPRINTER_INFO_1A;

typedef struct _PRINTER_INFO_1W {
  DWORD Flags;
  LPWSTR pDescription;
  LPWSTR pName;
  LPWSTR pComment;
} PRINTER_INFO_1W, *PPRINTER_INFO_1W, *LPPRINTER_INFO_1W;

typedef struct _PRINTER_INFO_2A {
  LPSTR pServerName;
  LPSTR pPrinterName;
  LPSTR pShareName;
  LPSTR pPortName;
  LPSTR pDriverName;
  LPSTR pComment;
  LPSTR pLocation;
  LPDEVMODEA pDevMode;
  LPSTR pSepFile;
  LPSTR pPrintProcessor;
  LPSTR pDatatype;
  LPSTR pParameters;
  PSECURITY_DESCRIPTOR pSecurityDescriptor;
  DWORD Attributes;
  DWORD Priority;
  DWORD DefaultPriority;
  DWORD StartTime;
  DWORD UntilTime;
  DWORD Status;
  DWORD cJobs;
  DWORD AveragePPM;
} PRINTER_INFO_2A, *PPRINTER_INFO_2A, *LPPRINTER_INFO_2A;

typedef struct _PRINTER_INFO_2W {
  LPWSTR pServerName;
  LPWSTR pPrinterName;
  LPWSTR pShareName;
  LPWSTR pPortName;
  LPWSTR pDriverName;
  LPWSTR pComment;
  LPWSTR pLocation;
  LPDEVMODEW pDevMode;
  LPWSTR pSepFile;
  LPWSTR pPrintProcessor;
  LPWSTR pDatatype;
  LPWSTR pParameters;
  PSECURITY_DESCRIPTOR pSecurityDescriptor;
  DWORD Attributes;
  DWORD Priority;
  DWORD DefaultPriority;
  DWORD StartTime;
  DWORD UntilTime;
  DWORD Status;
  DWORD cJobs;
  DWORD AveragePPM;
} PRINTER_INFO_2W, *PPRINTER_INFO_2W, *LPPRINTER_INFO_2W;

typedef struct _PRINTER_INFO_3 {
  PSECURITY_DESCRIPTOR pSecurityDescriptor;
} PRINTER_INFO_3, *PPRINTER_INFO_3, *LPPRINTER_INFO_3;

typedef struct _PRINTER_INFO_4A {
  LPSTR pPrinterName;
  LPSTR pServerName;
  DWORD Attributes;
} PRINTER_INFO_4A, *PPRINTER_INFO_4A, *LPPRINTER_INFO_4A;

typedef struct _PRINTER_INFO_4W {
  LPWSTR pPrinterName;
  LPWSTR pServerName;
  DWORD Attributes;
} PRINTER_INFO_4W, *PPRINTER_INFO_4W, *LPPRINTER_INFO_4W;

typedef struct _PRINTER_INFO_5A {
  LPSTR pPrinterName;
  LPSTR pPortName;
  DWORD Attributes;
  DWORD DeviceNotSelectedTimeout;
  DWORD TransmissionRetryTimeout;
} PRINTER_INFO_5A, *PPRINTER_INFO_5A, *LPPRINTER_INFO_5A;

typedef struct _PRINTER_INFO_5W {
  LPWSTR pPrinterName;
  LPWSTR pPortName;
  DWORD Attributes;
  DWORD DeviceNotSelectedTimeout;
  DWORD TransmissionRetryTimeout;
} PRINTER_INFO_5W, *PPRINTER_INFO_5W, *LPPRINTER_INFO_5W;

typedef struct _PRINTER_INFO_6 {
  DWORD dwStatus;
} PRINTER_INFO_6, *PPRINTER_INFO_6, *LPPRINTER_INFO_6;

typedef struct _PRINTER_INFO_7A {
  LPSTR pszObjectGUID;
  DWORD dwAction;
} PRINTER_INFO_7A, *PPRINTER_INFO_7A, *LPPRINTER_INFO_7A;

typedef struct _PRINTER_INFO_7W {
  LPWSTR pszObjectGUID;
  DWORD dwAction;
} PRINTER_INFO_7W, *PPRINTER_INFO_7W, *LPPRINTER_INFO_7W;

typedef struct _PRINTER_INFO_8A {
  LPDEVMODEA pDevMode;
} PRINTER_INFO_8A, *PPRINTER_INFO_8A, *LPPRINTER_INFO_8A;

typedef struct _PRINTER_INFO_8W {
  LPDEVMODEW pDevMode;
} PRINTER_INFO_8W, *PPRINTER_INFO_8W, *LPPRINTER_INFO_8W;

typedef struct _PRINTER_INFO_9A {
  LPDEVMODEA pDevMode;
} PRINTER_INFO_9A, *PPRINTER_INFO_9A, *LPPRINTER_INFO_9A;

typedef struct _PRINTER_INFO_9W {
  LPDEVMODEW pDevMode;
} PRINTER_INFO_9W, *PPRINTER_INFO_9W, *LPPRINTER_INFO_9W;

typedef struct _PRINTPROCESSOR_INFO_1A {
  LPSTR pName;
} PRINTPROCESSOR_INFO_1A, *PPRINTPROCESSOR_INFO_1A, *LPPRINTPROCESSOR_INFO_1A;

typedef struct _PRINTPROCESSOR_INFO_1W {
  LPWSTR pName;
} PRINTPROCESSOR_INFO_1W, *PPRINTPROCESSOR_INFO_1W, *LPPRINTPROCESSOR_INFO_1W;

typedef struct _PRINTER_NOTIFY_INFO_DATA {
  WORD Type;
  WORD Field;
  DWORD Reserved;
  DWORD Id;
  union {
    DWORD adwData[2];
    struct {
      DWORD cbBuf;
      PVOID pBuf;
    } Data;
  } NotifyData;
} PRINTER_NOTIFY_INFO_DATA, *PPRINTER_NOTIFY_INFO_DATA, *LPPRINTER_NOTIFY_INFO_DATA;

typedef struct _PRINTER_NOTIFY_INFO {
  DWORD Version;
  DWORD Flags;
  DWORD Count;
  PRINTER_NOTIFY_INFO_DATA aData[1];
} PRINTER_NOTIFY_INFO, *PPRINTER_NOTIFY_INFO, *LPPRINTER_NOTIFY_INFO;

typedef struct _FORM_INFO_1A {
  DWORD Flags;
  LPSTR pName;
  SIZEL Size;
  RECTL ImageableArea;
} FORM_INFO_1A, *PFORM_INFO_1A, *LPFORM_INFO_1A;

typedef struct _FORM_INFO_1W {
  DWORD Flags;
  LPWSTR pName;
  SIZEL Size;
  RECTL ImageableArea;
} FORM_INFO_1W, *PFORM_INFO_1W, *LPFORM_INFO_1W;

#if (NTDDI_VERSION >= NTDDI_VISTA)

typedef struct _FORM_INFO_2A {
  DWORD Flags;
  LPCSTR pName;
  SIZEL Size;
  RECTL ImageableArea;
  LPCSTR pKeyword;
  DWORD StringType;
  LPCSTR pMuiDll;
  DWORD dwResourceId;
  LPCSTR pDisplayName;
  LANGID wLangId;
} FORM_INFO_2A, *PFORM_INFO_2A, *LPFORM_INFO_2A;

typedef struct _FORM_INFO_2W {
  DWORD Flags;
  LPCWSTR pName;
  SIZEL Size;
  RECTL ImageableArea;
  LPCSTR pKeyword;
  DWORD StringType;
  LPCWSTR pMuiDll;
  DWORD dwResourceId;
  LPCWSTR pDisplayName;
  LANGID wLangId;
} FORM_INFO_2W, *PFORM_INFO_2W, *LPFORM_INFO_2W;

#endif /* (NTDDI_VERSION >= NTDDI_VISTA) */

typedef struct _PRINTER_DEFAULTSA {
  LPSTR pDatatype;
  LPDEVMODE pDevMode;
  ACCESS_MASK DesiredAccess;
} PRINTER_DEFAULTSA, *PPRINTER_DEFAULTSA, *LPPRINTER_DEFAULTSA;

typedef struct _PRINTER_DEFAULTSW {
  LPWSTR pDatatype;
  LPDEVMODE pDevMode;
  ACCESS_MASK DesiredAccess;
} PRINTER_DEFAULTSW, *PPRINTER_DEFAULTSW, *LPPRINTER_DEFAULTSW;

typedef struct _PROVIDOR_INFO_1A {
  LPSTR pName;
  LPSTR pEnvironment;
  LPSTR pDLLName;
} PROVIDOR_INFO_1A, *PPROVIDOR_INFO_1A, *LPPROVIDOR_INFO_1A;

typedef struct _PROVIDOR_INFO_1W {
  LPWSTR pName;
  LPWSTR pEnvironment;
  LPWSTR pDLLName;
} PROVIDOR_INFO_1W, *LPPROVIDOR_INFO_1W;

typedef struct _PROVIDOR_INFO_2A {
  LPSTR pOrder;
} PROVIDOR_INFO_2A, *PPROVIDOR_INFO_2A, *LPPROVIDOR_INFO_2A;

typedef struct _PROVIDOR_INFO_2W {
  LPWSTR pOrder;
} PROVIDOR_INFO_2W, *LPPROVIDOR_INFO_2W;

typedef struct _BINARY_CONTAINER {
  DWORD cbBuf;
  LPBYTE pData;
} BINARY_CONTAINER, *PBINARY_CONTAINER;

typedef struct _BIDI_DATA {
  DWORD dwBidiType;
  union {
    BOOL bData;
    INT iData;
    LPWSTR sData;
    FLOAT fData;
    BINARY_CONTAINER biData;
  } u;
} BIDI_DATA, *LPBIDI_DATA, *PBIDI_DATA;

typedef struct _BIDI_REQUEST_DATA {
  DWORD dwReqNumber;
  LPWSTR pSchema;
  BIDI_DATA data;
} BIDI_REQUEST_DATA, *LPBIDI_REQUEST_DATA, *PBIDI_REQUEST_DATA;

typedef struct _BIDI_REQUEST_CONTAINER {
  DWORD Version;
  DWORD Flags;
  DWORD Count;
  BIDI_REQUEST_DATA aData[1];
} BIDI_REQUEST_CONTAINER, *LPBIDI_REQUEST_CONTAINER, *PBIDI_REQUEST_CONTAINER;

typedef struct _BIDI_RESPONSE_DATA {
  DWORD dwResult;
  DWORD dwReqNumber;
  LPWSTR pSchema;
  BIDI_DATA data;
} BIDI_RESPONSE_DATA, *LPBIDI_RESPONSE_DATA, *PBIDI_RESPONSE_DATA;

typedef struct _BIDI_RESPONSE_CONTAINER {
  DWORD Version;
  DWORD Flags;
  DWORD Count;
  BIDI_RESPONSE_DATA aData[1];
} BIDI_RESPONSE_CONTAINER, *LPBIDI_RESPONSE_CONTAINER, *PBIDI_RESPONSE_CONTAINER;

#ifdef UNICODE

typedef JOB_INFO_1W JOB_INFO_1, *PJOB_INFO_1, *LPJOB_INFO_1;
typedef JOB_INFO_2W JOB_INFO_2, *PJOB_INFO_2, *LPJOB_INFO_2;
typedef JOB_INFO_4W JOB_INFO_4, *PJOB_INFO_4, *LPJOB_INFO_4;
typedef ADDJOB_INFO_1W ADDJOB_INFO_1, *PADDJOB_INFO_1, *LPADDJOB_INFO_1;
typedef DATATYPES_INFO_1W DATATYPES_INFO_1, *PDATATYPES_INFO_1, *LPDATATYPES_INFO_1;
typedef MONITOR_INFO_1W MONITOR_INFO_1, *PMONITOR_INFO_1, *LPMONITOR_INFO_1;
typedef MONITOR_INFO_2W MONITOR_INFO_2, *PMONITOR_INFO_2, *LPMONITOR_INFO_2;
typedef DOC_INFO_1W DOC_INFO_1, *PDOC_INFO_1, *LPDOC_INFO_1;
typedef DOC_INFO_2W DOC_INFO_2, *PDOC_INFO_2, *LPDOC_INFO_2;
typedef PORT_INFO_1W PORT_INFO_1, *PPORT_INFO_1, *LPPORT_INFO_1;
typedef PORT_INFO_2W PORT_INFO_2, *PPORT_INFO_2, *LPPORT_INFO_2;
typedef PORT_INFO_3W PORT_INFO_3, *PPORT_INFO_3, *LPPORT_INFO_3;
typedef DRIVER_INFO_2W DRIVER_INFO_2, *PDRIVER_INFO_2, *LPDRIVER_INFO_2;
typedef DRIVER_INFO_4W DRIVER_INFO_4, *PDRIVER_INFO_4, *LPDRIVER_INFO_4;
typedef DRIVER_INFO_5W DRIVER_INFO_5, *PDRIVER_INFO_5, *LPDRIVER_INFO_5;
typedef DRIVER_INFO_6W DRIVER_INFO_6, *PDRIVER_INFO_6, *LPDRIVER_INFO_6;
typedef DRIVER_INFO_8W DRIVER_INFO_8, *PDRIVER_INFO_8, *LPDRIVER_INFO_8;
typedef PRINTER_INFO_1W PRINTER_INFO_1, *PPRINTER_INFO_1, *LPPRINTER_INFO_1;
typedef PRINTER_INFO_2W PRINTER_INFO_2, *PPRINTER_INFO_2, *LPPRINTER_INFO_2;
typedef PRINTER_INFO_4W PRINTER_INFO_4, *PPRINTER_INFO_4, *LPPRINTER_INFO_4;
typedef PRINTER_INFO_5W PRINTER_INFO_5, *PPRINTER_INFO_5, *LPPRINTER_INFO_5;
typedef PRINTER_INFO_7W PRINTER_INFO_7, *PPRINTER_INFO_7, *LPPRINTER_INFO_7;
typedef PRINTER_INFO_8W PRINTER_INFO_8, *PPRINTER_INFO_8, *LPPRINTER_INFO_8;
typedef PRINTER_INFO_9W PRINTER_INFO_9, *PPRINTER_INFO_9, *LPPRINTER_INFO_9;
typedef PRINTPROCESSOR_INFO_1W PRINTPROCESSOR_INFO_1, *PPRINTPROCESSOR_INFO_1, *LPPRINTPROCESSOR_INFO_1;
typedef FORM_INFO_1W FORM_INFO_1, *PFORM_INFO_1, *LPFORM_INFO_1;
#if (NTDDI_VERSION >= NTDDI_VISTA)
typedef FORM_INFO_2W FORM_INFO_2, *PFORM_INFO_2, *LPFORM_INFO_2;
#endif
typedef PRINTER_DEFAULTSW PRINTER_DEFAULTS, *PPRINTER_DEFAULTS, *LPPRINTER_DEFAULTS;
typedef PROVIDOR_INFO_1W PROVIDOR_INFO_1, *PPROVIDOR_INFO_1, *LPPROVIDOR_INFO_1;
typedef PROVIDOR_INFO_2W PROVIDOR_INFO_2, *PPROVIDOR_INFO_2, *LPPROVIDOR_INFO_2;

#else /* UNICODE */

typedef JOB_INFO_1A JOB_INFO_1, *PJOB_INFO_1, *LPJOB_INFO_1;
typedef JOB_INFO_2A JOB_INFO_2, *PJOB_INFO_2, *LPJOB_INFO_2;
typedef JOB_INFO_4A JOB_INFO_4, *PJOB_INFO_4, *LPJOB_INFO_4;
typedef ADDJOB_INFO_1A ADDJOB_INFO_1, *PADDJOB_INFO_1, *LPADDJOB_INFO_1;
typedef DATATYPES_INFO_1A DATATYPES_INFO_1, *PDATATYPES_INFO_1, *LPDATATYPES_INFO_1;
typedef MONITOR_INFO_1A MONITOR_INFO_1, *PMONITOR_INFO_1, *LPMONITOR_INFO_1;
typedef MONITOR_INFO_2A MONITOR_INFO_2, *PMONITOR_INFO_2, *LPMONITOR_INFO_2;
typedef DOC_INFO_1A DOC_INFO_1, *PDOC_INFO_1, *LPDOC_INFO_1;
typedef DOC_INFO_2A DOC_INFO_2, *PDOC_INFO_2, *LPDOC_INFO_2;
typedef PORT_INFO_1A PORT_INFO_1, *PPORT_INFO_1, *LPPORT_INFO_1;
typedef PORT_INFO_2A PORT_INFO_2, *PPORT_INFO_2, *LPPORT_INFO_2;
typedef PORT_INFO_3A PORT_INFO_3, *PPORT_INFO_3, *LPPORT_INFO_3;
typedef DRIVER_INFO_2A DRIVER_INFO_2, *PDRIVER_INFO_2, *LPDRIVER_INFO_2;
typedef DRIVER_INFO_4A DRIVER_INFO_4, *PDRIVER_INFO_4, *LPDRIVER_INFO_4;
typedef DRIVER_INFO_5A DRIVER_INFO_5, *PDRIVER_INFO_5, *LPDRIVER_INFO_5;
typedef DRIVER_INFO_6A DRIVER_INFO_6, *PDRIVER_INFO_6, *LPDRIVER_INFO_6;
typedef DRIVER_INFO_8A DRIVER_INFO_8, *PDRIVER_INFO_8, *LPDRIVER_INFO_8;
typedef PRINTER_INFO_1A PRINTER_INFO_1, *PPRINTER_INFO_1, *LPPRINTER_INFO_1;
typedef PRINTER_INFO_2A PRINTER_INFO_2, *PPRINTER_INFO_2, *LPPRINTER_INFO_2;
typedef PRINTER_INFO_4A PRINTER_INFO_4, *PPRINTER_INFO_4, *LPPRINTER_INFO_4;
typedef PRINTER_INFO_5A PRINTER_INFO_5, *PPRINTER_INFO_5, *LPPRINTER_INFO_5;
typedef PRINTER_INFO_7A PRINTER_INFO_7, *PPRINTER_INFO_7, *LPPRINTER_INFO_7;
typedef PRINTER_INFO_8A PRINTER_INFO_8, *PPRINTER_INFO_8, *LPPRINTER_INFO_8;
typedef PRINTER_INFO_9A PRINTER_INFO_9, *PPRINTER_INFO_9, *LPPRINTER_INFO_9;
typedef PRINTPROCESSOR_INFO_1A PRINTPROCESSOR_INFO_1, *PPRINTPROCESSOR_INFO_1, *LPPRINTPROCESSOR_INFO_1;
typedef FORM_INFO_1A FORM_INFO_1, *PFORM_INFO_1, *LPFORM_INFO_1;
#if (NTDDI_VERSION >= NTDDI_VISTA)
typedef FORM_INFO_2A FORM_INFO_2, *PFORM_INFO_2, *LPFORM_INFO_2;
#endif
typedef PRINTER_DEFAULTSA PRINTER_DEFAULTS, *PPRINTER_DEFAULTS, *LPPRINTER_DEFAULTS;
typedef PROVIDOR_INFO_1A PROVIDOR_INFO_1, *PPROVIDOR_INFO_1, *LPPROVIDOR_INFO_1;
typedef PROVIDOR_INFO_2A PROVIDOR_INFO_2, *PPROVIDOR_INFO_2, *LPPROVIDOR_INFO_2;

#endif /* UNICODE */

BOOL WINAPI AbortPrinter(_In_ HANDLE);

BOOL
WINAPI
AddFormA(
  _In_ HANDLE hPrinter,
  _In_range_(1, 2) DWORD Level,
  _When_(Level == 1, _In_reads_bytes_(sizeof(FORM_INFO_1A)))
#if (NTDDI_VERSION >= NTDDI_VISTA)
  _When_(Level == 2, _In_reads_bytes_(sizeof(FORM_INFO_2A)))
#endif
    LPBYTE pForm);

BOOL
WINAPI
AddFormW(
  _In_ HANDLE hPrinter,
  _In_range_(1, 2) DWORD Level,
  _When_(Level == 1, _In_reads_bytes_(sizeof(FORM_INFO_1W)))
#if (NTDDI_VERSION >= NTDDI_VISTA)
  _When_(Level == 2, _In_reads_bytes_(sizeof(FORM_INFO_2W)))
#endif
  LPBYTE pForm);

BOOL
WINAPI
AddJobA(
  _In_ HANDLE hPrinter,
  _In_ DWORD Level,
  _Out_writes_bytes_opt_(cbBuf) LPBYTE pData,
  _In_ DWORD cbBuf,
  _Out_ LPDWORD pcbNeeded);

BOOL
WINAPI
AddJobW(
  _In_ HANDLE hPrinter,
  _In_ DWORD Level,
  _Out_writes_bytes_opt_(cbBuf) LPBYTE pData,
  _In_ DWORD cbBuf,
  _Out_ LPDWORD pcbNeeded);

BOOL
WINAPI
AddMonitorA(
  _In_opt_ LPSTR pName,
  _In_ DWORD Level,
  _In_reads_bytes_opt_(sizeof(MONITOR_INFO_2)) LPBYTE pMonitors);

BOOL
WINAPI
AddMonitorW(
  _In_opt_ LPWSTR pName,
  _In_ DWORD Level,
  _In_reads_bytes_opt_(sizeof(MONITOR_INFO_2)) LPBYTE pMonitors);

BOOL WINAPI AddPortA(_In_opt_ LPSTR, _In_ HWND, _In_ LPSTR);
BOOL WINAPI AddPortW(_In_opt_ LPWSTR, _In_ HWND, _In_ LPWSTR);

HANDLE
WINAPI
AddPrinterA(
  _In_opt_ LPSTR pName,
  _In_range_(1, 2) DWORD Level,
  _When_(Level == 1, _In_reads_bytes_(sizeof(PRINTER_INFO_1)))
  _When_(Level == 2, _In_reads_bytes_(sizeof(PRINTER_INFO_2)))
  LPBYTE pPrinter);

HANDLE
WINAPI
AddPrinterW(
  _In_opt_ LPWSTR pName,
  _In_range_(1, 2) DWORD Level,
  _When_(Level == 1, _In_reads_bytes_(sizeof(PRINTER_INFO_1)))
  _When_(Level == 2, _In_reads_bytes_(sizeof(PRINTER_INFO_2)))
  LPBYTE pPrinter);

BOOL WINAPI AddPrinterConnectionA(_In_ LPSTR);
BOOL WINAPI AddPrinterConnectionW(_In_ LPWSTR);
BOOL WINAPI AddPrinterDriverA(_In_opt_ LPSTR, _In_ DWORD, _In_ PBYTE);
BOOL WINAPI AddPrinterDriverW(_In_opt_ LPWSTR, _In_ DWORD, _In_ PBYTE);

BOOL
WINAPI
AddPrintProcessorA(
  _In_opt_ LPSTR pName,
  _In_opt_ LPSTR pEnvironment,
  _In_ LPSTR pPathName,
  _In_ LPSTR pPrintProcessorName);

BOOL
WINAPI
AddPrintProcessorW(
  _In_opt_ LPWSTR pName,
  _In_opt_ LPWSTR pEnvironment,
  _In_ LPWSTR pPathName,
  _In_ LPWSTR pPrintProcessorName);

BOOL
WINAPI
AddPrintProvidorA(
  _In_opt_ LPSTR pName,
  _In_range_(1, 2) DWORD Level,
  _When_(Level == 1, _In_reads_bytes_(sizeof(PROVIDOR_INFO_1)))
  _When_(Level == 2, _In_reads_bytes_(sizeof(PROVIDOR_INFO_2)))
    LPBYTE pProvidorInfo);

BOOL
WINAPI
AddPrintProvidorW(
  _In_opt_ LPWSTR pName,
  _In_range_(1, 2) DWORD Level,
  _When_(Level == 1, _In_reads_bytes_(sizeof(PROVIDOR_INFO_1)))
  _When_(Level == 2, _In_reads_bytes_(sizeof(PROVIDOR_INFO_2)))
    LPBYTE pProvidorInfo);

LONG
WINAPI
AdvancedDocumentPropertiesA(
  _In_ HWND hWnd,
  _In_ HANDLE hPrinter,
  _In_ LPSTR pDeviceName,
  _Inout_opt_ PDEVMODEA pDevModeOutput,
  _In_opt_ PDEVMODEA pDevModeInput);

LONG
WINAPI
AdvancedDocumentPropertiesW(
  _In_ HWND hWnd,
  _In_ HANDLE hPrinter,
  _In_ LPWSTR pDeviceName,
  _Inout_opt_ PDEVMODEW pDevModeOutput,
  _In_opt_ PDEVMODEW pDevModeInput);

BOOL WINAPI ClosePrinter(_In_ HANDLE);
BOOL WINAPI ConfigurePortA(_In_opt_ LPSTR, _In_ HWND, _In_ LPSTR);
BOOL WINAPI ConfigurePortW(_In_opt_ LPWSTR, _In_ HWND, _In_ LPWSTR);
HANDLE WINAPI ConnectToPrinterDlg(_In_ HWND, _In_ DWORD);
BOOL WINAPI DeleteFormA(_In_ HANDLE, _In_ LPSTR);
BOOL WINAPI DeleteFormW(_In_ HANDLE, _In_ LPWSTR);
BOOL WINAPI DeleteMonitorA(_In_opt_ LPSTR, _In_opt_ LPSTR, _In_ LPSTR);
BOOL WINAPI DeleteMonitorW(_In_opt_ LPWSTR, _In_opt_ LPWSTR, _In_ LPWSTR);
BOOL WINAPI DeletePortA(_In_opt_ LPSTR, _In_ HWND, _In_ LPSTR);
BOOL WINAPI DeletePortW(_In_opt_ LPWSTR, _In_ HWND, _In_ LPWSTR);
BOOL WINAPI DeletePrinter(_Inout_ HANDLE);
BOOL WINAPI DeletePrinterConnectionA(_In_ LPSTR);
BOOL WINAPI DeletePrinterConnectionW(_In_ LPWSTR);
DWORD WINAPI DeletePrinterDataA(_In_ HANDLE, _In_ LPSTR);
DWORD WINAPI DeletePrinterDataW(_In_ HANDLE, _In_ LPWSTR);

BOOL
WINAPI
DeletePrinterDriverA(
  _In_opt_ LPSTR pName,
  _In_opt_ LPSTR pEnvironment,
  _In_ LPSTR pDriverName);

BOOL
WINAPI
DeletePrinterDriverW(
  _In_opt_ LPWSTR pName,
  _In_opt_ LPWSTR pEnvironment,
  _In_ LPWSTR pDriverName);

BOOL
WINAPI
DeletePrintProcessorA(
  _In_opt_ LPSTR pName,
  _In_opt_ LPSTR pEnvironment,
  _In_ LPSTR pPrintProcessorName);

BOOL
WINAPI
DeletePrintProcessorW(
  _In_opt_ LPWSTR pName,
  _In_opt_ LPWSTR pEnvironment,
  _In_ LPWSTR pPrintProcessorName);

BOOL
WINAPI
DeletePrintProvidorA(
  _In_opt_ LPSTR pName,
  _In_opt_ LPSTR pEnvironment,
  _In_ LPSTR pPrintProvidorName);

BOOL
WINAPI
DeletePrintProvidorW(
  _In_opt_ LPWSTR pName,
  _In_opt_ LPWSTR pEnvironment,
  _In_ LPWSTR pPrintProvidorName);

LONG
WINAPI
DocumentPropertiesA(
  _In_opt_ HWND hWnd,
  _In_ HANDLE hPrinter,
  _In_ LPSTR pDeviceName,
  _Out_opt_ PDEVMODEA pDevModeOutput,
  _In_opt_ PDEVMODEA pDevModeInput,
  _In_ DWORD fMode);

LONG
WINAPI
DocumentPropertiesW(
  _In_opt_ HWND hWnd,
  _In_ HANDLE hPrinter,
  _In_ LPWSTR pDeviceName,
  _Out_opt_ PDEVMODEW pDevModeOutput,
  _In_opt_ PDEVMODEW pDevModeInput,
  _In_ DWORD fMode);

BOOL WINAPI EndDocPrinter(_In_ HANDLE);
BOOL WINAPI EndPagePrinter(_In_ HANDLE);

BOOL
WINAPI
EnumFormsA(
  _In_ HANDLE hPrinter,
  _In_ DWORD Level,
  _Out_writes_bytes_opt_(cbBuf) LPBYTE pForm,
  _In_ DWORD cbBuf,
  _Out_ LPDWORD pcbNeeded,
  _Out_ LPDWORD pcReturned);

BOOL
WINAPI
EnumFormsW(
  _In_ HANDLE hPrinter,
  _In_ DWORD Level,
  _Out_writes_bytes_opt_(cbBuf) LPBYTE pForm,
  _In_ DWORD cbBuf,
  _Out_ LPDWORD pcbNeeded,
  _Out_ LPDWORD pcReturned);

BOOL
WINAPI
EnumJobsA(
  _In_ HANDLE hPrinter,
  _In_ DWORD FirstJob,
  _In_ DWORD NoJobs,
  _In_ DWORD Level,
  _Out_writes_bytes_opt_(cbBuf) LPBYTE pJob,
  _In_ DWORD cbBuf,
  _Out_ LPDWORD pcbNeeded,
  _Out_ LPDWORD pcReturned);

BOOL
WINAPI
EnumJobsW(
  _In_ HANDLE hPrinter,
  _In_ DWORD FirstJob,
  _In_ DWORD NoJobs,
  _In_ DWORD Level,
  _Out_writes_bytes_opt_(cbBuf) LPBYTE pJob,
  _In_ DWORD cbBuf,
  _Out_ LPDWORD pcbNeeded,
  _Out_ LPDWORD pcReturned);

BOOL
WINAPI
EnumMonitorsA(
  _In_opt_ LPSTR pName,
  _In_ DWORD Level,
  _Out_writes_bytes_opt_(cbBuf) LPBYTE pMonitor,
  _In_ DWORD cbBuf,
  _Out_ LPDWORD pcbNeeded,
  _Out_ LPDWORD pcReturned);

BOOL
WINAPI
EnumMonitorsW(
  _In_opt_ LPWSTR pName,
  _In_ DWORD Level,
  _Out_writes_bytes_opt_(cbBuf) LPBYTE pMonitor,
  _In_ DWORD cbBuf,
  _Out_ LPDWORD pcbNeeded,
  _Out_ LPDWORD pcReturned);

BOOL
WINAPI
EnumPortsA(
  _In_opt_ LPSTR pName,
  _In_ DWORD Level,
  _Out_writes_bytes_opt_(cbBuf) LPBYTE pPort,
  _In_ DWORD cbBuf,
  _Out_ LPDWORD pcbNeeded,
  _Out_ LPDWORD pcReturned);

BOOL
WINAPI
EnumPortsW(
  _In_opt_ LPWSTR pName,
  _In_ DWORD Level,
  _Out_writes_bytes_opt_(cbBuf) LPBYTE pPort,
  _In_ DWORD cbBuf,
  _Out_ LPDWORD pcbNeeded,
  _Out_ LPDWORD pcReturned);

DWORD
WINAPI
EnumPrinterDataA(
  _In_ HANDLE hPrinter,
  _In_ DWORD dwIndex,
  _Out_writes_bytes_(cbValueName) LPSTR pValueName,
  _In_ DWORD cbValueName,
  _Out_ LPDWORD pcbValueName,
  _Out_opt_ LPDWORD pType,
  _Out_writes_to_opt_(cbData, *pcbData) LPBYTE pData,
  _In_ DWORD cbData,
  _When_(pData != NULL, _Pre_notnull_) _Out_opt_ LPDWORD pcbData);

DWORD
WINAPI
EnumPrinterDataW(
  _In_ HANDLE hPrinter,
  _In_ DWORD dwIndex,
  _Out_writes_bytes_(cbValueName) LPWSTR pValueName,
  _In_ DWORD cbValueName,
  _Out_ LPDWORD pcbValueName,
  _Out_opt_ LPDWORD pType,
  _Out_writes_to_opt_(cbData, *pcbData) LPBYTE pData,
  _In_ DWORD cbData,
  _When_(pData != NULL, _Pre_notnull_) _Out_opt_ LPDWORD pcbData);

BOOL
WINAPI
EnumPrinterDriversA(
  _In_opt_ LPSTR pName,
  _In_opt_ LPSTR pEnvironment,
  _In_ DWORD Level,
  _Out_writes_bytes_opt_(cbBuf) LPBYTE pDriverInfo,
  _In_ DWORD cbBuf,
  _Out_ LPDWORD pcbNeeded,
  _Out_ LPDWORD pcReturned);

BOOL
WINAPI
EnumPrinterDriversW(
  _In_opt_ LPWSTR pName,
  _In_opt_ LPWSTR pEnvironment,
  _In_ DWORD Level,
  _Out_writes_bytes_opt_(cbBuf) LPBYTE pDriverInfo,
  _In_ DWORD cbBuf,
  _Out_ LPDWORD pcbNeeded,
  _Out_ LPDWORD pcReturned);

_Success_(return != 0)
BOOL
WINAPI
EnumPrintersA(
  _In_ DWORD Flags,
  _In_opt_ LPSTR Name,
  _In_ DWORD Level,
  _Out_writes_bytes_to_opt_(cbBuf, *pcbNeeded) LPBYTE pPrinterEnum,
  _In_ DWORD cbBuf,
  _Out_ LPDWORD pcbNeeded,
  _Out_ LPDWORD pcReturned);

_Success_(return != 0)
BOOL
WINAPI
EnumPrintersW(
  _In_ DWORD Flags,
  _In_opt_ LPWSTR Name,
  _In_ DWORD Level,
  _Out_writes_bytes_to_opt_(cbBuf, *pcbNeeded) LPBYTE pPrinterEnum,
  _In_ DWORD cbBuf,
  _Out_ LPDWORD pcbNeeded,
  _Out_ LPDWORD pcReturned);

_Success_(return != 0)
BOOL
WINAPI
EnumPrintProcessorDatatypesA(
  _In_opt_ LPSTR pName,
  _In_ LPSTR pPrintProcessorName,
  _In_ DWORD Level,
  _Out_writes_bytes_opt_(cbBuf) LPBYTE pDatatypes,
  _In_ DWORD cbBuf,
  _Out_ LPDWORD pcbNeeded,
  _Out_ LPDWORD pcReturned);

_Success_(return != 0)
BOOL
WINAPI
EnumPrintProcessorDatatypesW(
  _In_opt_ LPWSTR pName,
  _In_ LPWSTR pPrintProcessorName,
  _In_ DWORD Level,
  _Out_writes_bytes_opt_(cbBuf) LPBYTE pDatatypes,
  _In_ DWORD cbBuf,
  _Out_ LPDWORD pcbNeeded,
  _Out_ LPDWORD pcReturned);

BOOL
WINAPI
EnumPrintProcessorsA(
  _In_opt_ LPSTR pName,
  _In_opt_ LPSTR pEnvironment,
  _In_ DWORD Level,
  _Out_writes_bytes_opt_(cbBuf) LPBYTE pPrintProcessorInfo,
  _In_ DWORD cbBuf,
  _Out_ LPDWORD pcbNeeded,
  _Out_ LPDWORD pcReturned);

BOOL
WINAPI
EnumPrintProcessorsW(
  _In_opt_ LPWSTR pName,
  _In_opt_ LPWSTR pEnvironment,
  _In_ DWORD Level,
  _Out_writes_bytes_opt_(cbBuf) LPBYTE pPrintProcessorInfo,
  _In_ DWORD cbBuf,
  _Out_ LPDWORD pcbNeeded,
  _Out_ LPDWORD pcReturned);

LONG
WINAPI
ExtDeviceMode(
  _In_opt_ HWND hWnd,
  _In_opt_ HANDLE hInst,
  _Inout_opt_ LPDEVMODEA pDevModeOutput,
  _In_opt_ LPSTR pDeviceName,
  _In_opt_ LPSTR pPort,
  _In_opt_ LPDEVMODEA pDevModeInput,
  _In_opt_ LPSTR pProfile,
  _In_ DWORD fMode);

BOOL WINAPI FindClosePrinterChangeNotification(_In_ HANDLE);

HANDLE
WINAPI
FindFirstPrinterChangeNotification(
  _In_ HANDLE hPrinter,
  DWORD fdwFilter,
  DWORD fdwOptions,
  _In_opt_ PVOID pPrinterNotifyOptions);

BOOL
WINAPI
FindNextPrinterChangeNotification(
  _In_ HANDLE hChange,
  _Out_opt_ PDWORD pdwChange,
  _In_opt_ LPVOID pvReserved,
  _Out_opt_ LPVOID *ppPrinterNotifyInfo);

BOOL WINAPI FreePrinterNotifyInfo(_In_ PPRINTER_NOTIFY_INFO);

#if _WIN32_WINNT >= 0x0500

BOOL
WINAPI
GetDefaultPrinterA(
  _Out_writes_opt_(*pcchBuffer) LPSTR pszBuffer,
  _Inout_ LPDWORD pcchBuffer);

BOOL
WINAPI
GetDefaultPrinterW(
  _Out_writes_opt_(*pcchBuffer) LPWSTR pszBuffer,
  _Inout_ LPDWORD pcchBuffer);

#endif

BOOL
WINAPI
GetFormA(
  _In_ HANDLE hPrinter,
  _In_ LPSTR pFormName,
  _In_ DWORD Level,
  _Out_writes_bytes_opt_(cbBuf) LPBYTE pForm,
  _In_ DWORD cbBuf,
  _Out_ LPDWORD pcbNeeded);

BOOL
WINAPI
GetFormW(
  _In_ HANDLE hPrinter,
  _In_ LPWSTR pFormName,
  _In_ DWORD Level,
  _Out_writes_bytes_opt_(cbBuf) LPBYTE pForm,
  _In_ DWORD cbBuf,
  _Out_ LPDWORD pcbNeeded);

BOOL
WINAPI
GetJobA(
  _In_ HANDLE hPrinter,
  _In_ DWORD JobId,
  _In_ DWORD Level,
  _Out_writes_bytes_opt_(cbBuf) LPBYTE pJob,
  _In_ DWORD cbBuf,
  _Out_ LPDWORD pcbNeeded);

BOOL
WINAPI
GetJobW(
  _In_ HANDLE hPrinter,
  _In_ DWORD JobId,
  _In_ DWORD Level,
  _Out_writes_bytes_opt_(cbBuf) LPBYTE pJob,
  _In_ DWORD cbBuf,
  _Out_ LPDWORD pcbNeeded);

BOOL
WINAPI
GetPrinterA(
  _In_ HANDLE hPrinter,
  _In_ DWORD Level,
  _Out_writes_bytes_opt_(cbBuf) LPBYTE pPrinter,
  _In_ DWORD cbBuf,
  _Out_ LPDWORD pcbNeeded);

BOOL
WINAPI
GetPrinterW(
  _In_ HANDLE hPrinter,
  _In_ DWORD Level,
  _Out_writes_bytes_opt_(cbBuf) LPBYTE pPrinter,
  _In_ DWORD cbBuf,
  _Out_ LPDWORD pcbNeeded);

DWORD
WINAPI
GetPrinterDataA(
  _In_ HANDLE hPrinter,
  _In_ LPSTR pValueName,
  _Out_opt_ LPDWORD pType,
  _Out_writes_bytes_opt_(nSize) LPBYTE pData,
  _In_ DWORD nSize,
  _Out_ LPDWORD pcbNeeded);

DWORD
WINAPI
GetPrinterDataW(
  _In_ HANDLE hPrinter,
  _In_ LPWSTR pValueName,
  _Out_opt_ LPDWORD pType,
  _Out_writes_bytes_opt_(nSize) LPBYTE pData,
  _In_ DWORD nSize,
  _Out_ LPDWORD pcbNeeded);

BOOL
WINAPI
GetPrinterDriverA(
  _In_ HANDLE  hPrinter,
  _In_opt_ LPSTR pEnvironment,
  _In_ DWORD Level,
  _Out_writes_bytes_opt_(cbBuf) LPBYTE pDriverInfo,
  _In_ DWORD cbBuf,
  _Out_ LPDWORD pcbNeeded);

BOOL
WINAPI
GetPrinterDriverW(
  _In_ HANDLE hPrinter,
  _In_opt_ LPWSTR pEnvironment,
  _In_ DWORD Level,
  _Out_writes_bytes_opt_(cbBuf) LPBYTE pDriverInfo,
  _In_ DWORD cbBuf,
  _Out_ LPDWORD pcbNeeded);

BOOL
WINAPI
GetPrinterDriverDirectoryA(
  _In_opt_ LPSTR pName,
  _In_opt_ LPSTR pEnvironment,
  _In_ DWORD Level,
  _Out_writes_bytes_opt_(cbBuf) LPBYTE pDriverDirectory,
  _In_ DWORD cbBuf,
  _Out_ LPDWORD pcbNeeded);

BOOL
WINAPI
GetPrinterDriverDirectoryW(
  _In_opt_ LPWSTR pName,
  _In_opt_ LPWSTR pEnvironment,
  _In_ DWORD Level,
  _Out_writes_bytes_opt_(cbBuf) LPBYTE pDriverDirectory,
  _In_ DWORD cbBuf,
  _Out_ LPDWORD pcbNeeded);

BOOL
WINAPI
GetPrintProcessorDirectoryA(
  _In_opt_ LPSTR pName,
  _In_opt_ LPSTR pEnvironment,
  _In_ DWORD Level,
  _Out_writes_bytes_opt_(cbBuf) LPBYTE pPrintProcessorInfo,
  _In_ DWORD cbBuf,
  _Out_ LPDWORD pcbNeeded);

BOOL
WINAPI
GetPrintProcessorDirectoryW(
  _In_opt_ LPWSTR pName,
  _In_opt_ LPWSTR pEnvironment,
  _In_ DWORD Level,
  _Out_writes_bytes_opt_(cbBuf) LPBYTE pPrintProcessorInfo,
  _In_ DWORD cbBuf,
  _Out_ LPDWORD pcbNeeded);

#if NTDDI_VERSION >= NTDDI_WINXPSP2
BOOL WINAPI IsValidDevmodeA(_In_opt_ PDEVMODEA, size_t);
BOOL WINAPI IsValidDevmodeW(_In_opt_ PDEVMODEW, size_t);
#endif

BOOL
WINAPI
OpenPrinterA(
  _In_opt_ LPSTR pPrinterName,
  _Out_ LPHANDLE phPrinter,
  _In_opt_ LPPRINTER_DEFAULTSA pDefault);

BOOL
WINAPI
OpenPrinterW(
  _In_opt_ LPWSTR pPrinterName,
  _Out_ LPHANDLE phPrinter,
  _In_opt_ LPPRINTER_DEFAULTSW pDefault);

DWORD
WINAPI
PrinterMessageBoxA(
  _In_ HANDLE hPrinter,
  DWORD Error,
  _In_ HWND hWnd,
  _In_ LPSTR pText,
  _In_ LPSTR pCaption,
  DWORD dwType);

DWORD
WINAPI
PrinterMessageBoxW(
  _In_ HANDLE hPrinter,
  DWORD Error,
  _In_ HWND hWnd,
  _In_ LPWSTR pText,
  _In_ LPWSTR pCaption,
  DWORD dwType);

BOOL WINAPI PrinterProperties(_In_ HWND, _In_ HANDLE);

BOOL
WINAPI
ReadPrinter(
  _In_ HANDLE hPrinter,
  _Out_writes_bytes_(cbBuf) LPVOID pBuf,
  _In_ DWORD cbBuf,
  _Out_ LPDWORD pNoBytesRead);

BOOL WINAPI ResetPrinterA(_In_ HANDLE, _In_opt_ LPPRINTER_DEFAULTSA);
BOOL WINAPI ResetPrinterW(_In_ HANDLE, _In_opt_ LPPRINTER_DEFAULTSW);
BOOL WINAPI ScheduleJob(_In_ HANDLE, _In_ DWORD);

BOOL
WINAPI
SetFormA(
  _In_ HANDLE hPrinter,
  _In_ LPSTR pFormName,
  _In_range_(1, 2) DWORD Level,
  _When_(Level == 1, _In_reads_bytes_(sizeof(FORM_INFO_1A))) LPBYTE pForm);

BOOL
WINAPI
SetFormW(
  _In_ HANDLE hPrinter,
  _In_ LPWSTR pFormName,
  _In_range_(1, 2) DWORD Level,
  _When_(Level == 1, _In_reads_bytes_(sizeof(FORM_INFO_1W))) LPBYTE pForm);

BOOL
WINAPI
SetJobA(
  _In_ HANDLE hPrinter,
  _In_ DWORD JobId,
  _In_range_(0, 4) DWORD Level,
  _When_(Level == 0, _Reserved_)
  _When_(Level == 1, _In_reads_bytes_opt_(sizeof(JOB_INFO_1)))
  _When_(Level == 2, _In_reads_bytes_opt_(sizeof(JOB_INFO_2)))
  _When_(Level == 3, _In_reads_bytes_opt_(sizeof(JOB_INFO_3)))
  _When_(Level == 4, _In_reads_bytes_opt_(sizeof(JOB_INFO_4)))
    LPBYTE pJob,
  _In_ DWORD Command);

BOOL
WINAPI
SetJobW(
  _In_ HANDLE hPrinter,
  _In_ DWORD JobId,
  _In_range_(0, 4) DWORD Level,
  _When_(Level == 0, _Reserved_)
  _When_(Level == 1, _In_reads_bytes_opt_(sizeof(JOB_INFO_1)))
  _When_(Level == 2, _In_reads_bytes_opt_(sizeof(JOB_INFO_2)))
  _When_(Level == 3, _In_reads_bytes_opt_(sizeof(JOB_INFO_3)))
  _When_(Level == 4, _In_reads_bytes_opt_(sizeof(JOB_INFO_4)))
    LPBYTE pJob,
  _In_ DWORD Command);

BOOL
WINAPI
SetPrinterA(
  _In_ HANDLE hPrinter,
  _In_range_(0, 9) DWORD Level,
  _When_(Level == 0 && Command == PRINTER_CONTROL_SET_STATUS, _In_reads_bytes_(sizeof(DWORD)))
  _When_(Level == 0 && Command != PRINTER_CONTROL_SET_STATUS, _In_opt_)
  _When_(Level == 1, _In_reads_bytes_(sizeof(PRINTER_INFO_1)))
  _When_(Level == 2, _In_reads_bytes_(sizeof(PRINTER_INFO_2)))
  _When_(Level == 3, _In_reads_bytes_(sizeof(PRINTER_INFO_3)))
  _When_(Level == 4, _In_reads_bytes_(sizeof(PRINTER_INFO_4)))
  _When_(Level == 5, _In_reads_bytes_(sizeof(PRINTER_INFO_5)))
  _When_(Level == 6, _In_reads_bytes_(sizeof(PRINTER_INFO_6)))
  _When_(Level == 7, _In_reads_bytes_(sizeof(PRINTER_INFO_7)))
  _When_(Level == 8, _In_reads_bytes_(sizeof(PRINTER_INFO_8)))
  _When_(Level == 9, _In_reads_bytes_(sizeof(PRINTER_INFO_9)))
    LPBYTE pPrinter,
  _In_ DWORD Command);

BOOL
WINAPI
SetPrinterW(
  _In_ HANDLE hPrinter,
  _In_range_(0, 9) DWORD Level,
  _When_(Level == 0 && Command == PRINTER_CONTROL_SET_STATUS, _In_reads_bytes_(sizeof(DWORD)))
  _When_(Level == 0 && Command != PRINTER_CONTROL_SET_STATUS, _In_opt_)
  _When_(Level == 1, _In_reads_bytes_(sizeof(PRINTER_INFO_1)))
  _When_(Level == 2, _In_reads_bytes_(sizeof(PRINTER_INFO_2)))
  _When_(Level == 3, _In_reads_bytes_(sizeof(PRINTER_INFO_3)))
  _When_(Level == 4, _In_reads_bytes_(sizeof(PRINTER_INFO_4)))
  _When_(Level == 5, _In_reads_bytes_(sizeof(PRINTER_INFO_5)))
  _When_(Level == 6, _In_reads_bytes_(sizeof(PRINTER_INFO_6)))
  _When_(Level == 7, _In_reads_bytes_(sizeof(PRINTER_INFO_7)))
  _When_(Level == 8, _In_reads_bytes_(sizeof(PRINTER_INFO_8)))
  _When_(Level == 9, _In_reads_bytes_(sizeof(PRINTER_INFO_9)))
    LPBYTE pPrinter,
  _In_ DWORD Command);

BOOL
WINAPI
SetPrinterDataA(
  _In_ HANDLE hPrinter,
  _In_ LPSTR pValueName,
  _In_ DWORD Type,
  _In_reads_bytes_(cbData) LPBYTE pData,
  _In_ DWORD cbData);

BOOL
WINAPI
SetPrinterDataW(
  _In_ HANDLE hPrinter,
  _In_ LPWSTR pValueName,
  _In_ DWORD Type,
  _In_reads_bytes_(cbData) LPBYTE pData,
  _In_ DWORD cbData);

#ifdef _WINE
LPSTR WINAPI StartDocDlgA(HANDLE hPrinter, DOCINFOA *doc);
LPWSTR WINAPI StartDocDlgW(HANDLE hPrinter, DOCINFOW *doc);
#define StartDocDlg WINELIB_NAME_AW(StartDocDlg)
#endif

DWORD
WINAPI
StartDocPrinterA(
  _In_ HANDLE hPrinter,
  _In_range_(1, 3) DWORD Level,
  _When_(Level == 1, _In_reads_bytes_(sizeof(DOC_INFO_1))) LPBYTE pDocInfo);

DWORD
WINAPI
StartDocPrinterW(
  _In_ HANDLE hPrinter,
  _In_range_(1, 3) DWORD Level,
  _When_(Level == 1, _In_reads_bytes_(sizeof(DOC_INFO_1))) LPBYTE pDocInfo);

BOOL WINAPI StartPagePrinter(_In_ HANDLE);
DWORD WINAPI WaitForPrinterChange(_In_ HANDLE, _In_ DWORD);

BOOL
WINAPI
WritePrinter(
  _In_ HANDLE hPrinter,
  _In_reads_bytes_(cbBuf) LPVOID pBuf,
  _In_ DWORD cbBuf,
  _Out_ LPDWORD pcWritten);

BOOL
WINAPI
XcvDataW(
  _In_ HANDLE hXcv,
  _In_ PCWSTR pszDataName,
  _In_reads_bytes_opt_(cbInputData) PBYTE pInputData,
  DWORD cbInputData,
  _Out_writes_bytes_opt_(cbOutputData) PBYTE pOutputData,
  DWORD cbOutputData,
  _Out_ PDWORD pcbOutputNeeded,
  _Out_opt_ PDWORD pdwStatus);

#ifdef UNICODE

#define AddForm AddFormW
#define AddJob AddJobW
#define AddMonitor AddMonitorW
#define AddPort AddPortW
#define AddPrinter AddPrinterW
#define AddPrinterConnection AddPrinterConnectionW
#define AddPrinterDriver AddPrinterDriverW
#define AddPrintProcessor AddPrintProcessorW
#define AddPrintProvidor AddPrintProvidorW
#define AdvancedDocumentProperties AdvancedDocumentPropertiesW
#define ConfigurePort ConfigurePortW
#define DeleteForm DeleteFormW
#define DeleteMonitor DeleteMonitorW
#define DeletePort DeletePortW
#define DeletePrinterConnection DeletePrinterConnectionW
#define DeletePrinterData DeletePrinterDataW
#define DeletePrinterDriver DeletePrinterDriverW
#define DeletePrintProcessor DeletePrinterProcessorW
#define DeletePrintProvidor DeletePrinterProvidorW
#define DocumentProperties DocumentPropertiesW
#define EnumForms EnumFormsW
#define EnumJobs EnumJobsW
#define EnumMonitors EnumMonitorsW
#define EnumPorts EnumPortsW
#define EnumPrinterData EnumPrinterDataW
#define EnumPrinterDrivers EnumPrinterDriversW
#define EnumPrinters EnumPrintersW
#define EnumPrintProcessorDatatypes EnumPrintProcessorDatatypesW
#define EnumPrintProcessors EnumPrintProcessorsW
#define GetDefaultPrinter GetDefaultPrinterW
#define GetForm GetFormW
#define GetJob GetJobW
#define GetPrinter GetPrinterW
#define GetPrinterData GetPrinterDataW
#define GetPrinterDriver GetPrinterDriverW
#define GetPrinterDriverDirectory GetPrinterDriverDirectoryW
#define GetPrintProcessorDirectory GetPrintProcessorDirectoryW
#define IsValidDevmode IsValidDevmodeW
#define OpenPrinter OpenPrinterW
#define PrinterMessageBox PrinterMessageBoxW
#define ResetPrinter ResetPrinterW
#define SetForm SetFormW
#define SetJob SetJobW
#define SetPrinter SetPrinterW
#define SetPrinterData SetPrinterDataW
#define StartDocPrinter StartDocPrinterW

#else /* UNICODE */

#define AddForm AddFormA
#define AddJob AddJobA
#define AddMonitor AddMonitorA
#define AddPort AddPortA
#define AddPrinter AddPrinterA
#define AddPrinterConnection AddPrinterConnectionA
#define AddPrinterDriver AddPrinterDriverA
#define AddPrintProcessor AddPrintProcessorA
#define AddPrintProvidor AddPrintProvidorA
#define AdvancedDocumentProperties AdvancedDocumentPropertiesA
#define ConfigurePort ConfigurePortA
#define DeleteForm DeleteFormA
#define DeleteMonitor DeleteMonitorA
#define DeletePort DeletePortA
#define DeletePrinterConnection DeletePrinterConnectionA
#define DeletePrinterData DeletePrinterDataA
#define DeletePrinterDriver DeletePrinterDriverA
#define DeletePrintProcessor DeletePrinterProcessorA
#define DeletePrintProvidor DeletePrinterProvidorA
#define DocumentProperties DocumentPropertiesA
#define EnumForms EnumFormsA
#define EnumJobs EnumJobsA
#define EnumMonitors EnumMonitorsA
#define EnumPorts EnumPortsA
#define EnumPrinterData EnumPrinterDataA
#define EnumPrinterDrivers EnumPrinterDriversA
#define EnumPrinters EnumPrintersA
#define EnumPrintProcessorDatatypes EnumPrintProcessorDatatypesA
#define EnumPrintProcessors EnumPrintProcessorsA
#define GetDefaultPrinter GetDefaultPrinterA
#define GetForm GetFormA
#define GetJob GetJobA
#define GetPrinter GetPrinterA
#define GetPrinterData GetPrinterDataA
#define GetPrinterDriver GetPrinterDriverA
#define GetPrinterDriverDirectory GetPrinterDriverDirectoryA
#define GetPrintProcessorDirectory GetPrintProcessorDirectoryA
#define IsValidDevmode IsValidDevmodeA
#define OpenPrinter OpenPrinterA
#define PrinterMessageBox PrinterMessageBoxA
#define ResetPrinter ResetPrinterA
#define SetForm SetFormA
#define SetJob SetJobA
#define SetPrinter SetPrinterA
#define SetPrinterData SetPrinterDataA
#define StartDocPrinter StartDocPrinterA

#endif /* UNICODE */

#endif /* !RC_INVOKED */

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#ifdef __cplusplus
}
#endif

#endif /* _WINSPOOL_ */
