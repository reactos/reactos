/* Missing Defines and structures 
 * These are either missing from the w32api package
 * the ReactOS build system is broken and needs to
 * fixed.
 */
#ifndef _ROS_WINE_PORT
#define _ROS_WINE_PORT

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

#define HFILE_ERROR ((HFILE)-1) /* Already in winbase.h - ros is fubar */

#define strncasecmp strncmp
#define snprintf _snprintf
#define strcasecmp _stricmp

#endif /* _ROS_WINE_PORT */
