#ifndef _CRT_PRECOMP_H
#define _CRT_PRECOMP_H

/* Some global constants to hack around the msvc build */
/* These will go away or be moved soon enough */
#define _CRT_NON_CONFORMING_SWPRINTFS
#define _CRT_SECURE_NO_DEPRECATE
#define _INC_WTIME_INL
#define _INC_UTIME_INL
#define _INC_TIME_INL

/* Headers to be compiled */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <conio.h>
#include <errno.h>
#include <fcntl.h>
#include <float.h>
#include <malloc.h>
#include <math.h>
#include <limits.h>

#include <sys/stat.h>
#include <share.h>

/* PSDK/NDK Headers */
#define WIN32_NO_STATUS
#include <windows.h>
#include <ndk/ntndk.h>
#include <reactos/helper.h>

#if !defined(_MSC_VER)
  #include <stdint.h>
#endif

/* CRT Internal data */
#include <internal/atexit.h>
#include <internal/console.h>
#include <internal/file.h>
#include <internal/ieee.h>
#include <internal/tls.h>

#endif /* _CRT_PRECOMP_H */
