#ifndef _INCLUDE_KERNEL32_H
#define _INCLUDE_KERNEL32_H
/* $Id: error.h,v 1.4 2003/04/26 00:25:01 hyperion Exp $ */
#include <windows.h>
#define NTOS_MODE_USER
#include <ntos.h>

#define SetLastErrorByStatus(__S__) \
 ((void)SetLastError(RtlNtStatusToDosError(__S__)))

#endif /* _INCLUDE_KERNEL32_H */
