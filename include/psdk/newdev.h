
#ifndef _INC_NEWDEV
#define _INC_NEWDEV

#include <pshpack1.h> 

#ifdef __cplusplus
extern "C" {
#endif


#define INSTALLFLAG_FORCE           0x00000001
#define INSTALLFLAG_READONLY        0x00000002
#define INSTALLFLAG_NONINTERACTIVE  0x00000004
#define INSTALLFLAG_BITS            0x00000007

BOOL
WINAPI
UpdateDriverForPlugAndPlayDevicesA(
  HWND hwndParent,
  LPCSTR HardwareId,
  LPCSTR FullInfPath,
  DWORD InstallFlags,
  PBOOL bRebootRequired OPTIONAL);

BOOL
WINAPI
UpdateDriverForPlugAndPlayDevicesW(
  HWND hwndParent,
  LPCWSTR HardwareId,
  LPCWSTR FullInfPath,
  DWORD InstallFlags,
  PBOOL bRebootRequired OPTIONAL);

#ifdef UNICODE
#define UpdateDriverForPlugAndPlayDevices UpdateDriverForPlugAndPlayDevicesW
#else
#define UpdateDriverForPlugAndPlayDevices UpdateDriverForPlugAndPlayDevicesA
#endif

#ifdef __cplusplus
}
#endif

#include <poppack.h>

#endif


