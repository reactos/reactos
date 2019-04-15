#ifndef _REACTOS_SUPPORT_CODE_H
#define _REACTOS_SUPPORT_CODE_H

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <malloc.h>
#include <windows.h>
#include <process.h>
#include <io.h>
#else
#include <alloca.h>
#include <unistd.h>
#endif

#include "../port/port.h"

// isotypes.h would provide these, but it's not available on MSVC < 2013.
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

void isohybrid_error(int eval, const char* fmt, ...);
void isohybrid_warning(const char* fmt, ...);

#define err(...)   isohybrid_error(__VA_ARGS__)
#define errx(...)  isohybrid_error(__VA_ARGS__)
#define warn(...)  isohybrid_warning(__VA_ARGS__)
#define warnx(...) isohybrid_warning(__VA_ARGS__)


#ifdef _WIN32
int fsync(int fd);
int getppid(void);
#endif

#ifdef _MSC_VER
#define fseeko _fseeki64
#define ftruncate _chsize
#endif

#endif
