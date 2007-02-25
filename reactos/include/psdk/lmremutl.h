#ifndef _LMREMUTL_H
#define _LMREMUTL_H
#if __GNUC__ >=3
#pragma GCC system_header
#endif

#ifdef __cplusplus
extern "C" {
#endif
#define SUPPORTS_REMOTE_ADMIN_PROTOCOL 2
#define SUPPORTS_RPC 4
#define SUPPORTS_SAM_PROTOCOL 8
#define SUPPORTS_UNICODE 16
#define SUPPORTS_LOCAL 32
#define SUPPORTS_ANY 0xFFFFFFFF
#define NO_PERMISSION_REQUIRED 1
#define ALLOCATE_RESPONSE 2
#define USE_SPECIFIC_TRANSPORT 0x80000000
#ifndef DESC_CHAR_UNICODE
typedef CHAR DESC_CHAR;
#else
typedef WCHAR DESC_CHAR;
#endif
typedef DESC_CHAR *LPDESC;
typedef struct _TIME_OF_DAY_INFO {
	DWORD tod_elapsedt;
	DWORD tod_msecs;
	DWORD tod_hours;
	DWORD tod_mins;
	DWORD tod_secs;
	DWORD tod_hunds;
	LONG tod_timezone;
	DWORD tod_tinterval;
	DWORD tod_day;
	DWORD tod_month;
	DWORD tod_year;
	DWORD tod_weekday;
} TIME_OF_DAY_INFO,*PTIME_OF_DAY_INFO,*LPTIME_OF_DAY_INFO;
NET_API_STATUS WINAPI NetRemoteTOD(LPCWSTR,PBYTE*);
NET_API_STATUS WINAPI NetRemoteComputerSupports(LPCWSTR,DWORD,PDWORD);
NET_API_STATUS RxRemoteApi(DWORD,LPCWSTR,LPDESC,LPDESC,LPDESC,LPDESC,LPDESC,LPDESC,LPDESC,DWORD,... );
#ifdef __cplusplus
}
#endif
#endif
