#include <stdarg.h>
#include "ntstatus.h"
#define WIN32_NO_STATUS
#define STANDALONE
#include "wine/test.h"
#include "winternl.h"
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "powrprof.h"
#include "assert.h"

#include "wine/unicode.h"
/*
   LONG WINAPI RegOpenCurrentUser(REGSAM a,PHKEY b)
   {
      *b = HKEY_CURRENT_USER;
      return ERROR_SUCCESS;
   }
 */
unsigned int g_NumPwrSchemes = 0;
unsigned int g_NumPwrSchemesEnumerated = 0;
unsigned int g_ActivePwrScheme = 3;
unsigned int g_TempPwrScheme = 99;

#if 0 // FIXME: needed to build. Please update pwrprof winetest.
typedef struct _PROCESSOR_POWER_INFORMATION {
   ULONG Number;
   ULONG MaxMhz;
   ULONG CurrentMhz;
   ULONG MhzLimit;
   ULONG MaxIdleState;
   ULONG CurrentIdleState;
} PROCESSOR_POWER_INFORMATION,
*PPROCESSOR_POWER_INFORMATION;
#endif

POWER_POLICY g_PowerPolicy;

static const WCHAR szMachPowerPoliciesSubKey[] = { 'S', 'O', 'F', 'T', 'W', 'A', 'R',
                                                   'E', '\\', 'M', 'i', 'c', 'r', 'o', 's', 'o', 'f', 't', '\\', 'W', 'i', 'n', 'd',
                                                   'o', 'w', 's', '\\', 'C', 'u', 'r', 'r', 'e', 'n', 't', 'V', 'e', 'r', 's', 'i',
                                                   'o', 'n', '\\', 'C', 'o', 'n', 't', 'r', 'o', 'l', 's', ' ', 'F', 'o', 'l', 'd',
                                                   'e', 'r', '\\', 'P', 'o', 'w', 'e', 'r', 'C', 'f', 'g', '\\', 'P', 'o', 'w', 'e',
                                                   'r', 'P', 'o', 'l', 'i', 'c', 'i', 'e', 's', 0};

static const WCHAR szTempPwrScheme[] = { '9', '9', 0 };

ULONG DbgPrint(PCCH X,...)
{
   return (ULONG)NULL;
}

void test_CallNtPowerInformation(void)
{
   DWORD retval;
   ADMINISTRATOR_POWER_POLICY apolicy;
   ULONGLONG atime, ctime;
   PROCESSOR_POWER_INFORMATION ppi, *pppi;
   PROCESSOR_POWER_POLICY ppp;
   SYSTEM_BATTERY_STATE sbs;
   SYSTEM_POWER_CAPABILITIES spc;
   SYSTEM_POWER_INFORMATION spi;
   SYSTEM_POWER_POLICY spp;
   HANDLE x=NULL;

   /* AdministratorPowerPolicy tests */
   retval = CallNtPowerInformation(AdministratorPowerPolicy, 0, 0, 0, 0);
   ok(retval == STATUS_SUCCESS, "function result wrong expected STATUS_SUCCESS but got %d\n", (UINT)retval);
   retval = CallNtPowerInformation(AdministratorPowerPolicy, 0, 0, &apolicy, sizeof(ADMINISTRATOR_POWER_POLICY));
   ok(retval == STATUS_SUCCESS, "function expected STATUS_SUCCESS but got %d\n", (UINT)retval);
   retval = CallNtPowerInformation(AdministratorPowerPolicy, &apolicy, sizeof(ADMINISTRATOR_POWER_POLICY), 0, 0);
   ok(retval != STATUS_PRIVILEGE_NOT_HELD, "Privileg not held!!!! more errors to expect");
   ok(retval == STATUS_SUCCESS, "function expected STATUS_SUCCESS but got %d\n", (UINT)retval);

   /* LastSleepTime tests */
   retval = CallNtPowerInformation(LastSleepTime, 0, 0, 0, 0);
   ok(retval == STATUS_SUCCESS, "function result wrong expected STATUS_SUCCESS but got %d\n", (UINT)retval);
   retval = CallNtPowerInformation(LastSleepTime, &atime, sizeof(sizeof(ULONGLONG)), 0, 0);
   ok(retval == STATUS_INVALID_PARAMETER, "function result wrong expected STATUS_INVALID_PARAMETER but got %d\n", (UINT)retval);
   retval = CallNtPowerInformation(LastSleepTime, &atime, sizeof(ULONGLONG), &ctime, sizeof(ULONGLONG));
   ok(retval == STATUS_INVALID_PARAMETER, "function result wrong expected STATUS_INVALID_PARAMETER but got %d\n", (UINT)retval);
   retval = CallNtPowerInformation(LastSleepTime, 0, 0, &atime, sizeof(ULONGLONG));
   ok(retval == STATUS_SUCCESS, "function expected STATUS_SUCCESS but got %d\n",(UINT)retval);

   /* LastWakeTime tests */
   retval = CallNtPowerInformation(LastWakeTime, 0, 0, 0, 0);
   ok(retval == STATUS_SUCCESS, "function result wrong expected STATUS_SUCCESS but got %d\n", (UINT)retval);
   retval = CallNtPowerInformation(LastWakeTime, &atime, sizeof(sizeof(ULONGLONG)), 0, 0);
   ok(retval == STATUS_INVALID_PARAMETER, "function result wrong expected STATUS_INVALID_PARAMETER but got %d\n", (UINT)retval);
   retval = CallNtPowerInformation(LastWakeTime, &atime, sizeof(ULONGLONG), &ctime, sizeof(ULONGLONG));
   ok(retval == STATUS_INVALID_PARAMETER, "function result wrong expected STATUS_INVALID_PARAMETER but got %d\n", (UINT)retval);
   retval = CallNtPowerInformation(LastWakeTime, 0, 0, &atime, sizeof(ULONGLONG));
   ok(retval == STATUS_SUCCESS, "function expected STATUS_SUCCESS but got %d\n",(UINT)retval);

   /* ProcessorInformation tests */
   retval = CallNtPowerInformation(ProcessorInformation, 0, 0, 0, 0);
   ok(retval == STATUS_SUCCESS, "function result wrong expected STATUS_SUCCESS but got %d\n", (UINT)retval);
   retval = CallNtPowerInformation(ProcessorInformation, 0, 0, &ppi, sizeof(PROCESSOR_POWER_INFORMATION));
   ok(retval == STATUS_SUCCESS, "function result wrong expected STATUS_SUCCESS but got %d\n", (UINT)retval);
   retval = CallNtPowerInformation(ProcessorInformation, &ppi, sizeof(PROCESSOR_POWER_INFORMATION), 0, 0);
   ok(retval == STATUS_SUCCESS, "function result wrong expected STATUS_SUCCESS but got %d\n", (UINT)retval);
   retval = CallNtPowerInformation(ProcessorInformation, &ppi, sizeof(PROCESSOR_POWER_INFORMATION), &ppi, sizeof(PROCESSOR_POWER_INFORMATION));

   ok(retval == STATUS_SUCCESS, "function result wrong expected STATUS_SUCCESS but got %d\n", (UINT)retval);
   retval = CallNtPowerInformation(ProcessorInformation, 0, 0, &pppi, sizeof(PPROCESSOR_POWER_INFORMATION));
   ok(retval == STATUS_SUCCESS, "function result wrong expected STATUS_SUCCESS but got %d\n", (UINT)retval);
   retval = CallNtPowerInformation(ProcessorInformation, &pppi, sizeof(PPROCESSOR_POWER_INFORMATION), 0, 0);
   ok(retval == STATUS_SUCCESS, "function result wrong expected STATUS_SUCCESS but got %d\n", (UINT)retval);
   retval = CallNtPowerInformation(ProcessorInformation, &pppi, sizeof(PPROCESSOR_POWER_INFORMATION), &pppi, sizeof(PPROCESSOR_POWER_INFORMATION));
   ok(retval == STATUS_SUCCESS, "function result wrong expected STATUS_SUCCESS but got %d\n", (UINT)retval);

   /* ProcessorPowerPolicyAc tests */
   retval = CallNtPowerInformation(ProcessorPowerPolicyAc, 0, 0, 0, 0);
   ok(retval == STATUS_SUCCESS, "function result wrong expected STATUS_SUCCESS but got %d\n", (UINT)retval);
   retval = CallNtPowerInformation(ProcessorPowerPolicyAc, 0, 0, &ppp, sizeof(PROCESSOR_POWER_POLICY));
   ok(retval == STATUS_SUCCESS, "function result wrong expected STATUS_SUCCESS but got %d\n", (UINT)retval);
   retval = CallNtPowerInformation(ProcessorPowerPolicyAc, &ppp, sizeof(PROCESSOR_POWER_POLICY), 0, 0);
   ok(retval == STATUS_SUCCESS, "function result wrong expected STATUS_SUCCESS but got %d\n", (UINT)retval);
   retval = CallNtPowerInformation(ProcessorPowerPolicyAc, &ppp, sizeof(PROCESSOR_POWER_POLICY), &ppp, sizeof(PROCESSOR_POWER_POLICY));
   ok(retval == STATUS_SUCCESS, "function result wrong expected STATUS_SUCCESS but got %d\n", (UINT)retval);

   /* ProcessorPowerPolicyCurrent tests */
   retval = CallNtPowerInformation(ProcessorPowerPolicyCurrent, 0, 0, 0, 0);
   ok(retval == STATUS_SUCCESS, "function result wrong expected STATUS_SUCCESS but got %d\n", (UINT)retval);
   retval = CallNtPowerInformation(ProcessorPowerPolicyCurrent, 0, 0, &ppp, sizeof(PROCESSOR_POWER_POLICY));
   ok(retval == STATUS_SUCCESS, "function result wrong expected STATUS_SUCCESS but got %d\n", (UINT)retval);
   retval = CallNtPowerInformation(ProcessorPowerPolicyCurrent, &ppp, sizeof(PROCESSOR_POWER_POLICY), 0, 0);
   ok(retval == STATUS_INVALID_PARAMETER, "function result wrong expected STATUS_INVALID_PARAMETER but got %d\n", (UINT)retval);
   retval = CallNtPowerInformation(ProcessorPowerPolicyCurrent, &ppp, sizeof(PROCESSOR_POWER_POLICY), &ppp, sizeof(PROCESSOR_POWER_POLICY));
   ok(retval == STATUS_INVALID_PARAMETER, "function result wrong expected STATUS_INVALID_PARAMETER but got %d\n", (UINT)retval);

   /* ProcessorPowerPolicyDc tests */
   retval = CallNtPowerInformation(ProcessorPowerPolicyDc, 0, 0, 0, 0);
   ok(retval == STATUS_SUCCESS, "function result wrong expected STATUS_SUCCESS but got %d\n", (UINT)retval);
   retval = CallNtPowerInformation(ProcessorPowerPolicyDc, 0, 0, &ppp, sizeof(PROCESSOR_POWER_POLICY));
   ok(retval == STATUS_SUCCESS, "function result wrong expected STATUS_SUCCESS but got %d\n", (UINT)retval);
   retval = CallNtPowerInformation(ProcessorPowerPolicyDc, &ppp, sizeof(PROCESSOR_POWER_POLICY), 0, 0);
   ok(retval == STATUS_SUCCESS, "function result wrong expected STATUS_SUCCESS but got %d\n", (UINT)retval);
   retval = CallNtPowerInformation(ProcessorPowerPolicyDc, &ppp, sizeof(PROCESSOR_POWER_POLICY), &ppp, sizeof(PROCESSOR_POWER_POLICY));
   ok(retval == STATUS_SUCCESS, "function result wrong expected STATUS_SUCCESS but got %d\n", (UINT)retval);

   /* SystemBatteryState tests */
   retval = CallNtPowerInformation(SystemBatteryState, 0, 0, 0, 0);
   ok(retval == STATUS_SUCCESS, "function result wrong expected STATUS_SUCCESS but got %d\n", (UINT)retval);
   retval = CallNtPowerInformation(SystemBatteryState, 0, 0, &sbs, sizeof(SYSTEM_BATTERY_STATE));
   ok(retval == STATUS_SUCCESS, "function result wrong expected STATUS_SUCCESS but got %d\n", (UINT)retval);
   retval = CallNtPowerInformation(SystemBatteryState, &sbs, sizeof(SYSTEM_BATTERY_STATE), 0, 0);
   ok(retval == STATUS_SUCCESS, "function result wrong expected STATUS_SUCCESS but got %d\n", (UINT)retval);
   retval = CallNtPowerInformation(SystemBatteryState, &sbs, sizeof(SYSTEM_BATTERY_STATE), &sbs, sizeof(SYSTEM_BATTERY_STATE));
   ok(retval == STATUS_SUCCESS, "function result wrong expected STATUS_SUCCESS but got %d\n", (UINT)retval);

   /* SystemExecutionState tests */
   retval = CallNtPowerInformation(SystemExecutionState, 0, 0, 0, 0);
   ok(retval == STATUS_SUCCESS, "function result wrong expected STATUS_SUCCESS but got %d\n", (UINT)retval);

   /* SystemPowerCapabilities tests */
   retval = CallNtPowerInformation(SystemPowerCapabilities, 0, 0, 0, 0);
   ok(retval == STATUS_SUCCESS, "function result wrong expected STATUS_SUCCESS but got %d\n", (UINT)retval);
   retval = CallNtPowerInformation(SystemPowerCapabilities, 0, 0, &spc, sizeof(SYSTEM_POWER_CAPABILITIES));
   ok(retval == STATUS_SUCCESS, "function result wrong expected STATUS_SUCCESS but got %d\n", (UINT)retval);
   retval = CallNtPowerInformation(SystemPowerCapabilities, &spc, sizeof(SYSTEM_POWER_CAPABILITIES), 0, 0);
   ok(retval == STATUS_INVALID_PARAMETER, "function result wrong expected STATUS_INVALID_PARAMETER but got %d\n", (UINT)retval);
   retval = CallNtPowerInformation(SystemPowerCapabilities, &spc, sizeof(SYSTEM_POWER_CAPABILITIES), &spc, sizeof(SYSTEM_POWER_CAPABILITIES));
   ok(retval == STATUS_INVALID_PARAMETER, "function result wrong expected STATUS_INVALID_PARAMETER but got %d\n", (UINT)retval);

   /* SystemPowerInformation tests */
   retval = CallNtPowerInformation(SystemPowerInformation, 0, 0, 0, 0);
   ok(retval == STATUS_SUCCESS, "function result wrong expected STATUS_SUCCESS but got %d\n", (UINT)retval);
   retval = CallNtPowerInformation(SystemPowerInformation, 0, 0, &spi, sizeof(SYSTEM_POWER_INFORMATION));
   ok(retval == STATUS_SUCCESS, "function result wrong expected STATUS_SUCCESS but got %d\n", (UINT)retval);
   retval = CallNtPowerInformation(SystemPowerInformation, &spi, sizeof(SYSTEM_POWER_INFORMATION), 0, 0);
   ok(retval == STATUS_SUCCESS, "function result wrong expected STATUS_SUCCESS but got %d\n", (UINT)retval);
   retval = CallNtPowerInformation(SystemPowerInformation, &spi, sizeof(SYSTEM_POWER_INFORMATION), &spi, sizeof(SYSTEM_POWER_INFORMATION));
   ok(retval == STATUS_SUCCESS, "function result wrong expected STATUS_SUCCESS but got %d\n", (UINT)retval);
   retval = CallNtPowerInformation(SystemPowerInformation, &spp, sizeof(SYSTEM_POWER_POLICY), 0, 0);
   ok(retval == STATUS_SUCCESS, "function result wrong expected STATUS_SUCCESS but got %d\n", (UINT)retval);
   retval = CallNtPowerInformation(SystemPowerInformation, &spp, sizeof(SYSTEM_POWER_POLICY), &spi, sizeof(SYSTEM_POWER_INFORMATION));
   ok(retval == STATUS_SUCCESS, "function result wrong expected STATUS_SUCCESS but got %d\n", (UINT)retval);

   /* SystemPowerPolicyAc tests */
   retval = CallNtPowerInformation(SystemPowerPolicyAc, 0, 0, 0, 0);
   ok(retval == STATUS_SUCCESS, "function result wrong expected STATUS_SUCCESS but got %d\n", (UINT)retval);
   retval = CallNtPowerInformation(SystemPowerPolicyAc, 0, 0, &spp, sizeof(SYSTEM_POWER_POLICY));
   ok(retval == STATUS_SUCCESS, "function result wrong expected STATUS_SUCCESS but got %d\n", (UINT)retval);
   retval = CallNtPowerInformation(SystemPowerPolicyAc, &spp, sizeof(SYSTEM_POWER_POLICY), 0, 0);
   ok(retval == STATUS_SUCCESS, "function result wrong expected STATUS_SUCCESS but got %d\n", (UINT)retval);
   retval = CallNtPowerInformation(SystemPowerPolicyAc, &spp, sizeof(SYSTEM_POWER_POLICY), &spp, sizeof(SYSTEM_POWER_POLICY));
   ok(retval == STATUS_SUCCESS, "function result wrong expected STATUS_SUCCESS but got %d\n", (UINT)retval);

   /* SystemPowerPolicyCurrent tests */
   retval = CallNtPowerInformation(SystemPowerPolicyCurrent, 0, 0, 0, 0);
   ok(retval == STATUS_SUCCESS, "function result wrong expected STATUS_SUCCESS but got %d\n", (UINT)retval);
   retval = CallNtPowerInformation(SystemPowerPolicyCurrent, 0, 0, &spp, sizeof(SYSTEM_POWER_POLICY));
   ok(retval == STATUS_SUCCESS, "function result wrong expected STATUS_SUCCESS but got %d\n", (UINT)retval);
   retval = CallNtPowerInformation(SystemPowerPolicyCurrent, &spp, sizeof(SYSTEM_POWER_POLICY), 0, 0);
   ok(retval == STATUS_INVALID_PARAMETER, "function result wrong expected STATUS_INVALID_PARAMETER but got %d\n", (UINT)retval);
   retval = CallNtPowerInformation(SystemPowerPolicyCurrent, &spp, sizeof(SYSTEM_POWER_POLICY), &spp, sizeof(SYSTEM_POWER_POLICY));
   ok(retval == STATUS_INVALID_PARAMETER, "function result wrong expected STATUS_INVALID_PARAMETER but got %d\n", (UINT)retval);

   /* SystemPowerPolicyDc tests */
   retval = CallNtPowerInformation(SystemPowerPolicyDc, 0, 0, 0, 0);
   ok(retval == STATUS_SUCCESS, "function result wrong expected STATUS_SUCCESS but got %d\n", (UINT)retval);
   retval = CallNtPowerInformation(SystemPowerPolicyDc, 0, 0, &spp, sizeof(SYSTEM_POWER_POLICY));
   ok(retval == STATUS_SUCCESS, "function result wrong expected STATUS_SUCCESS but got %d\n", (UINT)retval);
   retval = CallNtPowerInformation(SystemPowerPolicyDc, &spp, sizeof(SYSTEM_POWER_POLICY), 0, 0);
   ok(retval == STATUS_SUCCESS, "function result wrong expected STATUS_SUCCESS but got %d\n", (UINT)retval);
   retval = CallNtPowerInformation(SystemPowerPolicyDc, &spp, sizeof(SYSTEM_POWER_POLICY), &spp, sizeof(SYSTEM_POWER_POLICY));
   ok(retval == STATUS_SUCCESS, "function result wrong expected STATUS_SUCCESS but got %d\n", (UINT)retval);

   /* SystemReserveHiberFile tests */
/*
   retval = CallNtPowerInformation(SystemReserveHiberFile, 0, 0, 0, 0);
   ok(retval == STATUS_INVALID_PARAMETER, "function result wrong expected STATUS_INVALID_PARAMETER but got %ld\n", retval);
   bln=TRUE;
   retval = CallNtPowerInformation(SystemReserveHiberFile, &bln, sizeof(bln), 0, 0);
   ok(retval == STATUS_DISK_FULL, "function result wrong expected STATUS_DISK_FULL but got %ld\n", nret);
   bln=FALSE;
   retval = CallNtPowerInformation(SystemReserveHiberFile, &bln, sizeof(bln), 0, 0);
   ok(retval == STATUS_SUCCESS, "function result wrong expected STATUS_SUCCESS but got %ld\n", nret);

   bln2=TRUE;
   nret = CallNtPowerInformation(SystemReserveHiberFile, 0, 0, &bln2, sizeof(bln2));
   ok(nret == STATUS_DATATYPE_MISALIGNMENT, "function result wrong expected STATUS_DATATYPE_MISALIGNMENT but got %ld\n", nret);
   bln2=FALSE;
   nret = CallNtPowerInformation(SystemReserveHiberFile, 0, 0, &bln2, sizeof(bln2));
   ok(nret == STATUS_DATATYPE_MISALIGNMENT, "function result wrong expected STATUS_DATATYPE_MISALIGNMENT but got %ld\n", nret);

   bln=TRUE;
   bln2=TRUE;
   nret = CallNtPowerInformation(SystemReserveHiberFile, &bln, sizeof(bln), &bln2, sizeof(bln2));
   ok(nret == STATUS_DATATYPE_MISALIGNMENT, "function result wrong expected STATUS_DATATYPE_MISALIGNMENT but got %ld\n", nret);
   bln2=FALSE;
   nret = CallNtPowerInformation(SystemReserveHiberFile, &bln, sizeof(bln), &bln2, sizeof(bln2));
   ok(nret == STATUS_DATATYPE_MISALIGNMENT, "function result wrong expected STATUS_DATATYPE_MISALIGNMENT but got %ld\n", nret);
   bln=FALSE;
   bln2=TRUE;
   nret = CallNtPowerInformation(SystemReserveHiberFile, &bln, sizeof(bln), &bln2, sizeof(bln2));
   ok(nret == STATUS_DATATYPE_MISALIGNMENT, "function result wrong expected STATUS_DATATYPE_MISALIGNMENT but got %ld\n", nret);
   bln2=FALSE;
   nret = CallNtPowerInformation(SystemReserveHiberFile, &bln, sizeof(bln), &bln2, sizeof(bln2));
   ok(nret == STATUS_DATATYPE_MISALIGNMENT, "function result wrong expected STATUS_DATATYPE_MISALIGNMENT but got %ld\n", nret);
 */

   /* VerifyProcessorPowerPolicyAc tests */
   retval = CallNtPowerInformation(VerifyProcessorPowerPolicyAc, 0, 0, 0, 0);
   ok(retval == STATUS_INVALID_PARAMETER, "function result wrong expected STATUS_INVALID_PARAMETER but got %d\n", (UINT)retval);
   retval = CallNtPowerInformation(VerifyProcessorPowerPolicyAc, 0, 0, &ppp, sizeof(PROCESSOR_POWER_POLICY));
   ok(retval == STATUS_INVALID_PARAMETER, "function result wrong expected STATUS_INVALID_PARAMETER but got %d\n", (UINT)retval);
   retval = CallNtPowerInformation(VerifyProcessorPowerPolicyAc, &ppp, sizeof(PROCESSOR_POWER_POLICY), 0, 0);
   ok(retval == STATUS_INVALID_PARAMETER, "function result wrong expected STATUS_INVALID_PARAMETER but got %d\n", (UINT)retval);
   retval = CallNtPowerInformation(VerifyProcessorPowerPolicyAc, &ppp, sizeof(PROCESSOR_POWER_POLICY), &ppp, sizeof(PROCESSOR_POWER_POLICY));
   ok(retval == STATUS_SUCCESS, "function result wrong expected STATUS_SUCCESS but got %d\n", (UINT)retval);

   /* VerifyProcessorPowerPolicyDc tests */
   retval = CallNtPowerInformation(VerifyProcessorPowerPolicyDc, 0, 0, 0, 0);
   ok(retval == STATUS_INVALID_PARAMETER, "function result wrong expected STATUS_INVALID_PARAMETER but got %d\n", (UINT)retval);
   retval = CallNtPowerInformation(VerifyProcessorPowerPolicyDc, 0, 0, &ppp, sizeof(PROCESSOR_POWER_POLICY));
   ok(retval == STATUS_INVALID_PARAMETER, "function result wrong expected STATUS_INVALID_PARAMETER but got %d\n", (UINT)retval);
   retval = CallNtPowerInformation(VerifyProcessorPowerPolicyDc, &ppp, sizeof(PROCESSOR_POWER_POLICY), 0, 0);
   ok(retval == STATUS_INVALID_PARAMETER, "function result wrong expected STATUS_INVALID_PARAMETER but got %d\n", (UINT)retval);
   retval = CallNtPowerInformation(VerifyProcessorPowerPolicyDc, &ppp, sizeof(PROCESSOR_POWER_POLICY), &ppp, sizeof(PROCESSOR_POWER_POLICY));
   ok(retval == STATUS_SUCCESS, "function result wrong expected STATUS_SUCCESS but got %d\n", (UINT)retval);

   /* VerifySystemPolicyAc tests */
   retval = CallNtPowerInformation(VerifySystemPolicyAc, 0, 0, 0, 0);
   ok(retval == STATUS_INVALID_PARAMETER, "function result wrong expected STATUS_INVALID_PARAMETER but got %d\n", (UINT)retval);
   retval = CallNtPowerInformation(VerifySystemPolicyAc, 0, 0, &spp, sizeof(SYSTEM_POWER_POLICY));
   ok(retval == STATUS_INVALID_PARAMETER, "function result wrong expected STATUS_INVALID_PARAMETER but got %d\n", (UINT)retval);
   retval = CallNtPowerInformation(VerifySystemPolicyAc, &spp, sizeof(SYSTEM_POWER_POLICY), 0, 0);
   ok(retval == STATUS_INVALID_PARAMETER, "function result wrong expected STATUS_INVALID_PARAMETER but got %d\n", (UINT)retval);
   retval = CallNtPowerInformation(VerifySystemPolicyAc, &spp, sizeof(SYSTEM_POWER_POLICY), &spp, sizeof(SYSTEM_POWER_POLICY));
   ok(retval == STATUS_SUCCESS, "function result wrong expected STATUS_SUCCESS but got %d\n", (UINT)retval);

   /* VerifySystemPolicyDc tests */
   retval = CallNtPowerInformation(VerifySystemPolicyDc, 0, 0, 0, 0);
   ok(retval == STATUS_INVALID_PARAMETER, "function result wrong expected STATUS_INVALID_PARAMETER but got %d\n", (UINT)retval);
   retval = CallNtPowerInformation(VerifySystemPolicyDc, 0, 0, &spp, sizeof(SYSTEM_POWER_POLICY));
   ok(retval == STATUS_INVALID_PARAMETER, "function result wrong expected STATUS_INVALID_PARAMETER but got %d\n", (UINT)retval);
   retval = CallNtPowerInformation(VerifySystemPolicyDc, &spp, sizeof(SYSTEM_POWER_POLICY), 0, 0);
   ok(retval == STATUS_INVALID_PARAMETER, "function result wrong expected STATUS_INVALID_PARAMETER but got %d\n", (UINT)retval);
   retval = CallNtPowerInformation(VerifySystemPolicyDc, &spp, sizeof(SYSTEM_POWER_POLICY), &spp, sizeof(SYSTEM_POWER_POLICY));
   ok(retval == STATUS_SUCCESS, "function result wrong expected STATUS_SUCCESS but got %d\n", (UINT)retval);

   /* SystemPowerStateHandler tests */
   retval = CallNtPowerInformation(SystemPowerStateHandler, 0, 0, 0, 0);
   ok(retval == STATUS_INVALID_PARAMETER, "function result wrong expected STATUS_INVALID_PARAMETER but got %d\n", (UINT)retval);
   retval = CallNtPowerInformation(SystemPowerStateHandler, 0, 0, x, sizeof(HANDLE));
   ok(retval == STATUS_INVALID_PARAMETER, "function result wrong expected STATUS_INVALID_PARAMETER but got %d\n", (UINT)retval);

   /* ProcessorStateHandler tests */
   retval = CallNtPowerInformation(ProcessorStateHandler, 0, 0, 0, 0);
   ok(retval == STATUS_INVALID_PARAMETER, "function result wrong expected STATUS_INVALID_PARAMETER but got %d\n", (UINT)retval);
   retval = CallNtPowerInformation(ProcessorStateHandler, 0, 0, x, sizeof(HANDLE));
   ok(retval == STATUS_INVALID_PARAMETER, "function result wrong expected STATUS_INVALID_PARAMETER but got %d\n", (UINT)retval);

   /* ProcessorStateHandler2 tests */
   retval = CallNtPowerInformation(ProcessorStateHandler2, 0, 0, 0, 0);
   ok(retval == STATUS_ACCESS_DENIED, "function result wrong expected STATUS_ACCESS_DENIED but got %d\n", (UINT)retval);
   retval = CallNtPowerInformation(ProcessorStateHandler2, 0, 0, x, sizeof(HANDLE));
   ok(retval == STATUS_ACCESS_DENIED, "function result wrong expected STATUS_ACCESS_DENIED but got %d\n", (UINT)retval);

   /* SystemPowerStateNotifyHandler tests */
   retval = CallNtPowerInformation(SystemPowerStateNotifyHandler, 0, 0, 0, 0);
   ok(retval == STATUS_INVALID_PARAMETER, "function result wrong expected STATUS_INVALID_PARAMETER but got %d\n", (UINT)retval);
   retval = CallNtPowerInformation(SystemPowerStateNotifyHandler, 0, 0, x, sizeof(HANDLE));
   ok(retval == STATUS_INVALID_PARAMETER, "function result wrong expected STATUS_INVALID_PARAMETER but got %d\n", (UINT)retval);

}

/*
   @implemented
 */
void test_CanUserWritePwrScheme(void)
{
   DWORD error, retval;

   retval = CanUserWritePwrScheme();

   error = GetLastError();

   if (retval)
      ok(retval, "function failed?");
   else
      ok(error == ERROR_ACCESS_DENIED, "function last error wrong expected ERROR_ACCESS_DENIED but got %d\n", (UINT)error);

}
BOOLEAN CALLBACK test_callback_EnumPwrScheme(UINT uiIndex, DWORD dwName, LPWSTR sName, DWORD dwDesc,
                                             LPWSTR sDesc, PPOWER_POLICY pp,LPARAM lParam )
{
   ok(uiIndex == g_NumPwrSchemes, "expected power scheme index of %d but got %d\n", g_NumPwrSchemes, uiIndex);
   g_NumPwrSchemes++;

   ok(lParam == 0xDEADBEEF, "expected function lParam to be 0xDEADBEEF but got %d\n", (UINT)lParam);

   return TRUE;
}

BOOLEAN CALLBACK test_callback_stop_EnumPwrScheme(UINT uiIndex, DWORD dwName, LPWSTR sName, DWORD dwDesc,
                                                  LPWSTR sDesc, PPOWER_POLICY pp,LPARAM lParam )
{
   ok((!uiIndex || g_NumPwrSchemesEnumerated + 1 == uiIndex), "expected power scheme %d but got %d\n",g_NumPwrSchemesEnumerated+1, uiIndex);
   g_NumPwrSchemesEnumerated = uiIndex;

   ok(uiIndex <= (UINT)lParam, "enumeration should have already been stopped at index %d current index %d\n", (UINT)lParam, uiIndex);
   if (uiIndex == (UINT)lParam)
      return FALSE;
   else
      return TRUE;
}

void test_DeletePwrScheme(void)
{
   DWORD retval;
   HKEY hSubKey = NULL;


   /*
    *  try inexistant profile number, should fail
    */

   retval = DeletePwrScheme(0xFFFFFFFF);
   ok(!retval, "function should have failed error %x\n",(UINT)GetLastError());

   /*
    *  delete active power scheme, should fail
    */

   retval = GetActivePwrScheme(&g_ActivePwrScheme);
   ok(retval, "function was expected to succeed, error %x\n",(UINT)GetLastError());

   retval = DeletePwrScheme(g_ActivePwrScheme);
   ok(!retval, "function should have failed\n");
   ok(GetLastError() == ERROR_ACCESS_DENIED, "function should have failed with ERROR_ACCESS_DENIED but got %x\n", (UINT)GetLastError());

   /*
    * delete a temporarly created power scheme
    */
   retval = DeletePwrScheme(g_TempPwrScheme);
   ok(retval, "function should have succeeded\n");

/*
 *	clean up, delete illegal entry, witch was created for this test
 */

   if (RegOpenKeyW(HKEY_LOCAL_MACHINE, szMachPowerPoliciesSubKey, &hSubKey) == ERROR_SUCCESS)
   {
      if (RegDeleteKeyW(hSubKey, szTempPwrScheme) != STATUS_SUCCESS)
         printf("failed to delete subkey %i (testentry)\n", g_TempPwrScheme);
      RegCloseKey(hSubKey);
   }

}

void test_EnumPwrSchemes(void)
{
   BOOLEAN retval;

   /*
    * test EnumPwrScheme with null pointer callback
    */

   retval = EnumPwrSchemes(0, 0);
   ok(!retval, "function was expected to fail\n");
   ok(GetLastError() == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER but got %x\n",(UINT)GetLastError());

   /*
    * enumerate power schemes, should succeed
    */

   retval = EnumPwrSchemes(test_callback_EnumPwrScheme, 0xDEADBEEF);
   ok(retval, "function was expected to succeed %d\n",retval);
   ok(g_NumPwrSchemes, "Warning: no power schemes available\n");

   /*
    * stop enumeration after first power scheme
    */

   retval = EnumPwrSchemes(test_callback_stop_EnumPwrScheme, (LPARAM)0);
   ok(!retval, "function was expected to false\n");

   /*
    *  enumerate half of all avalailble profiles
    */

   g_NumPwrSchemesEnumerated = 0;
   retval = EnumPwrSchemes(test_callback_stop_EnumPwrScheme, (LPARAM)g_NumPwrSchemes / 2);
   ok(retval, "function was expected to succeed but got %i\n", (UINT)retval);
   ok(g_NumPwrSchemesEnumerated == g_NumPwrSchemes / 2, "function did not enumerate requested num of profiles %d enumerated %d\n", g_NumPwrSchemes / 2, g_NumPwrSchemesEnumerated);


}

void test_GetSetActivePwrScheme(void)
{
   DWORD retval;
   UINT current_scheme = 2;
   UINT temp_scheme = 0;

   /*
    * read active power scheme
    */

   retval = GetActivePwrScheme(&g_ActivePwrScheme);

   ok(retval, "function was expected to succeed, error %x\n",(UINT)GetLastError());
   ok(retval <= g_NumPwrSchemes, "expected index lower as power scheme count %d but got %d\n", g_NumPwrSchemes, g_ActivePwrScheme);

   /*
    *  sets active power scheme to inexistant profile
    * -> corrupts power scheme enumeration on Windows XP SP2
    */
   //corrupts registry
   //retval = SetActivePwrScheme(0xFFFFFFFF, 0, 0);
   //ok(!retval, "function was expected to fail");
   //current_scheme = min(active_scheme+1, g_NumPwrSchemes-1);

   /*
    * sets the active power scheme to profile with index 0
    */

   retval = SetActivePwrScheme(current_scheme, 0, 0);
   ok(retval, "function was expected to succeed, error %x\n",(UINT)GetLastError());

   /*
    *  read back the active power scheme
    */

   retval = GetActivePwrScheme(&temp_scheme);
   ok(retval, "function was expected to succeed, error %x\n",(UINT)GetLastError());
   ok(temp_scheme == current_scheme, "expected %d but got %d\n", (UINT)current_scheme, (UINT)temp_scheme);

   /*
    * restore previous active power scheme
    */

   retval = SetActivePwrScheme(g_ActivePwrScheme, 0, 0);
   ok(retval, "Warning: failed to restore old active power scheme %d\n", (UINT)g_ActivePwrScheme);
}

void test_GetCurrentPowerPolicies(void)
{
   GLOBAL_POWER_POLICY gpp;
   POWER_POLICY pp;
   BOOLEAN ret;
   UINT current_scheme = 2;

   g_ActivePwrScheme=3;
   ret = GetActivePwrScheme(&g_ActivePwrScheme);

   ok(ret, "function was expected to succeed, error %x\n",(UINT)GetLastError());
   ret = SetActivePwrScheme(0, &gpp, 0);

   ok(!ret, "function was expected to fail, error %x\n",(UINT)GetLastError());

   ret = SetActivePwrScheme(0, 0, &pp);
   ok(!ret, "function was expected to fail, error %x\n",(UINT)GetLastError());

   ret = SetActivePwrScheme(0, &gpp, &pp);
   ok(!ret, "function was expected to fail, error %x\n",(UINT)GetLastError());

   ret = SetActivePwrScheme(current_scheme, &gpp, 0);
   ok(!ret, "function was expected to fail, error %x\n",(UINT)GetLastError());

   ret = SetActivePwrScheme(current_scheme, 0, &pp);
   ok(!ret, "function was expected to fail, error %x\n",(UINT)GetLastError());

   ret = SetActivePwrScheme(current_scheme, &gpp, &pp);
   ok(!ret, "function was expected to fail, error %x\n",(UINT)GetLastError());

   ret = SetActivePwrScheme(g_ActivePwrScheme, 0, 0);
   ok(ret, "Warning: failed to restore old active power scheme %d\n", (UINT)g_ActivePwrScheme);

   ret = GetCurrentPowerPolicies(0,0);
   ok(ret, "function was expected to succeed, error %x\n",(UINT)GetLastError());

   ret = GetCurrentPowerPolicies(&gpp,0);
   ok(ret, "function was expected to succeed, error %x\n",(UINT)GetLastError());

   ret = GetCurrentPowerPolicies(0,&pp);
   ok(ret, "function was expected to succeed, error %x\n",(UINT)GetLastError());

   ret = GetCurrentPowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed, error %x\n",(UINT)GetLastError());
   ok(gpp.mach.Revision  == 1,"Global Mach Revision was expected to be 1 got %i",(UINT)gpp.mach.Revision);
   ok(gpp.user.Revision  == 1,"Global User Revision was expected to be 1 got %i",(UINT)gpp.mach.Revision);
   ok(pp.mach.Revision  == 1,"Mach Revision was expected to be 1 got %i",(UINT)gpp.mach.Revision);
   ok(pp.user.Revision  == 1,"User Revision was expected to be 1 got %i",(UINT)gpp.mach.Revision);


   ret = GetActivePwrScheme(&g_ActivePwrScheme);
   ok(ret, "function was expected to succeed, error %x\n",(UINT)GetLastError());

   ret = SetActivePwrScheme(0, &gpp, 0);
   ok(ret, "function was expected to succeed, error %x\n",(UINT)GetLastError());

   ret = SetActivePwrScheme(0, 0, &pp);
   ok(ret, "function was expected to succeed, error %x\n",(UINT)GetLastError());

   ret = SetActivePwrScheme(0, &gpp, &pp);
   ok(ret, "function was expected to succeed, error %x\n",(UINT)GetLastError());

   ret = SetActivePwrScheme(current_scheme, &gpp, 0);
   ok(ret, "function was expected to succeed, error %x\n",(UINT)GetLastError());

   ret = SetActivePwrScheme(current_scheme, 0, &pp);
   ok(ret, "function was expected to succeed, error %x\n",(UINT)GetLastError());

   ret = SetActivePwrScheme(current_scheme, &gpp, &pp);
   ok(ret, "function was expected to succeed, error %x\n",(UINT)GetLastError());

   ret = SetActivePwrScheme(g_ActivePwrScheme, 0, 0);
   ok(ret, "Warning: failed to restore old active power scheme %d\n", (UINT)g_ActivePwrScheme);

}

void test_GetPwrCapabilities(void)
{
   SYSTEM_POWER_CAPABILITIES spc;
   BOOLEAN ret;

   ret = GetPwrCapabilities(0);
   ok(!ret, "function was expected to fail\n");
   if (!ret)
   {
      ok(GetLastError() == ERROR_INVALID_PARAMETER,"function was expectet to return ERROR_INVALID_PARAMETER, but returns: %x\n",(UINT)GetLastError());
   }
   ret = GetPwrCapabilities(&spc);
   ok(ret, "function was expected to succeed, error %x\n",(UINT)GetLastError());
}

void test_GetPwrDiskSpindownRange(void)
{
   DWORD retval;
   UINT min = 0;
   UINT max = 0;

   /*
    *  invalid parameter checks
    */

   retval = GetPwrDiskSpindownRange(NULL, NULL);
   ok(!retval, "function was expected to fail\n");
   ok(GetLastError() == ERROR_INVALID_PARAMETER, "expected error ERROR_INVALID_PARAMETER but got %x\n", (UINT)GetLastError());

   retval = GetPwrDiskSpindownRange(&max, NULL);
   ok(!retval, "function was expected to fail\n");
   ok(GetLastError() == ERROR_INVALID_PARAMETER, "expected error ERROR_INVALID_PARAMETER but got %x\n", (UINT)GetLastError());

   retval = GetPwrDiskSpindownRange(NULL, &min);
   ok(!retval, "function was expected to fail\n");
   ok(GetLastError() == ERROR_INVALID_PARAMETER, "expected error ERROR_INVALID_PARAMETER but got %x\n", (UINT)GetLastError());

   /*
    * read disk spindown range
    */

   retval = GetPwrDiskSpindownRange(&max, &min);
   ok(retval, "function was expected to succeed error %x\n",(UINT)GetLastError());
   ok(min <= max, "range mismatch min %d max %d\n",min, max);
}

void test_IsAdminOverrideActive(void)
{
   ADMINISTRATOR_POWER_POLICY app;
   BOOLEAN ret;

   ret = IsAdminOverrideActive(0);
   ok(!ret, "function was expected to fail, error %x\n",(UINT)GetLastError());

   ret = IsAdminOverrideActive(&app);
   ok(!ret, "function was expected to fail, error %x\n",(UINT)GetLastError());

   app.MinSleep = 0;
   app.MaxSleep = 0;
   app.MinVideoTimeout = 0;
   app.MaxVideoTimeout = 0;
   app.MinSpindownTimeout = 0;
   app.MaxSpindownTimeout = 0;

   ret = IsAdminOverrideActive(&app);
   ok(!ret, "function was expected to fail, error %x\n",(UINT)GetLastError());

   app.MinSleep = 1;
   app.MaxSleep = 2;
   app.MinVideoTimeout = 3;
   app.MaxVideoTimeout = 4;
   app.MinSpindownTimeout = 5;
   app.MaxSpindownTimeout = 6;

   ret = IsAdminOverrideActive(&app);
   ok(!ret, "function was expected to fail, error %x\n",(UINT)GetLastError());

}

void test_IsPwrHibernateAllowed(void)
{
/*
        BOOLEAN ret;

        ret = IsPwrHibernateAllowed();
        ok(!ret, "function was expected to fail, error %x\n",(UINT)GetLastError());
 */
}

void test_IsPwrShutdownAllowed(void)
{
/*
        BOOLEAN ret;

        ret = IsPwrShutdownAllowed();
        ok(ret, "function was expected to succeed, error %x\n",(UINT)GetLastError());
 */
}

void test_IsPwrSuspendAllowed(void)
{
/*
        BOOLEAN ret;

        ret = IsPwrSuspendAllowed();
        ok(ret, "function was expected to succed, error %x\n",(UINT)GetLastError());
 */
}

void test_ReadGlobalPwrPolicy(void)
{
   GLOBAL_POWER_POLICY gpp;
   BOOLEAN ret;

   ret = ReadGlobalPwrPolicy(&gpp);
   ok(ret, "function was expected to succeed, error %x\n",(UINT)GetLastError());
   ok(gpp.mach.Revision  == 1,"Global Mach Revision was expected to be 1 got %i",(UINT)gpp.mach.Revision);
   ok(gpp.user.Revision  == 1,"Global User Revision was expected to be 1 got %i",(UINT)gpp.mach.Revision);



}

void test_ReadProcessorPwrScheme(void)
{
   MACHINE_PROCESSOR_POWER_POLICY mppp;
   BOOLEAN ret;
   UINT i = 0;
   DWORD err;

   do
   {
      RtlZeroMemory(&mppp, sizeof(MACHINE_PROCESSOR_POWER_POLICY));
      ret = ReadProcessorPwrScheme(i,&mppp);
      if (ret)
      {
         ok(mppp.Revision == 1,"Main Revision was expected to be 1 got %i",(UINT)mppp.Revision);
         ok(mppp.ProcessorPolicyAc.Revision == 1,"PowerAC Revision was expected to be 1 got %i",(UINT)mppp.ProcessorPolicyAc.Revision);
         ok(mppp.ProcessorPolicyDc.Revision == 1,"PowerDC Revision was expected to be 1 got %i",(UINT)mppp.ProcessorPolicyDc.Revision);
      }
      else
      {
         err = GetLastError();
         ok(err == 0,"Failed Error %x\n",(UINT)err);
         return;
      }
      i++;
      if (i == g_NumPwrSchemes)
         return;
   } while (TRUE);

}

void test_ReadPwrScheme(void)
{
   DWORD retval;

   /*
    * read power scheme with null pointer -> crashs on Windows XP SP2
    */
   //retval = ReadPwrScheme(0, NULL);
   //ok(!retval, "function was expected to fail\n");
   //ok(GetLastError() == STATUS_INVALID_PARAMETER, "expected error ... but got %x\n", GetLastError());

   /*
    *  read a power scheme with an invalid index, leads to the creation of the key
    *  -> corrupts power scheme enumeration
    */
   //retval = ReadPwrScheme(0xFFFFFFFF, &powerPolicy);
   //ok(!retval, "function was expected to fail\n");
   //ok(GetLastError() == ERROR_ACCESS_DENIED, "expected error ERROR_ACCESS_DENIED but got %x\n", GetLastError());

   /*
    * read current active power scheme
    */

   retval = ReadPwrScheme(g_ActivePwrScheme, &g_PowerPolicy);
   ok(retval, "function was expected to succeed error %x\n",(UINT)GetLastError());

}

void test_SetSuspendState(void)
{
//	SetSuspendState(FALSE,FALSE,FALSE)
}

void test_ValidatePowerPolicies(void)
{
   GLOBAL_POWER_POLICY gpp;
   POWER_POLICY pp;
   BOOLEAN ret;

   SetLastError(0);
   ret = ValidatePowerPolicies(0,0);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());

   ret = ValidatePowerPolicies(&gpp,0);
   ok(!ret, "function was expected to fail return %i\n",(UINT)ret);
   ok(GetLastError() == ERROR_REVISION_MISMATCH,"function was expected to fail with ERROR_REVISION_MISMATCH(%i,%i), but error :%i\n",(UINT)gpp.user.Revision,(UINT)gpp.mach.Revision,(UINT)GetLastError());

   ret = ValidatePowerPolicies(0,&pp);
   ok(!ret, "function was expected to fail return %i\n",(UINT)ret);
   ok(GetLastError() == ERROR_REVISION_MISMATCH,"function was expected to fail with ERROR_REVISION_MISMATCH, but error :%i\n",(UINT)GetLastError());

   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(!ret, "function was expected to fail return %i\n",(UINT)ret);
   ok(GetLastError() == ERROR_REVISION_MISMATCH,"function was expected to fail with ERROR_REVISION_MISMATCH, but error :%i\n",(UINT)GetLastError());

   gpp.user.Revision = 1;
   gpp.mach.Revision = 1;

   ret = ValidatePowerPolicies(&gpp,0);
   ok(!ret, "function was expected to fail return %i\n",(UINT)ret);
   ok(GetLastError() == ERROR_INVALID_DATA,"function was expected to fail with ERROR_INVALID_DATA, but error :%i\n",(UINT)GetLastError());

   gpp.mach.LidOpenWakeAc = PowerSystemWorking;
   gpp.mach.LidOpenWakeDc = PowerSystemWorking;

   ret = ValidatePowerPolicies(&gpp,0);
   ok(!ret, "function was expected to fail return %i\n",(UINT)ret);
   ok(GetLastError() == ERROR_INVALID_DATA,"function was expected to fail with ERROR_INVALID_DATA, but error :%i\n",(UINT)GetLastError());

   gpp.user.PowerButtonAc.Action = PowerActionNone;
   gpp.user.PowerButtonDc.Action = PowerActionNone;
   gpp.user.SleepButtonAc.Action = PowerActionNone;
   gpp.user.SleepButtonDc.Action = PowerActionNone;
   gpp.user.LidCloseAc.Action = PowerActionNone;
   gpp.user.LidCloseDc.Action = PowerActionNone;

   gpp.user.DischargePolicy[0].Enable=FALSE;
   gpp.user.DischargePolicy[1].Enable=FALSE;
   gpp.user.DischargePolicy[2].Enable=FALSE;
   gpp.user.DischargePolicy[3].Enable=FALSE;
   gpp.user.DischargePolicy[4].Enable=FALSE;
   ret = ValidatePowerPolicies(&gpp,0);
   ok(ret, "function was expected to succeed return %i\n",(UINT)GetLastError());
   if (!ret)
   {
      ok(GetLastError() == ERROR_INVALID_DATA,"function was expected to fail with ERROR_INVALID_DATA, but error :%i\n",(UINT)GetLastError());
   }

   pp.user.Revision = 1;
   pp.mach.Revision = 1;

   ret = ValidatePowerPolicies(0,&pp);
   ok(!ret, "function was expected to fail return %i\n",(UINT)ret);
   ok(GetLastError() == ERROR_INVALID_DATA || GetLastError() == ERROR_GEN_FAILURE,"function was expected to fail with ERROR_GEN_FAILURE or ERROR_INVALID_DATA, but error :%i\n",(UINT)GetLastError());

   pp.mach.MinSleepAc = PowerSystemWorking;
   pp.mach.MinSleepDc = PowerSystemWorking;
   pp.mach.ReducedLatencySleepAc = PowerSystemWorking;
   pp.mach.ReducedLatencySleepDc = PowerSystemWorking;
   pp.mach.OverThrottledAc.Action = PowerActionNone;
   pp.mach.OverThrottledDc.Action = PowerActionNone;

   pp.user.IdleAc.Action = PowerActionWarmEject+1;
   pp.user.IdleDc.Action = PowerActionNone-1;
   pp.user.MaxSleepAc = PowerSystemMaximum+1;
   pp.user.MaxSleepDc = PowerSystemUnspecified;

   ret = ValidatePowerPolicies(0,&pp);
   ok(!ret, "function was expected to fail return %i\n",(UINT)GetLastError());
   if (!ret)
   {
      ok(GetLastError() == ERROR_INVALID_DATA,"function was expected to fail with ERROR_INVALID_DATA, but error :%i\n",(UINT)GetLastError());
   }

   pp.user.IdleAc.Action = PowerActionNone;
   pp.user.IdleDc.Action = PowerActionNone;
   pp.user.MaxSleepAc = PowerSystemWorking;
   pp.user.MaxSleepDc = PowerSystemWorking;

   ret = ValidatePowerPolicies(0,&pp);
   ok(ret, "function was expected to succeed, error %i\n",(UINT)GetLastError());
   if (!ret)
   {
      ok(GetLastError() == ERROR_GEN_FAILURE,"function was expected to fail with ERROR_GEN_FAILURE, but error :%i\n",(UINT)GetLastError());
   }

   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed, error %i\n",(UINT)GetLastError());
   if (!ret)
   {
//		ok(GetLastError() == ERROR_GEN_FAILURE,"function was expected to fail with ERROR_GEN_FAILURE, but error :%i\n",(UINT)GetLastError());
   }


   ret = GetCurrentPowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());

   ret = ValidatePowerPolicies(&gpp,0);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());

   ret = ValidatePowerPolicies(0,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());

   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());

}

void test_WriteGlobalPwrPolicy(void)
{
//	WriteGlobalPwrPolicy(&gpp);
}

void test_WriteProcessorPwrScheme(void)
{
//	WriteProcessorPwrScheme(0,&mppp);
}

void test_WritePwrScheme(void)
{
   DWORD retval;
   static const WCHAR szTestSchemeName[] = {'P','o','w','r','p','r','o','f',0};
   static const WCHAR szTestSchemeDesc[] = {'P','o','w','r','p','r','o','f',' ','S','c','h','e','m','e',0};

   /*
    * create a temporarly profile, will be deleted in test_DeletePwrScheme
    */

   retval = WritePwrScheme(&g_TempPwrScheme, (LPWSTR)szTestSchemeName, (LPWSTR)szTestSchemeDesc, &g_PowerPolicy);
   ok(retval, "Warning: function should have succeeded\n");
}

void func_power(void)
{
   test_CallNtPowerInformation();
   test_CanUserWritePwrScheme();
   test_EnumPwrSchemes();
   test_GetSetActivePwrScheme();
   test_ReadPwrScheme();
   test_WritePwrScheme();
   test_DeletePwrScheme();
   test_GetPwrDiskSpindownRange();

   test_GetCurrentPowerPolicies();

   test_GetPwrCapabilities();
   test_IsAdminOverrideActive();
   test_IsPwrHibernateAllowed();
   test_IsPwrShutdownAllowed();
   test_IsPwrSuspendAllowed();
   test_ReadGlobalPwrPolicy();
   test_ReadProcessorPwrScheme();
   test_SetSuspendState();
   test_ValidatePowerPolicies();
   test_WriteGlobalPwrPolicy();
   test_WriteProcessorPwrScheme();

}

void func_ros_init(void)
{
   HKEY hUser, hKeyPowrCfg, hKeyGlobalPowrPol, hKeyPowerPolicies, hKeytmp;
   DWORD err;
   GLOBAL_USER_POWER_POLICY gupp;
   GLOBAL_MACHINE_POWER_POLICY gmpp;
   USER_POWER_POLICY upp;
   MACHINE_POWER_POLICY mpp;
   MACHINE_PROCESSOR_POWER_POLICY mppp;
   GLOBAL_POWER_POLICY gpp;
   POWER_POLICY pp;

   int i;

   static const WCHAR szUserPowrCfgKey[] = { 'C', 'o', 'n', 't', 'r', 'o', 'l', ' ',
                                             'P', 'a', 'n', 'e', 'l', '\\', 'P', 'o', 'w', 'e', 'r', 'C', 'f', 'g', 0};

   static const WCHAR szCurrentPowerPolicy[] = {'C', 'u', 'r', 'r', 'e', 'n', 't', 'P',
                                                'o', 'w', 'e', 'r', 'P', 'o', 'l', 'i', 'c', 'y', 0};
   static const WCHAR szcpp[] = {'3', 0 };

   static const WCHAR szGlobalPowerPolicy[] = { 'G', 'l', 'o', 'b', 'a', 'l', 'P', 'o',
                                                'w', 'e', 'r', 'P', 'o', 'l', 'i', 'c', 'y', 0};
   static const WCHAR szPolicies[] = {'P', 'o', 'l', 'i', 'c', 'i', 'e', 's', 0};
   static const WCHAR szPowerPolicies[] = { 'P', 'o', 'w', 'e', 'r', 'P', 'o', 'l', 'i',
                                            'c', 'i', 'e', 's', 0};

   static const WCHAR szProcessorPolicies[] = { 'P', 'r', 'o', 'c', 'e', 's', 's', 'o', 'r',
                                                'P', 'o', 'l', 'i', 'c', 'i', 'e', 's', 0};

   static const WCHAR szcName[] = {'N', 'a', 'm', 'e', 0};
   static const WCHAR szcDescription[] = {'D', 'e', 's', 'c', 'r', 'i', 'p', 't', 'i', 'o', 'n', 0};

   static WCHAR szName[] = {'N', 'a', 'm', 'e', '(', '0', ')', 0};
   static WCHAR szDescription[] = {'D', 'e', 's', 'c', 'r', 'i', 'p', 't', 'i', 'o', 'n', '(', '0', ')', 0};

   static const WCHAR szMachPowrCfgKey[] = {'S', 'O', 'F', 'T', 'W', 'A', 'R', 'E', '\\', 'M', 'i',
                                            'c', 'r', 'o', 's', 'o', 'f', 't', '\\', 'W', 'i', 'n', 'd', 'o', 'w', 's', '\\', 'C', 'u',
                                            'r', 'r', 'e', 'n', 't', 'V', 'e', 'r', 's', 'i', 'o', 'n', '\\', 'C', 'o', 'n', 't', 'r',
                                            'o', 'l', 's', ' ', 'F', 'o', 'l', 'd', 'e', 'r', '\\', 'P', 'o', 'w', 'e', 'r', 'C', 'f', 'g', 0};

   static const WCHAR szLastID[] = {'L', 'a', 's', 't', 'I', 'D', 0};
   static const WCHAR szDiskSpinDownMax[] = {'D', 'i', 's', 'k', 'S', 'p', 'i', 'n', 'D', 'o', 'w', 'n', 'M', 'a', 'x', 0};
   static const WCHAR szDiskSpinDownMin[] = {'D', 'i', 's', 'k', 'S', 'p', 'i', 'n', 'D', 'o', 'w', 'n', 'M', 'i', 'n', 0};

   static const WCHAR szLastIDValue[] = {'5', 0};
   static const WCHAR szDiskSpinDownMaxValue[] = {'3', '6', '0', '0', 0};
   static const WCHAR szDiskSpinDownMinValue[] = {'3', 0};

   WCHAR tmp[20];
   /*
    * Erstelle die Registry-struktur und Daten, welche dafür erforderlich ist damit diese Tests funktionieren
    */

   /*
    * User
    */
   err = RegOpenCurrentUser(KEY_ALL_ACCESS,&hUser);
   ok(err == ERROR_SUCCESS,"Öffnen des Aktuellen Users Fehlgeschlagen\n");
   if (err == ERROR_SUCCESS)
   {
      err = RegCreateKey(hUser,szUserPowrCfgKey,&hKeyPowrCfg);
      ok(err == ERROR_SUCCESS,"Create Key UserPowrCfg failed with error %i\n",(UINT)err);
      ok(hKeyPowrCfg != NULL,"Erstellen des Eintrages Powercfg fehlgeschalgen\n");
      err = RegSetValueExW(hKeyPowrCfg,szCurrentPowerPolicy,(DWORD)NULL,REG_SZ,(CONST BYTE *)szcpp,strlenW(szcpp)*sizeof(WCHAR));
      ok(err == ERROR_SUCCESS,"Set Value CurrentPowerPolicy failed with error %i\n",(UINT)err);
      err = RegCreateKey(hKeyPowrCfg,szGlobalPowerPolicy,&hKeyGlobalPowrPol);
      ok(err == ERROR_SUCCESS,"Create Key GlobalPowerPolicy failed with error %i\n",(UINT)err);
      gupp.Revision = 1;
      gupp.PowerButtonAc.Action = PowerActionNone;
      gupp.PowerButtonDc.Action = PowerActionNone;
      gupp.SleepButtonAc.Action = PowerActionNone;
      gupp.SleepButtonDc.Action = PowerActionNone;
      gupp.LidCloseAc.Action = PowerActionNone;
      gupp.LidCloseDc.Action = PowerActionNone;
      for (i=0; i<NUM_DISCHARGE_POLICIES; i++)
      {
         gupp.DischargePolicy[0].Enable=FALSE;
      }

      err = RegSetValueExW(hKeyGlobalPowrPol,szPolicies,(DWORD)NULL,REG_BINARY,(CONST BYTE *)&gupp,sizeof(GLOBAL_USER_POWER_POLICY));
      ok(err == ERROR_SUCCESS,"Set Value GlobalPowrPol failed with error %i\n",(UINT)err);
      err = RegCloseKey(hKeyGlobalPowrPol);
      ok(err == ERROR_SUCCESS,"Close Key GlobalPowrPol failed with error %i\n",(UINT)err);
      err = RegCreateKey(hKeyPowrCfg,szPowerPolicies,&hKeyPowerPolicies);
      ok(err == ERROR_SUCCESS,"Create Key PowerPolicies failed with error %i\n",(UINT)err);

      upp.Revision = 1;
      upp.IdleAc.Action = PowerActionNone;
      upp.IdleDc.Action = PowerActionNone;
      upp.MaxSleepAc = PowerSystemWorking;
      upp.MaxSleepDc = PowerSystemWorking;
      upp.VideoTimeoutAc = 0;
      upp.VideoTimeoutDc = 0;
      upp.SpindownTimeoutAc = 0;
      upp.SpindownTimeoutDc = 0;

      for (i = 0; i<6; i++)
      {
         _itow(i,tmp,10);
         err = RegCreateKey(hKeyPowerPolicies,tmp,&hKeytmp);
         ok(err == ERROR_SUCCESS,"Create Key PowerPolicies(%i) failed with error %i\n",i,(UINT)err);
         szName[5]++;
         szDescription[12]++;
         err = RegSetValueExW(hKeytmp,szcName,(DWORD)NULL,REG_SZ,(CONST BYTE *)szName,strlenW(szName)*sizeof(WCHAR));
         err = RegSetValueExW(hKeytmp,szcDescription,(DWORD)NULL,REG_SZ,(CONST BYTE *)szDescription,strlenW(szDescription)*sizeof(WCHAR));
         err = RegSetValueExW(hKeytmp,szPolicies,(DWORD)NULL,REG_BINARY,(CONST BYTE *)&upp,sizeof(USER_POWER_POLICY));
         err = RegCloseKey(hKeytmp);
      }
      err = RegCloseKey(hKeyPowerPolicies);
      err = RegCloseKey(hKeyPowrCfg);
      err = RegCloseKey(hUser);
   }

   /*
    * Mach
    */
   err = RegCreateKey(HKEY_LOCAL_MACHINE,szMachPowrCfgKey,&hKeyPowrCfg);
   ok(err == ERROR_SUCCESS,"Create Key MachPowrCfgKey failed with error %i\n",(UINT)err);
   err = RegSetValueExW(hKeyPowrCfg,szLastID,(DWORD)NULL,REG_SZ,(CONST BYTE *)szLastIDValue,strlenW(szLastIDValue)*sizeof(WCHAR));
   err = RegSetValueExW(hKeyPowrCfg,szDiskSpinDownMax,(DWORD)NULL,REG_SZ,(CONST BYTE *)szDiskSpinDownMaxValue,strlenW(szDiskSpinDownMaxValue)*sizeof(WCHAR));
   err = RegSetValueExW(hKeyPowrCfg,szDiskSpinDownMin,(DWORD)NULL,REG_SZ,(CONST BYTE *)szDiskSpinDownMinValue,strlenW(szDiskSpinDownMinValue)*sizeof(WCHAR));

   err = RegCreateKey(hKeyPowrCfg,szGlobalPowerPolicy,&hKeyGlobalPowrPol);
   ok(err == ERROR_SUCCESS,"Create Key Mach GlobalPowerPolicy failed with error %i\n",(UINT)err);
   gmpp.Revision = 1;
   gmpp.LidOpenWakeAc = PowerSystemWorking;
   gmpp.LidOpenWakeDc = PowerSystemWorking;
   gmpp.BroadcastCapacityResolution=0;

   err = RegSetValueExW(hKeyGlobalPowrPol,szPolicies,(DWORD)NULL,REG_BINARY,(CONST BYTE *)&gmpp,sizeof(GLOBAL_MACHINE_POWER_POLICY));

   err = RegCloseKey(hKeyGlobalPowrPol);
   err = RegCreateKey(hKeyPowrCfg,szPowerPolicies,&hKeyPowerPolicies);
   ok(err == ERROR_SUCCESS,"Create Key Mach PowerPolicies failed with error %i\n",(UINT)err);

   mpp.Revision = 1;
   mpp.MinSleepAc = PowerSystemWorking;
   mpp.MinSleepDc = PowerSystemWorking;
   mpp.ReducedLatencySleepAc = PowerSystemWorking;
   mpp.ReducedLatencySleepDc = PowerSystemWorking;
   mpp.OverThrottledAc.Action = PowerActionNone;
   mpp.OverThrottledDc.Action = PowerActionNone;
   mpp.DozeS4TimeoutAc=0;
   mpp.DozeS4TimeoutDc=0;

   for (i = 0; i<6; i++)
   {
      _itow(i,tmp,10);
      err = RegCreateKey(hKeyPowerPolicies,tmp,&hKeytmp);
      ok(err == ERROR_SUCCESS,"Create Key Mach PowerPolicies(%i) failed with error %i\n",(UINT)i,(UINT)err);
      err = RegSetValueExW(hKeytmp,szPolicies,(DWORD)NULL,REG_BINARY,(CONST BYTE *)&mpp,sizeof(MACHINE_POWER_POLICY));
      err = RegCloseKey(hKeytmp);
   }
   err = RegCloseKey(hKeyPowerPolicies);

   err = RegCreateKey(hKeyPowrCfg,szProcessorPolicies,&hKeyPowerPolicies);
   ok(err == ERROR_SUCCESS,"Create Key Mach ProcessorPolicies failed with error %i\n",(UINT)err);

   mppp.Revision = 1;
   mppp.ProcessorPolicyAc.Revision = 1;
   mppp.ProcessorPolicyDc.Revision = 1;

   for (i = 0; i<6; i++)
   {
      _itow(i,tmp,10);
      err = RegCreateKey(hKeyPowerPolicies,tmp,&hKeytmp);
      ok(err == ERROR_SUCCESS,"Create Key Mach ProcessorPolicies(%i) failed with error %i\n",i,(UINT)err);
      err = RegSetValueExW(hKeytmp,szPolicies,(DWORD)NULL,REG_BINARY,(CONST BYTE *)&mppp,sizeof(MACHINE_PROCESSOR_POWER_POLICY));
      err = RegCloseKey(hKeytmp);
   }
   err = RegCloseKey(hKeyPowerPolicies);

   err = RegCloseKey(hKeyPowrCfg);

   err = GetCurrentPowerPolicies(&gpp,&pp);
   ok(err, "function was expected to succeed error %i\n",(UINT)GetLastError());

   err = ValidatePowerPolicies(&gpp,&pp);
   ok(err, "function was expected to succeed error %i\n",(UINT)GetLastError());

   /*
      [HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Controls Folder\PowerCfg\GlobalPowerPolicy]
      "CursorProperties"=...

    */

}
