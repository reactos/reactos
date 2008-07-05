/*
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

#define NTOS_MODE_USER
#define WIN32_NO_STATUS
#include <windows.h>
#include <ndk/ntndk.h>

typedef struct _SYSTEM_THREADS
 {
    LARGE_INTEGER KernelTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER CreateTime;
    ULONG WaitTime;
    PVOID StartAddress;
    CLIENT_ID ClientId;
    KPRIORITY Priority;
    LONG BasePriority;
    ULONG ContextSwitches;
    ULONG ThreadState;
    ULONG WaitReason;
 } SYSTEM_THREADS, *PSYSTEM_THREADS;

 typedef struct _SYSTEM_PROCESSES
 {
    ULONG NextEntryOffset;
    ULONG NumberOfThreads;
    LARGE_INTEGER SpareLi1;
    LARGE_INTEGER SpareLi2;
    LARGE_INTEGER SpareLi3;
    LARGE_INTEGER CreateTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER KernelTime;
    UNICODE_STRING ImageName;
    KPRIORITY BasePriority;
    HANDLE UniqueProcessId;
    HANDLE InheritedFromUniqueProcessId;
    ULONG HandleCount;
    ULONG SessionId;
    ULONG PageDirectoryFrame;

    /*
     * This part corresponds to VM_COUNTERS_EX.
     * NOTE: *NOT* THE SAME AS VM_COUNTERS!
     */
    ULONG PeakVirtualSize;
    ULONG VirtualSize;
    ULONG PageFaultCount;
    ULONG PeakWorkingSetSize;
    ULONG WorkingSetSize;
    ULONG QuotaPeakPagedPoolUsage;
    ULONG QuotaPagedPoolUsage;
    ULONG QuotaPeakNonPagedPoolUsage;
    ULONG QuotaNonPagedPoolUsage;
    ULONG PagefileUsage;
    ULONG PeakPagefileUsage;
    ULONG PrivateUsage;

    /* This part corresponds to IO_COUNTERS */
    LARGE_INTEGER ReadOperationCount;
    LARGE_INTEGER WriteOperationCount;
    LARGE_INTEGER OtherOperationCount;
    LARGE_INTEGER ReadTransferCount;
    LARGE_INTEGER WriteTransferCount;
    LARGE_INTEGER OtherTransferCount;

         SYSTEM_THREADS       Threads [1];
 } SYSTEM_PROCESSES, *PSYSTEM_PROCESSES;


//                     x00000000 00000000 000:00:00  000:00:00 ()
static char title[]  = "P     PID     PPID     KTime      UTime   NAME\n";
static char title1[] = "t              TID     KTime      UTime   State      WaitResson\n";
static char title2[] = "w     PID     Hwnd  WndStile        TID   WndName\n";


struct status {
    DWORD state;
    const char  desc[10];
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
    char  desc[17];
}   waitreason[35 + 1] = {
   {0, "Executive        "},
   {1, "FreePage         "},
   {2, "PageIn           "},
   {3, "PoolAllocation   "},
   {4, "DelayExecution   "},
   {5, "Suspended        "},
   {6, "UserRequest      "},
   {7, "WrExecutive      "},
   {8, "WrFreePage       "},
   {9, "WrPageIn         "},
   {10,"WrPoolAllocation "},
   {11,"WrDelayExecution "},
   {12,"WrSuspended      "},
   {13,"WrUserRequest    "},
   {14,"WrEventPair      "},
   {15,"WrQueue          "},
   {16,"WrLpcReceive     "},
   {17,"WrLpcReply       "},
   {18,"WrVirtualMemory  "},
   {19,"WrPageOut        "},
   {20,"WrRendezvous     "},
   {21,"Spare2           "},
   {22,"WrGuardedMutex   "},
   {23,"Spare4           "},
   {24,"Spare5           "},
   {25,"Spare6           "},
   {26,"WrKernel         "},
   {27,"WrResource       "},
   {28,"WrPushLock       "},
   {29,"WrMutex          "},
   {30,"WrQuantumEnd     "},
   {31,"WrDispatchInt    "},
   {32,"WrPreempted      "},
   {33,"WrYieldExecution "},
   {34,"MaximumWaitReason"},
   {-1,"       ?         "}
};

static BOOL CALLBACK
EnumThreadProc(HWND hwnd, LPARAM lp)
{
	DWORD r, pid, tid;
	LONG style;
	char buf[256];
    HANDLE Stdout = GetStdHandle(STD_OUTPUT_HANDLE);

	GetWindowText(hwnd, (LPTSTR)lp, 30);

	if(hwnd != 0)
	{
	style = GetWindowLong(hwnd, GWL_STYLE);

	tid = GetWindowThreadProcessId(hwnd, &pid);

	wsprintf (buf,"w%8d %8x  %08x   %8d   %s\n",pid, hwnd , style, tid, lp );
       	WriteFile(Stdout, buf, lstrlen(buf), &r, NULL);
       	}
	return (TRUE);
}

int main()
{
    DWORD r;
    ANSI_STRING astring;
    HANDLE Stdout = GetStdHandle(STD_OUTPUT_HANDLE);
    PSYSTEM_PROCESSES SystemProcesses = NULL;
    PSYSTEM_PROCESSES CurrentProcess;
    ULONG BufferSize, ReturnSize;
    NTSTATUS Status;
    char buf[256];
    char buf1[256];

    WriteFile(Stdout, title, lstrlen(title), &r, NULL);
    WriteFile(Stdout, title1, lstrlen(title1), &r, NULL);
    WriteFile(Stdout, title2, lstrlen(title2), &r, NULL);

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

	unsigned int ti;
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

		ptime = CurrentProcess->Threads[ti].KernelTime;
		thour = (ptime.QuadPart / (10000000LL * 3600LL));
		tmin  = (ptime.QuadPart / (10000000LL * 60LL)) % 60LL;
		tsec  = (ptime.QuadPart / 10000000LL) % 60LL;

		ptime = CurrentProcess->Threads[ti].UserTime;
		thour1 = (ptime.QuadPart / (10000000LL * 3600LL));
		tmin1  = (ptime.QuadPart / (10000000LL * 60LL)) % 60LL;
		tsec1  = (ptime.QuadPart / 10000000LL) % 60LL;

		statt = thread_stat;
                while (statt->state != CurrentProcess->Threads[ti].ThreadState  && statt->state >= 0)
                	statt++;

		waitt = waitreason;
                while (waitt->state != CurrentProcess->Threads[ti].WaitReason  && waitt->state >= 0)
                        waitt++;

		wsprintf (buf1,
		          "t%         %8d %3d:%02d:%02d  %3d:%02d:%02d   %s %s\n",
		          CurrentProcess->Threads[ti].ClientId.UniqueThread,
		          thour, tmin, tsec, thour1, tmin1, tsec1,
		          statt->desc , waitt->desc);
        	WriteFile(stdout, buf1, lstrlen(buf1), &r, NULL);

		EnumThreadWindows(PtrToUlong(CurrentProcess->Threads[ti].ClientId.UniqueThread),
		                  (WNDENUMPROC) EnumThreadProc,
		                  (LPARAM)(LPTSTR) szWindowName );
	   }

	   CurrentProcess = (PSYSTEM_PROCESSES)((ULONG_PTR)CurrentProcess +
	                     (ULONG_PTR)CurrentProcess->NextEntryOffset);
	}
  	return (0);
}
