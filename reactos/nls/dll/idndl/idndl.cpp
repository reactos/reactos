#define WIN32_LEAN_AND_MEAN
#define STRICT

#define WINVER 0x0600

#include <windows.h>

extern "C"
{
#include <idndl.h>
}

int
WINAPI
DownlevelGetLocaleScripts
(
	LPCWSTR lpLocaleName,
	LPWSTR lpScripts,
	int cchScripts
)
{
	return GetLocaleInfoEx(lpLocaleName, LOCALE_SSCRIPTS, lpScripts, cchScripts);
}

// EOF
