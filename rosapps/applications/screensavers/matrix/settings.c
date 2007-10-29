//
//	settings.c
//
//	Load/Save settings from registry
//
#include <stdlib.h>
#include <windows.h>
#include <tchar.h>
#include "globals.h"
#include "message.h"
#include "matrix.h"

TCHAR	g_szMessages[MAX_MESSAGES][MAXMSG_LENGTH];
int		g_nNumMessages		 = 0;
int		g_nMessageSpeed		 = 5;
TCHAR	g_szFontName[512]	 = _T("Arial");

int		g_nMatrixSpeed		 = 10;
int		g_nDensity			 = 32;
int		g_nFontSize			 = 12;
BOOL	g_fRandomizeMessages = FALSE;
BOOL	g_fFontBold			 = TRUE;
BOOL	g_fScreenSaving		 = FALSE;

HFONT	g_hFont;

void LoadSettings()
{
	HKEY hkey;
	LONG value;
	ULONG len;
	TCHAR *hugechar = malloc(4096);
	TCHAR *hptr = hugechar;

	if(hugechar == 0)
		return;

	hugechar[0] = _T('\0');

	RegCreateKeyEx(HKEY_CURRENT_USER, _T("Software\\Catch22\\Matrix Screen Saver"), 0,
		_T(""), 0, KEY_READ, NULL, &hkey, NULL);

	len = sizeof value;
	if(ERROR_SUCCESS == RegQueryValueEx(hkey, _T("MessageSpeed"), 0, 0, (BYTE *)&value, &len))
	{
		if(value >= MSGSPEED_MIN && value <= MSGSPEED_MAX)
			g_nMessageSpeed = value;
	}

	if(ERROR_SUCCESS == RegQueryValueEx(hkey, _T("MatrixSpeed"),  0, 0, (BYTE *)&value, &len))
	{
		if(value >= SPEED_MIN && value <= SPEED_MAX)
			g_nMatrixSpeed  = value;
	}

	if(ERROR_SUCCESS == RegQueryValueEx(hkey, _T("Density"),      0, 0, (BYTE *)&value, &len))
	{
		if(value >= DENSITY_MIN && value <= DENSITY_MAX)
			g_nDensity      = value;
	}

	if(ERROR_SUCCESS == RegQueryValueEx(hkey, _T("FontSize"),      0, 0, (BYTE *)&value, &len))
	{
		if(value >= FONT_MIN && value <= FONT_MAX)
			g_nFontSize	 = value;
	}

	if(ERROR_SUCCESS == RegQueryValueEx(hkey, _T("FontBold"),      0, 0, (BYTE *)&value, &len))
		g_fFontBold = (value == 0 ? FALSE : TRUE);

	if(ERROR_SUCCESS == RegQueryValueEx(hkey, _T("Randomize"),      0, 0, (BYTE *)&value, &len))
		g_fRandomizeMessages = (value == 0 ? FALSE : TRUE);

	len = 4096;
	if(ERROR_SUCCESS == RegQueryValueEx(hkey, _T("FontName"),  0, 0, (BYTE *)hugechar, &len))
	{
		if(len > 0)
			lstrcpy(g_szFontName, hugechar);
		else
			lstrcpy(g_szFontName, _T("Arial"));

	}

	len = 4096;

	if(ERROR_SUCCESS == RegQueryValueEx(hkey, _T("Messages"),      0, 0 , (BYTE *)hugechar, &len))
	{
		while(len > 0 && *hptr && isascii(*hptr))
		{
			if(lstrlen(hptr) > 0)
			{
				lstrcpyn(g_szMessages[g_nNumMessages], hptr, MAXMSG_LENGTH);
				++g_nNumMessages;
				hptr += lstrlen(hptr) + 1;
			}
		}
	}
	else
	{
		/* built-in coded message for first run */
		lstrcpyn(g_szMessages[0], _T("ReactOS"), MAXMSG_LENGTH);
		++g_nNumMessages;
	}

	RegCloseKey(hkey);
	free(hugechar);
}

void SaveSettings()
{
	HKEY hkey;
	TCHAR *hugechar = malloc(4096);
	TCHAR *msgptr = hugechar;
	int totallen = 0;
	int i,len;
	LONG value;

	if(hugechar == 0)
		return;

	hugechar[0] = _T('\0');
	msgptr = hugechar;

	RegCreateKeyEx(HKEY_CURRENT_USER, _T("Software\\Catch22\\Matrix Screen Saver"), 0,
		_T(""), 0, KEY_WRITE, NULL, &hkey, NULL);

	value = g_nMessageSpeed;
	RegSetValueEx(hkey, _T("MessageSpeed"), 0, REG_DWORD, (BYTE *)&value, sizeof value);

	value = g_nMatrixSpeed;
	RegSetValueEx(hkey, _T("MatrixSpeed"), 0, REG_DWORD, (BYTE *)&value, sizeof value);

	value = g_nDensity;
	RegSetValueEx(hkey, _T("Density"), 0, REG_DWORD, (BYTE *)&value, sizeof value);

	value = g_nFontSize;
	RegSetValueEx(hkey, _T("FontSize"), 0, REG_DWORD, (BYTE *)&value, sizeof value);

	value = g_fRandomizeMessages;
	RegSetValueEx(hkey, _T("Randomize"), 0, REG_DWORD, (BYTE *)&value, sizeof value);

	value = g_fFontBold;
	RegSetValueEx(hkey, _T("FontBold"), 0, REG_DWORD, (BYTE *)&value, sizeof value);

	RegSetValueEx(hkey, _T("FontName"), 0, REG_SZ, (BYTE *)g_szFontName, lstrlen(g_szFontName));

	for(i = 0; i < g_nNumMessages; i++)
	{
		len = lstrlen(g_szMessages[i]);

		if(len > 0 && totallen+len < 4096)
		{
			lstrcpyn(msgptr, g_szMessages[i], 4096-totallen);
			totallen += len + 1;
			msgptr += len + 1;
		}
	}

	*msgptr = _T('\0');
	totallen++;

	RegSetValueEx(hkey, _T("Messages"), 0, REG_MULTI_SZ, (BYTE *)hugechar, totallen);
	RegCloseKey(hkey);

	free(hugechar);
}
