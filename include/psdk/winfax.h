/*
 * winfax.h
 *
 * FAX API Support
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAIMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#ifndef __WINFAX_H
#define __WINFAX_H

#ifdef __cplusplus
extern "C" {
#endif

typedef DWORD HCALL;

typedef struct _FAX_TIME
{
  WORD Hour;
  WORD Minute;
} FAX_TIME, *PFAX_TIME;

typedef enum
{
  JSA_NOW = 0,
  JSA_SPECIFIC_TIME,
  JSA_DISCOUNT_PERIOD
} FAX_ENUM_JOB_SEND_ATTRIBUTES;

typedef enum
{
  DRT_NONE = 0,
  DRT_EMAIL,
  DRT_INBOX
} FAX_ENUM_DELIVERY_REPORT_TYPES;

typedef enum
{
  FAXLOG_CATEGORY_INIT = 1,
  FAXLOG_CATEGORY_OUTBOUND,
  FAXLOG_CATEGORY_INBOUND,
  FAXLOG_CATEGORY_UNKNOWN
} FAX_ENUM_LOG_CATEGORIES;

typedef enum
{
  FAXLOG_LEVEL_NONE = 0,
  FAXLOG_LEVEL_MIN,
  FAXLOG_LEVEL_MED,
  FAXLOG_LEVEL_MAX
} FAX_ENUM_LOG_LEVELS;

typedef enum
{
  PORT_OPEN_QUERY = 1,
  PORT_OPEN_MODIFY
} FAX_ENUM_PORT_OPEN_TYPE;

typedef enum
{
  JC_UNKNOWN = 0,
  JC_DELETE,
  JC_PAUSE,
  JC_RESUME
} FAX_ENUM_JOB_COMMANDS;

#define JT_UNKNOWN      0
#define JT_SEND         1
#define JT_RECEIVE      2
#define JT_ROUTING      3
#define JT_FAIL_RECEIVE 4

#define JS_PENDING          0x0
#define JS_INPROGRESS       0x1
#define JS_DELETING         0x2
#define JS_FAILED           0x4
#define JS_PAUSED           0x8
#define JS_NOLINE           0x10
#define JS_RETRYING         0x20
#define JS_RETRIES_EXCEEDED 0x40

#define FPS_DIALING          0x20000001
#define FPS_SENDING          0x20000002
#define FPS_RECEIVING        0x20000004
#define FPS_COMPLETED        0x20000008
#define FPS_HANDLED          0x20000010
#define FPS_UNAVAILABLE      0x20000020
#define FPS_BUSY             0x20000040
#define FPS_NO_ANSWER        0x20000080
#define FPS_BAD_ADDRESS      0x20000100
#define FPS_NO_DIAL_TONE     0x20000200
#define FPS_DISCONNECTED     0x20000400
#define FPS_FATAL_ERROR      0x20000800
#define FPS_NOT_FAX_CALL     0x20001000
#define FPS_CALL_DELAYED     0x20002000
#define FPS_CALL_BLACKLISTED 0x20004000
#define FPS_INITIALIZING     0x20008000
#define FPS_OFFLINE          0x20010000
#define FPS_RINGING          0x20020000
#define FPS_AVAILABLE        0x20100000
#define FPS_ABORTING         0x20200000
#define FPS_ROUTING          0x20400000
#define FPS_ANSWERED         0x20800000

#define FPF_RECEIVE 0x1
#define FPF_SEND    0x2
#define FPF_VIRTUAL 0x4

typedef struct _FAX_JOB_PARAMA
{
  DWORD SizeOfStruct;
  LPCSTR RecipientNumber;
  LPCSTR RecipientName;
  LPCSTR Tsid;
  LPCSTR SenderName;
  LPCSTR SenderCompany;
  LPCSTR SenderDept;
  LPCSTR BillingCode;
  DWORD ScheduleAction;
  SYSTEMTIME ScheduleTime;
  DWORD DeliveryReportType;
  LPCSTR DeliveryReportAddress;
  LPCSTR DocumentName;
  HCALL CallHandle;
  DWORD_PTR Reserved[3];
} FAX_JOB_PARAMA, *PFAX_JOB_PARAMA;

typedef struct _FAX_JOB_PARAMW
{
  DWORD SizeOfStruct;
  LPCWSTR RecipientNumber;
  LPCWSTR RecipientName;
  LPCWSTR Tsid;
  LPCWSTR SenderName;
  LPCWSTR SenderCompany;
  LPCWSTR SenderDept;
  LPCWSTR BillingCode;
  DWORD ScheduleAction;
  SYSTEMTIME ScheduleTime;
  DWORD DeliveryReportType;
  LPCWSTR DeliveryReportAddress;
  LPCWSTR DocumentName;
  HCALL CallHandle;
  DWORD_PTR Reserved[3];
} FAX_JOB_PARAMW, *PFAX_JOB_PARAMW;

typedef struct _FAX_COVERAGE_INFOA
{
  DWORD SizeOfStruct;

  LPCSTR CoverPageName;
  BOOL UseServerCoverPage;

  LPCSTR RecName;
  LPCSTR RecFaxNumber;
  LPCSTR RecCompany;
  LPCSTR RecStreetAddress;
  LPCSTR RecCity;
  LPCSTR RecState;
  LPCSTR RecZip;
  LPCSTR RecCountry;
  LPCSTR RecTitle;
  LPCSTR RecDepartment;
  LPCSTR RecOfficeLocation;
  LPCSTR RecHomePhone;
  LPCSTR RecOfficePhone;

  LPCSTR SdrName;
  LPCSTR SdrFaxNumber;
  LPCSTR SdrCompany;
  LPCSTR SdrAddress;
  LPCSTR SdrTitle;
  LPCSTR SdrDepartment;
  LPCSTR SdrOfficeLocation;
  LPCSTR SdrHomePhone;
  LPCSTR SdrOfficePhone;

  LPCSTR Node;
  LPCSTR Subject;
  SYSTEMTIME TimeSent;
  DWORD PageCount;
} FAX_COVERAGE_INFOA, *PFAX_COVERAGE_INFOA;

typedef struct _FAX_COVERAGE_INFOW
{
  DWORD SizeOfStruct;

  LPCWSTR CoverPageName;
  BOOL UseServerCoverPage;

  LPCWSTR RecName;
  LPCWSTR RecFaxNumber;
  LPCWSTR RecCompany;
  LPCWSTR RecStreetAddress;
  LPCWSTR RecCity;
  LPCWSTR RecState;
  LPCWSTR RecZip;
  LPCWSTR RecCountry;
  LPCWSTR RecTitle;
  LPCWSTR RecDepartment;
  LPCWSTR RecOfficeLocation;
  LPCWSTR RecHomePhone;
  LPCWSTR RecOfficePhone;

  LPCWSTR SdrName;
  LPCWSTR SdrFaxNumber;
  LPCWSTR SdrCompany;
  LPCWSTR SdrAddress;
  LPCWSTR SdrTitle;
  LPCWSTR SdrDepartment;
  LPCWSTR SdrOfficeLocation;
  LPCWSTR SdrHomePhone;
  LPCWSTR SdrOfficePhone;

  LPCWSTR Node;
  LPCWSTR Subject;
  SYSTEMTIME TimeSent;
  DWORD PageCount;
} FAX_COVERAGE_INFOW, *PFAX_COVERAGE_INFOW;

typedef struct _FAX_GLOBAL_ROUTING_INFOA
{
  DWORD SizeOfStruct;
  DWORD Priority;
  LPCSTR Guid;
  LPCSTR FriendlyName;
  LPCSTR FunctionName;
  LPCSTR ExtensionImageName;
  LPCSTR ExtensionFriendlyName;
} FAX_GLOBAL_ROUTING_INFOA, *PFAX_GLOBAL_ROUTING_INFOA;

typedef struct _FAX_GLOBAL_ROUTING_INFOW
{
  DWORD SizeOfStruct;
  DWORD Priority;
  LPCWSTR Guid;
  LPCWSTR FriendlyName;
  LPCWSTR FunctionName;
  LPCWSTR ExtensionImageName;
  LPCWSTR ExtensionFriendlyName;
} FAX_GLOBAL_ROUTING_INFOW, *PFAX_GLOBAL_ROUTING_INFOW;

typedef struct _FAX_JOB_ENTRYA
{
  DWORD SizeOfStruct;
  DWORD JobId;
  LPCSTR UserName;
  DWORD JobType;
  DWORD QueueStatus;
  DWORD Status;
  DWORD Size;
  DWORD PageCount;
  LPCSTR RecipientNumber;
  LPCSTR RecipientName;
  LPCSTR Tsid;
  LPCSTR SenderName;
  LPCSTR SenderCompany;
  LPCSTR SenderDept;
  LPCSTR BillingCode;
  DWORD ScheduleAction;
  SYSTEMTIME ScheduleTime;
  DWORD DeliveryReportType;
  LPCSTR DeliveryReportAddress;
  LPCSTR DocumentName;
} FAX_JOB_ENTRYA, *PFAX_JOB_ENTRYA;

typedef struct _FAX_JOB_ENTRYW
{
  DWORD SizeOfStruct;
  DWORD JobId;
  LPCWSTR UserName;
  DWORD JobType;
  DWORD QueueStatus;
  DWORD Status;
  DWORD Size;
  DWORD PageCount;
  LPCWSTR RecipientNumber;
  LPCWSTR RecipientName;
  LPCWSTR Tsid;
  LPCWSTR SenderName;
  LPCWSTR SenderCompany;
  LPCWSTR SenderDept;
  LPCWSTR BillingCode;
  DWORD ScheduleAction;
  SYSTEMTIME ScheduleTime;
  DWORD DeliveryReportType;
  LPCWSTR DeliveryReportAddress;
  LPCWSTR DocumentName;
} FAX_JOB_ENTRYW, *PFAX_JOB_ENTRYW;

typedef struct _FAX_PORT_INFOA
{
  DWORD SizeOfStruct;
  DWORD DeviceId;
  DWORD State;
  DWORD Flags;
  DWORD Rings;
  DWORD Priority;
  LPCSTR DeviceName;
  LPCSTR Tsid;
  LPCSTR Csid;
} FAX_PORT_INFOA, *PFAX_PORT_INFOA;

typedef struct _FAX_PORT_INFOW
{
  DWORD SizeOfStruct;
  DWORD DeviceId;
  DWORD State;
  DWORD Flags;
  DWORD Rings;
  DWORD Priority;
  LPCWSTR DeviceName;
  LPCWSTR Tsid;
  LPCWSTR Csid;
} FAX_PORT_INFOW, *PFAX_PORT_INFOW;

typedef struct _FAX_ROUTING_METHODA
{
  DWORD SizeOfStruct;
  DWORD DeviceId;
  BOOL Enabled;
  LPCSTR DeviceName;
  LPCSTR Guid;
  LPCSTR FriendlyName;
  LPCSTR FunctionName;
  LPCSTR ExtensionImageName;
  LPCSTR ExtensionFriendlyName;
} FAX_ROUTING_METHODA, *PFAX_ROUTING_METHODA;

typedef struct _FAX_ROUTING_METHODW
{
  DWORD SizeOfStruct;
  DWORD DeviceId;
  BOOL Enabled;
  LPCWSTR DeviceName;
  LPCWSTR Guid;
  LPCWSTR FriendlyName;
  LPCWSTR FunctionName;
  LPCWSTR ExtensionImageName;
  LPCWSTR ExtensionFriendlyName;
} FAX_ROUTING_METHODW, *PFAX_ROUTING_METHODW;

typedef struct _FAX_CONFIGURATIONA
{
  DWORD SizeOfStruct;
  DWORD Retries;
  DWORD RetryDelay;
  BOOL Branding;
  DWORD DirtyDays;
  BOOL UseDeviceTsid;
  BOOL ServerCp;
  BOOL PauseServerQueue;
  FAX_TIME StartCheapTime;
  FAX_TIME StopCheapTime;
  BOOL ArchiveOutgoingFaxes;
  LPCSTR ArchiveDirectory;
  LPCSTR InboundProfile;
} FAX_CONFIGURATIONA, *PFAX_CONFIGURATIONA;

typedef struct _FAX_CONFIGURATIONW
{
  DWORD SizeOfStruct;
  DWORD Retries;
  DWORD RetryDelay;
  BOOL Branding;
  DWORD DirtyDays;
  BOOL UseDeviceTsid;
  BOOL ServerCp;
  BOOL PauseServerQueue;
  FAX_TIME StartCheapTime;
  FAX_TIME StopCheapTime;
  BOOL ArchiveOutgoingFaxes;
  LPCWSTR ArchiveDirectory;
  LPCWSTR InboundProfile;
} FAX_CONFIGURATIONW, *PFAX_CONFIGURATIONW;

typedef struct _FAX_DEVICE_STATUSA
{
  DWORD SizeOfStruct;
  LPCSTR CallerId;
  LPCSTR Csid;
  DWORD CurrentPage;
  DWORD DeviceId;
  LPCSTR DeviceName;
  LPCSTR DocumentName;
  DWORD JobType;
  LPCSTR PhoneNumber;
  LPCSTR RoutingString;
  LPCSTR SenderName;
  LPCSTR RecipientName;
  DWORD Size;
  FILETIME StartTime;
  DWORD Status;
  LPCSTR StatusString;
  FILETIME SubmittedTime;
  DWORD TotalPages;
  LPCSTR Tsid;
  LPCSTR UserName;
} FAX_DEVICE_STATUSA, *PFAX_DEVICE_STATUSA;

typedef struct _FAX_DEVICE_STATUSW
{
  DWORD SizeOfStruct;
  LPCWSTR CallerId;
  LPCWSTR Csid;
  DWORD CurrentPage;
  DWORD DeviceId;
  LPCWSTR DeviceName;
  LPCWSTR DocumentName;
  DWORD JobType;
  LPCWSTR PhoneNumber;
  LPCWSTR RoutingString;
  LPCWSTR SenderName;
  LPCWSTR RecipientName;
  DWORD Size;
  FILETIME StartTime;
  DWORD Status;
  LPCWSTR StatusString;
  FILETIME SubmittedTime;
  DWORD TotalPages;
  LPCWSTR Tsid;
  LPCWSTR UserName;
} FAX_DEVICE_STATUSW, *PFAX_DEVICE_STATUSW;

typedef struct _FAX_LOG_CATEGORYA
{
  LPCSTR Name;
  DWORD Category;
  DWORD Level;
} FAX_LOG_CATEGORYA, *PFAX_LOG_CATEGORYA;

typedef struct _FAX_LOG_CATEGORYW
{
  LPCWSTR Name;
  DWORD Category;
  DWORD Level;
} FAX_LOG_CATEGORYW, *PFAX_LOG_CATEGORYW;

typedef struct _FAX_CONTEXT_INFOA
{
  DWORD SizeOfStruct;
  HDC hDC;
  CHAR ServerName[MAX_COMPUTERNAME_LENGTH + 1];
} FAX_CONTEXT_INFOA, *PFAX_CONTEXT_INFOA;

typedef struct _FAX_CONTEXT_INFOW
{
  DWORD SizeOfStruct;
  HDC hDC;
  WCHAR ServerName[MAX_COMPUTERNAME_LENGTH + 1];
} FAX_CONTEXT_INFOW, *PFAX_CONTEXT_INFOW;

typedef struct _FAX_PRINT_INFOA
{
  DWORD SizeOfStruct;
  LPCSTR DocName;
  LPCSTR RecipientName;
  LPCSTR RecipientNumber;
  LPCSTR SenderName;
  LPCSTR SenderCompany;
  LPCSTR SenderDept;
  LPCSTR SenderBillingCode;
  LPCSTR Reserved;
  LPCSTR DrEmailAddress;
  LPCSTR OutputFileName;
} FAX_PRINT_INFOA, *PFAX_PRINT_INFOA;

typedef struct _FAX_PRINT_INFOW
{
  DWORD SizeOfStruct;
  LPCWSTR DocName;
  LPCWSTR RecipientName;
  LPCWSTR RecipientNumber;
  LPCWSTR SenderName;
  LPCWSTR SenderCompany;
  LPCWSTR SenderDept;
  LPCWSTR SenderBillingCode;
  LPCWSTR Reserved;
  LPCWSTR DrEmailAddress;
  LPCWSTR OutputFileName;
} FAX_PRINT_INFOW, *PFAX_PRINT_INFOW;

typedef BOOL (CALLBACK *PFAX_RECIPIENT_CALLBACKA)(HANDLE FaxHandle, DWORD RecipientNumber, LPVOID Context, PFAX_JOB_PARAMA JobParams, PFAX_COVERAGE_INFOA CoverpageInfo);
typedef BOOL (CALLBACK *PFAX_RECIPIENT_CALLBACKW)(HANDLE FaxHandle, DWORD RecipientNumber, LPVOID Context, PFAX_JOB_PARAMW JobParams, PFAX_COVERAGE_INFOW CoverpageInfo);

#ifndef _DISABLE_TIDENTS

#ifdef UNICODE
typedef FAX_JOB_PARAMW FAX_JOB_PARAM;
typedef PFAX_JOB_PARAMW PFAX_JOB_PARAM;
typedef FAX_COVERAGE_INFOW FAX_COVERAGE_INFO;
typedef PFAX_COVERAGE_INFOW PFAX_COVERAGE_INFO;
typedef FAX_GLOBAL_ROUTING_INFOW FAX_GLOBAL_ROUTING_INFO;
typedef PFAX_GLOBAL_ROUTING_INFOW PFAX_GLOBAL_ROUTING_INFO;
typedef FAX_JOB_ENTRYW FAX_JOB_ENTRY;
typedef PFAX_JOB_ENTRYW PFAX_JOB_ENTRY;
typedef FAX_PORT_INFOW FAX_PORT_INFO;
typedef PFAX_PORT_INFOW PFAX_PORT_INFO;
typedef FAX_ROUTING_METHODW FAX_ROUTING_METHOD;
typedef PFAX_ROUTING_METHODW PFAX_ROUTING_METHOD;
typedef FAX_CONFIGURATIONW FAX_CONFIGURATION;
typedef PFAX_CONFIGURATIONW PFAX_CONFIGURATION;
typedef FAX_DEVICE_STATUSW FAX_DEVICE_STATUS;
typedef PFAX_DEVICE_STATUSW PFAX_DEVICE_STATUS;
typedef FAX_LOG_CATEGORYW FAX_LOG_CATEGORY;
typedef PFAX_LOG_CATEGORYW PFAX_LOG_CATEGORY;
typedef FAX_CONTEXT_INFOW FAX_CONTEXT_INFO;
typedef PFAX_CONTEXT_INFOW PFAX_CONTEXT_INFO;
typedef FAX_PRINT_INFOW FAX_PRINT_INFO;
typedef PFAX_PRINT_INFOW PFAX_PRINT_INFO;
typedef PFAX_RECIPIENT_CALLBACKA PFAX_RECIPIENT_CALLBACK;
#define FaxCompleteJobParams FaxCompleteJobParamsW
#define FaxConnectFaxServer FaxConnectFaxServerW
#define FaxEnableRoutingMethod FaxEnableRoutingMethodW
#define FaxEnumGlobalRoutingInfo FaxEnumGlobalRoutingInfoW
#define FaxEnumJobs FaxEnumJobsW
#define FaxEnumPorts FaxEnumPortsW
#define FaxEnumRoutingMethods FaxEnumRoutingMethodsW
#define FaxGetConfiguration FaxGetConfigurationW
#define FaxGetDeviceStatus FaxGetDeviceStatusW
#define FaxGetJob FaxGetJobW
#define FaxGetLoggingCategories FaxGetLoggingCategoriesW
#define FaxGetPort FaxGetPortW
#define FaxGetRoutingInfo FaxGetRoutingInfoW
#define FaxSendDocument FaxSendDocumentW
#define FaxSendDocumentForBroadcast FaxSendDocumentForBroadcastW
#define FaxSetConfiguration FaxSetConfigurationW
#define FaxSetGlobalRoutingInfo FaxSetGlobalRoutingInfoW
#define FaxSetJob FaxSetJobW
#define FaxSetLoggingCategories FaxSetLoggingCategoriesW
#define FaxSetPort FaxSetPortW
#define FaxSetRoutingInfo FaxSetRoutingInfoW
#define FaxStartPrintJob FaxStartPrintJobW
#else /* !UNICODE */
typedef FAX_JOB_PARAMA FAX_JOB_PARAM;
typedef PFAX_JOB_PARAMA PFAX_JOB_PARAM;
typedef FAX_COVERAGE_INFOA FAX_COVERAGE_INFO;
typedef PFAX_COVERAGE_INFOA PFAX_COVERAGE_INFO;
typedef FAX_GLOBAL_ROUTING_INFOA FAX_GLOBAL_ROUTING_INFO;
typedef PFAX_GLOBAL_ROUTING_INFOA PFAX_GLOBAL_ROUTING_INFO;
typedef FAX_JOB_ENTRYA FAX_JOB_ENTRY;
typedef PFAX_JOB_ENTRYA PFAX_JOB_ENTRY;
typedef FAX_PORT_INFOA FAX_PORT_INFO;
typedef PFAX_PORT_INFOA PFAX_PORT_INFO;
typedef FAX_ROUTING_METHODA FAX_ROUTING_METHOD;
typedef PFAX_ROUTING_METHODA PFAX_ROUTING_METHOD;
typedef FAX_CONFIGURATIONA FAX_CONFIGURATION;
typedef PFAX_CONFIGURATIONA PFAX_CONFIGURATION;
typedef FAX_DEVICE_STATUSA FAX_DEVICE_STATUS;
typedef PFAX_DEVICE_STATUSA PFAX_DEVICE_STATUS;
typedef FAX_LOG_CATEGORYA FAX_LOG_CATEGORY;
typedef PFAX_LOG_CATEGORYA PFAX_LOG_CATEGORY;
typedef FAX_CONTEXT_INFOA FAX_CONTEXT_INFO;
typedef PFAX_CONTEXT_INFOA PFAX_CONTEXT_INFO;
typedef FAX_PRINT_INFOA FAX_PRINT_INFO;
typedef PFAX_PRINT_INFOA PFAX_PRINT_INFO;
typedef PFAX_RECIPIENT_CALLBACKW PFAX_RECIPIENT_CALLBACK;
#define FaxCompleteJobParams FaxCompleteJobParamsA
#define FaxConnectFaxServer FaxConnectFaxServerA
#define FaxEnableRoutingMethod FaxEnableRoutingMethodA
#define FaxEnumGlobalRoutingInfo FaxEnumGlobalRoutingInfoA
#define FaxEnumJobs FaxEnumJobsA
#define FaxEnumPorts FaxEnumPortsA
#define FaxEnumRoutingMethods FaxEnumRoutingMethodsA
#define FaxGetConfiguration FaxGetConfigurationA
#define FaxGetDeviceStatus FaxGetDeviceStatusA
#define FaxGetJob FaxGetJobA
#define FaxGetLoggingCategories FaxGetLoggingCategoriesA
#define FaxGetPort FaxGetPortA
#define FaxGetRoutingInfo FaxGetRoutingInfoA
#define FaxSendDocument FaxSendDocumentA
#define FaxSendDocumentForBroadcast FaxSendDocumentForBroadcastA
#define FaxSetConfiguration FaxSetConfigurationA
#define FaxSetGlobalRoutingInfo FaxSetGlobalRoutingInfoA
#define FaxSetJob FaxSetJobA
#define FaxSetLoggingCategories FaxSetLoggingCategoriesA
#define FaxSetPort FaxSetPortA
#define FaxSetRoutingInfo FaxSetRoutingInfoA
#define FaxStartPrintJob FaxStartPrintJobA
#endif /* UNICODE */

#endif /* _DISABLE_TIDENTS */

typedef BOOL (CALLBACK *PFAX_ROUTING_INSTALLATION_CALLBACKW)(HANDLE FaxHandle, LPVOID Context, LPWSTR MethodName, LPWSTR FriendlyName, LPWSTR FunctionName, LPWSTR Guid);
#define PFAX_ROUTING_INSTALLATION_CALLBACK PFAX_ROUTING_INSTALLATION_CALLBACKW
#define FaxRegisterRoutingExtension FaxRegisterRoutingExtensionW
#define FaxRegisterServiceProvider FaxRegisterServiceProviderW

BOOL WINAPI FaxAbort(HANDLE FaxHandle, DWORD JobId);
BOOL WINAPI FaxAccessCheck(HANDLE FaxHandle, DWORD AccessMask);
BOOL WINAPI FaxClose(HANDLE FaxHandle);
BOOL WINAPI FaxCompleteJobParamsA(PFAX_JOB_PARAMA *JobParams, PFAX_COVERAGE_INFOA *CoverageInfo);
BOOL WINAPI FaxCompleteJobParamsW(PFAX_JOB_PARAMW *JobParams, PFAX_COVERAGE_INFOW *CoverageInfo);
BOOL WINAPI FaxConnectFaxServerA(LPCSTR MachineName, LPHANDLE FaxHandle);
BOOL WINAPI FaxConnectFaxServerW(LPCWSTR MachineName, LPHANDLE FaxHandle);
BOOL WINAPI FaxEnableRoutingMethodA(HANDLE FaxPortHandle, LPCSTR RoutingGuid, BOOL Enabled);
BOOL WINAPI FaxEnableRoutingMethodW(HANDLE FaxPortHandle, LPCWSTR RoutingGuid, BOOL Enabled);
BOOL WINAPI FaxEnumGlobalRoutingInfoA(HANDLE FaxHandle, PFAX_GLOBAL_ROUTING_INFOA *RoutingInfo, LPDWORD MethodsReturned);
BOOL WINAPI FaxEnumGlobalRoutingInfoW(HANDLE FaxHandle, PFAX_GLOBAL_ROUTING_INFOW *RoutingInfo, LPDWORD MethodsReturned);
BOOL WINAPI FaxEnumJobsA(HANDLE FaxHandle, PFAX_JOB_ENTRYA *JobEntry, LPDWORD JobsReturned);
BOOL WINAPI FaxEnumJobsW(HANDLE FaxHandle, PFAX_JOB_ENTRYW *JobEntry, LPDWORD JobsReturned);
BOOL WINAPI FaxEnumPortsA(HANDLE FaxHandle, PFAX_PORT_INFOA *PortInfo, LPDWORD PortsReturned);
BOOL WINAPI FaxEnumPortsW(HANDLE FaxHandle, PFAX_PORT_INFOW *PortInfo, LPDWORD PortsReturned);
BOOL WINAPI FaxEnumRoutingMethodsA(HANDLE FaxPortHandle, PFAX_ROUTING_METHODA *RoutingMethod, LPDWORD MethodsReturned);
BOOL WINAPI FaxEnumRoutingMethodsW(HANDLE FaxPortHandle, PFAX_ROUTING_METHODW *RoutingMethod, LPDWORD MethodsReturned);
VOID WINAPI FaxFreeBuffer(LPVOID Buffer);
BOOL WINAPI FaxGetConfigurationA(HANDLE FaxHandle, PFAX_CONFIGURATIONA *FaxConfig);
BOOL WINAPI FaxGetConfigurationW(HANDLE FaxHandle, PFAX_CONFIGURATIONW *FaxConfig);
BOOL WINAPI FaxGetDeviceStatusA(HANDLE FaxPortHandle, PFAX_DEVICE_STATUSA *DeviceStatus);
BOOL WINAPI FaxGetDeviceStatusW(HANDLE FaxPortHandle, PFAX_DEVICE_STATUSW *DeviceStatus);
BOOL WINAPI FaxGetJobA(HANDLE FaxHandle, DWORD JobId, PFAX_JOB_ENTRYA *JobEntry);
BOOL WINAPI FaxGetJobW(HANDLE FaxHandle, DWORD JobId, PFAX_JOB_ENTRYW *JobEntry);
BOOL WINAPI FaxGetLoggingCategoriesA(HANDLE FaxHandle, PFAX_LOG_CATEGORYA *Categories, LPDWORD NumberCategories);
BOOL WINAPI FaxGetLoggingCategoriesW(HANDLE FaxHandle, PFAX_LOG_CATEGORYW *Categories, LPDWORD NumberCategories);
BOOL WINAPI FaxGetPageData(HANDLE FaxHandle, DWORD JobId, LPBYTE *Buffer, LPDWORD BufferSize, LPDWORD ImageWidth, LPDWORD ImageHeight);
BOOL WINAPI FaxGetPortA(HANDLE FaxPortHandle, PFAX_PORT_INFOA *PortInfo);
BOOL WINAPI FaxGetPortW(HANDLE FaxPortHandle, PFAX_PORT_INFOW *PortInfo);
BOOL WINAPI FaxGetRoutingInfoA(HANDLE FaxPortHandle, LPCSTR RoutingGuid, LPBYTE *RoutingInfoBuffer, LPDWORD RoutingInfoBufferSize);
BOOL WINAPI FaxGetRoutingInfoW(HANDLE FaxPortHandle, LPCWSTR RoutingGuid, LPBYTE *RoutingInfoBuffer, LPDWORD RoutingInfoBufferSize);
BOOL WINAPI FaxInitializeEventQueue(HANDLE FaxHandle, HANDLE CompletionPort, ULONG_PTR CompletionKey, HWND hWnd, UINT MessageStart);
BOOL WINAPI FaxOpenPort(HANDLE FaxHandle, DWORD DeviceId, DWORD Flags, LPHANDLE FaxPortHandle);
BOOL WINAPI FaxPrintCoverPageA(CONST FAX_CONTEXT_INFOA *FaxContextInfo, CONST FAX_COVERAGE_INFOA *CoverPageInfo);
BOOL WINAPI FaxPrintCoverPageW(CONST FAX_CONTEXT_INFOW *FaxContextInfo, CONST FAX_COVERAGE_INFOW *CoverPageInfo);
BOOL WINAPI FaxRegisterRoutingExtensionW(HANDLE FaxHandle, LPCWSTR ExtensionName, LPCWSTR FriendlyName, LPCWSTR ImageName, PFAX_ROUTING_INSTALLATION_CALLBACK CallBack, LPVOID Context);
BOOL WINAPI FaxRegisterServiceProviderW(LPCWSTR DeviceProvider, LPCWSTR FriendlyName, LPCWSTR ImageName, LPCWSTR TspName);
BOOL WINAPI FaxSendDocumentA(HANDLE FaxHandle, LPCSTR FileName, PFAX_JOB_PARAMA JobParams, CONST FAX_COVERAGE_INFOA *CoverpageInfo, LPDWORD FaxJobId);
BOOL WINAPI FaxSendDocumentForBroadcastA(HANDLE FaxHandle, LPCSTR FileName, LPDWORD FaxJobId, PFAX_RECIPIENT_CALLBACKA FaxRecipientCallback, LPVOID Context);
BOOL WINAPI FaxSendDocumentForBroadcastW(HANDLE FaxHandle, LPCWSTR FileName, LPDWORD FaxJobId, PFAX_RECIPIENT_CALLBACKW FaxRecipientCallback, LPVOID Context);
BOOL WINAPI FaxSendDocumentW(HANDLE FaxHandle, LPCWSTR FileName, PFAX_JOB_PARAMW JobParams, CONST FAX_COVERAGE_INFOW *CoverpageInfo, LPDWORD FaxJobId);
BOOL WINAPI FaxSetConfigurationA(HANDLE FaxHandle, CONST FAX_CONFIGURATIONA *FaxConfig);
BOOL WINAPI FaxSetConfigurationW(HANDLE FaxHandle, CONST FAX_CONFIGURATIONW *FaxConfig);
BOOL WINAPI FaxSetGlobalRoutingInfoA(HANDLE FaxHandle, CONST FAX_GLOBAL_ROUTING_INFOA *RoutingInfo);
BOOL WINAPI FaxSetGlobalRoutingInfoW(HANDLE FaxHandle, CONST FAX_GLOBAL_ROUTING_INFOW *RoutingInfo);
BOOL WINAPI FaxSetJobA(HANDLE FaxHandle, DWORD JobId, DWORD Command, CONST FAX_JOB_ENTRYA *JobEntry);
BOOL WINAPI FaxSetJobW(HANDLE FaxHandle, DWORD JobId, DWORD Command, CONST FAX_JOB_ENTRYW *JobEntry);
BOOL WINAPI FaxSetLoggingCategoriesA(HANDLE FaxHandle, CONST FAX_LOG_CATEGORYA *Categories, DWORD NumberCategories);
BOOL WINAPI FaxSetLoggingCategoriesW(HANDLE FaxHandle, CONST FAX_LOG_CATEGORYW *Categories, DWORD NumberCategories);
BOOL WINAPI FaxSetPortA(HANDLE FaxPortHandle, CONST FAX_PORT_INFOA *PortInfo);
BOOL WINAPI FaxSetPortW(HANDLE FaxPortHandle, CONST FAX_PORT_INFOW *PortInfo);
BOOL WINAPI FaxSetRoutingInfoA(HANDLE FaxPortHandle, LPCSTR RoutingGuid, CONST BYTE *RoutingInfoBuffer, DWORD RoutingInfoBufferSize);
BOOL WINAPI FaxSetRoutingInfoW(HANDLE FaxPortHandle, LPCWSTR RoutingGuid, CONST BYTE *RoutingInfoBuffer, DWORD RoutingInfoBufferSize);
BOOL WINAPI FaxStartPrintJobA(LPCSTR PrinterName, CONST FAX_PRINT_INFOA *PrintInfo, LPDWORD FaxJobId, PFAX_CONTEXT_INFOA FaxContextInfo);
BOOL WINAPI FaxStartPrintJobW(LPCWSTR PrinterName, CONST FAX_PRINT_INFOW *PrintInfo, LPDWORD FaxJobId, PFAX_CONTEXT_INFOW FaxContextInfo);

#ifdef __cplusplus
}
#endif
#endif /* __WINFAX_H */

/* EOF */
