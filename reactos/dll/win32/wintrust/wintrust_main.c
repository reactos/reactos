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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "guiddef.h"
#include "wintrust.h"
#include "mscat.h"
#include "objbase.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wintrust);

/***********************************************************************
 *		CryptCATAdminAcquireContext (WINTRUST.@)
 */
BOOL WINAPI CryptCATAdminAcquireContext(HCATADMIN* catAdmin,
					const GUID *sysSystem, DWORD dwFlags )
{
    FIXME("%p %s %lx\n", catAdmin, debugstr_guid(sysSystem), dwFlags);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/***********************************************************************
 *             CryptCATAdminCalcHashFromFileHandle (WINTRUST.@)
 */
BOOL WINAPI CryptCATAdminCalcHashFromFileHandle(HANDLE hFile, DWORD* pcbHash,
                                                BYTE* pbHash, DWORD dwFlags )
{
    FIXME("%p %p %p %lx\n", hFile, pcbHash, pbHash, dwFlags);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/***********************************************************************
 *		CryptCATAdminReleaseContext (WINTRUST.@)
 */
BOOL WINAPI CryptCATAdminReleaseContext(HCATADMIN hCatAdmin, DWORD dwFlags )
{
    FIXME("%p %lx\n", hCatAdmin, dwFlags);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/***********************************************************************
 *		WinVerifyTrust (WINTRUST.@)
 */
LONG WINAPI WinVerifyTrust( HWND hwnd, GUID *ActionID,  WINTRUST_DATA* ActionData )
{
    static const GUID WINTRUST_ACTION_GENERIC_VERIFY_V2 = { 0xaac56b, 0xcd44, 0x11d0,
                                                          { 0x8c,0xc2,0x00,0xc0,0x4f,0xc2,0x95,0xee }};

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

    if (IsEqualCLSID(ActionID, &WINTRUST_ACTION_GENERIC_VERIFY_V2))
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
    FIXME("%p %ld %d %ld\n", pProvData, idxSigner, fCounterSigner,
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
 *		WintrustAddActionID (WINTRUST.@)
 */
BOOL WINAPI WintrustAddActionID( GUID* pgActionID, DWORD fdwFlags,
                                 CRYPT_REGISTER_ACTIONID* psProvInfo)
{
    FIXME("%p %lx %p\n", pgActionID, fdwFlags, psProvInfo);
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
    FIXME("stub: %lx\n", dwPolicyFlags);
    return TRUE;
}

/***********************************************************************
  *             DllRegisterServer (WINTRUST.@)
  */
HRESULT WINAPI DllRegisterServer(void)
{
     FIXME("stub\n");
     return S_OK;
}

/***********************************************************************
  *             DllUnregisterServer (WINTRUST.@)
  */
HRESULT WINAPI DllUnregisterServer(void)
{
     FIXME("stub\n");
     return S_OK;
}
