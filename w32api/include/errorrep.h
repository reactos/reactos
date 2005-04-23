#ifndef _ERRORREP_H
#define _ERRORREP_H
#if __GNUC__ >= 3
#pragma GCC system_header
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if (_WIN32_WINNT >= 0x0501)
typedef enum {
	frrvErr = 3,
	frrvErrNoDW = 4,
	frrvErrTimeout = 5,
	frrvLaunchDebugger = 6,
	frrvOk = 0,
	frrvOkHeadless = 7,
	frrvOkManifest = 1,
	frrvOkQueued = 2
} EFaultRepRetVal;
BOOL WINAPI AddERExcludedApplicationA(LPCSTR);
BOOL WINAPI AddERExcludedApplicationW(LPCWSTR);
EFaultRepRetVal WINAPI ReportFault(LPEXCEPTION_POINTERS,DWORD);
#endif

#ifdef UNICODE
#if (_WIN32_WINNT >= 0x0501)
#define AddERExcludedApplication AddERExcludedApplicationW
#endif
#else
#if (_WIN32_WINNT >= 0x0501)
#define AddERExcludedApplication AddERExcludedApplicationA
#endif
#endif

#ifdef __cplusplus
}
#endif
#endif
