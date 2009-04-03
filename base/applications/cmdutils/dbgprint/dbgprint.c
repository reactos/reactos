/* $Id: dbgprint.c 24720 2006-11-11 16:07:35Z janderwald $
 *
 * PROJECT:         ReactOS DbgPrint Utility
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            tools/dbgprint/dbgprint.c
 * PURPOSE:         outputs a text via DbgPrint API
 * PROGRAMMERS:     Johannes Anderwald (johannes.anderwald@student.tugraz.at)
 *                  Christoph von Wittich (Christoph_vW@ReactOS.org)
 */

#include <windows.h>
#include <tchar.h>
#include <debug.h>
#include <stdio.h>

int _tmain(int argc, TCHAR ** argv)
{
	TCHAR * buf;
	int bufsize;
	int i;
	int offset;

	bufsize = 0;
	for(i = 1; i < argc; i++)
	{
		bufsize += _tcslen(argv[i]) + 1;
	}

	if (!bufsize)
	{
		return -1;
	}

	if (_tcsstr(argv[1], "--winetest") && (argc == 3))
	{
		char   psBuffer[128];
		char   psBuffer2[128];
		char   cmd[255];
		char   test[300];
		FILE   *pPipe;
		FILE   *pPipe2;

		/* get available tests */
		pPipe = _tpopen(argv[2], "r");
		if (pPipe != NULL)
		{
			while(fgets(psBuffer, 128, pPipe))
			{
				if (psBuffer[0] == ' ')
				{
					strcpy(cmd, argv[2]);
					strcat(cmd, " ");
					strcat(cmd, psBuffer+4);
					/* run the current test */
					strcpy(test, "\n\nRunning ");
					strcat(test, cmd);
					OutputDebugStringA(test);
					pPipe2 = _popen(cmd, "r");
					if (pPipe2 != NULL)
					{
						while(fgets(psBuffer2, 128, pPipe2))
						{
							char *nlptr2 = strchr(psBuffer2, '\n');
							if (nlptr2)
								*nlptr2 = '\0';
							puts(psBuffer2);
							OutputDebugStringA(psBuffer2);
						}
						_pclose(pPipe2);
					}
				}
			}
			_pclose(pPipe);
		}
	}
	else if (_tcsstr(argv[1], "--process") && (argc == 3))
	{
		char   psBuffer[128];
		FILE   *pPipe;

		pPipe = _tpopen(argv[2], "r");
		if (pPipe != NULL)
		{
			while(fgets(psBuffer, 128, pPipe))
			{
				puts(psBuffer);
				OutputDebugStringA(psBuffer);
			}
			_pclose(pPipe);
		}
	}
	else
	{
		buf = HeapAlloc(GetProcessHeap(), 0, (bufsize+1) * sizeof(TCHAR));
		if (!buf)
		{
			return -1;
		}

		offset = 0;
		for(i = 1; i < argc; i++)
		{
			int length = _tcslen(argv[i]);
			_tcsncpy(&buf[offset], argv[i], length);
			offset += length;
			if (i + 1 < argc)
			{
				buf[offset] = _T(' ');
			}
			else
			{
				buf[offset] = _T('\n');
				buf[offset+1] = _T('\0');
			}
			offset++;
		}
		_putts(buf);
		OutputDebugString(buf);
		HeapFree(GetProcessHeap(), 0, buf);
	}
	return 0;
}
