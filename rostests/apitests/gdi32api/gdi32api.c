#include "gdi32api.h"

HINSTANCE g_hInstance;
PGDI_TABLE_ENTRY GdiHandleTable;

BOOL
IsFunctionPresent(LPWSTR lpszFunction)
{
	return TRUE;
}

int APIENTRY
WinMain(HINSTANCE hInstance,
        HINSTANCE hPrevInstance,
        LPSTR     lpCmdLine,
        int       nCmdShow)
{
	GDIQUERYPROC GdiQueryTable;

	g_hInstance = hInstance;

	GdiQueryTable = (GDIQUERYPROC)GetProcAddress(GetModuleHandleW(L"GDI32.DLL"), "GdiQueryTable");
	if(!GdiQueryTable)
	{
		return -1;
	}
	GdiHandleTable = GdiQueryTable();
	if(!GdiHandleTable)
	{
		return -1;
	}

	return TestMain(L"gdi32api", L"gdi32.dll");
}
