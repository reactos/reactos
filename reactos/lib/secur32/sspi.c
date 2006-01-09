#include <precomp.h>

#define NDEBUG
#include <debug.h>


SECURITY_STATUS
WINAPI
EnumerateSecurityPackagesW (
	PULONG pulong,
	PSecPkgInfoW* psecpkginfow
	)
{
	DPRINT1("%s() not implemented!\n", __FUNCTION__);
	return ERROR_CALL_NOT_IMPLEMENTED;
}

SECURITY_STATUS
WINAPI
EnumerateSecurityPackagesA(
	PULONG pulong,
	PSecPkgInfoA* psecpkginfoa
	)
{
	DPRINT1("%s() not implemented!\n", __FUNCTION__);
	return ERROR_CALL_NOT_IMPLEMENTED;
}

SECURITY_STATUS
WINAPI
FreeContextBuffer (
	PVOID pvoid
	)
{
	DPRINT1("%s() not implemented!\n", __FUNCTION__);
	return ERROR_CALL_NOT_IMPLEMENTED;
}

SECURITY_STATUS
WINAPI
AcquireCredentialsHandleA ( 
    SEC_CHAR* pszPrincipal,
    SEC_CHAR* pszPackage,
    ULONG fUsage,
    PLUID pID,
    PVOID pAuth,
    SEC_GET_KEY_FN pGetKeyFn,
    PVOID pvGetKeyArgument,
    PCredHandle phCred,
    PTimeStamp pExpires
    )
{
	DPRINT1("%s() not implemented!\n", __FUNCTION__);
	return ERROR_CALL_NOT_IMPLEMENTED;
}

SECURITY_STATUS
WINAPI
InitializeSecurityContextA ( 
    PCredHandle phCred,
    PCtxtHandle phContext,
    SEC_CHAR* pszTarget,
    ULONG fContextReq,
    ULONG Reserved,
    ULONG TargetData,
    PSecBufferDesc pInput,
    ULONG Reserved2,
    PCtxtHandle phNewContext,
    PSecBufferDesc pOut,
    PULONG pfContextAttributes,
    PTimeStamp pExpires
    )
{
	DPRINT1("%s() not implemented!\n", __FUNCTION__);
	return ERROR_CALL_NOT_IMPLEMENTED;
}
