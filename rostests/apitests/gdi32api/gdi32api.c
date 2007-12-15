#include "gdi32api.h"

HINSTANCE g_hInstance;
PGDI_TABLE_ENTRY GdiHandleTable;

BOOL
IsFunctionPresent(LPWSTR lpszFunction)
{
	return TRUE;
}

static
PGDI_TABLE_ENTRY
MyGdiQueryTable()
{
	PTEB pTeb = NtCurrentTeb();
	PPEB pPeb = pTeb->ProcessEnvironmentBlock;
	return pPeb->GdiSharedHandleTable;
}

int APIENTRY
WinMain(HINSTANCE hInstance,
        HINSTANCE hPrevInstance,
        LPSTR     lpCmdLine,
        int       nCmdShow)
{
	g_hInstance = hInstance;

	GdiHandleTable = MyGdiQueryTable();
	if(!GdiHandleTable)
	{
		return -1;
	}

	return TestMain(L"gdi32api", L"gdi32.dll");
}
