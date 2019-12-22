#include "fxlibrarycommon.h"
#include "common/fxglobals.h" 

NTSTATUS
FxLibraryCommonCommission(VOID)
{
	DECLARE_CONST_UNICODE_STRING(usName, L"RtlGetVersion");
	PFN_RTL_GET_VERSION pRtlGetVersion = NULL;
	NTSTATUS   status;

	__Print((LITERAL(WDF_LIBRARY_COMMISSION) "\n"));

	//
	// Commission this version's DLL globals.
	//
	status = FxLibraryGlobalsCommission();

	if (!NT_SUCCESS(status))
	{
		__Print(("FxLibraryGlobalsCommission failed %X\n", status));
		return status;
	}

	//
	// Attempt to load RtlGetVersion (works for > w2k).
	//
	pRtlGetVersion = (PFN_RTL_GET_VERSION)MmGetSystemRoutineAddress(
		(PUNICODE_STRING)&usName
	);

	//
	// Now attempt to get this OS's version.
	//
	if (pRtlGetVersion != NULL)
	{
		pRtlGetVersion(&gOsVersion);
	}

	__Print(("OsVersion(%d.%d)\n",
		gOsVersion.dwMajorVersion,
		gOsVersion.dwMinorVersion));

	return STATUS_SUCCESS;
}

NTSTATUS
FxLibraryCommonDecommission(
	VOID
)
{
	__Print((LITERAL(WDF_LIBRARY_DECOMMISSION) ": enter\n"));

	//
	// Decommission this version's DLL globals.
	//
	FxLibraryGlobalsDecommission();

	//
	// Note: This is the absolute last action from WDF library (dynamic or static).
	//       The image is likely to be deleted after returning.
	//
	__Print((LITERAL(WDF_LIBRARY_DECOMMISSION) ": exit\n"));

	return STATUS_SUCCESS;
}

NTSTATUS
FxLibraryCommonRegisterClient(
	__inout PWDF_BIND_INFO        Info,
	__deref_out PWDF_DRIVER_GLOBALS* WdfDriverGlobals,
	__in_opt PCLIENT_INFO          ClientInfo
)
{
	NTSTATUS           status;	

	status = STATUS_INVALID_PARAMETER;

	__Print((LITERAL(WDF_LIBRARY_REGISTER_CLIENT) ": enter\n"));

	if (Info == NULL || WdfDriverGlobals == NULL || Info->FuncTable == NULL)
	{
		__Print((LITERAL(WDF_LIBRARY_REGISTER_CLIENT)
			": NULL parameter -- %s\n",
			(Info == NULL) ? "PWDF_BIND_INFO" :
			(WdfDriverGlobals == NULL) ? "PWDF_DRIVER_GLOBALS *" :
			(Info->FuncTable == NULL) ? "PWDF_BIND_INFO->FuncTable" :
			"unknown"));
		goto Done;
	}

	ASSERT(Info->FuncCount);


	*WdfDriverGlobals = NULL;

	
Done:
	__Print((LITERAL(WDF_LIBRARY_REGISTER_CLIENT)
		": exit: status %X\n", status));

	return status;
}


NTSTATUS
FxLibraryCommonUnregisterClient(
	__in PWDF_BIND_INFO        Info,
	__in PWDF_DRIVER_GLOBALS   WdfDriverGlobals
)
{
	NTSTATUS status;

	__Print((LITERAL(WDF_LIBRARY_UNREGISTER_CLIENT) ": enter\n"));

	ASSERT(Info);
	ASSERT(WdfDriverGlobals);
	
	status = STATUS_UNSUCCESSFUL;


	__Print((LITERAL(WDF_LIBRARY_UNREGISTER_CLIENT)
		": exit: status %X\n", status));

	return status;
}
