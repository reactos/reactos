/* Console Task Manager

   ctm.c - main program file

   Written by: Aleksey Bragin (aleksey@studiocerebral.com)
   
   Most of the code dealing with getting system parameters is taken from
   ReactOS Task Manager written by Brian Palmer (brianp@reactos.org)

   History:
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


//#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows //headers
#include <windows.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <process.h>
#include <stdio.h>

#include <ddk/ntddk.h>
#include <epsapi.h>

#include "ctm.h"

#define MAX_PROC 17
//#define TIMES

HANDLE hStdin;
HANDLE hStdout;

DWORD inConMode;
DWORD outConMode;

//PROCNTQSI		NtQuerySystemInformation= NULL;

const int		ProcPerScreen = 17; // 17 processess are displayed on one page
ULONG			ProcessCountOld = 0;
ULONG			ProcessCount = 0;

double			dbIdleTime;
double			dbKernelTime;
double			dbSystemTime;
LARGE_INTEGER		liOldIdleTime = {{0,0}};
double			OldKernelTime = 0;
LARGE_INTEGER		liOldSystemTime = {{0,0}};

PPERFDATA		pPerfDataOld = NULL;	// Older perf data (saved to establish delta values)
PPERFDATA		pPerfData = NULL;		// Most recent copy of perf data

int selection=0;
int scrolled=0;		// offset from which process start showing

#define NEW_CONSOLE

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
	char lpStr[80];
	int idx;
	DWORD numChars;
	int lines;

	// Header
	pos.X = 2; pos.Y = 2;
	strcpy(lpStr, "Console TaskManager v0.1 by Aleksey Bragin <aleksey@studiocerebral.com>");
	WriteConsoleOutputCharacter(hStdout, lpStr, strlen(lpStr), pos, &numChars);

	pos.X = 2; pos.Y = 3;
	strcpy(lpStr, "+-------------------------------+-------+-----+-----------+-------------+");
	WriteConsoleOutputCharacter(hStdout, lpStr, strlen(lpStr), pos, &numChars);

	pos.X = 2; pos.Y = 4;
	strcpy(lpStr, "| Image name                    | PID   | CPU | Mem Usage | Page Faults |");
	WriteConsoleOutputCharacter(hStdout, lpStr, strlen(lpStr), pos, &numChars);

	pos.X = 2; pos.Y = 5;
	strcpy(lpStr, "+-------------------------------+-------+-----+-----------+-------------+");
	WriteConsoleOutputCharacter(hStdout, lpStr, strlen(lpStr), pos, &numChars);

	// Footer
	pos.X = 2; pos.Y = 23;
	strcpy(lpStr, "+-------------------------------+-------+-----+-----------+-------------+");
	WriteConsoleOutputCharacter(hStdout, lpStr, strlen(lpStr), pos, &numChars);

	// Menu
	pos.X = 2; pos.Y = 24;
	strcpy(lpStr, "Press: q - quit, k - kill process                                        ");
	WriteConsoleOutputCharacter(hStdout, lpStr, strlen(lpStr), pos, &numChars);

	// Processess
	lines = ProcessCount;
	if (lines > MAX_PROC)
		lines = MAX_PROC;
	for (idx=0; idx<lines; idx++)
	{
		int len;
		char imgName[MAX_PATH];
		char lpPid[8];
		char lpCpu[6];
		char lpMemUsg[12];
		char lpPageFaults[15];
		WORD wColor;

		// data
		// image name
		pos.X = 3; pos.Y = 6+idx;
		memset(imgName, 0, MAX_PATH);
		WideCharToMultiByte(CP_ACP, 0, pPerfData[scrolled+idx].ImageName, -1,
			imgName, MAX_PATH, NULL, NULL);
		len = strlen(imgName);
		WriteConsoleOutputCharacter(hStdout, "                             ", 30, pos, &numChars);
		WriteConsoleOutputCharacter(hStdout, imgName, (len > 30) ? 30 : len, pos, &numChars);

		// PID
		pos.X = 35; pos.Y = 6+idx;
		sprintf(lpPid, "%6ld", pPerfData[scrolled+idx].ProcessId);
		WriteConsoleOutputCharacter(hStdout, lpPid, strlen(lpPid), pos, &numChars);

		// CPU
		pos.X = 43; pos.Y = 6+idx;
		sprintf(lpCpu, "%3d%%", pPerfData[scrolled+idx].CPUUsage);
		WriteConsoleOutputCharacter(hStdout, lpCpu, strlen(lpCpu), pos, &numChars);

		// Mem usage
		pos.X = 49; pos.Y = 6+idx;
		sprintf(lpMemUsg, "%6ld", pPerfData[scrolled+idx].WorkingSetSizeBytes / 1024);
		WriteConsoleOutputCharacter(hStdout, lpMemUsg, strlen(lpMemUsg), pos, &numChars);

		// Page Fault
		pos.X = 61; pos.Y = 6+idx;
		sprintf(lpPageFaults, "%12ld", pPerfData[scrolled+idx].PageFaultCount);
		WriteConsoleOutputCharacter(hStdout, lpPageFaults, strlen(lpPageFaults), pos, &numChars);

		// columns
		pos.X = 2; pos.Y = 6+idx;
		WriteConsoleOutputCharacter(hStdout, "|", 1, pos, &numChars);
		pos.X = 34; pos.Y = 6+idx;
		WriteConsoleOutputCharacter(hStdout, "|", 1, pos, &numChars);
		pos.X = 42; pos.Y = 6+idx;
		WriteConsoleOutputCharacter(hStdout, "|", 1, pos, &numChars);
		pos.X = 48; pos.Y = 6+idx;
		WriteConsoleOutputCharacter(hStdout, "|", 1, pos, &numChars);
		pos.X = 60; pos.Y = 6+idx;
		WriteConsoleOutputCharacter(hStdout, "|", 1, pos, &numChars);
		pos.X = 74; pos.Y = 6+idx;
		WriteConsoleOutputCharacter(hStdout, "|", 1, pos, &numChars);


		// Attributes now...
		pos.X = 3; pos.Y = 6+idx;
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
			31,            // number of cells to fill 
			pos,            // first cell to write to 
			&numChars);       // actual number written 
	}

	return;
}

// returns TRUE if exiting
int ProcessKeys(int numEvents)
{
	if ((ProcessCount-scrolled < 17) && (ProcessCount > 17))
		scrolled = ProcessCount-17;

	unsigned char key = GetKeyPressed(numEvents);
	if (key == VK_Q)
		return TRUE;
	else if (key == VK_K)
	{
		// user wants to kill some process, get his acknowledgement
		DWORD pId;
		COORD pos;
		char lpStr[100];

		pos.X = 2; pos.Y = 24;
		strcpy(lpStr, "Are you sure you want to kill this process? (y/n)");
		WriteConsoleOutputCharacter(hStdout, lpStr, strlen(lpStr), pos, &pId);

		do {
			GetNumberOfConsoleInputEvents(hStdin, &pId);
			key = GetKeyPressed(pId);
		} while (key == 0);
	
		if (key == VK_Y)
		{
			HANDLE hProcess;
			pId = pPerfData[selection+scrolled].ProcessId;
			hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pId);
			
			if (hProcess)
			{
				if (!TerminateProcess(hProcess, 0))
				{
					strcpy(lpStr, "Unable to terminate this process...                                      ");
					WriteConsoleOutputCharacter(hStdout, lpStr, strlen(lpStr), pos, &pId);
					Sleep(1000);
				}

				CloseHandle(hProcess);
			}
			else
			{
					sprintf(lpStr, "Unable to terminate process %3d (unable to OpenProcess)               ", pId);
					WriteConsoleOutputCharacter(hStdout, lpStr, strlen(lpStr), pos, &pId);
					Sleep(1000);
			}
		}
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
		if ((selection < MAX_PROC-1) && (selection < ProcessCount-1))
			selection++;
		else if ((selection == MAX_PROC-1) && (selection+scrolled < ProcessCount-1))
			scrolled++;
	}
	
	return FALSE;
}

void PerfInit()
{
//	NtQuerySystemInformation = //(PROCNTQSI)GetProcAddress(GetModuleHandle(_T("ntdll.dll")), //"NtQuerySystemInformation");
}

void PerfDataRefresh()
{
	LONG							status;
	ULONG							ulSize;
	LPBYTE							pBuffer;
	ULONG							BufferSize;
	ULONG							Idx, Idx2;
	HANDLE							hProcess;
	HANDLE							hProcessToken;
	PSYSTEM_PROCESSES		pSPI;
	PPERFDATA						pPDOld;
	TCHAR							szTemp[MAX_PATH];
	DWORD							dwSize;
	double							CurrentKernelTime;
	PSYSTEM_PROCESSORTIME_INFO		SysProcessorTimeInfo;
	SYSTEM_PERFORMANCE_INFO	SysPerfInfo;
	SYSTEM_TIMEOFDAY_INFORMATION            SysTimeInfo;

#ifdef TIMES
	// Get new system time
	status = NtQuerySystemInformation(SystemTimeInformation, &SysTimeInfo, sizeof(SysTimeInfo), 0);
	if (status != NO_ERROR)
		return;

	// Get new CPU's idle time
	status = NtQuerySystemInformation(SystemPerformanceInformation, &SysPerfInfo, sizeof(SysPerfInfo), NULL);
	if (status != NO_ERROR)
		return;
#endif
	// Get processor information	
	SysProcessorTimeInfo = (PSYSTEM_PROCESSORTIME_INFO)malloc(sizeof(SYSTEM_PROCESSORTIME_INFO) * 1/*SystemBasicInfo.bKeNumberProcessors*/);
	status = NtQuerySystemInformation(SystemProcessorTimes, SysProcessorTimeInfo, sizeof(SYSTEM_PROCESSORTIME_INFO) * 1/*SystemBasicInfo.bKeNumberProcessors*/, &ulSize);


	// Get process information
	PsaCaptureProcessesAndThreads((PSYSTEM_PROCESSES *)&pBuffer);

#ifdef TIMES
	for (CurrentKernelTime=0, Idx=0; Idx<1/*SystemBasicInfo.bKeNumberProcessors*/; Idx++) {
		CurrentKernelTime += Li2Double(SysProcessorTimeInfo[Idx].KernelTime);
		CurrentKernelTime += Li2Double(SysProcessorTimeInfo[Idx].DpcTime);
		CurrentKernelTime += Li2Double(SysProcessorTimeInfo[Idx].InterruptTime);
	}

	// If it's a first call - skip idle time calcs
	if (liOldIdleTime.QuadPart != 0) {
		// CurrentValue = NewValue - OldValue
		dbIdleTime = Li2Double(SysPerfInfo.liIdleTime) - Li2Double(liOldIdleTime);
		dbKernelTime = CurrentKernelTime - OldKernelTime;
		dbSystemTime = Li2Double(SysTimeInfo.CurrentTime) - Li2Double(liOldSystemTime);

		// CurrentCpuIdle = IdleTime / SystemTime
		dbIdleTime = dbIdleTime / dbSystemTime;
		dbKernelTime = dbKernelTime / dbSystemTime;
		
		// CurrentCpuUsage% = 100 - (CurrentCpuIdle * 100) / NumberOfProcessors
		dbIdleTime = 100.0 - dbIdleTime * 100.0; /* / (double)SystemBasicInfo.bKeNumberProcessors;// + 0.5; */
		dbKernelTime = 100.0 - dbKernelTime * 100.0; /* / (double)SystemBasicInfo.bKeNumberProcessors;// + 0.5; */
	}

	// Store new CPU's idle and system time
	liOldIdleTime = SysPerfInfo.liIdleTime;
	liOldSystemTime = SysTimeInfo.CurrentTime;
	OldKernelTime = CurrentKernelTime;
#endif

	// Determine the process count
	// We loop through the data we got from PsaCaptureProcessesAndThreads
	// and count how many structures there are (until PsaWalkNextProcess
        // returns NULL)
	ProcessCountOld = ProcessCount;
	ProcessCount = 0;
        pSPI = PsaWalkFirstProcess((PSYSTEM_PROCESSES)pBuffer);
	while (pSPI) {
		ProcessCount++;
		pSPI = PsaWalkNextProcess(pSPI);
	}

	// Now alloc a new PERFDATA array and fill in the data
	if (pPerfDataOld) {
		//delete[] pPerfDataOld;
		free(pPerfDataOld);
	}
	pPerfDataOld = pPerfData;
	//pPerfData = new PERFDATA[ProcessCount];
	pPerfData = (PPERFDATA)malloc(sizeof(PERFDATA) * ProcessCount);
        pSPI = PsaWalkFirstProcess((PSYSTEM_PROCESSES)pBuffer);
	for (Idx=0; Idx<ProcessCount; Idx++) {
		// Get the old perf data for this process (if any)
		// so that we can establish delta values
		pPDOld = NULL;
		for (Idx2=0; Idx2<ProcessCountOld; Idx2++) {
			if (pPerfDataOld[Idx2].ProcessId == pSPI->ProcessId) {
				pPDOld = &pPerfDataOld[Idx2];
				break;
			}
		}

		// Clear out process perf data structure
		memset(&pPerfData[Idx], 0, sizeof(PERFDATA));

		if (pSPI->ProcessName.Buffer)
			wcsncpy(pPerfData[Idx].ImageName, pSPI->ProcessName.Buffer, pSPI->ProcessName.MaximumLength);
		else
			wcscpy(pPerfData[Idx].ImageName, L"System Idle Process");

		pPerfData[Idx].ProcessId = pSPI->ProcessId;

		if (pPDOld)	{
#ifdef TIMES
			double	CurTime = Li2Double(pSPI->KernelTime) + Li2Double(pSPI->UserTime);
			double	OldTime = Li2Double(pPDOld->KernelTime) + Li2Double(pPDOld->UserTime);
			double	CpuTime = (CurTime - OldTime) / dbSystemTime;
			CpuTime = CpuTime * 100.0; /* / (double)SystemBasicInfo.bKeNumberProcessors;// + 0.5;*/

			pPerfData[Idx].CPUUsage = (ULONG)CpuTime;
#else
			pPerfData[Idx].CPUUsage = 0;
#endif
		}

		pPerfData[Idx].CPUTime.QuadPart = pSPI->UserTime.QuadPart + pSPI->KernelTime.QuadPart;
		pPerfData[Idx].WorkingSetSizeBytes = pSPI->VmCounters.WorkingSetSize;
		pPerfData[Idx].PeakWorkingSetSizeBytes = pSPI->VmCounters.PeakWorkingSetSize;
		if (pPDOld)
			pPerfData[Idx].WorkingSetSizeDelta = labs((LONG)pSPI->VmCounters.WorkingSetSize - (LONG)pPDOld->WorkingSetSizeBytes);
		else
			pPerfData[Idx].WorkingSetSizeDelta = 0;
		pPerfData[Idx].PageFaultCount = pSPI->VmCounters.PageFaultCount;
		if (pPDOld)
			pPerfData[Idx].PageFaultCountDelta = labs((LONG)pSPI->VmCounters.PageFaultCount - (LONG)pPDOld->PageFaultCount);
		else
			pPerfData[Idx].PageFaultCountDelta = 0;
		pPerfData[Idx].VirtualMemorySizeBytes = pSPI->VmCounters.VirtualSize;
		pPerfData[Idx].PagedPoolUsagePages = pSPI->VmCounters.QuotaPagedPoolUsage;
		pPerfData[Idx].NonPagedPoolUsagePages = pSPI->VmCounters.QuotaNonPagedPoolUsage;
		pPerfData[Idx].BasePriority = pSPI->BasePriority;
		pPerfData[Idx].HandleCount = pSPI->HandleCount;
		pPerfData[Idx].ThreadCount = pSPI->ThreadCount;
		//pPerfData[Idx].SessionId = pSPI->SessionId;

#ifdef EXTRA_INFO
		hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pSPI->ProcessId);
		if (hProcess) {
			if (OpenProcessToken(hProcess, TOKEN_QUERY|TOKEN_DUPLICATE|TOKEN_IMPERSONATE, &hProcessToken)) {
				ImpersonateLoggedOnUser(hProcessToken);
				memset(szTemp, 0, sizeof(TCHAR[MAX_PATH]));
				dwSize = MAX_PATH;
				GetUserName(szTemp, &dwSize);
#ifndef UNICODE
				MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, szTemp, -1, pPerfData[Idx].UserName, MAX_PATH);
/*
int MultiByteToWideChar(
  UINT CodePage,         // code page
  DWORD dwFlags,         // character-type options
  LPCSTR lpMultiByteStr, // string to map
  int cbMultiByte,       // number of bytes in string
  LPWSTR lpWideCharStr,  // wide-character buffer
  int cchWideChar        // size of buffer
);
 */
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
	//delete[] pBuffer;
	PsaFreeCapture(pBuffer);

	free(SysProcessorTimeInfo);
}

// Code partly taken from slw32tty.c from mc/slang
unsigned int GetKeyPressed(int events)
{
	long key;
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


int main(int *argc, char **argv)
{
	GetInputOutputHandles();

	if (hStdin == INVALID_HANDLE_VALUE || hStdout == INVALID_HANDLE_VALUE)
	{
		printf("ctm: can't use console.");
		return -1;
	}

	if (GetConsoleMode(hStdin, &inConMode) == 0)
	{
		printf("ctm: can't GetConsoleMode() for input console.");
		return -1;
	}

	if (GetConsoleMode(hStdout, &outConMode) == 0)
	{
		printf("ctm: can't GetConsoleMode() for output console.");
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

		//WriteConsole(hStdin, " ", 1, &numEvents, NULL); // TODO: Make another way (this is ugly, I know)
		GetNumberOfConsoleInputEvents(hStdin, &numEvents);

		if (numEvents > 0)
		{
			if (ProcessKeys(numEvents) == TRUE)
				break;
		}
		else
			Sleep(40); // TODO: Should be done more efficient (might be another thread handling input/etc)*/
	}

	RestoreConsole();
	return 0;
}
