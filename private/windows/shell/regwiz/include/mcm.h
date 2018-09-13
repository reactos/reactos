#ifndef MCM_H
#define MCM_H

#ifndef TAPI_H
//#define TAPI_CURRENT_VERSION 0x00010004
#include <tapi.h>
#endif

#include <tchar.h>

#define INVALID_PORTID	0xFFFFFFFF
#define fTrue			1
#define fFalse			0
#define Try				__try
#define Leave			__leave
#define Finally			__finally

typedef enum _MODEMSTATUS
{
	kMsModemOk,
	kMsModemNotFound,
	kMsModemTooSlow
}MODEMSTATUS;

#ifdef __cplusplus
extern "C"
{
#endif
MODEMSTATUS MSEnsureModemTAPI (HINSTANCE hInstance, HWND hwnd);
BOOL FGetModemSpeed(HINSTANCE hInstance, DWORD dwDevice, PDWORD pdwSpeed);
BOOL FGetDeviceID(HINSTANCE hInstance, HLINEAPP *phLineApp, PDWORD pdwAPI, PDWORD pdwDevice, DWORD dwIndex);
MODEMSTATUS MSDetectModemTAPI(HINSTANCE hInstance);
#ifdef __cplusplus
}
#endif
#endif // _TAPI
