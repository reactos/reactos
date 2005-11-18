#define CRT_SECURE_NO_DEPRECATE

#define WIN32_NO_STATUS
#include <windows.h>
#define NTOS_MODE_USER
#include <ndk/ntndk.h>

#if !defined(_MSC_VER)
  #include <stdint.h>
#endif
