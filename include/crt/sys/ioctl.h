/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within this package.
 */
#ifndef _SYS_IOCTL_H
#define _SYS_IOCTL_H

#include <sys/cdefs.h>

#define WINDOWS_POST	0	/* Set write() behavior to PostMessage() */
#define WINDOWS_SEND	1	/* Set write() behavior to SendMessage() */
#define WINDOWS_HWND	2	/* Set hWnd for read() calls */

__BEGIN_DECLS

int ioctl (int __fd, int __cmd, ...);

__END_DECLS

#endif
