/*
 * Copyright 2001 Rein Klazes
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

#include "config.h"

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "guiddef.h"
#include "wintrust.h"
#include "softpub.h"
#include "mscat.h"
#include "objbase.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wintrust);


/***********************************************************************
 *		DllMain  (WINTRUST.@)
 */
BOOL WINAPI DllMain( HINSTANCE inst, DWORD reason, LPVOID reserved )
{
    switch(reason)
    {
    case DLL_WINE_PREATTACH:
        return FALSE;  /* prefer native version */
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls( inst );
        break;
    }
    return TRUE;
}

/***********************************************************************
 *		TrustIsCertificateSelfSigned (WINTRUST.@)
 */
BOOL WINAPI TrustIsCertificateSelfSigned( PCCERT_CONTEXT cert )
{
    BOOL ret;

    TRACE("%p\n", cert);
    ret = CertCompareCertificateName(cert->dwCertEncodingType,
     &cert->pCertInfo->Subject, &cert->pCertInfo->Issuer);
    return ret;
}

/***********************************************************************
 *		WinVerifyTrust (WINTRUST.@)
 *
 * Verifies an object by calling the specified trust provider.
 *
 * PARAMS
 *   hwnd       [I] Handle to a caller window.
 *   ActionID   [I] Pointer to a GUID that identifies the action to perform.
 *   ActionData [I] Information used by the trust provider to verify the object.
 *
 * RETURNS
 *   Success: Zero.
 *   Failure: A TRUST_E_* error code.
 *
 * NOTES
 *   Trust providers can be found at:
 *   HKLM\SOFTWARE\Microsoft\Cryptography\Providers\Trust\
 */
LONG WINAPI WinVerifyTrust( HWND hwnd, GUID *ActionID, LPVOID ActionData )
{
    static const GUID gen_verify_v2 = WINTRUST_ACTION_GENERIC_VERIFY_V2;

    FIXME("%p %s %p\n", hwnd, debugstr_guid(ActionID), ActionData);

    /* Trust providers can be found at:
     * HKLM\SOFTWARE\Microsoft\Cryptography\Providers\Trust\CertCheck\
     *
     * Process Explorer expects a correct implementation, so we
     * return TRUST_E_PROVIDER_UNKNOWN.
     *
     * Girotel needs ERROR_SUCCESS.
     *
     * For now return TRUST_E_PROVIDER_UNKNOWN only when
     * ActionID = WINTRUST_ACTION_GENERIC_VERIFY_V2.
     *
     */

    if (IsEqualCLSID(ActionID, &gen_verify_v2))
        return TRUST_E_PROVIDER_UNKNOWN;

    return ERROR_SUCCESS;
}

/***********************************************************************
 *		WinVerifyTrustEx (WINTRUST.@)
 */
HRESULT WINAPI WinVerifyTrustEx( HWND hwnd, GUID *ActionID,
 WINTRUST_DATA* ActionData )
{
    FIXME("%p %s %p\n", hwnd, debugstr_guid(ActionID), ActionData);
    return S_OK;
}

/***********************************************************************
 *		WTHelperGetProvSignerFromChain (WINTRUST.@)
 */
CRYPT_PROVIDER_SGNR * WINAPI WTHelperGetProvSignerFromChain(
 CRYPT_PROVIDER_DATA *pProvData, DWORD idxSigner, BOOL fCounterSigner,
 DWORD idxCounterSigner)
{
    FIXME("%p %d %d %d\n", pProvData, idxSigner, fCounterSigner,
     idxCounterSigner);
    return NULL;
}

/***********************************************************************
 *		WTHelperProvDataFromStateData (WINTRUST.@)
 */
CRYPT_PROVIDER_DATA * WINAPI WTHelperProvDataFromStateData(HANDLE hStateData)
{
    FIXME("%p\n", hStateData);
    return NULL;
}

/***********************************************************************
 *              WintrustLoadFunctionPointers (WINTRUST.@)
 */
BOOL WINAPI WintrustLoadFunctionPointers( GUID* pgActionID,
                                          CRYPT_PROVIDER_FUNCTIONS* pPfns )
{
    FIXME("%s %p\n", debugstr_guid(pgActionID), pPfns);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/***********************************************************************
 *		WintrustGetRegPolicyFlags (WINTRUST.@)
 */
void WINAPI WintrustGetRegPolicyFlags( DWORD* pdwPolicyFlags )
{
    FIXME("%p\n", pdwPolicyFlags);
    *pdwPolicyFlags = 0;
}

/***********************************************************************
 *		WintrustSetRegPolicyFlags (WINTRUST.@)
 */
BOOL WINAPI WintrustSetRegPolicyFlags( DWORD dwPolicyFlags)
{
    FIXME("stub: %x\n", dwPolicyFlags);
    return TRUE;
}
