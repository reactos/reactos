#include "mslocusr.h"
#include "msluglob.h"
#include <md5.h>

HRESULT VerifySupervisorPassword(LPCSTR pszPassword)
{
#ifdef MSLOCUSR_USE_SUPERVISOR_PASSWORD
	if (!::fSupervisorKeyInit) {
		HKEY hkeyRating;
		LONG err;

		err = RegOpenKey(HKEY_LOCAL_MACHINE, ::szRATINGS, &hkeyRating);
		if (hkeyRating !=  NULL) {
			DWORD cbData = sizeof(::abSupervisorKey);
			DWORD dwType;
			err = ::RegQueryValueEx(hkeyRating, ::szRatingsSupervisorKeyName, NULL,
									&dwType, (LPBYTE)::abSupervisorKey, &cbData);
            if (err != ERROR_SUCCESS)
    			err = ::RegQueryValueEx(hkeyRating, ::szUsersSupervisorKeyName, NULL,
	    								&dwType, (LPBYTE)::abSupervisorKey, &cbData);
			::RegCloseKey(hkeyRating);
			if (err == ERROR_SUCCESS) {
				if (dwType != REG_BINARY || cbData != sizeof(::abSupervisorKey)) {
					return E_UNEXPECTED;
				}
				::fSupervisorKeyInit = TRUE;
			}
		}
		else
			err = ERROR_FILE_NOT_FOUND;

		if (err != ERROR_SUCCESS) {
			return HRESULT_FROM_WIN32(err);
		}
	}

	if (pszPassword == NULL)
		return ResultFromScode(S_FALSE);

	MD5_CTX ctx;

	MD5Init(&ctx);
	MD5Update(&ctx, (const BYTE *)pszPassword, ::strlenf(pszPassword)+1);
	MD5Final(&ctx);

	return ResultFromScode(::memcmpf(::abSupervisorKey, ctx.digest, sizeof(::abSupervisorKey)) ? S_FALSE : S_OK);

#else

    return S_OK;        /* everybody's a supervisor */

#endif
}


HRESULT ChangeSupervisorPassword(LPCSTR pszOldPassword, LPCSTR pszNewPassword)
{
#ifdef MSLOCUSR_USE_SUPERVISOR_PASSWORD
	HRESULT hres;

	hres = ::VerifySupervisorPassword(pszOldPassword);
	if (hres == S_FALSE) {
		return E_ACCESSDENIED;
	}

	MD5_CTX ctx;

	MD5Init(&ctx);
	MD5Update(&ctx, (const BYTE *)pszNewPassword, ::strlenf(pszNewPassword)+1);
	MD5Final(&ctx);

	::memcpyf(::abSupervisorKey, ctx.digest, sizeof(::abSupervisorKey));
	::fSupervisorKeyInit = TRUE;

	HKEY hkeyRating;

	LONG err = RegOpenKey(HKEY_LOCAL_MACHINE, ::szRATINGS, &hkeyRating);
	if (err == ERROR_SUCCESS) {
        char abTemp[sizeof(::abSupervisorKey)];
        LPCSTR pszValueToSet;
        DWORD dwType;
        DWORD cbData = sizeof(abTemp);
        if (::RegQueryValueEx(hkeyRating, ::szRatingsSupervisorKeyName, NULL,
							  &dwType, (LPBYTE)abTemp, &cbData) == ERROR_SUCCESS)
            pszValueToSet = ::szRatingsSupervisorKeyName;
        else
            pszValueToSet = ::szUsersSupervisorKeyName;

		::RegSetValueEx(hkeyRating, pszValueToSet, NULL,
						REG_BINARY, (const BYTE *)::abSupervisorKey, sizeof(::abSupervisorKey));
		::RegCloseKey(hkeyRating);
	}
#endif

	return NOERROR;
}
