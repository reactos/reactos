#define _CRT_SECURE_NO_DEPRECATE
#define _INC_WTIME_INL
#define _INC_UTIME_INL
#define _INC_TIME_INL

#include <stdio.h>
#include <errno.h>
#include <malloc.h>
#include <math.h>

#define WIN32_NO_STATUS
#include <windows.h>
#define NTOS_MODE_USER
#include <ndk/ntndk.h>

#if !defined(_MSC_VER)
  #include <stdint.h>
#endif

/* This file is a hack and should for the most part go away */
#include <internal/file.h>
