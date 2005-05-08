/* $Id$
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
/*
	Thanks to Filip Navara patch for fixing the Xp crash problem.
*/

#include <windows.h>
/* NOTE: W32API ddk/ntapi.h header has wrong definition of SYSTEM_PROCESSES. */
#include <ntos/types.h>


//                     x00000000 00000000 000:00:00  000:00:00 ()
static char* title  = "P     PID     PPID     KTime      UTime   NAME\n";
static char* title1 = "t              TID     KTime      UTime   State      WaitResson\n";
static char* title2 = "w     PID     Hwnd  WndStile        TID   WndName\n";


struct status {
    DWORD state;
    char  desc[10];
}   thread_stat[8 + 1] = { 
    {0,	"Init      "},
    {1,	"Ready     "},
    {2,	"Running   "},
    {3,	"Standby   "},
    {4,	"Terminated"},
    {5,	"Wait      "},
    {6,	"Transition"},
    {7, "Unknown   "},
    {-1,"    ?     "}
};

struct waitres {
    DWORD state;
    char  desc[11];
}   waitreason[28 + 1] = { 
    {0,	"Executive  "},
    {1,	"FreePage   "},
    {2,	"PageIn     "},
    {3,	"PoolAlloc  "},
    {4,	"DelayExec  "},
    {5,	"Suspended  "},
    {6,	"UserReq    "},
    {7, "WrExecutive"},
    {8,	"WrFreePage "},
    {9,	"WrPageIn   "},
    {10,"WrPoolAlloc"},
    {11,"WrDelayExec"},
    {12,"WrSuspended"},
    {13,"WrUserReq  "},
    {14,"WrEventPair"},
    {15,"WrQueue    "},
    {16,"WrLpcRec   "},
    {17,"WrLpcReply "},
    {18,"WrVirtualMm"},
    {19,"WrPageOut  "},
    {20,"WrRendez   "},
    {21,"Spare1     "},
    {22,"Spare2     "},
    {23,"Spare3     "},
    {24,"Spare4     "},
    {25,"Spare5     "},
    {26,"Spare6     "},
    {27,"WrKernel   "},
    {-1,"     ?     "}
};

BOOL CALLBACK
EnumThreadProc(HWND hwnd, LPARAM lp)
{
	DWORD r, pid, tid;
	LONG style;
        HANDLE stdout = GetStdHandle(STD_OUTPUT_HANDLE);
	char buf[256];
	
	GetWindowText(hwnd, (LPTSTR)lp, 30);
	
	if(hwnd != 0)
	{
	style = GetWindowLong(hwnd, GWL_STYLE);

	tid = GetWindowThreadProcessId(hwnd, &pid);

	wsprintf (buf,"w%8d %8x  %08x   %8d   %s\n",pid, hwnd , style, tid, lp );
       	WriteFile(stdout, buf, lstrlen(buf), &r, NULL);
       	}
	return (TRUE);
}

int main()
{
    DWORD r;
    ANSI_STRING astring;
    HANDLE stdout = GetStdHandle(STD_OUTPUT_HANDLE);
    PSYSTEM_PROCESS_INFORMATION SystemProcesses = NULL;
    PSYSTEM_PROCESS_INFORMATION CurrentProcess;
    ULONG BufferSize, ReturnSize;
    NTSTATUS Status;
    char buf[256];
    char buf1[256];
    
    WriteFile(stdout, title, lstrlen(title), &r, NULL);
    WriteFile(stdout, title1, lstrlen(title1), &r, NULL);
    WriteFile(stdout, title2, lstrlen(title2), &r, NULL);

    /* Get process information. */
    BufferSize = 0;
    do
    {
        BufferSize += 0x10000;
        SystemProcesses = HeapAlloc(GetProcessHeap(), 0, BufferSize);
        Status = NtQuerySystemInformation(SystemProcessInformation,
                                          SystemProcesses, BufferSize,
                                          &ReturnSize);
        if (Status == STATUS_INFO_LENGTH_MISMATCH)
            HeapFree(GetProcessHeap(), 0, SystemProcesses);
    } while (Status == STATUS_INFO_LENGTH_MISMATCH);

    /* If querying system information failed, bail out. */
    if (!NT_SUCCESS(Status))
        return 1;

    /* For every process print the information. */
    CurrentProcess = SystemProcesses;
    while (CurrentProcess->NextEntryOffset != 0)
    {
        int hour, hour1, thour, thour1;
        unsigned char minute, minute1, tmin, tmin1;
        unsigned char  seconds, seconds1, tsec, tsec1;

	int ti;
	LARGE_INTEGER ptime;

	ptime.QuadPart = CurrentProcess->KernelTime.QuadPart;
	hour    = (ptime.QuadPart / (10000000LL * 3600LL));
	minute  = (ptime.QuadPart / (10000000LL * 60LL)) % 60LL;
	seconds = (ptime.QuadPart / 10000000LL) % 60LL;
	
	ptime.QuadPart = CurrentProcess->UserTime.QuadPart;
	hour1    = (ptime.QuadPart / (10000000LL * 3600LL));
	minute1  = (ptime.QuadPart / (10000000LL * 60LL)) % 60LL;
	seconds1 = (ptime.QuadPart / 10000000LL) % 60LL;

	RtlUnicodeStringToAnsiString(&astring, &CurrentProcess->ImageName, TRUE);

        wsprintf(buf,"P%8d %8d %3d:%02d:%02d  %3d:%02d:%02d   ProcName: %s\n",
                 CurrentProcess->UniqueProcessId, CurrentProcess->InheritedFromUniqueProcessId,
                 hour, minute, seconds, hour1, minute1, seconds1,
                 astring.Buffer);
        WriteFile(stdout, buf, lstrlen(buf), &r, NULL);
        
        RtlFreeAnsiString(&astring);

	for (ti = 0; ti < CurrentProcess->NumberOfThreads; ti++)
	   {
		struct status *statt;
		struct waitres *waitt;
		char szWindowName[30] = {" "};

		ptime = CurrentProcess->TH[ti].KernelTime;
		thour = (ptime.QuadPart / (10000000LL * 3600LL));
		tmin  = (ptime.QuadPart / (10000000LL * 60LL)) % 60LL;
		tsec  = (ptime.QuadPart / 10000000LL) % 60LL;

		ptime  = CurrentProcess->TH[ti].UserTime;
		thour1 = (ptime.QuadPart / (10000000LL * 3600LL));
		tmin1  = (ptime.QuadPart / (10000000LL * 60LL)) % 60LL;
		tsec1  = (ptime.QuadPart / 10000000LL) % 60LL;

		statt = thread_stat;
                while (statt->state != CurrentProcess->TH[ti].ThreadState  && statt->state >= 0)
                	statt++;

		waitt = waitreason;
                while (waitt->state != CurrentProcess->TH[ti].WaitReason  && waitt->state >= 0)
                        waitt++;

		wsprintf (buf1, 
		          "t%         %8d %3d:%02d:%02d  %3d:%02d:%02d   %s %s\n",
		          CurrentProcess->TH[ti].ClientId.UniqueThread,
		          thour, tmin, tsec, thour1, tmin1, tsec1,
		          statt->desc , waitt->desc);
        	WriteFile(stdout, buf1, lstrlen(buf1), &r, NULL);

		EnumThreadWindows((DWORD)CurrentProcess->TH[ti].ClientId.UniqueThread,
		                  (ENUMWINDOWSPROC) EnumThreadProc,
		                  (LPARAM)(LPTSTR) szWindowName );
	   }
	   CurrentProcess = (PSYSTEM_PROCESS_INFORMATION)((ULONG_PTR)CurrentProcess +
	                     CurrentProcess->NextEntryOffset);
	} 
  	return (0);
}
