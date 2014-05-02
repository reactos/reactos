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
