/* $Id: ps.c,v 1.1 2003/01/04 18:36:28 robd Exp $
 *
 *  ReactOS ps - process list console viewer
 *
 *  ps.c
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <windows.h>
#include <tlhelp32.h>

static char* title = "     PID   PARENT  TIME NAME\n";
char buf[256];

int main()
{
    DWORD r;
    HANDLE pl;
    PROCESSENTRY32 pe;
    HANDLE stdout = GetStdHandle(STD_OUTPUT_HANDLE);

    WriteFile(stdout, title, lstrlen(title), &r, NULL);
    pl = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    pe.dwSize = sizeof(PROCESSENTRY32);
    pe.th32ParentProcessID = 0;

    if (Process32First(pl, &pe)) do {
        int hour;
        int minute;
        WORD fatdate;
        WORD fattime;
        HANDLE p = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pe.th32ProcessID);
        FILETIME cr;
        FILETIME ex;
        FILETIME kt;
        FILETIME ut;
        GetProcessTimes(p, &cr, &ex, &kt, &ut);
        FileTimeToDosDateTime(&cr, &fatdate, &fattime);
        hour = (fattime & 0xf800) >> 11;
        minute = (fattime & 0x07e0) >> 5;
        wsprintf(buf,"%08X %08X %2d:%02d %s\n", pe.th32ProcessID, pe.th32ParentProcessID, hour, minute, pe.szExeFile);
        WriteFile(stdout, buf, lstrlen(buf), &r, NULL);
        CloseHandle(p);
        pe.th32ParentProcessID = 0;
  } while (Process32Next(pl, &pe));

  CloseHandle(pl);
}
/*
WINBOOL
STDCALL
FileTimeToDosDateTime(
		      CONST FILETIME *lpFileTime,
		      LPWORD lpFatDate,
		      LPWORD lpFatTime
		      );
 */
