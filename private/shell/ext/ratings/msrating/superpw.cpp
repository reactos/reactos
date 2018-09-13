#include "msrating.h"
#include "msluglob.h"
#include <md5.h>

extern HKEY CreateRegKeyNT(LPCSTR pszKey);
extern BOOL RunningOnNT(void);

HRESULT VerifySupervisorPassword(LPCSTR pszPassword)
{
	if (!::fSupervisorKeyInit) {
		HKEY hkeyRating;
		LONG err;

		hkeyRating = CreateRegKeyNT(::szRATINGS);
		if (hkeyRating !=  NULL) {
			DWORD cbData = sizeof(::abSupervisorKey);
			DWORD dwType;
			err = ::RegQueryValueEx(hkeyRating, ::szRatingsSupervisorKeyName, NULL,
									&dwType, (LPBYTE)::abSupervisorKey, &cbData);
            if (err != ERROR_SUCCESS) {
                if (pszPassword == NULL) {
                    ::RegCloseKey(hkeyRating);
                    return E_FAIL;
                }                    
    			err = ::RegQueryValueEx(hkeyRating, ::szUsersSupervisorKeyName, NULL,
	    								&dwType, (LPBYTE)::abSupervisorKey, &cbData);
            }
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
}


HRESULT ChangeSupervisorPassword(LPCSTR pszOldPassword, LPCSTR pszNewPassword)
{
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

    hres = NOERROR;

	HKEY hkeyRating;

	hkeyRating = CreateRegKeyNT(::szRATINGS);
	if (hkeyRating != NULL) {
        BYTE abTemp[sizeof(::abSupervisorKey)];
		DWORD cbData = sizeof(::abSupervisorKey);
		DWORD dwType;
		if (::RegQueryValueEx(hkeyRating, ::szRatingsSupervisorKeyName, NULL,
							  &dwType, abTemp, &cbData) != ERROR_SUCCESS)
            hres = S_FALSE; /* tell caller we're creating the new key */

		::RegSetValueEx(hkeyRating, ::szRatingsSupervisorKeyName, NULL,
						REG_BINARY, (const BYTE *)::abSupervisorKey, sizeof(::abSupervisorKey));
		::RegCloseKey(hkeyRating);
	}

	return hres;
}


HRESULT RemoveSupervisorPassword(void)
{
	HKEY hkeyRating;
	LONG err;

	hkeyRating = CreateRegKeyNT(::szRATINGS);
	if (hkeyRating !=  NULL) {
		err = ::RegDeleteValue(hkeyRating, ::szRatingsSupervisorKeyName);
		::RegCloseKey(hkeyRating);
	}

	::fSupervisorKeyInit = FALSE;
	return HRESULT_FROM_WIN32(err);
}


