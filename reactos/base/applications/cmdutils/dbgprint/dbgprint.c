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

	if (_tcsstr(argv[1], "--process") && (argc == 3)) 
	{
		char   psBuffer[128];
		FILE   *pPipe;

		pPipe = _tpopen(argv[2], "r");
		if (pPipe != NULL)
		{
			while(fgets(psBuffer, 128, pPipe))
			{
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
		OutputDebugString(buf);
		HeapFree(GetProcessHeap(), 0, buf);
	}
	return 0;
}
