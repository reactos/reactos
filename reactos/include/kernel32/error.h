#ifndef _INCLUDE_KERNEL32_H
#define _INCLUDE_KERNEL32_H
/* $Id: error.h,v 1.5 2003/11/18 05:09:17 royce Exp $ */
#include <windows.h>
#define NTOS_MODE_USER
#ifndef _NTOS_H
#error you must include <ntos.h> before you can include kernel32/error.h
/*#include <ntos.h>*/
#endif/*_NTOS_H*/

#define SetLastErrorByStatus(__S__) \
 ((void)SetLastError(RtlNtStatusToDosError(__S__)))

#endif /* _INCLUDE_KERNEL32_H */
