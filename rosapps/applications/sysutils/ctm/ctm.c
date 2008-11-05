/* Console Task Manager

   ctm.c - main program file

   Written by: Aleksey Bragin (aleksey@studiocerebral.com)

   Most of the code dealing with getting system parameters is taken from
   ReactOS Task Manager written by Brian Palmer (brianp@reactos.org)

   Localization features added by Hervé Poussineau (hpoussin@reactos.org)

   History:
    24 October 2004 - added localization features
	09 April 2003 - v0.1, fixed bugs, added features, ported to mingw
   	20 March 2003 - v0.03, works good under ReactOS, and allows process
			killing
	18 March 2003 - Initial version 0.01, doesn't work under RectOS

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */


#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows //headers
#define WIN32_NO_STATUS
#include <windows.h>

#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <process.h>
#include <stdio.h>

#define NTOS_MODE_USER
#include <ndk/ntndk.h>

#include <epsapi/epsapi.h>

#include "ctm.h"
#include "resource.h"

#define TIMES

HANDLE hStdin;
HANDLE hStdout;
HINSTANCE hInst;

DWORD inConMode;
DWORD outConMode;

DWORD columnRightPositions[6];
TCHAR lpSeparator[80];
TCHAR lpHeader[80];
TCHAR lpMemUnit[3];
TCHAR lpIdleProcess[80];
TCHAR lpTitle[80];
TCHAR lpHeader[80];
TCHAR lpMenu[80];
TCHAR lpEmpty[80];

TCHAR KEY_QUIT, KEY_KILL;
TCHAR KEY_YES, KEY_NO;

int			ProcPerScreen = 17; // 17 processess are displayed on one page
int			ScreenLines=25;
ULONG			ProcessCountOld = 0;
ULONG			ProcessCount = 0;

double			dbIdleTime;
double			dbKernelTime;
double			dbSystemTime;
LARGE_INTEGER		liOldIdleTime = {{0,0}};
LARGE_INTEGER		liOldKernelTime = {{0,0}};
LARGE_INTEGER		liOldSystemTime = {{0,0}};

PPERFDATA		pPerfDataOld = NULL;	// Older perf data (saved to establish delta values)
PPERFDATA		pPerfData = NULL;		// Most recent copy of perf data

int selection=0;
int scrolled=0;		// offset from which process start showing
int first = 0;		// first time in DisplayScreen
SYSTEM_BASIC_INFORMATION    SystemBasicInfo;

CONSOLE_SCREEN_BUFFER_INFO screenBufferInfo;
#define NEW_CONSOLE

// Functions that are needed by epsapi
void *PsaiMalloc(SIZE_T size) { return malloc(size); }
void *PsaiRealloc(void *ptr, SIZE_T size) { return realloc(ptr, size); }
void PsaiFree(void *ptr) { free(ptr); }

// Prototypes
unsigned int GetKeyPressed();

void GetInputOutputHandles()
{
#ifdef NEW_CONSOLE
	HANDLE console = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE,
										FILE_SHARE_READ | FILE_SHARE_WRITE,
										0, CONSOLE_TEXTMODE_BUFFER, 0);

	if (SetConsoleActiveScreenBuffer(console) == FALSE)
	{
		hStdin = GetStdHandle(STD_INPUT_HANDLE);
		hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
	}
	else
	{
		hStdin = GetStdHandle(STD_INPUT_HANDLE);//console;
		hStdout = console;
	}
#else
	hStdin = GetStdHandle(STD_INPUT_HANDLE);
	hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
#endif
}

void RestoreConsole()
{
	SetConsoleMode(hStdin, inConMode);
	SetConsoleMode(hStdout, outConMode);

#ifdef NEW_CONSOLE
	SetConsoleActiveScreenBuffer(GetStdHandle(STD_OUTPUT_HANDLE));
#endif
}

void DisplayScreen()
{
	COORD pos;
	COORD size;
	TCHAR lpStr[80];
	DWORD numChars;
	int lines;
	int idx;
	GetConsoleScreenBufferInfo(hStdout,&screenBufferInfo);
	size=screenBufferInfo.dwSize;
	ScreenLines=size.Y;
	ProcPerScreen = ScreenLines-7;
	if (first == 0)
	{
		// Header
		pos.X = 1; pos.Y = 1;
		WriteConsoleOutputCharacter(hStdout, lpTitle, _tcslen(lpTitle), pos, &numChars);

		pos.X = 1; pos.Y = 2;
		WriteConsoleOutputCharacter(hStdout, lpSeparator, _tcslen(lpSeparator), pos, &numChars);

		pos.X = 1; pos.Y = 3;
		WriteConsoleOutputCharacter(hStdout, lpHeader, _tcslen(lpHeader), pos, &numChars);

		pos.X = 1; pos.Y = 4;
		WriteConsoleOutputCharacter(hStdout, lpSeparator, _tcslen(lpSeparator), pos, &numChars);

		// Footer
		pos.X = 1; pos.Y = ScreenLines-2;
		WriteConsoleOutputCharacter(hStdout, lpSeparator, _tcslen(lpSeparator), pos, &numChars);

		// Menu
		pos.X = 1; pos.Y = ScreenLines-1;
		WriteConsoleOutputCharacter(hStdout, lpEmpty, _tcslen(lpEmpty), pos, &numChars);
		WriteConsoleOutputCharacter(hStdout, lpMenu, _tcslen(lpMenu), pos, &numChars);

		first = 1;
	}

	// Processess
	lines = ProcessCount;
	if (lines > ProcPerScreen)
		lines = ProcPerScreen;
	for (idx=0; idx<ProcPerScreen; idx++)
	{
		int len, i;
		TCHAR lpNumber[5];
		TCHAR lpPid[8];
		TCHAR lpCpu[6];
		TCHAR lpMemUsg[12];
		TCHAR lpPageFaults[15];
		WORD wColor;

		for (i = 0; i < 80; i++)
			lpStr[i] = _T(' ');

		// data
		if (idx < lines && scrolled + idx < ProcessCount)
		{

			// number
			_stprintf(lpNumber, _T("%3d"), idx+scrolled);
			_tcsncpy(&lpStr[2], lpNumber, 3);

			// image name
#ifdef _UNICODE
		   len = wcslen(pPerfData[scrolled+idx].ImageName);
#else
		   WideCharToMultiByte(CP_ACP, 0, pPerfData[scrolled+idx].ImageName, -1,
			               imgName, MAX_PATH, NULL, NULL);
		   len = strlen(imgName);
#endif
			if (len > columnRightPositions[1])
			{
				len = columnRightPositions[1];
			}
#ifdef _UNICODE
		   wcsncpy(&lpStr[columnRightPositions[0]+3], pPerfData[scrolled+idx].ImageName, len);
#else
		   strncpy(&lpStr[columnRightPositions[0]+3], imgName, len);
#endif

			// PID
			_stprintf(lpPid, _T("%6ld"), pPerfData[scrolled+idx].ProcessId);
			_tcsncpy(&lpStr[columnRightPositions[2] - 6], lpPid, 6);

#ifdef TIMES
			// CPU
			_stprintf(lpCpu, _T("%3d%%"), pPerfData[scrolled+idx].CPUUsage);
			_tcsncpy(&lpStr[columnRightPositions[3] - 4], lpCpu, 4);
#endif

			// Mem usage
			 _stprintf(lpMemUsg, _T("%6ld %s"), pPerfData[scrolled+idx].WorkingSetSizeBytes / 1024, lpMemUnit);
			 _tcsncpy(&lpStr[columnRightPositions[4] - 9], lpMemUsg, 9);

			// Page Fault
			_stprintf(lpPageFaults, _T("%12ld"), pPerfData[scrolled+idx].PageFaultCount);
			_tcsncpy(&lpStr[columnRightPositions[5] - 12], lpPageFaults, 12);
		}

		// columns
		lpStr[0] = _T(' ');
		lpStr[1] = _T('|');
		for (i = 0; i < 6; i++)
			lpStr[columnRightPositions[i] + 1] = _T('|');
                pos.X = 0; pos.Y = 5+idx;
		WriteConsoleOutputCharacter(hStdout, lpStr, 80, pos, &numChars);

		// Attributes now...
		pos.X = columnRightPositions[0] + 1; pos.Y = 5+idx;
		if (selection == idx)
		{
			wColor = BACKGROUND_GREEN |
				FOREGROUND_RED |
				FOREGROUND_GREEN |
				FOREGROUND_BLUE;
		}
		else
		{
			wColor = BACKGROUND_BLUE |
					FOREGROUND_RED |
					FOREGROUND_GREEN |
					FOREGROUND_BLUE;
		}

		FillConsoleOutputAttribute(
			hStdout,          // screen buffer handle
			wColor,           // color to fill with
			columnRightPositions[1] - 4,	// number of cells to fill
			pos,            // first cell to write to
			&numChars);       // actual number written
	}

	return;
}

// returns TRUE if exiting
int ProcessKeys(int numEvents)
{
	DWORD numChars;
	if ((ProcessCount-scrolled < 17) && (ProcessCount > 17))
		scrolled = ProcessCount-17;

	TCHAR key = GetKeyPressed(numEvents);
	if (key == KEY_QUIT)
		return TRUE;
	else if (key == KEY_KILL)
	{
		// user wants to kill some process, get his acknowledgement
		DWORD pId;
		COORD pos;
		TCHAR lpStr[100];

		pos.X = 1; pos.Y =ScreenLines-1;
		if (LoadString(hInst, IDS_KILL_PROCESS, lpStr, 100))
			WriteConsoleOutputCharacter(hStdout, lpStr, _tcslen(lpStr), pos, &numChars);

		do {
			GetNumberOfConsoleInputEvents(hStdin, &pId);
			key = GetKeyPressed(pId);
		} while (key != KEY_YES && key != KEY_NO);

		if (key == KEY_YES)
		{
			HANDLE hProcess;
			pId = pPerfData[selection+scrolled].ProcessId;
			hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pId);

			if (hProcess)
			{
				if (!TerminateProcess(hProcess, 0))
				{
					if (LoadString(hInst, IDS_KILL_PROCESS_ERR1, lpStr, 80))
					{
						WriteConsoleOutputCharacter(hStdout, lpEmpty, _tcslen(lpEmpty), pos, &numChars);
						WriteConsoleOutputCharacter(hStdout, lpStr, _tcslen(lpStr), pos, &numChars);
					}
					Sleep(1000);
				}

				CloseHandle(hProcess);
			}
			else
			{
				if (LoadString(hInst, IDS_KILL_PROCESS_ERR2, lpStr, 80))
				{
					WriteConsoleOutputCharacter(hStdout, lpEmpty, _tcslen(lpEmpty), pos, &numChars);
					_stprintf(lpStr, lpStr, pId);
					WriteConsoleOutputCharacter(hStdout, lpStr, _tcslen(lpStr), pos, &numChars);
				}
				Sleep(1000);
			}
		}

		first = 0;
	}
	else if (key == VK_UP)
	{
		if (selection > 0)
			selection--;
		else if ((selection == 0) && (scrolled > 0))
			scrolled--;
	}
	else if (key == VK_DOWN)
	{
		if ((selection < ProcPerScreen-1) && (selection < ProcessCount-1))
			selection++;
		else if ((selection == ProcPerScreen-1) && (selection+scrolled < ProcessCount-1))
			scrolled++;
	}
	else if (key == VK_PRIOR)
	{
		if (scrolled>ProcPerScreen-1)
			scrolled-=ProcPerScreen-1;
		else
		{
			scrolled=0; //First
			selection=0;
		}
		//selection=0;
	}
	else if (key == VK_NEXT)
	{
		scrolled+=ProcPerScreen-1;
		if (scrolled>ProcessCount-ProcPerScreen)
		{
			scrolled=ProcessCount-ProcPerScreen; //End
			selection=ProcPerScreen-1;
		}

		//selection=ProcPerScreen-1;
		if (ProcessCount<=ProcPerScreen) //If there are less process than fits on the screen
		{
			scrolled=0;
			selection=(ProcessCount%ProcPerScreen)-1;
		}
	}
	else if  (key == VK_HOME)
	{
		selection=0;
		scrolled=0;
	}
	else if  (key == VK_END)
	{
		selection=ProcPerScreen-1;
		scrolled=ProcessCount-ProcPerScreen;
		if (ProcessCount<=ProcPerScreen) //If there are less process than fits on the screen
		{
			scrolled=0;
			selection=(ProcessCount%ProcPerScreen)-1;
		}
	}
	return FALSE;
}

void PerfInit()
{
    NtQuerySystemInformation(SystemBasicInformation, &SystemBasicInfo, sizeof(SystemBasicInfo), 0);
}

void PerfDataRefresh()
{
	LONG							status;
	ULONG							ulSize;
	LPBYTE							pBuffer;
	ULONG							Idx, Idx2;
	PSYSTEM_PROCESS_INFORMATION		pSPI;
	PPERFDATA						pPDOld;
#ifdef EXTRA_INFO
	HANDLE							hProcess;
	HANDLE							hProcessToken;
	TCHAR							szTemp[MAX_PATH];
	DWORD							dwSize;
#endif
#ifdef TIMES
	LARGE_INTEGER 						liCurrentKernelTime;
	LARGE_INTEGER						liCurrentIdleTime;
	LARGE_INTEGER						liCurrentTime;
#endif
	PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION		SysProcessorTimeInfo;
	SYSTEM_TIMEOFDAY_INFORMATION				SysTimeInfo;

#ifdef TIMES
	// Get new system time
	status = NtQuerySystemInformation(SystemTimeInformation, &SysTimeInfo, sizeof(SysTimeInfo), 0);
	if (status != NO_ERROR)
		return;
#endif
	// Get processor information
	SysProcessorTimeInfo = (PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION)malloc(sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) * SystemBasicInfo.NumberOfProcessors);
	status = NtQuerySystemInformation(SystemProcessorPerformanceInformation, SysProcessorTimeInfo, sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) * SystemBasicInfo.NumberOfProcessors, &ulSize);


	// Get process information
	PsaCaptureProcessesAndThreads((PSYSTEM_PROCESS_INFORMATION *)&pBuffer);

#ifdef TIMES
	liCurrentKernelTime.QuadPart = 0;
	liCurrentIdleTime.QuadPart = 0;
	for (Idx=0; Idx<SystemBasicInfo.NumberOfProcessors; Idx++) {
		liCurrentKernelTime.QuadPart += SysProcessorTimeInfo[Idx].KernelTime.QuadPart;
		liCurrentKernelTime.QuadPart += SysProcessorTimeInfo[Idx].DpcTime.QuadPart;
		liCurrentKernelTime.QuadPart += SysProcessorTimeInfo[Idx].InterruptTime.QuadPart;
		liCurrentIdleTime.QuadPart += SysProcessorTimeInfo[Idx].IdleTime.QuadPart;
	}

	// If it's a first call - skip idle time calcs
	if (liOldIdleTime.QuadPart != 0) {
		// CurrentValue = NewValue - OldValue
		liCurrentTime.QuadPart = liCurrentIdleTime.QuadPart - liOldIdleTime.QuadPart;
		dbIdleTime = Li2Double(liCurrentTime);
		liCurrentTime.QuadPart = liCurrentKernelTime.QuadPart - liOldKernelTime.QuadPart;
		dbKernelTime = Li2Double(liCurrentTime);
		liCurrentTime.QuadPart = SysTimeInfo.CurrentTime.QuadPart - liOldSystemTime.QuadPart;
	    	dbSystemTime = Li2Double(liCurrentTime);

		// CurrentCpuIdle = IdleTime / SystemTime
		dbIdleTime = dbIdleTime / dbSystemTime;
		dbKernelTime = dbKernelTime / dbSystemTime;

		// CurrentCpuUsage% = 100 - (CurrentCpuIdle * 100) / NumberOfProcessors
		dbIdleTime = 100.0 - dbIdleTime * 100.0 / (double)SystemBasicInfo.NumberOfProcessors;// + 0.5;
		dbKernelTime = 100.0 - dbKernelTime * 100.0 / (double)SystemBasicInfo.NumberOfProcessors;// + 0.5;
	}

	// Store new CPU's idle and system time
	liOldIdleTime = liCurrentIdleTime;
	liOldSystemTime = SysTimeInfo.CurrentTime;
	liOldKernelTime = liCurrentKernelTime;
#endif

	// Determine the process count
	// We loop through the data we got from PsaCaptureProcessesAndThreads
	// and count how many structures there are (until PsaWalkNextProcess
        // returns NULL)
	ProcessCountOld = ProcessCount;
	ProcessCount = 0;
        pSPI = PsaWalkFirstProcess((PSYSTEM_PROCESS_INFORMATION)pBuffer);
	while (pSPI) {
		ProcessCount++;
		pSPI = PsaWalkNextProcess(pSPI);
	}

	// Now alloc a new PERFDATA array and fill in the data
	if (pPerfDataOld) {
		free(pPerfDataOld);
	}
	pPerfDataOld = pPerfData;
	pPerfData = (PPERFDATA)malloc(sizeof(PERFDATA) * ProcessCount);
        pSPI = PsaWalkFirstProcess((PSYSTEM_PROCESS_INFORMATION)pBuffer);
	for (Idx=0; Idx<ProcessCount; Idx++) {
		// Get the old perf data for this process (if any)
		// so that we can establish delta values
		pPDOld = NULL;
		for (Idx2=0; Idx2<ProcessCountOld; Idx2++) {
			if (pPerfDataOld[Idx2].ProcessId == (ULONG)(pSPI->UniqueProcessId) &&
			    /* check also for the creation time, a new process may have an id of an old one */
			    pPerfDataOld[Idx2].CreateTime.QuadPart == pSPI->CreateTime.QuadPart) {
				pPDOld = &pPerfDataOld[Idx2];
				break;
			}
		}

		// Clear out process perf data structure
		memset(&pPerfData[Idx], 0, sizeof(PERFDATA));

		if (pSPI->ImageName.Buffer) {
			wcsncpy(pPerfData[Idx].ImageName, pSPI->ImageName.Buffer, pSPI->ImageName.Length / sizeof(WCHAR));
                        pPerfData[Idx].ImageName[pSPI->ImageName.Length / sizeof(WCHAR)] = 0;
		}
		else
		{
#ifdef _UNICODE
			wcscpy(pPerfData[Idx].ImageName, lpIdleProcess);
#else
			MultiByteToWideChar(CP_ACP, 0, lpIdleProcess, strlen(lpIdleProcess), pPerfData[Idx].ImageName, MAX_PATH);
#endif
		}

		pPerfData[Idx].ProcessId = (ULONG)(pSPI->UniqueProcessId);
		pPerfData[Idx].CreateTime = pSPI->CreateTime;

		if (pPDOld)	{
#ifdef TIMES
			double	CurTime = Li2Double(pSPI->KernelTime) + Li2Double(pSPI->UserTime);
			double	OldTime = Li2Double(pPDOld->KernelTime) + Li2Double(pPDOld->UserTime);
			double	CpuTime = (CurTime - OldTime) / dbSystemTime;
			CpuTime = CpuTime * 100.0 / (double)SystemBasicInfo.NumberOfProcessors; // + 0.5;

			pPerfData[Idx].CPUUsage = (ULONG)CpuTime;
#else
			pPerfData[Idx].CPUUsage = 0;
#endif
		}

		pPerfData[Idx].CPUTime.QuadPart = pSPI->UserTime.QuadPart + pSPI->KernelTime.QuadPart;
		pPerfData[Idx].WorkingSetSizeBytes = pSPI->WorkingSetSize;
		pPerfData[Idx].PeakWorkingSetSizeBytes = pSPI->PeakWorkingSetSize;
		if (pPDOld)
			pPerfData[Idx].WorkingSetSizeDelta = labs((LONG)pSPI->WorkingSetSize - (LONG)pPDOld->WorkingSetSizeBytes);
		else
			pPerfData[Idx].WorkingSetSizeDelta = 0;
		pPerfData[Idx].PageFaultCount = pSPI->PageFaultCount;
		if (pPDOld)
			pPerfData[Idx].PageFaultCountDelta = labs((LONG)pSPI->PageFaultCount - (LONG)pPDOld->PageFaultCount);
		else
			pPerfData[Idx].PageFaultCountDelta = 0;
		pPerfData[Idx].VirtualMemorySizeBytes = pSPI->VirtualSize;
		pPerfData[Idx].PagedPoolUsagePages = pSPI->QuotaPagedPoolUsage;
		pPerfData[Idx].NonPagedPoolUsagePages = pSPI->QuotaNonPagedPoolUsage;
		pPerfData[Idx].BasePriority = pSPI->BasePriority;
		pPerfData[Idx].HandleCount = pSPI->HandleCount;
		pPerfData[Idx].ThreadCount = pSPI->NumberOfThreads;
		//pPerfData[Idx].SessionId = pSPI->SessionId;

#ifdef EXTRA_INFO
		hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, (DWORD)pSPI->UniqueProcessId);
		if (hProcess) {
			if (OpenProcessToken(hProcess, TOKEN_QUERY|TOKEN_DUPLICATE|TOKEN_IMPERSONATE, &hProcessToken)) {
				ImpersonateLoggedOnUser(hProcessToken);
				memset(szTemp, 0, sizeof(TCHAR[MAX_PATH]));
				dwSize = MAX_PATH;
				GetUserName(szTemp, &dwSize);
#ifndef UNICODE
				MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, szTemp, -1, pPerfData[Idx].UserName, MAX_PATH);
#endif
				RevertToSelf();
				CloseHandle(hProcessToken);
			}
			CloseHandle(hProcess);
		}
#endif
#ifdef TIMES
		pPerfData[Idx].UserTime.QuadPart = pSPI->UserTime.QuadPart;
		pPerfData[Idx].KernelTime.QuadPart = pSPI->KernelTime.QuadPart;
#endif
		pSPI = PsaWalkNextProcess(pSPI);
	}
	PsaFreeCapture(pBuffer);

	free(SysProcessorTimeInfo);
}

// Code partly taken from slw32tty.c from mc/slang
unsigned int GetKeyPressed(int events)
{
	DWORD bytesRead;
	INPUT_RECORD record;
	int i;


	for (i=0; i<events; i++)
	{
		if (!ReadConsoleInput(hStdin, &record, 1, &bytesRead)) {
			return 0;
		}

		if (record.EventType == KEY_EVENT && record.Event.KeyEvent.bKeyDown)
			return record.Event.KeyEvent.wVirtualKeyCode;//.uChar.AsciiChar;
	}

	return 0;
}


int _tmain(int argc, char **argv)
{
	int i;
	TCHAR lpStr[80];

	for (i = 0; i < 80; i++)
		lpEmpty[i] = lpHeader[i] = _T(' ');
	lpEmpty[79] = _T('\0');

	/* Initialize global variables */
	hInst = 0 /* FIXME: which value? [used with LoadString(hInst, ..., ..., ...)] */;

	if (LoadString(hInst, IDS_COLUMN_NUMBER, lpStr, 80))
	{
		columnRightPositions[0] = _tcslen(lpStr) + 3;
		_tcsncpy(&lpHeader[2], lpStr, _tcslen(lpStr));
	}
	if (LoadString(hInst, IDS_COLUMN_IMAGENAME, lpStr, 80))
	{
		columnRightPositions[1] = columnRightPositions[0] + _tcslen(lpStr) + 3;
		_tcsncpy(&lpHeader[columnRightPositions[0] + 2], lpStr, _tcslen(lpStr));
	}
	if (LoadString(hInst, IDS_COLUMN_PID, lpStr, 80))
	{
		columnRightPositions[2] = columnRightPositions[1] + _tcslen(lpStr) + 3;
		_tcsncpy(&lpHeader[columnRightPositions[1] + 2], lpStr, _tcslen(lpStr));
	}
	if (LoadString(hInst, IDS_COLUMN_CPU, lpStr, 80))
	{
		columnRightPositions[3] = columnRightPositions[2] + _tcslen(lpStr) + 3;
		_tcsncpy(&lpHeader[columnRightPositions[2] + 2], lpStr, _tcslen(lpStr));
	}
	if (LoadString(hInst, IDS_COLUMN_MEM, lpStr, 80))
	{
		columnRightPositions[4] = columnRightPositions[3] + _tcslen(lpStr) + 3;
		_tcsncpy(&lpHeader[columnRightPositions[3] + 2], lpStr, _tcslen(lpStr));
	}
	if (LoadString(hInst, IDS_COLUMN_PF, lpStr, 80))
	{
		columnRightPositions[5] = columnRightPositions[4] + _tcslen(lpStr) + 3;
		_tcsncpy(&lpHeader[columnRightPositions[4] + 2], lpStr, _tcslen(lpStr));
	}

	for (i = 0; i < columnRightPositions[5]; i++)
		lpSeparator[i] = _T('-');
	lpHeader[0] = _T('|');
	lpSeparator[0] = _T('+');
	for (i = 0; i < 6; i++)
	{
		lpHeader[columnRightPositions[i]] = _T('|');
		lpSeparator[columnRightPositions[i]] = _T('+');
	}
	lpSeparator[columnRightPositions[5] + 1] = _T('\0');
	lpHeader[columnRightPositions[5] + 1] = _T('\0');


	if (!LoadString(hInst, IDS_APP_TITLE, lpTitle, 80))
		lpTitle[0] = _T('\0');
	if (!LoadString(hInst, IDS_COLUMN_MEM_UNIT, lpMemUnit, 3))
		lpMemUnit[0] = _T('\0');
	if (!LoadString(hInst, IDS_MENU, lpMenu, 80))
		lpMenu[0] = _T('\0');
	if (!LoadString(hInst, IDS_IDLE_PROCESS, lpIdleProcess, 80))
		lpIdleProcess[0] = _T('\0');

	if (LoadString(hInst, IDS_MENU_QUIT, lpStr, 2))
		KEY_QUIT = lpStr[0];
	if (LoadString(hInst, IDS_MENU_KILL_PROCESS, lpStr, 2))
		KEY_KILL = lpStr[0];
	if (LoadString(hInst, IDS_YES, lpStr, 2))
		KEY_YES = lpStr[0];
	if (LoadString(hInst, IDS_NO, lpStr, 2))
		KEY_NO = lpStr[0];

	GetInputOutputHandles();

	if (hStdin == INVALID_HANDLE_VALUE || hStdout == INVALID_HANDLE_VALUE)
	{
		if (LoadString(hInst, IDS_CTM_GENERAL_ERR1, lpStr, 80))
			_tprintf(lpStr);
		return -1;
	}

	if (GetConsoleMode(hStdin, &inConMode) == 0)
	{
		if (LoadString(hInst, IDS_CTM_GENERAL_ERR2, lpStr, 80))
			_tprintf(lpStr);
		return -1;
	}

	if (GetConsoleMode(hStdout, &outConMode) == 0)
	{
		if (LoadString(hInst, IDS_CTM_GENERAL_ERR3, lpStr, 80))
			_tprintf(lpStr);
		return -1;
	}

	SetConsoleMode(hStdin, 0); //FIXME: Should check for error!
	SetConsoleMode(hStdout, 0); //FIXME: Should check for error!

	PerfInit();

	while (1)
	{
		DWORD numEvents;

		PerfDataRefresh();
		DisplayScreen();

		/* WaitForSingleObject for console handles is not implemented in ROS */
		WaitForSingleObject(hStdin, 1000);
		GetNumberOfConsoleInputEvents(hStdin, &numEvents);

		if (numEvents > 0)
		{
			if (ProcessKeys(numEvents) == TRUE)
				break;
		}
	}

	RestoreConsole();
	return 0;
}
