/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for AddFontResourceEx
 * PROGRAMMERS:     Timo Kreuzer
 */

#include "precomp.h"

void Test_AddFontResourceExW()
{
	WCHAR szFileName[MAX_PATH];
	int result;

	/* Test NULL filename */
	SetLastError(ERROR_SUCCESS);

	/* Windows crashes, need SEH here */
	_SEH2_TRY
	{
    	result = AddFontResourceExW(NULL, 0, 0);
	}
	_SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
	{
	    result = -1;
	    SetLastError(_SEH2_GetExceptionCode());
	}
	_SEH2_END
	ok(result == -1, "AddFontResourceExW should throw an exception!, result == %d\n", result);
	ok(GetLastError() == 0xc0000005, "GetLastError()==%lx\n", GetLastError());

	/* Test "" filename */
	SetLastError(ERROR_SUCCESS);
	result = AddFontResourceExW(L"", 0, 0);
	ok(result == 0, "AddFontResourceExW(L"", 0, 0) succeeded, result==%d\n", result);
	ok(GetLastError() == ERROR_INVALID_PARAMETER, "GetLastError()==%ld\n", GetLastError());

	GetEnvironmentVariableW(L"systemroot", szFileName, MAX_PATH);
	wcscat(szFileName, L"\\Fonts\\cour.ttf");

	/* Test flags = 0 */
	SetLastError(ERROR_SUCCESS);
	result = AddFontResourceExW(szFileName, 0, 0);
	ok(result == 1, "AddFontResourceExW(L"", 0, 0) failed, result==%d\n", result);
	ok(GetLastError() == ERROR_SUCCESS, "GetLastError()==%ld\n", GetLastError());

	SetLastError(ERROR_SUCCESS);
	result = AddFontResourceExW(szFileName, 256, 0);
	ok(result == 0, "AddFontResourceExW(L"", 0, 0) failed, result==%d\n", result);
	ok(GetLastError() == ERROR_INVALID_PARAMETER, "GetLastError()==%ld\n", GetLastError());

	/* Test invalid pointer as last parameter */
	result = AddFontResourceExW(szFileName, 0, (void*)-1);
	ok(result != 0, "AddFontResourceExW(L"", 0, 0) failed, result==%d\n", result);
	ok(GetLastError() == ERROR_INVALID_PARAMETER, "GetLastError()==%ld\n", GetLastError());

}

START_TEST(AddFontResourceEx)
{
    Test_AddFontResourceExW();
}

