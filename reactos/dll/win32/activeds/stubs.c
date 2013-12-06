#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <objbase.h>
#include <iads.h>
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
