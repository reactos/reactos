
#include <prsht.h>

#define TARGET_DEVICE_FRIENDLY_NAME         "TargetDeviceFriendlyName"
#define TARGET_DEVICE_OPEN_EXCLUSIVELY      "TargetDeviceOpenExclusively"
#define VFW_HIDE_SETTINGS_PAGE              0x00000001
#define VFW_HIDE_VIDEOSRC_PAGE              0x00000002
#define VFW_HIDE_CAMERACONTROL_PAGE         0x00000004
#define VFW_HIDE_ALL_PAGES                  (VFW_HIDE_SETTINGS_PAGE | VFW_HIDE_VIDEOSRC_PAGE  | VFW_HIDE_CAMERACONTROL_PAGE)
#define VFW_OEM_ADD_PAGE                    0x80000000
#define VFW_USE_DEVICE_HANDLE               0x00000001
#define VFW_USE_STREAM_HANDLE               0x00000002
#define VFW_QUERY_DEV_CHANGED               0x00000100


typedef  DWORD (CALLBACK FAR * VFWWDMExtensionProc)
  (LPVOID pfnDeviceIoControl, LPFNADDPROPSHEETPAGE pfnAddPropertyPage, LPARAM lParam);

typedef BOOL (CALLBACK FAR * LPFNEXTDEVIO)(
    LPARAM lParam,
    DWORD dwFlags,
    DWORD dwIoControlCode, 
    LPVOID lpInBuffer, 
    DWORD nInBufferSize,
    LPVOID lpOutBuffer,
    DWORD nOutBufferSize,
    LPDWORD lpBytesReturned,
    LPOVERLAPPED lpOverlapped);

