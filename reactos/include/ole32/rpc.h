/**************************************
 *    RPC interface
 *
 */
#ifndef __WINE_RPC_H
#define __WINE_RPC_H

#if !defined(RPC_NO_WINDOWS_H) && !defined(__WINE__)
#include "windows.h"
#endif

#define __RPC_FAR
#define __RPC_API  WINAPI
#define __RPC_USER WINAPI
#define __RPC_STUB WINAPI
#define RPC_ENTRY  WINAPI
typedef long RPC_STATUS;

typedef void* I_RPC_HANDLE;

#ifndef _GUID_DEFINED
#define _GUID_DEFINED
typedef struct _GUID
{
    DWORD Data1;
    WORD  Data2;
    WORD  Data3;
    BYTE  Data4[8];
} GUID;
#endif

#ifndef UUID_DEFINED
#define UUID_DEFINED
typedef GUID UUID;
#endif

RPC_STATUS RPC_ENTRY UuidCreate(UUID *Uuid);

#include "rpcdce.h"
/* #include "rpcnsi.h" */
/* #include "rpcnterr.h" */
/* #include "excpt.h" */
//#include "winerror.h"

#endif /*__WINE_RPC_H */
