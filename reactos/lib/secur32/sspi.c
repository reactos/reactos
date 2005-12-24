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
