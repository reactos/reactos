/*
 * gditest
 */

#include <windows.h>


int main (void)
{
	HDC Desktop;

	GdiDllInitialize (NULL, DLL_PROCESS_ATTACH, NULL);
	Desktop = CreateDCA("DISPLAY", NULL, NULL, NULL);

	return 0;
}
