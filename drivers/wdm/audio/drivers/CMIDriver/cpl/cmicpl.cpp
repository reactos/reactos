/*
Copyright (c) 2006-2007 dogbert <dogber1@gmail.com>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the author may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <windows.h>
#include <shellapi.h>
#include <tchar.h>
#include <cpl.h>
#include "resource.h"

BOOL APIENTRY DllMain (HANDLE hModule, ULONG gna, LPVOID lpReserved)
{
	return TRUE;
}

BOOL execControlPanel()
{
	TCHAR szSysDir[1024];
	if (!GetSystemDirectory(szSysDir, sizeof(szSysDir) / sizeof(TCHAR))) {
		return FALSE;
	}
	_tcscat(szSysDir, _T("\\cmicontrol.exe")); //unsafe
	ShellExecute(NULL, _T("open"), szSysDir, NULL, NULL, SW_SHOWNORMAL);
	return TRUE;
}

LONG APIENTRY CPlApplet (HWND hWnd, UINT uMsg, LPARAM lParam1, LPARAM lParam2)
{
	switch (uMsg)
	{
		case CPL_INIT:
			return TRUE;

		case CPL_GETCOUNT:
			return 1;

		case CPL_INQUIRE:
		{
			LPCPLINFO pCplInfo = (LPCPLINFO)lParam2;

			if (!pCplInfo) {
				return TRUE;
			}

			if ((UINT)lParam1 == 0) {
				pCplInfo->idIcon = IDI_CPLICON;
				pCplInfo->idName = IDS_CPLNAME;
				pCplInfo->idInfo = IDS_CPLINFO;
			}
			break;
		}

		case CPL_NEWINQUIRE:
			break;

		case CPL_DBLCLK:
		case CPL_STARTWPARMS:
			if ((UINT)lParam1 == 0) {
				if (!execControlPanel()) {
					return TRUE;
				}
			}
			break;
		case CPL_EXIT:
			break;

		default:
			break;
	}

	return 0;
}

