/* Missing Defines and structures 
 * These are either missing from the w32api package
 * the ReactOS build system is broken and needs to
 * fixed.
 */

/* Wine wrapper for ReactOS */
#ifndef __REACTOS_WINE_WINEROS_H
#define __REACTOS_WINE_WINEROS_H

typedef unsigned short u_short;
typedef unsigned long u_long;

typedef signed char      INT8, *PINT8;
typedef signed short     INT16, *PINT16;
typedef signed int       INT32, *PINT32;
typedef unsigned char    UINT8, *PUINT8;
typedef unsigned short   UINT16, *PUINT16;
typedef unsigned int     UINT32, *PUINT32;
typedef signed int       LONG32, *PLONG32;
typedef unsigned int     ULONG32, *PULONG32;
typedef unsigned int     DWORD32, *PDWORD32;
typedef INT16     *LPINT16;
typedef UINT16    *LPUINT16;

#ifndef HFILE_ERROR 
#define HFILE_ERROR ((HFILE)-1) /* Already in winbase.h - ros is fubar */
#endif

#define strncasecmp strncmp
#define snprintf _snprintf
#define strcasecmp _stricmp

#ifndef FD_SETSIZE
#define FD_SETSIZE 64
#endif

#endif  /* __REACTOS_WINE_WINEROS_H */
