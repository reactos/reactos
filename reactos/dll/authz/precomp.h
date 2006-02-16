#ifndef __AUTHZ_PRECOMP_H
#define __AUTHZ_PRECOMP_H

#define _AUTHZ_
#include <windows.h>
#include <authz.h>

ULONG DbgPrint(PCH Format,...);
#ifndef DPRINT1
#define DPRINT1 DbgPrint
#endif

#ifndef UNIMPLEMENTED
#define UNIMPLEMENTED DbgPrint("AUTHZ.DLL: %s is UNIMPLEMENTED!\n", __FUNCTION__)
#endif

#if DBG

#define RESMAN_TAG  0x89ABCDEF
#define CLIENTCTX_TAG  0x789ABCDE
#define VALIDATE_RESMAN_HANDLE(handle) ASSERT(((PAUTHZ_RESMAN)handle)->Tag == RESMAN_TAG)
#define VALIDATE_CLIENTCTX_HANDLE(handle) ASSERT(((PAUTHZ_CLIENT_CONTEXT)handle)->Tag == CLIENTCTX_TAG)
#ifndef ASSERT
#define ASSERT(cond) if (!(cond)) { DbgPrint("%s:%i: ASSERTION %s failed!\n", __FILE__, __LINE__, #cond ); }
#endif

#else

#define VALIDATE_RESMAN_HANDLE(handle)
#define VALIDATE_CLIENTCTX_HANDLE(handle)
#ifndef ASSERT
#define ASSERT(cond)
#endif

#endif

typedef struct _AUTHZ_RESMAN
{
#if DBG
    DWORD Tag;
#endif

    PFN_AUTHZ_DYNAMIC_ACCESS_CHECK pfnAccessCheck;
    PFN_AUTHZ_COMPUTE_DYNAMIC_GROUPS pfnComputeDynamicGroups;
    PFN_AUTHZ_FREE_DYNAMIC_GROUPS pfnFreeDynamicGroups;
    
    DWORD flags;
    PSID UserSid;
    LUID AuthenticationId;

    WCHAR ResourceManagerName[1];
} AUTHZ_RESMAN, *PAUTHZ_RESMAN;

typedef struct _AUTHZ_CLIENT_CONTEXT
{
#if DBG
    DWORD Tag;
#endif

    PSID UserSid;

    AUTHZ_RESOURCE_MANAGER_HANDLE AuthzResourceManager;
    LUID Luid;
    LARGE_INTEGER ExpirationTime;
    AUTHZ_CLIENT_CONTEXT_HANDLE ServerContext;
    PVOID DynamicGroupArgs;
} AUTHZ_CLIENT_CONTEXT, *PAUTHZ_CLIENT_CONTEXT;

#endif /* __AUTHZ_PRECOMP_H */
/* EOF */
