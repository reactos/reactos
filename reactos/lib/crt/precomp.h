#define CRT_SECURE_NO_DEPRECATE
#define _INC_WTIME_INL
#define _INC_UTIME_INL
#define _INC_TIME_INL

#define WIN32_NO_STATUS
#include <windows.h>
#define NTOS_MODE_USER
#include <ndk/ntndk.h>

#if !defined(_MSC_VER)
  #include <stdint.h>
#endif
