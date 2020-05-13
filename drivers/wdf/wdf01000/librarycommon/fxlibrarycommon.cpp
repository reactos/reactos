#include "fxlibrarycommon.h"
#include "common/fxglobals.h"
#include "common/fxverifier.h"
#include "common/fxregkey.h"
#include "common/fxautoregistry.h"
#include "common/fxpool.h"
#include "common/fxtrace.h"
#include "fxdynamics.h"
#include "common/fxdriver.h"


NTSTATUS
NTAPI
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
NTAPI
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
NTAPI
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

	if (Info->FuncCount > WdfVersion.FuncCount)
	{
        __Print((LITERAL(WDF_LIBRARY_REGISTER_CLIENT)
                 ": version mismatch detected in function table count: client"
                 "has 0x%x,  library has 0x%x\n",
                 Info->FuncCount, WdfVersion.FuncCount));
        goto Done;
    }

	if (Info->FuncCount <= 396)
	{
        //
        // Make sure table count matches exactly with previously
        // released framework version table sizes.
        //
        switch (Info->FuncCount)
		{

        //case WdfFunctionTableNumEntries_V1_25: // 453 - win10 1803 RS4
        //case WdfFunctionTableNumEntries_V1_23: // 451 - win10 1709 RS3
        //case WdfFunctionTableNumEntries_V1_21: // 448 - win10 1703 RS2
        //case WdfFunctionTableNumEntries_V1_19: // 446 - win10 1607 RS1
     // case WdfFunctionTableNumEntries_V1_17: // 444 - win10 1511 TH2
        //case WdfFunctionTableNumEntries_V1_15: // 444 - win10 1507 TH1
        //case WdfFunctionTableNumEntries_V1_13: // 438 - win8.1
        //case WdfFunctionTableNumEntries_V1_11: // 432 - win8
        case 396:  // 396 - win7
     // case WdfFunctionTableNumEntries_V1_7:  // 387 - vista sp1
        case 387:  // 387 - vista
        case 386:  // 386
        case 383:  // 383
            break;

        default:
            __Print((LITERAL(WDF_LIBRARY_REGISTER_CLIENT)
                     ": Function table count 0x%x doesn't match any previously "
                     "released framework version table size\n",
                     Info->FuncCount));
            goto Done;
        }
    }
	else
	{
		goto Done;
	}
	
	//
    // Allocate an new FxDriverGlobals area for this driver.
    //
    *WdfDriverGlobals = FxAllocateDriverGlobals();

	if (*WdfDriverGlobals) 
	{
		BOOLEAN isFunctinTableHookingOn = FALSE;
		PFX_DRIVER_GLOBALS fxDriverGlobals = NULL;

        //
        // Check the registry to see if Enhanced verifier is on for this driver.
        // if registry read fails, options value remains unchanged.
        // store enhanced verifier options in driver globals
        //
        fxDriverGlobals = GetFxDriverGlobals(*WdfDriverGlobals);
		GetEnhancedVerifierOptions(ClientInfo, &fxDriverGlobals->FxEnhancedVerifierOptions);
		isFunctinTableHookingOn = IsFxVerifierFunctionTableHooking(fxDriverGlobals);

		if (isFunctinTableHookingOn)
		{
			__Print((LITERAL(WDF_LIBRARY_REGISTER_CLIENT)
                     ": Enhanced Verification is ON \n"));
			
			//copy verifyed func table
			status = STATUS_UNSUCCESSFUL;
		}
		else
		{
			RtlCopyMemory(Info->FuncTable, &WdfVersion.Functions, sizeof(size_t) * WdfVersion.FuncCount);
		}

		status = STATUS_SUCCESS;

        __Print((LITERAL(WDF_LIBRARY_REGISTER_CLIENT)
                 ": WdfFunctions %p\n", Info->FuncTable));
	}
	
Done:
	__Print((LITERAL(WDF_LIBRARY_REGISTER_CLIENT)
		": exit: status %X\n", status));

	return status;
}


NTSTATUS
NTAPI
FxLibraryCommonUnregisterClient(
	__in PWDF_BIND_INFO        Info,
	__in PWDF_DRIVER_GLOBALS   WdfDriverGlobals
)
{
    NTSTATUS status;

    __Print((LITERAL(WDF_LIBRARY_UNREGISTER_CLIENT) ": enter\n"));

    ASSERT(Info);
    ASSERT(WdfDriverGlobals);

    if (Info != NULL && WdfDriverGlobals != NULL)
	{
        PFX_DRIVER_GLOBALS pFxDriverGlobals;

        status = STATUS_SUCCESS;

        pFxDriverGlobals = GetFxDriverGlobals(WdfDriverGlobals);

        //
        // Destroy this FxDriver instance, if its still indicated.
        //
        if (pFxDriverGlobals->Driver != NULL)
		{
            //
            // Association support, we are a root with no parent
            //
            pFxDriverGlobals->Driver->DeleteObject();

            FxDestroy(pFxDriverGlobals);
        }

        //
        // Stop IFR logging
        //
        FxIFRStop(pFxDriverGlobals);

        //
        // unlock enhanced-verifier image sections
        //
        if (IsFxVerifierFunctionTableHooking(pFxDriverGlobals))
		{
            UnlockVerifierSection(pFxDriverGlobals);
        }

        //
        // This will free the client's FxDriverGlobals area
        //
        FxFreeDriverGlobals(WdfDriverGlobals);
    }
    else
	{
        status = STATUS_UNSUCCESSFUL;
    }

    __Print((LITERAL(WDF_LIBRARY_UNREGISTER_CLIENT)
             ": exit: status %X\n", status));

    return status;
}

BOOLEAN
IsClientInfoValid(
    _In_ PCLIENT_INFO ClientInfo
    )
{
    if (ClientInfo == NULL ||
        ClientInfo->Size != sizeof(CLIENT_INFO) ||
        ClientInfo->RegistryPath == NULL ||
        ClientInfo->RegistryPath->Length == 0 ||
        ClientInfo->RegistryPath->Buffer == NULL)
	{
        return FALSE;
    }
    return TRUE;
}

VOID
GetEnhancedVerifierOptions(
    __in PCLIENT_INFO ClientInfo,
    __out PULONG Options
    )
{
    NTSTATUS status;
    ULONG value;
    FxAutoRegKey hKey, hWdf;
    DECLARE_CONST_UNICODE_STRING(parametersPath, L"Parameters\\Wdf");
    DECLARE_CONST_UNICODE_STRING(valueName, WDF_ENHANCED_VERIFIER_OPTIONS_VALUE_NAME);

    *Options = 0;
    if (!IsClientInfoValid(ClientInfo) ||
        Options == NULL)
	{

        __Print((LITERAL(WDF_LIBRARY_REGISTER_CLIENT)
                 ": Invalid ClientInfo received from wdfldr \n"));
        return;
    }

    status = FxRegKey::_OpenKey(NULL,
                                ClientInfo->RegistryPath,
                                &hWdf.m_Key,
                                KEY_READ);
    if (!NT_SUCCESS(status))
	{
        return;
    }

    status = FxRegKey::_OpenKey(hWdf.m_Key,
                                &parametersPath,
                                &hKey.m_Key,
                                KEY_READ);

    if (!NT_SUCCESS(status))
	{
        return;
    }

    status = FxRegKey::_QueryULong(hKey.m_Key, &valueName, &value);

    //
    // Examine key values and set Options only on success.
    //
    if (NT_SUCCESS(status))
	{
        if (value)
		{
            *Options = value;
        }
    }
}

VOID
LibraryLogEvent(
    __in PDRIVER_OBJECT DriverObject,
    __in NTSTATUS       ErrorCode,
    __in NTSTATUS       FinalStatus,
    __in PWSTR          ErrorInsertionString,
    __in_bcount(RawDataLen) PVOID    RawDataBuf,
    __in USHORT         RawDataLen
    )
/*++


Routine Description:

    Logs an error to the system event log.

    Arguments:

    DriverObject - Pointer to driver object reporting the error.

    ErrorCode    - Indicates the type of error, system or driver-defined.

    ErrorInsertionString - Null-terminated Unicode string inserted into error
    description, as defined by error code.

Return Value:

None.

--*/
{
    PIO_ERROR_LOG_PACKET errorLogEntry;
    size_t               errorLogEntrySize;                  // [including null]
    size_t               errorInsertionStringByteSize = 0;

    if (ErrorInsertionString)
	{
        errorInsertionStringByteSize = wcslen(ErrorInsertionString) * sizeof(WCHAR);
        errorInsertionStringByteSize += sizeof(UNICODE_NULL);
    }

    errorLogEntrySize = sizeof(IO_ERROR_LOG_PACKET) + RawDataLen + errorInsertionStringByteSize;

    //
    // Log an error.
    //
    //
    // prefast complains about comparison of constant with constant here
    //
#ifdef _MSC_VER
#pragma prefast(suppress:__WARNING_CONST_CONST_COMP, "If ErrorInsertionString is not null then this is not a constant")
#endif
    if (errorLogEntrySize <= ERROR_LOG_MAXIMUM_SIZE)
	{

        errorLogEntry = (PIO_ERROR_LOG_PACKET)IoAllocateErrorLogEntry(DriverObject,
            (UCHAR)errorLogEntrySize);

        if (errorLogEntry != NULL)
		{

            RtlZeroMemory(errorLogEntry, errorLogEntrySize);

            errorLogEntry->ErrorCode = ErrorCode;
            errorLogEntry->FinalStatus = FinalStatus;
            errorLogEntry->NumberOfStrings = (ErrorInsertionString) ? 1 : 0;
            errorLogEntry->DumpDataSize = RawDataLen;
            errorLogEntry->StringOffset = (FIELD_OFFSET(IO_ERROR_LOG_PACKET, DumpData)) + errorLogEntry->DumpDataSize;

            //
            // Insertion strings follow dumpdata and since there is no dumpdata we place the
            // insertion string at the start offset of the dumpdata.
            //
            if (RawDataBuf)
			{
                RtlCopyMemory(errorLogEntry->DumpData,
                    RawDataBuf,
                    RawDataLen);
            }

            if (ErrorInsertionString)
			{
                RtlCopyMemory(((PCHAR)errorLogEntry->DumpData) + RawDataLen,
                    ErrorInsertionString,
                    errorInsertionStringByteSize);
            }

            IoWriteErrorLogEntry(errorLogEntry);
        }
    }

    return;
}
