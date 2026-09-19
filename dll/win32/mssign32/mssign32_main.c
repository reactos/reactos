/*
 * MSSIGN32 implementation
 *
 * Copyright 2009 Austin English
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wincrypt.h"

#include "wine/debug.h"
#include "wine/heap.h"
#include "wine/mssign.h"

WINE_DEFAULT_DEBUG_CHANNEL(mssign);

HRESULT WINAPI PvkGetCryptProv(HWND hwnd, LPCWSTR pwszCaption, LPCWSTR pwszCapiProvider,
                    DWORD dwProviderType, LPCWSTR pwszPvkFile, LPCWSTR pwszKeyContainerName,
                    DWORD *pdwKeySpec, LPWSTR *ppwszTmpContainer, HCRYPTPROV *phCryptProv)
{
    FIXME("%p %s %s %ld %s %s %p %p %p stub\n", hwnd, debugstr_w(pwszCaption), debugstr_w(pwszCapiProvider),
                    dwProviderType, debugstr_w(pwszPvkFile), debugstr_w(pwszKeyContainerName),
                    pdwKeySpec, ppwszTmpContainer, phCryptProv);

    return E_FAIL;
}

BOOL WINAPI PvkPrivateKeyAcquireContextFromMemory(LPCWSTR pwszProvName, DWORD dwProvType,
                    BYTE *pbData, DWORD cbData, HWND hwndOwner, LPCWSTR pwszKeyName,
                    DWORD *pdwKeySpec, HCRYPTPROV *phCryptProv, LPWSTR *ppwszTmpContainer)
{
    FIXME("%s %ld %p %ld %p %s %p %p %p stub\n", debugstr_w(pwszProvName), dwProvType,
                    pbData, cbData, hwndOwner, debugstr_w(pwszKeyName), pdwKeySpec,
                    phCryptProv, ppwszTmpContainer);

    return FALSE;
}

void WINAPI PvkFreeCryptProv(HCRYPTPROV hProv, LPCWSTR pwszCapiProvider, DWORD dwProviderType,
                    LPWSTR pwszTmpContainer)
{
    FIXME("%08Ix %s %ld %s stub\n", hProv, debugstr_w(pwszCapiProvider), dwProviderType,
                    debugstr_w(pwszTmpContainer));
}

HRESULT WINAPI SignerSign(SIGNER_SUBJECT_INFO *subject, SIGNER_CERT *cert, SIGNER_SIGNATURE_INFO *signature,
        SIGNER_PROVIDER_INFO *provider, const WCHAR *timestamp, CRYPT_ATTRIBUTES *attrs, void *sip_data)
{
    FIXME("%p %p %p %p %s %p %p stub\n", subject, cert, signature, provider, debugstr_w(timestamp), attrs, sip_data);
    return E_NOTIMPL;
}

HRESULT WINAPI SignerSignEx(DWORD flags, SIGNER_SUBJECT_INFO *subject_info, SIGNER_CERT *signer_cert,
                            SIGNER_SIGNATURE_INFO *signature_info, SIGNER_PROVIDER_INFO *provider_info,
                            const WCHAR *http_time_stamp, CRYPT_ATTRIBUTES *request, void *sip_data,
                            SIGNER_CONTEXT **signer_context)
{
    FIXME("%lx %p %p %p %p %s %p %p %p stub\n", flags, subject_info, signer_cert, signature_info, provider_info,
                    wine_dbgstr_w(http_time_stamp), request, sip_data, signer_cert);
    return E_NOTIMPL;
}

HRESULT WINAPI SignerFreeSignerContext(SIGNER_CONTEXT *signer_context)
{
    heap_free(signer_context);
    return S_OK;
}
