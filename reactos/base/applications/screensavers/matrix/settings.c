//
//	settings.c
//
//	Load/Save settings from registry
//
#include <windows.h>
#include "globals.h"
#include "message.h"
#include "matrix.h"

char	g_szMessages[MAX_MESSAGES][MAXMSG_LENGTH];
int		g_nNumMessages		 = 0;
int		g_nMessageSpeed		 = 5;
char	g_szFontName[512]	 = "Arial";

int		g_nMatrixSpeed		 = 150;
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
	char *hugechar = malloc(4096);
	char *hptr = hugechar;

	if(hugechar == 0)
		return;

	hugechar[0] = '\0';

	RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\Catch22\\Matrix Screen Saver", 0,
		"", 0, KEY_READ, NULL, &hkey, NULL);

	len = sizeof value;
	if(ERROR_SUCCESS == RegQueryValueEx(hkey, "MessageSpeed", 0, 0, (BYTE *)&value, &len))
	{
		if(value >= MSGSPEED_MIN && value <= MSGSPEED_MAX)
			g_nMessageSpeed = value;
	}

	if(ERROR_SUCCESS == RegQueryValueEx(hkey, "MatrixSpeed",  0, 0, (BYTE *)&value, &len))
	{
		if(value >= SPEED_MIN && value <= SPEED_MAX)
			g_nMatrixSpeed  = value;
	}
	
	if(ERROR_SUCCESS == RegQueryValueEx(hkey, "Density",      0, 0, (BYTE *)&value, &len))
	{
		if(value >= DENSITY_MIN && value <= DENSITY_MAX)
			g_nDensity      = value;
	}

	if(ERROR_SUCCESS == RegQueryValueEx(hkey, "FontSize",      0, 0, (BYTE *)&value, &len))
	{
		if(value >= FONT_MIN && value <= FONT_MAX)
			g_nFontSize	 = value;
	}

	if(ERROR_SUCCESS == RegQueryValueEx(hkey, "FontBold",      0, 0, (BYTE *)&value, &len))
		g_fFontBold = (value == 0 ? FALSE : TRUE);

	if(ERROR_SUCCESS == RegQueryValueEx(hkey, "Randomize",      0, 0, (BYTE *)&value, &len))
		g_fRandomizeMessages = (value == 0 ? FALSE : TRUE);

	len = 4096;
	if(ERROR_SUCCESS == RegQueryValueEx(hkey, "FontName",  0, 0, (BYTE *)hugechar, &len))
	{
		if(len > 0)
			lstrcpy(g_szFontName, hugechar);
		else
			lstrcpy(g_szFontName, "Arial");

	}

	len = 4096;

	if(ERROR_SUCCESS == RegQueryValueEx(hkey, "Messages",      0, 0 , (BYTE *)hugechar, &len))
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

	RegCloseKey(hkey);
	free(hugechar);
}

void SaveSettings()
{
	HKEY hkey;
	char *hugechar = malloc(4096);
	char *msgptr = hugechar;
	int totallen = 0;
	int i,len;
	LONG value;

	if(hugechar == 0)
		return;

	hugechar[0] = '\0';
	msgptr = hugechar;

	RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\Catch22\\Matrix Screen Saver", 0,
		"", 0, KEY_WRITE, NULL, &hkey, NULL);

	value = g_nMessageSpeed;
	RegSetValueEx(hkey, "MessageSpeed", 0, REG_DWORD, (BYTE *)&value, sizeof value);

	value = g_nMatrixSpeed;
	RegSetValueEx(hkey, "MatrixSpeed", 0, REG_DWORD, (BYTE *)&value, sizeof value);

	value = g_nDensity;
	RegSetValueEx(hkey, "Density", 0, REG_DWORD, (BYTE *)&value, sizeof value);

	value = g_nFontSize;
	RegSetValueEx(hkey, "FontSize", 0, REG_DWORD, (BYTE *)&value, sizeof value);

	value = g_fRandomizeMessages;
	RegSetValueEx(hkey, "Randomize", 0, REG_DWORD, (BYTE *)&value, sizeof value);

	value = g_fFontBold;
	RegSetValueEx(hkey, "FontBold", 0, REG_DWORD, (BYTE *)&value, sizeof value);

	RegSetValueEx(hkey, "FontName", 0, REG_SZ, (BYTE *)g_szFontName, lstrlen(g_szFontName));

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

	*msgptr = '\0';
	totallen++;

	RegSetValueEx(hkey, "Messages", 0, REG_MULTI_SZ, (BYTE *)hugechar, totallen);
	RegCloseKey(hkey);

	free(hugechar);
}