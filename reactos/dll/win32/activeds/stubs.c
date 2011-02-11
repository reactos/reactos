#include <windows.h>
#include <iads.h>
#include <stubs.h>

// FIXME: should go to iads.h
typedef struct _adsvalue *PADSVALUE;

HRESULT
WINAPI
ADsFreeEnumerator(
    IN IEnumVARIANT *pEnumVariant)
{
	UNIMPLEMENTED;
	return E_NOTIMPL;
}

HRESULT
WINAPI
ADsBuildVarArrayStr(
    LPWSTR *lppPathNames,
    DWORD dwPathNames,
    VARIANT *pVar)
{
	UNIMPLEMENTED;
	return E_NOTIMPL;
}

HRESULT
WINAPI
ADsBuildVarArrayInt(
    LPDWORD lpdwObjectTypes,
    DWORD dwObjectTypes,
    VARIANT *pVar)
{
	UNIMPLEMENTED;
	return E_NOTIMPL;
}

VOID
WINAPI
ADsSetLastError(
    IN DWORD dwErr,
    IN LPCWSTR pszError,
    IN LPCWSTR pszProvider)

{
	UNIMPLEMENTED;
}

LPVOID
WINAPI
AllocADsMem(DWORD cb)
{
	UNIMPLEMENTED;
	return NULL;
}

LPVOID
WINAPI
ReallocADsMem(
   LPVOID pOldMem,
   DWORD cbOld,
   DWORD cbNew)
{
	UNIMPLEMENTED;
	return NULL;
}

LPWSTR
WINAPI
AllocADsStr(LPCWSTR pStr)
{
	UNIMPLEMENTED;
	return NULL;
}

BOOL
WINAPI
FreeADsStr(IN LPWSTR pStr)
{
	UNIMPLEMENTED;
	return FALSE;
}

BOOL
WINAPI
ReallocADsStr(
   IN OUT LPWSTR *ppStr,
   IN LPWSTR pStr)
{
	UNIMPLEMENTED;
	return FALSE;
}

HRESULT
WINAPI
ADsEncodeBinaryData(
   PBYTE pbSrcData,
   DWORD dwSrcLen,
   OUT LPWSTR *ppszDestData)
{
	UNIMPLEMENTED;
	return E_NOTIMPL;
}

HRESULT
WINAPI
PropVariantToAdsType(
    VARIANT * pVariant,
    DWORD dwNumVariant,
    PADSVALUE *ppAdsValues,
    PDWORD pdwNumValues)
{
	UNIMPLEMENTED;
	return E_NOTIMPL;
}

HRESULT
WINAPI
AdsTypeToPropVariant(
    PADSVALUE pAdsValues,
    DWORD dwNumValues,
    VARIANT * pVariant)
{
	UNIMPLEMENTED;
	return E_NOTIMPL;
}

void
WINAPI
AdsFreeAdsValues(
    PADSVALUE pAdsValues,
    DWORD dwNumValues)
{
	UNIMPLEMENTED;
}

HRESULT
WINAPI
ADsDecodeBinaryData(
   LPCWSTR szSrcData,
   PBYTE  *ppbDestData,
   ULONG  *pdwDestLen)
{
	UNIMPLEMENTED;
	return E_NOTIMPL;
}

int AdsTypeToPropVariant2()
{
	DPRINT1("WARNING: calling stub AdsTypeToPropVariant2()\n");
	__wine_spec_unimplemented_stub("activeds.dll", __FUNCTION__);
	return 0;
}

int PropVariantToAdsType2()
{
	DPRINT1("WARNING: calling stub PropVariantToAdsType2()\n");
	__wine_spec_unimplemented_stub("activeds.dll", __FUNCTION__);
	return 0;
}

int ConvertSecDescriptorToVariant()
{
	DPRINT1("WARNING: calling stub ConvertSecDescriptorToVariant()\n");
	__wine_spec_unimplemented_stub("activeds.dll", __FUNCTION__);
	return 0;
}

int ConvertSecurityDescriptorToSecDes()
{
	DPRINT1("WARNING: calling stub ConvertSecurityDescriptorToSecDes()\n");
	__wine_spec_unimplemented_stub("activeds.dll", __FUNCTION__);
	return 0;
}

