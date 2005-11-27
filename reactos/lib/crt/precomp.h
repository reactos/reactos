#ifndef _CRT_PRECOMP_H
#define _CRT_PRECOMP_H

/* Some global constants to hack around the msvc build */
#define _CRT_SECURE_NO_DEPRECATE
#define _INC_WTIME_INL
#define _INC_UTIME_INL
#define _INC_TIME_INL

/* Headers to be compiled */
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <float.h>
#include <malloc.h>
#include <math.h>
#include <limits.h>

/* PSDK/NDK Headers */
#define WIN32_NO_STATUS
#include <windows.h>
#include <ndk/ntndk.h>
#include <reactos/helper.h>

#if !defined(_MSC_VER)
  #include <stdint.h>
#endif

/* CRT Internal data */
#include <internal/file.h>
#include <internal/ieee.h>

#endif /* _CRT_PRECOMP_H */
