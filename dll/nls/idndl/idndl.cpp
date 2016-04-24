#define WIN32_LEAN_AND_MEAN
#define STRICT

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
