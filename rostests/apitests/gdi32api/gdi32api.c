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

BOOL
IsHandleValid(HGDIOBJ hobj)
{
    USHORT Index = (ULONG_PTR)hobj;
    PGDI_TABLE_ENTRY pentry = &GdiHandleTable[Index];

    if (pentry->KernelData == NULL ||
        pentry->KernelData < (PVOID)0x80000000 ||
        (USHORT)pentry->FullUnique != (USHORT)((ULONG_PTR)hobj >> 16))
    {
        return FALSE;
    }
    
    return TRUE;
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
