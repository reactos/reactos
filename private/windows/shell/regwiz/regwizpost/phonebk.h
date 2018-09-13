


#ifndef _PHONE_BOOK_
#define _PHONE_BOOK_
#include "phbk.h"
#ifdef __cplusplus
extern "C" {
#endif

DllExportH PhoneBookLoad(LPCSTR pszISPCode, DWORD_PTR far *pdwPhoneID);
DllExportH PhoneBookSuggestNumbers(DWORD_PTR dwPhoneID, PSUGGESTINFO lpSuggestInfo);
DllExportH PhoneBookDisplaySignUpNumbers (DWORD_PTR dwPhoneID,
														LPSTR far *ppszPhoneNumbers,
														LPSTR far *ppszDunFiles,
														WORD far *pwPhoneNumbers,
														DWORD far *pdwCountry,
														WORD far *pwRegion,
														BYTE fType,
														BYTE bMask,
														HWND hwndParent,
														DWORD dwFlags);


#ifdef __cplusplus
			}
#endif

#endif	// _PHONE_BOOK_

