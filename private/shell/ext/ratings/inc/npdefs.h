/*****************************************************************/
/**               Microsoft Windows for Workgroups              **/
/**           Copyright (C) Microsoft Corp., 1991-1992          **/
/*****************************************************************/

/* NPCOMMON.H -- Internal standard header for network provider common library.
 *
 * ATTENTION: This file is used by 16bit components and should be
 *			  maintained as such.
 *
 * History:
 *  03/22/93    gregj   Created
 *  04/02/93    lens    Added _INC_NPDEFS define and _INC_WINDOWS test.
 *
 */
#ifndef _INC_NPDEFS
#define _INC_NPDEFS

#ifndef RC_INVOKED
#pragma warning(disable:4147)       // warning about ignoring __loadds on function
                                    // ptr decls, of which there are 5 in windows.h
#pragma warning(disable:4118)       // warning about not accepting the intrinsic function pragma
                                    // during a fast compile

// Macro to quiet compiler for an unused formal parameter.
#define UNUSED(x) ((void)(x))
#endif

#ifndef _INC_WINDOWS
#include <windows.h>
#endif

#ifdef IS_32
#ifndef _INC_NETSPI
#include <netspi.h>
#endif

#ifndef _STRING_HXX_
#include <npstring.h>
#endif
#endif  /* IS_32 */

// Fixup for when RESOURCETYPE_ANY was not compatible with NT.
// Codework: remove from system.
#define RESOURCETYPE_ANY1 RESOURCETYPE_RESERVED

#define CHAR char
#define INT int

typedef unsigned short WCHAR;
typedef unsigned short USHORT;
typedef WORD HANDLE16;

#ifndef APIENTRY
#define APIENTRY    FAR PASCAL
#endif

#define FAR_NETLIB              /* our netlib is in netapi.dll, and always far */

#ifndef IS_32
#ifndef LOADDS
#define LOADDS __loadds
#endif
#else
#define LOADDS
#endif

#ifndef HNRES
typedef HANDLE HNRES;
#endif


// That is common return type used in both common and mnr projects
#ifndef MNRSTATUS
#ifdef IS_32
#define MNRSTATUS UINT
#else
#define MNRSTATUS WORD
#endif
#endif

#define MNRENTRY DWORD APIENTRY

// Find size of structure upto and including a field that may be the last field in the structure.
#define SIZE_TO_FIELD(s,f) (sizeof(((s *)NULL)->f) + (LPBYTE)&(((s *)NULL)->f) - (LPBYTE)NULL)

// Null strings are quite often taken to be either a NULL pointer or a zero
#define IS_EMPTY_STRING(pch) ( !pch || !*(pch) )

/*******************************************************************

Macro Description:

    This macro is used to test that a LoadLibrary call succeeded.

Arguments:

    hModule          - the handle returned from the LoadLibrary call.

Notes:

    Win31 documentation says that errors are less than HINSTANCE_ERROR
    and that success is greater than 32. Since HINSTANCE_ERROR is 32,
    this leaves the value of 32 as being undefined!

*******************************************************************/

#ifdef IS_32
#define MNRVALIDDLLHANDLE(hdll) (hdll != NULL)
#else
#define MNRVALIDDLLHANDLE(hdll) (hdll > HINSTANCE_ERROR)
#endif

/*******************************************************************

Macro Description:

    This macro is used to determine if a buffer passed in has valid 
    addresses and writeable memory.

Arguments:

    lpBuffer	- the address of the buffer.

    lpcbBuffer	- the address of a DWORD containing the size of the
				  buffer that is filled in on return with the 
				  required size of the buffer if the buffer
				  is not large enough.

Evalutes to:

	An expression that returns TRUE or FALSE.

Notes:

	Only valid for Win32 applications to call.
    The macro does weak validation as it is used generically in many APIs.
	In particular, this means that the macro succeeds a NULL lpcbBuffer, 
    and zero *lpcbBuffer. In neither of these cases does it validate
	that lpBuffer is a valid address (and relies upon the behavior of 
    IsBadWritePtr when *lpcbBuffer is zero).

*******************************************************************/

#define IS_BAD_WRITE_BUFFER(lpBuffer,lpcbBuffer) \
((lpcbBuffer != NULL) && \
 (IsBadWritePtr(lpcbBuffer, sizeof(DWORD)) || \
  IsBadWritePtr(lpBuffer, *lpcbBuffer))) 

#ifdef IS_32
extern "C" { /* Know we're using C++ internally */

NLS_STR FAR * NPSERVICE NPSGetStatusText(DWORD dwError, 
										 LPBOOL pbStatic);

DWORD NPSERVICE NPSCopyNLS(NLS_STR FAR *pnlsSourceString, 
						   LPVOID lpDestBuffer, 
						   LPDWORD lpBufferSize);

}
#endif  /* IS_32 */

#endif  /* !_INC_NPDEFS */

