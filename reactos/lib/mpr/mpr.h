typedef struct
{
    DWORD  cbStructure;
    HWND   hwndOwner;
    LPCSTR lpResource;
    LPSTR  lpUsername;
    DWORD  cbUsername;
    LPSTR  lpPassword;
    DWORD  cbPassword;
    LPSTR  lpOrgUnit;
    DWORD  cbOrgUnit;
    LPCSTR lpOUTitle;
    LPCSTR lpExplainText;
    LPCSTR lpDefaultUserName;
    DWORD  dwFlags;

} AUTHDLGSTRUCTA, *LPAUTHDLGSTRUCTA;

typedef struct
{
    DWORD   cbStructure;
    DWORD   dwNotifyStatus;
    DWORD   dwOperationStatus;
    LPVOID  lpNPContext;

} NOTIFYINFO, *LPNOTIFYINFO;

typedef struct tagPASSWORD_CACHE_ENTRY {
    WORD cbEntry;
    WORD cbResource;
    WORD cbPassword;
    BYTE iEntry;
    BYTE nType;
    BYTE abResource[1];
} PASSWORD_CACHE_ENTRY;

enum NOTIFYTYPE { NotifyAddConnection,
                  NotifyCancelConnection,
                  NotifyGetConnectionPerformance };
		  
#define HPROVIDER LPVOID
#define WN_NOT_SUPPORTED 0x0001
#define WN_NOT_CONNECTED ERROR_NOT_CONNECTED
#define WN_BAD_LOCALNAME ERROR_BAD_DEVICE
typedef HPROVIDER *PHPROVIDER;
typedef DWORD (CALLBACK *NOTIFYCALLBACK)(LPNOTIFYINFO,LPVOID);
typedef BOOL (CALLBACK *ENUMPASSWORDPROC)(PASSWORD_CACHE_ENTRY *, DWORD);
