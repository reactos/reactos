#include "precomp.h"

#include <stubs.h>

// FIXME: should go to iads.h
typedef struct _adsvalue *PADSVALUE;

HRESULT
WINAPI
PropVariantToAdsType(
    VARIANT * pVariant,
    DWORD dwNumVariant,
    PADSVALUE *ppAdsValues,
    PDWORD pdwNumValues)
{
    DbgPrint("ACTIVEDS: %s is unimplemented, please try again later.\n", __FUNCTION__);
    return E_NOTIMPL;
}

HRESULT
WINAPI
AdsTypeToPropVariant(
    PADSVALUE pAdsValues,
    DWORD dwNumValues,
    VARIANT * pVariant)
{
    DbgPrint("ACTIVEDS: %s is unimplemented, please try again later.\n", __FUNCTION__);
    return E_NOTIMPL;
}

void
WINAPI
AdsFreeAdsValues(
    PADSVALUE pAdsValues,
    DWORD dwNumValues)
{
    DbgPrint("ACTIVEDS: %s is unimplemented, please try again later.\n", __FUNCTION__);
}

HRESULT
WINAPI
ADsDecodeBinaryData(
   LPCWSTR szSrcData,
   PBYTE  *ppbDestData,
   ULONG  *pdwDestLen)
{
    DbgPrint("ACTIVEDS: %s is unimplemented, please try again later.\n", __FUNCTION__);
    return E_NOTIMPL;
}

int AdsTypeToPropVariant2()
{
    DbgPrint("WARNING: calling stub AdsTypeToPropVariant2()\n");
    __wine_spec_unimplemented_stub("activeds.dll", __FUNCTION__);
    return 0;
}

int PropVariantToAdsType2()
{
    DbgPrint("WARNING: calling stub PropVariantToAdsType2()\n");
    __wine_spec_unimplemented_stub("activeds.dll", __FUNCTION__);
    return 0;
}

int ConvertSecDescriptorToVariant()
{
    DbgPrint("WARNING: calling stub ConvertSecDescriptorToVariant()\n");
    __wine_spec_unimplemented_stub("activeds.dll", __FUNCTION__);
    return 0;
}

int ConvertSecurityDescriptorToSecDes()
{
    DbgPrint("WARNING: calling stub ConvertSecurityDescriptorToSecDes()\n");
    __wine_spec_unimplemented_stub("activeds.dll", __FUNCTION__);
    return 0;
}

int ConvertTrusteeToSid()
{
    DbgPrint("WARNING: calling stub ConvertTrusteeToSid()\n");
    __wine_spec_unimplemented_stub("activeds.dll", __FUNCTION__);
    return 0;
}

HRESULT
WINAPI
BinarySDToSecurityDescriptor(
    _In_ PSECURITY_DESCRIPTOR pSecurityDescriptor,
    _Out_ VARIANT* pVarsec,
    _In_ LPCWSTR pszServerName,
    _In_ LPCWSTR userName,
    _In_ LPCWSTR passWord,
    _In_ DWORD dwFlags)
{
    DbgPrint("ACTIVEDS: %s is unimplemented, please try again later.\n", __FUNCTION__);
    return E_NOTIMPL;
}

HRESULT
WINAPI
SecurityDescriptorToBinarySD(
  _In_ VARIANT vVarSecDes,
  _Out_ PSECURITY_DESCRIPTOR *ppSecurityDescriptor,
  _Out_ PDWORD pdwSDLength,
  _In_ LPCWSTR pszServerName,
  _In_ LPCWSTR userName,
  _In_ LPCWSTR passWord,
  _In_ DWORD dwFlags)
{
    DbgPrint("ACTIVEDS: %s is unimplemented, please try again later.\n", __FUNCTION__);
    return E_NOTIMPL;
}

