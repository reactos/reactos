/*
 * This example demonstrates dynamical loading of (internal) Win32 DLLS.
 * I dont know why this was called hello5 in the wine tree - sedwards
 */
#include <stdio.h>
#include <windows.h>

int PASCAL WinMain (HINSTANCE inst, HINSTANCE prev, LPSTR cmdline, int show)
{
	SYSTEM_INFO	si;
	void CALLBACK (*fnGetSystemInfo)(LPSYSTEM_INFO si);
	HMODULE	kernel32;

	kernel32 = LoadLibrary("KERNEL32");
	if (kernel32<32) {
		fprintf(stderr,"FATAL: could not load KERNEL32!\n");
		return 0;
	}
	fnGetSystemInfo = (void*)GetProcAddress(kernel32,"GetSystemInfo");
	if (!fnGetSystemInfo) {
		fprintf(stderr,"FATAL: could not find GetSystemInfo!\n");
		return 0;
	}
	fnGetSystemInfo(&si);
	fprintf(stderr,"QuerySystemInfo returns:\n");
	fprintf(stderr,"	wProcessorArchitecture: %d\n",si.u.s.wProcessorArchitecture);
	fprintf(stderr,"	dwPageSize: %ld\n",si.dwPageSize);
	fprintf(stderr,"	lpMinimumApplicationAddress: %p\n",si.lpMinimumApplicationAddress);
	fprintf(stderr,"	lpMaximumApplicationAddress: %p\n",si.lpMaximumApplicationAddress);
	fprintf(stderr,"	dwActiveProcessorMask: %ld\n",si.dwActiveProcessorMask);
	fprintf(stderr,"	dwNumberOfProcessors: %ld\n",si.dwNumberOfProcessors);
	fprintf(stderr,"	dwProcessorType: %ld\n",si.dwProcessorType);
	fprintf(stderr,"	dwAllocationGranularity: %ld\n",si.dwAllocationGranularity);
	fprintf(stderr,"	wProcessorLevel: %d\n",si.wProcessorLevel);
	fprintf(stderr,"	wProcessorRevision: %d\n",si.wProcessorRevision);
	return 0;
}
