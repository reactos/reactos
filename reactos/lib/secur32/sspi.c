#include <windows.h>
#define NTOS_MODE_USER
#include <ndk/ntndk.h>
#include <lsass/lsass.h>

#define NDEBUG
#include <debug.h>

#include <ntsecapi.h>
#include <security.h>
#include <sspi.h>



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
FreeContextBuffer (
	PVOID pvoid
	)
{
	DPRINT1("%s() not implemented!\n", __FUNCTION__);
	return ERROR_CALL_NOT_IMPLEMENTED;
}
