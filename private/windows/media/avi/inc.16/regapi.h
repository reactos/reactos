/* *********************************************************************
 * RegAPI.h Header file for registry base api function prototypes 
 *      for those who link to Real mode registry Library
 * Microsoft Corporation 
 * Copyright 1993
 * 
 * Author:  Nagarajan Subramaniyan 
 * Created: 11/5/92
 *  
 * Modification history:
 *      1/20/94     DONALDM     Wrapped LPSTR and others with ifndef
 *                              _INC_WINDOWS, since windows.h typedefs
 *                              these things.  WARNING YOU MUST INCLUDE
 *                              WINDOWS.H before REGAPI.H.
 *      1/25/94     DONALDM     Removed WINDOWS specific stuff since this
 *                              file should ONLY BE USED BY DOS APPS!!!
 * **********************************************************************
*/

//---------------------------------------------------------------------
#ifdef _INC_WINDOWS
    #pragma message( "WARNING RegAPI.H is a DOS ONLY header file" )

#else	//ifndef INC_WINDOWS

#ifndef HKEY
    #define HKEY        DWORD
    #define LPHKEY	HKEY FAR *
#endif

#ifndef FAR
    #define FAR         _far
#endif

#ifndef NEAR
    #define NEAR        _near
#endif

#ifndef PASCAL
    #define PASCAL      _pascal
#endif

#ifndef CDECL
    #define CDECL       _cdecl
#endif

#ifndef CONST
    #define CONST       const
#endif

typedef char FAR*       LPSTR;
typedef const char FAR* LPCSTR;

typedef BYTE FAR*       LPBYTE;
typedef const BYTE FAR* LPCBYTE;

typedef void FAR*       LPVOID;

#ifdef STRICT
typedef signed long     LONG;
#else
#define LONG            long
#endif

#ifndef WINAPI
#define WINAPI      _far _pascal
#endif


#endif		// ifndef INC_WINDOWS

//--------------------------------------------------------------------------

		
/* allowed data types */
#ifndef REG_SZ
#define REG_SZ      0x0001
#endif

#ifndef REG_BINARY
#define REG_BINARY  0x0003
#endif	//ifndef REG_SZ

/* Pre-defined KEYS */

#ifndef HKEY_LOCAL_MACHINE

#define HKEY_CLASSES_ROOT       ((HKEY)  0x80000000)
#define HKEY_CURRENT_USER       ((HKEY)  0x80000001)
#define HKEY_LOCAL_MACHINE      ((HKEY)  0x80000002)
#define HKEY_USERS              ((HKEY)	 0x80000003)
#define HKEY_PERFORMANCE_DATA   ((HKEY)  0x80000004)
#define HKEY_CURRENT_CONFIG     ((HKEY)  0x80000005)
#define HKEY_DYN_DATA           ((HKEY)  0x80000006)

#endif	// ifndef HKEY_LOCAL_MACHINE

#ifndef REG_NONE
#define REG_NONE    0       // unknown data type 
#endif

/* note that these values are different from win 3.1; these are the same as
    the one used by Win 32 
*/

/* XLATOFF */

/* real mode Registry API entry points, if using direct entry points */ 

/* MODULE: RBAPI.c      */
/* Win 3.1 Compatible APIs */

LONG FAR _cdecl KRegOpenKey(HKEY, LPCSTR, LPHKEY);
LONG FAR _cdecl KRegCreateKey(HKEY, LPCSTR, LPHKEY);
LONG FAR _cdecl KRegCloseKey(HKEY);
LONG FAR _cdecl KRegDeleteKey(HKEY, LPCSTR);
 LONG FAR _cdecl KRegSetValue16(HKEY, LPCSTR, DWORD, LPCSTR, DWORD);
 LONG FAR _cdecl KRegQueryValue16(HKEY, LPCSTR, LPSTR, LONG FAR*);
LONG FAR _cdecl KRegEnumKey(HKEY, DWORD, LPSTR, DWORD);

/* New APIs from win 32 */
LONG FAR _cdecl KRegDeleteValue(HKEY, LPCSTR);
LONG FAR _cdecl KRegEnumValue(HKEY, DWORD, LPCSTR,
                      LONG FAR *, DWORD, LONG FAR *, LPBYTE,
                      LONG FAR *);
LONG FAR _cdecl KRegQueryValueEx(HKEY, LPCSTR, LONG FAR *, LONG FAR *,
 		    LPBYTE, LONG FAR *);
LONG FAR _cdecl KRegSetValueEx(HKEY, LPCSTR, DWORD, DWORD, LPBYTE, DWORD);
LONG FAR _cdecl KRegFlushKey(HKEY);
LONG FAR _cdecl KRegSaveKey(HKEY, LPCSTR,LPVOID);
LONG FAR _cdecl KRegLoadKey(HKEY, LPCSTR,LPCSTR);
LONG FAR _cdecl KRegUnLoadKey(HKEY, LPCSTR);


/* other APIs */
DWORD FAR _cdecl KRegInit(LPSTR lpszSystemFile,LPSTR lpszUserFile,DWORD dwFlags);
        // should be called before any other Reg APIs 
// If one of the file name ptrs is a NULL ptr, RegInit will ignore init
// for that file and all Predefined keys for that file.
//
// FLAG BITS for dwFlags:


#define REGINIT_CREATENEW   1   
        /* create new file if give file not found/cannot be opened */

#define REGINIT_RECOVER     2
        /* do init and if file is corrupt try to recover before 
            giving up 
        */

#define REGINIT_REPLACE_IFBAD   4       
        /* do init, recover if file is corrupt and if recover is
            is impossible, replace with an empty file
        */

VOID    FAR _cdecl CleanupRegistry();
        /* This procedure frees all memory allocated by Registry */
        /* if you call this, to use the registry again, you need */
        /* to call RegInit again */

DWORD FAR _cdecl KRegFlush(VOID);
        // flushes the registry files to disk
        // should be done before termination. No harm in calling
        // if registry is not dirty.

WORD FAR _cdecl KRegSetErrorCheckingLevel(WORD wErrLevel);
	// Set to 0 to disable checksum, 255 to enable checksum

#if 0
DWORD FAR _cdecl KRegFlushKey(HKEY);
#endif


/* Internal  APIs - do not use */
/* Modified from Win 3.1 */
DWORD FAR _cdecl KRegQueryValue (HKEY hKey,LPSTR lpszSubKey, LPSTR lpszValueName,DWORD FAR *lpdwType,LPSTR lpValueBuf, DWORD FAR *ldwBufSize);
DWORD FAR _cdecl KRegSetValue(HKEY hKey,LPSTR lpszSubKey,LPSTR lpszValueName,DWORD dwType,LPBYTE lpszValue,DWORD dwValSize);


/* XLATON */
#ifndef SETUPX_INC
/* defines for changing registry API names for direct callers */
#define RegInit         KRegInit
#define RegFlush        KRegFlush
#define RegOpenKey      KRegOpenKey
#define RegCreateKey    KRegCreateKey
#define RegCloseKey     KRegCloseKey
#define RegDeleteKey    KRegDeleteKey
#define RegDeleteValue  KRegDeleteValue
#define RegEnumKey      KRegEnumKey
#define RegEnumValue    KRegEnumValue
#define RegQueryValue   KRegQueryValue16
#define RegQueryValueEx KRegQueryValueEx
#define RegSetValue     KRegSetValue16
#define RegSetValueEx   KRegSetValueEx
#define RegFlushKey     KRegFlushKey
#endif          /* #ifndef IS_SETUP */

// Equates for registry function for calling the single entry point
// Registry

#define OpenKey     0L
#define CreateKey   1L
#define CloseKey    2L
#define DeleteKey   3L
#define SetValue    4L
#define QueryValue  5L
#define EnumKey     6L
#define DeleteValue 7L
#define EnumValue   8L
#define QueryValueEx    9L
#define SetValueEx  10L
#define FlushKey    11L
#define Init        12L
#define Flush       13L

/* return codes from Chicago Registration functions */
#ifndef ERROR_BADDB
#define ERROR_BADDB                      1009L
#endif

#ifndef ERROR_MORE_DATA
#define ERROR_MORE_DATA                  234L    
#endif

#ifndef ERROR_BADKEY
#define ERROR_BADKEY             1010L
#endif

#ifndef ERROR_CANTOPEN
#define ERROR_CANTOPEN                   1011L
#endif

#ifndef ERROR_CANTREAD
#define ERROR_CANTREAD                   1012L
#define ERROR_CANTWRITE                  1013L
#endif

#ifndef ERROR_REGISTRY_CORRUPT
#define ERROR_REGISTRY_CORRUPT           1015L
#define ERROR_REGISTRY_IO_FAILED         1016L
#endif

#ifndef ERROR_KEY_DELETED
#define ERROR_KEY_DELETED                1018L
#endif

#ifndef ERROR_OUTOFMEMORY
#define ERROR_OUTOFMEMORY          14L
#endif

#ifndef ERROR_INVALID_PARAMETER
#define ERROR_INVALID_PARAMETER        87L
#endif

#ifndef ERROR_NO_MORE_ITEMS
#define ERROR_NO_MORE_ITEMS       259L
#endif  


#ifndef  ERROR_SUCCESS           
#define ERROR_SUCCESS           0L
#endif

#ifndef  ERROR_ACCESS_DENIED     
#define ERROR_ACCESS_DENIED     8L
#endif

