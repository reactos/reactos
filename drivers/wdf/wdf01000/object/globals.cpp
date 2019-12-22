#include "common/fxglobals.h"

extern "C" {

VOID
FxLibraryGlobalsVerifyVersion()
{
	OSVERSIONINFOEXW osvi;
	DWORDLONG dwlConditionMask = 0;
	NTSTATUS status;

	//Check Windows Xp Sp2
	RtlZeroMemory(&osvi, sizeof(OSVERSIONINFOEXW));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);
	osvi.dwMajorVersion = 5;
	osvi.dwMinorVersion = 1;
	osvi.wServicePackMajor = 2;

	VER_SET_CONDITION(dwlConditionMask, VER_MAJORVERSION, VER_EQUAL);
	VER_SET_CONDITION(dwlConditionMask, VER_MINORVERSION, VER_EQUAL);
	VER_SET_CONDITION(dwlConditionMask, VER_SERVICEPACKMAJOR, VER_LESS);

	status = RtlVerifyVersionInfo(&osvi, VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR, dwlConditionMask);
	if (NT_SUCCESS(status))
	{
		FxLibraryGlobals.AllowWmiUpdates = FALSE;
	}

	//Check Windows Server 2003
	RtlZeroMemory(&osvi, sizeof(OSVERSIONINFOEXW));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);
	osvi.dwMajorVersion = 5;
	osvi.dwMinorVersion = 2;

	VER_SET_CONDITION(dwlConditionMask, VER_MAJORVERSION, VER_EQUAL);
	VER_SET_CONDITION(dwlConditionMask, VER_MINORVERSION, VER_EQUAL);
	VER_SET_CONDITION(dwlConditionMask, VER_SERVICEPACKMAJOR, VER_EQUAL);

	status = RtlVerifyVersionInfo(&osvi, VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR, dwlConditionMask);
	if (NT_SUCCESS(status))
	{
		FxLibraryGlobals.AllowWmiUpdates = FALSE;
	}

	//Check Windows Vista
	RtlZeroMemory(&osvi, sizeof(OSVERSIONINFOEXW));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);
	osvi.dwMajorVersion = 6;

	VER_SET_CONDITION(dwlConditionMask, VER_MAJORVERSION, VER_GREATER);

	status = RtlVerifyVersionInfo(&osvi, VER_MAJORVERSION, dwlConditionMask);
	if (NT_SUCCESS(status))
	{
		FxLibraryGlobals.UseTargetSystemPowerState = TRUE;
	}
}

NTSTATUS
FxLibraryGlobalsCommission(VOID)
{
	return STATUS_SUCCESS;
}

VOID
FxLibraryGlobalsDecommission(
	VOID
)
{
	return;
}

}