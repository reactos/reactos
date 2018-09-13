#ifndef _notfcvt_h
#define _notfcvt_h

typedef enum
{
     PT_NOT_INITIALIZED     = 0   //
    ,PT_NORMAL              = 1
    ,PT_WITHREPLY
    ,PT_REPORT_TO_SENDER
    ,PT_REPORT_TO_DEST
    ,PT_INVALID
    ,PT_GROUPLEADER
    ,PT_GROUPMEMBER

} PACKAGE_TYPE;

// package flags inidicate what the package is doing and is
typedef enum
{
     PF_READY            = 0x00000001
    ,PF_RUNNING          = 0x00000002
    ,PF_WAITING          = 0x00000004
    ,PF_REVOKED          = 0x00000008
    ,PF_SUSPENDED        = 0x00000010
    ,PF_ABORTED          = 0x00000020

    // the pacakge was delivered cross process
    ,PF_CROSSPROCESS     = 0x00010000
    ,PF_SCHEDULED        = 0x00020000
    ,PF_DELIVERED        = 0x00040000
    ,PF_DISPATCHED       = 0x00080000
    
    // idle flags
    ,PF_WAITING_USER_IDLE= 0x00100000
    
} _PACKAGE_FLAGS;
typedef DWORD PACKAGE_FLAGS;

typedef enum _tagPACKAGE_CONTENT_ENUM
{
     PC_EMPTY            = 0x00000000
    ,PC_CLSIDSENDER      = 0x00000001
    ,PC_CLSIDDEST        = 0x00000002
    ,PC_GROUPCOOKIE      = 0x00000004
    ,PC_RUNCOOKIE        = 0x00000008
    ,PC_TASKTRIGGER      = 0x00000010
    ,PC_TASKDATA         = 0x00000020
    ,PC_BASECOOKIE       = 0x00000040

    ,PC_CLSID            = 0x00000100
    ,PC_SINK             = 0x00000200
    ,PC_THREADID         = 0x00000400
} PACKAGE_CONTENT_ENUM;

typedef DWORD PACKAGE_CONTENT;

struct NOTIFICATIONITEMEXTRA
{
    DELIVERMODE             deliverMode;
    FILETIME                dateNextRun;        //  Ignore
    FILETIME                datePrevRun;
    NOTIFICATIONCOOKIE      RunningCookie;      //  Ignore
    NOTIFICATIONCOOKIE      BaseCookie;         //  Ignore
    PACKAGE_TYPE            PackageType;
    PACKAGE_FLAGS           PackageFlags;
    PACKAGE_CONTENT         PackageContent;
    DWORD                   dwThreadIdDestPort; //  Ignore
    HWND                    hWndDestPort;       //  Ignore
};

typedef struct _tagSaveSTATPROPMAP
{
    DWORD           cbSize;
    DWORD           cbStrLen;
    DWORD           dwFlags;
    DWORD           cbVarSizeExtra;
} SaveSTATPROPMAP;

#ifndef __msnotify_h__

EXTERN_C const GUID NOTFCOOKIE_SCHEDULE_GROUP_DAILY;
EXTERN_C const GUID NOTFCOOKIE_SCHEDULE_GROUP_WEEKLY;
EXTERN_C const GUID NOTFCOOKIE_SCHEDULE_GROUP_MONTHLY;
EXTERN_C const GUID NOTFCOOKIE_SCHEDULE_GROUP_MANUAL;

typedef GUID NOTIFICATIONTYPE;
typedef GUID NOTIFICATIONCOOKIE;
typedef DWORD NOTIFICATIONFLAGS;

typedef 
enum _tagDELIVERMODE
    {	DM_DELIVER_PREFERED	= 0x1,
	DM_DELIVER_DELAYED	= 0x2,
	DM_DELIVER_LAST_DELAYED	= 0x4,
	DM_ONLY_IF_RUNNING	= 0x20,
	DM_THROTTLE_MODE	= 0x80,
	DM_NEED_COMPLETIONREPORT	= 0x100,
	DM_NEED_PROGRESSREPORT	= 0x200,
	DM_DELIVER_DEFAULT_THREAD	= 0x400,
	DM_DELIVER_DEFAULT_PROCESS	= 0x800
    }	_DELIVERMODE;

typedef DWORD DELIVERMODE;

typedef struct  _tagTASKDATA
    {
    ULONG cbSize;
    DWORD dwReserved;
    DWORD dwTaskFlags;
    DWORD dwPriority;
    DWORD dwDuration;
    DWORD nParallelTasks;
    }	TASK_DATA;

typedef struct _tagTASKDATA __RPC_FAR *PTASK_DATA;

typedef void *LPNOTIFICATION;
typedef struct  _tagNotificationItem
    {
    ULONG cbSize;
    LPNOTIFICATION pNotification;
    NOTIFICATIONTYPE NotificationType;
    NOTIFICATIONFLAGS NotificationFlags;
    DELIVERMODE DeliverMode;
    NOTIFICATIONCOOKIE NotificationCookie;
    TASK_TRIGGER TaskTrigger;
    TASK_DATA TaskData;
    NOTIFICATIONCOOKIE groupCookie;
    CLSID clsidSender;
    CLSID clsidDest;
    FILETIME dateLastRun;
    FILETIME dateNextRun;
    DWORD dwNotificationState;
    }	NOTIFICATIONITEM;

typedef struct _tagNotificationItem __RPC_FAR *PNOTIFICATIONITEM;

typedef DWORD GROUPMODE;

#endif // __msnotify_h__

typedef enum
{
     GS_Created     = 0
    ,GS_Running     = 1
    ,GS_Initialized = 2


} GROUP_STATE;

typedef enum
{
     GT_NORMAL     = 0x00000001
    ,GT_STATIC     = 0x00000002

} _GROUP_TYPE;

typedef DWORD GROUP_TYPE;

typedef struct _tagSCHEDULEGROUPITEM
{
    ULONG               cbSize;
    ULONG               cElements;     // the # of packages in the group
    NOTIFICATIONCOOKIE  GroupCookie;
    GROUPMODE           grfGroupMode;
    GROUP_STATE         grpState;
    TASK_TRIGGER        TaskTrigger;
    TASK_DATA           TaskData;
    GROUP_TYPE          GroupType;
    GROUPINFO           GroupInfo;
} SCHEDULEGROUPITEM, *PSCHEDULEGROUPITEM;

HRESULT ConvertIE4Subscriptions();

#endif // _notfcvt_h
