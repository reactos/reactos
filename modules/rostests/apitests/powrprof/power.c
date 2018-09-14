/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Tests for powrprof.dll
 * PROGRAMMER:      Alex Wurzinger
 *                  Johannes Anderwald
 *                  Martin Rottensteiner
 */

#include <apitest.h>

#include <stdio.h>
#include <stdarg.h>
#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <winreg.h>
#include <ndk/potypes.h>
#include <powrprof.h>

unsigned int g_NumPwrSchemes = 0;
unsigned int g_NumPwrSchemesEnumerated = 0;
unsigned int g_ActivePwrScheme = 3;
unsigned int g_TempPwrScheme = 99;

POWER_POLICY g_PowerPolicy;

static const WCHAR szMachPowerPoliciesSubKey[] = { 'S', 'O', 'F', 'T', 'W', 'A', 'R',
                                                   'E', '\\', 'M', 'i', 'c', 'r', 'o', 's', 'o', 'f', 't', '\\', 'W', 'i', 'n', 'd',
                                                   'o', 'w', 's', '\\', 'C', 'u', 'r', 'r', 'e', 'n', 't', 'V', 'e', 'r', 's', 'i',
                                                   'o', 'n', '\\', 'C', 'o', 'n', 't', 'r', 'o', 'l', 's', ' ', 'F', 'o', 'l', 'd',
                                                   'e', 'r', '\\', 'P', 'o', 'w', 'e', 'r', 'C', 'f', 'g', '\\', 'P', 'o', 'w', 'e',
                                                   'r', 'P', 'o', 'l', 'i', 'c', 'i', 'e', 's', 0};

static const WCHAR szTempPwrScheme[] = { '9', '9', 0 };

void test_ValidatePowerPolicies_Next(PGLOBAL_POWER_POLICY pGPP_original,PPOWER_POLICY pPP_original);

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
   ok(retval != STATUS_PRIVILEGE_NOT_HELD, "Privileg not held!!!! more errors to expect\n");
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
         printf("#1 failed to delete subkey %i (testentry)\n", g_TempPwrScheme);
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

   ok(!ret, "function was expected to fail\n");

   ret = SetActivePwrScheme(0, 0, &pp);
   ok(!ret, "function was expected to fail\n");

   ret = SetActivePwrScheme(0, &gpp, &pp);
   ok(!ret, "function was expected to fail\n");

   ret = SetActivePwrScheme(current_scheme, &gpp, 0);
   ok(!ret, "function was expected to fail\n");

   ret = SetActivePwrScheme(current_scheme, 0, &pp);
   ok(!ret, "function was expected to fail\n");

   ret = SetActivePwrScheme(current_scheme, &gpp, &pp);
   ok(!ret, "function was expected to fail\n");

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
   ok(gpp.mach.Revision  == 1,"Global Mach Revision was expected to be 1 got %i\n",(UINT)gpp.mach.Revision);
   ok(gpp.user.Revision  == 1,"Global User Revision was expected to be 1 got %i\n",(UINT)gpp.mach.Revision);
   ok(pp.mach.Revision  == 1,"Mach Revision was expected to be 1 got %i\n",(UINT)gpp.mach.Revision);
   ok(pp.user.Revision  == 1,"User Revision was expected to be 1 got %i\n",(UINT)gpp.mach.Revision);


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
   ok(!ret, "function was expected to fail\n");

   ret = IsAdminOverrideActive(&app);
   ok(!ret, "function was expected to fail\n");

   app.MinSleep = 0;
   app.MaxSleep = 0;
   app.MinVideoTimeout = 0;
   app.MaxVideoTimeout = 0;
   app.MinSpindownTimeout = 0;
   app.MaxSpindownTimeout = 0;

   ret = IsAdminOverrideActive(&app);
   ok(!ret, "function was expected to fail\n");

   app.MinSleep = 1;
   app.MaxSleep = 2;
   app.MinVideoTimeout = 3;
   app.MaxVideoTimeout = 4;
   app.MinSpindownTimeout = 5;
   app.MaxSpindownTimeout = 6;

   ret = IsAdminOverrideActive(&app);
   ok(!ret, "function was expected to fail\n");

}

void test_IsPwrHibernateAllowed(void)
{
/*
        BOOLEAN ret;

        ret = IsPwrHibernateAllowed();
        ok(!ret, "function was expected to fail\n");
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
   ok(gpp.mach.Revision  == 1,"Global Mach Revision was expected to be 1 got %i\n",(UINT)gpp.mach.Revision);
   ok(gpp.user.Revision  == 1,"Global User Revision was expected to be 1 got %i\n",(UINT)gpp.mach.Revision);



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
         ok(mppp.Revision == 1,"Main Revision was expected to be 1 got %i\n",(UINT)mppp.Revision);
         ok(mppp.ProcessorPolicyAc.Revision == 1,"PowerAC Revision was expected to be 1 got %i\n",(UINT)mppp.ProcessorPolicyAc.Revision);
         ok(mppp.ProcessorPolicyDc.Revision == 1,"PowerDC Revision was expected to be 1 got %i\n",(UINT)mppp.ProcessorPolicyDc.Revision);
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


BOOLEAN globalcompare(GLOBAL_POWER_POLICY gpp, GLOBAL_POWER_POLICY gpp_compare)
{
	//return TRUE;
	int i,j;
	BOOLEAN ret;

	ret = TRUE;
	if (gpp.mach.BroadcastCapacityResolution != gpp_compare.mach.BroadcastCapacityResolution)
	{
		printf("mach.BroadcastCapacityResolution failed %lu != %lu\n",gpp.mach.BroadcastCapacityResolution,gpp_compare.mach.BroadcastCapacityResolution);
		ret = FALSE;
	}
	if (gpp.mach.LidOpenWakeAc != gpp_compare.mach.LidOpenWakeAc)
	{
		printf("mach.LidOpenWakeAc failed %i != %i\n",gpp.mach.LidOpenWakeAc,gpp_compare.mach.LidOpenWakeAc);
		ret = FALSE;
	}
	if (gpp.mach.LidOpenWakeDc != gpp_compare.mach.LidOpenWakeDc)
	{
		printf("mach.LidOpenWakeDc failed %i != %i\n",gpp.mach.LidOpenWakeDc,gpp_compare.mach.LidOpenWakeDc);
		ret = FALSE;
	}
	if (gpp.mach.Revision != gpp_compare.mach.Revision)
	{
		printf("mach.Revision failed %lu != %lu\n",gpp.mach.Revision,gpp_compare.mach.Revision);
		ret = FALSE;
	}

	if (gpp.user.PowerButtonAc.Action != gpp_compare.user.PowerButtonAc.Action)
	{
		printf("user.PowerButtonAc.Action failed %i != %i\n",gpp.user.PowerButtonAc.Action,gpp_compare.user.PowerButtonAc.Action);
		ret = FALSE;
	}
	if (gpp.user.PowerButtonAc.EventCode != gpp_compare.user.PowerButtonAc.EventCode)
	{
		printf("user.PowerButtonAc.EventCode failed %lu != %lu\n",gpp.user.PowerButtonAc.EventCode,gpp_compare.user.PowerButtonAc.EventCode);
		ret = FALSE;
	}
	if (gpp.user.PowerButtonAc.Flags != gpp_compare.user.PowerButtonAc.Flags)
	{
		printf("user.PowerButtonAc.Flags failed %lu != %lu\n",gpp.user.PowerButtonAc.Flags,gpp_compare.user.PowerButtonAc.Flags);
		ret = FALSE;
	}
	if (gpp.user.PowerButtonDc.Action != gpp_compare.user.PowerButtonDc.Action)
	{
		printf("user.PowerButtonDc.Action failed %i != %i\n",gpp.user.PowerButtonDc.Action,gpp_compare.user.PowerButtonDc.Action);
		ret = FALSE;
	}
	if (gpp.user.PowerButtonDc.EventCode != gpp_compare.user.PowerButtonDc.EventCode)
	{
		printf("user.PowerButtonDc.EventCode failed %lu != %lu\n",gpp.user.PowerButtonDc.EventCode,gpp_compare.user.PowerButtonDc.EventCode);
		ret = FALSE;
	}
	if (gpp.user.PowerButtonDc.Flags != gpp_compare.user.PowerButtonDc.Flags)
	{
		printf("user.PowerButtonDc.Flags failed %lu != %lu\n",gpp.user.PowerButtonDc.Flags,gpp_compare.user.PowerButtonDc.Flags);
		ret = FALSE;
	}
    if (gpp.user.SleepButtonAc.Action != gpp_compare.user.SleepButtonAc.Action)
	{
		printf("user.SleepButtonAc.Action failed %i != %i\n",gpp.user.SleepButtonAc.Action,gpp_compare.user.SleepButtonAc.Action);
		ret = FALSE;
	}
	if (gpp.user.SleepButtonAc.EventCode != gpp_compare.user.SleepButtonAc.EventCode)
	{
		printf("user.SleepButtonAc.EventCode failed %lu != %lu\n",gpp.user.SleepButtonAc.EventCode,gpp_compare.user.SleepButtonAc.EventCode);
		ret = FALSE;
	}
	if (gpp.user.SleepButtonAc.Flags != gpp_compare.user.SleepButtonAc.Flags)
	{
		printf("user.SleepButtonAc.Flags failed %lu != %lu\n",gpp.user.SleepButtonAc.Flags,gpp_compare.user.SleepButtonAc.Flags);
		ret = FALSE;
	}
	if (gpp.user.SleepButtonDc.Action != gpp_compare.user.SleepButtonDc.Action)
	{
		printf("user.SleepButtonDc.Action failed %i != %i\n",gpp.user.SleepButtonDc.Action,gpp_compare.user.SleepButtonDc.Action);
		ret = FALSE;
	}
	if (gpp.user.SleepButtonDc.EventCode != gpp_compare.user.SleepButtonDc.EventCode)
	{
		printf("user.SleepButtonDc.EventCode failed %lu != %lu\n",gpp.user.SleepButtonDc.EventCode,gpp_compare.user.SleepButtonDc.EventCode);
		ret = FALSE;
	}
	if (gpp.user.SleepButtonDc.Flags != gpp_compare.user.SleepButtonDc.Flags)
	{
		printf("user.SleepButtonDc.Flags failed %lu != %lu\n",gpp.user.SleepButtonDc.Flags,gpp_compare.user.SleepButtonDc.Flags);
		ret = FALSE;
	}
	if (gpp.user.LidCloseAc.Action != gpp_compare.user.LidCloseAc.Action)
	{
		printf("user.LidCloseAc.Action failed %i != %i\n",gpp.user.LidCloseAc.Action,gpp_compare.user.LidCloseAc.Action);
		ret = FALSE;
	}
	if (gpp.user.LidCloseAc.EventCode != gpp_compare.user.LidCloseAc.EventCode)
	{
		printf("user.LidCloseAc.EventCode failed %lu != %lu\n",gpp.user.LidCloseAc.EventCode,gpp_compare.user.LidCloseAc.EventCode);
		ret = FALSE;
	}
	if (gpp.user.LidCloseAc.Flags != gpp_compare.user.LidCloseAc.Flags)
	{
		printf("user.LidCloseAc.Flags failed %lu != %lu\n",gpp.user.LidCloseAc.Flags,gpp_compare.user.LidCloseAc.Flags);
		ret = FALSE;
	}
	if (gpp.user.LidCloseDc.Action != gpp_compare.user.LidCloseDc.Action)
	{
		printf("user.LidCloseDc.Action failed %i != %i\n",gpp.user.LidCloseDc.Action,gpp_compare.user.LidCloseDc.Action);
		ret = FALSE;
	}
	if (gpp.user.LidCloseDc.EventCode != gpp_compare.user.LidCloseDc.EventCode)
	{
		printf("user.LidCloseDc.EventCode failed %lu != %lu\n",gpp.user.LidCloseDc.EventCode,gpp_compare.user.LidCloseDc.EventCode);
		ret = FALSE;
	}
	if (gpp.user.LidCloseDc.Flags != gpp_compare.user.LidCloseDc.Flags)
	{
		printf("user.LidCloseDc.Flags failed %lu != %lu\n",gpp.user.LidCloseDc.Flags,gpp_compare.user.LidCloseDc.Flags);
		ret = FALSE;
	}

	for(i=0;i<NUM_DISCHARGE_POLICIES;i++)
	{
		if (gpp.user.DischargePolicy[i].Enable != gpp_compare.user.DischargePolicy[i].Enable)
		{
			printf("user.DischargePolicy(%i).Enable failed %i != %i\n",i, gpp.user.DischargePolicy[i].Enable,gpp_compare.user.DischargePolicy[i].Enable);
		    ret = FALSE;
		}
		for (j=0;j<3;j++)
		{
			if (gpp.user.DischargePolicy[i].Spare[j] != gpp_compare.user.DischargePolicy[i].Spare[j])
			{
				printf("user.DischargePolicy(%i).Spare[j] failed %i != %i\n",i, gpp.user.DischargePolicy[i].Spare[j],gpp_compare.user.DischargePolicy[i].Spare[j]);
				ret = FALSE;
			}
		}
		if (gpp.user.DischargePolicy[i].BatteryLevel != gpp_compare.user.DischargePolicy[i].BatteryLevel)
		{
			printf("user.DischargePolicy(%i).BatteryLevel failed %lu != %lu\n",i, gpp.user.DischargePolicy[i].BatteryLevel,gpp_compare.user.DischargePolicy[i].BatteryLevel);
		    ret = FALSE;
		}
		if (gpp.user.DischargePolicy[i].PowerPolicy.Action != gpp_compare.user.DischargePolicy[i].PowerPolicy.Action)
		{
			printf("user.DischargePolicy(%i).PowerPolicy.Action failed %i != %i\n",i, gpp.user.DischargePolicy[i].PowerPolicy.Action,gpp_compare.user.DischargePolicy[i].PowerPolicy.Action);
		    ret = FALSE;
		}
		if (gpp.user.DischargePolicy[i].PowerPolicy.Flags != gpp_compare.user.DischargePolicy[i].PowerPolicy.Flags)
		{
			printf("user.DischargePolicy(%i).PowerPolicy.Flags failed %lu != %lu\n",i, gpp.user.DischargePolicy[i].PowerPolicy.Flags,gpp_compare.user.DischargePolicy[i].PowerPolicy.Flags);
		    ret = FALSE;
		}
		if (gpp.user.DischargePolicy[i].PowerPolicy.EventCode != gpp_compare.user.DischargePolicy[i].PowerPolicy.EventCode)
		{
			printf("user.DischargePolicy(%i).PowerPolicy.EventCode failed %lu != %lu\n",i, gpp.user.DischargePolicy[i].PowerPolicy.EventCode,gpp_compare.user.DischargePolicy[i].PowerPolicy.EventCode);
		    ret = FALSE;
		}
		if (gpp.user.DischargePolicy[i].MinSystemState != gpp_compare.user.DischargePolicy[i].MinSystemState)
		{
			printf("user.DischargePolicy(%i).MinSystemState failed %i != %i\n",i, gpp.user.DischargePolicy[i].MinSystemState,gpp_compare.user.DischargePolicy[i].MinSystemState);
		    ret = FALSE;
		}
	}
    if (gpp.user.GlobalFlags != gpp_compare.user.GlobalFlags)
	{
		printf("user.GlobalFlags failed %lu != %lu\n",gpp.user.GlobalFlags,gpp_compare.user.GlobalFlags);
		ret = FALSE;
	}
	if (gpp.user.Revision != gpp_compare.user.Revision)
	{
		printf("user.Revision failed %lu != %lu\n",gpp.user.Revision,gpp_compare.user.Revision);
		ret = FALSE;
	}
    return ret;
}
BOOLEAN compare(POWER_POLICY pp, POWER_POLICY pp_compare)
{
	//return TRUE;
	BOOLEAN ret=TRUE;

	if (pp.mach.DozeS4TimeoutAc != pp_compare.mach.DozeS4TimeoutAc)
	{
		printf("mach.DozeS4TimeoutAc failed %lu != %lu\n",pp.mach.DozeS4TimeoutAc,pp_compare.mach.DozeS4TimeoutAc);
		ret = FALSE;
	}
	if (pp.mach.DozeS4TimeoutDc != pp_compare.mach.DozeS4TimeoutDc)
	{
		printf("mach.DozeS4TimeoutDc failed %lu != %lu\n",pp.mach.DozeS4TimeoutDc,pp_compare.mach.DozeS4TimeoutDc);
		ret = FALSE;
	}
	if (pp.mach.MinSleepAc != pp_compare.mach.MinSleepAc)
	{
		printf("mach.MinSleepAc failed %i != %i\n",pp.mach.MinSleepAc,pp_compare.mach.MinSleepAc);
		ret = FALSE;
	}
	if (pp.mach.MinSleepDc != pp_compare.mach.MinSleepDc)
	{
		printf("mach.MinSleepDc failed %i != %i\n",pp.mach.MinSleepDc,pp_compare.mach.MinSleepDc);
		ret = FALSE;
	}
	if (pp.mach.DozeTimeoutAc != pp_compare.mach.DozeTimeoutAc)
	{
		printf("mach.DozeTimeoutAc failed %lu != %lu\n",pp.mach.DozeTimeoutAc,pp_compare.mach.DozeTimeoutAc);
		ret = FALSE;
	}
	if (pp.mach.DozeTimeoutDc != pp_compare.mach.DozeTimeoutDc)
	{
		printf("mach.DozeTimeoutDc failed %lu != %lu\n",pp.mach.DozeTimeoutDc,pp_compare.mach.DozeTimeoutDc);
		ret = FALSE;
	}
	if (pp.mach.ReducedLatencySleepAc != pp_compare.mach.ReducedLatencySleepAc)
	{
		printf("mach.ReducedLatencySleepAc failed %i != %i\n",pp.mach.ReducedLatencySleepAc,pp_compare.mach.ReducedLatencySleepAc);
		ret = FALSE;
	}
	if (pp.mach.ReducedLatencySleepDc != pp_compare.mach.ReducedLatencySleepDc)
	{
		printf("mach.ReducedLatencySleepDc failed %i != %i\n",pp.mach.ReducedLatencySleepDc,pp_compare.mach.ReducedLatencySleepDc);
		ret = FALSE;
	}
	if (pp.mach.MinThrottleAc != pp_compare.mach.MinThrottleAc)
	{
		printf("mach.MinThrottleAc failed %i != %i\n",pp.mach.MinThrottleAc,pp_compare.mach.MinThrottleAc);
		ret = FALSE;
	}
	if (pp.mach.MinThrottleDc != pp_compare.mach.MinThrottleDc)
	{
		printf("mach.MinThrottleDc failed %i != %i\n",pp.mach.MinThrottleDc,pp_compare.mach.MinThrottleDc);
		ret = FALSE;
	}

	if (pp.mach.OverThrottledAc.Action != pp_compare.mach.OverThrottledAc.Action)
	{
		printf("mach.OverThrottledAc.Action failed %i != %i\n",pp.mach.OverThrottledAc.Action,pp_compare.mach.OverThrottledAc.Action);
		ret = FALSE;
	}
	if (pp.mach.OverThrottledAc.Flags != pp_compare.mach.OverThrottledAc.Flags)
	{
		printf("mach.OverThrottledAc.Flags failed %lu != %lu\n",pp.mach.OverThrottledAc.Flags,pp_compare.mach.OverThrottledAc.Flags);
		ret = FALSE;
	}
	if (pp.mach.OverThrottledAc.EventCode != pp_compare.mach.OverThrottledAc.EventCode)
	{
		printf("mach.OverThrottledAc.EventCode failed %lu != %lu\n",pp.mach.OverThrottledAc.EventCode,pp_compare.mach.OverThrottledAc.EventCode);
		ret = FALSE;
	}
	if (pp.mach.OverThrottledDc.Action != pp_compare.mach.OverThrottledDc.Action)
	{
		printf("mach.OverThrottledDc.Action failed %i != %i\n",pp.mach.OverThrottledDc.Action,pp_compare.mach.OverThrottledDc.Action);
		ret = FALSE;
	}
	if (pp.mach.OverThrottledDc.Flags != pp_compare.mach.OverThrottledDc.Flags)
	{
		printf("mach.OverThrottledDc.Flags failed %lu != %lu\n",pp.mach.OverThrottledDc.Flags,pp_compare.mach.OverThrottledDc.Flags);
		ret = FALSE;
	}
	if (pp.mach.OverThrottledDc.EventCode != pp_compare.mach.OverThrottledDc.EventCode)
	{
		printf("mach.OverThrottledDc.EventCode failed %lu != %lu\n",pp.mach.OverThrottledDc.EventCode,pp_compare.mach.OverThrottledDc.EventCode);
		ret = FALSE;
	}

	if (pp.mach.pad1[0] != pp_compare.mach.pad1[0])
	{
		printf("mach.pad1[0] failed %i != %i\n",pp.mach.pad1[0],pp_compare.mach.pad1[0]);
		ret = FALSE;
	}
	if (pp.mach.pad1[1] != pp_compare.mach.pad1[1])
	{
		printf("mach.pad1[1] failed %i != %i\n",pp.mach.pad1[1],pp_compare.mach.pad1[1]);
		ret = FALSE;
	}
	if (pp.mach.Revision != pp_compare.mach.Revision)
	{
		printf("mach.Revision failed %lu != %lu\n",pp.mach.Revision,pp_compare.mach.Revision);
		ret = FALSE;
	}

	if (pp.user.IdleAc.Action != pp_compare.user.IdleAc.Action)
	{
		printf("user.IdleAc.Action failed %i != %i\n",pp.user.IdleAc.Action,pp_compare.user.IdleAc.Action);
		ret = FALSE;
	}
	if (pp.user.IdleAc.Flags != pp_compare.user.IdleAc.Flags)
	{
		printf("user.IdleAc.Flags failed %lu != %lu\n",pp.user.IdleAc.Flags,pp_compare.user.IdleAc.Flags);
		ret = FALSE;
	}
	if (pp.user.IdleAc.EventCode != pp_compare.user.IdleAc.EventCode)
	{
		printf("user.IdleAc.EventCode failed %lu != %lu\n",pp.user.IdleAc.EventCode,pp_compare.user.IdleAc.EventCode);
		ret = FALSE;
	}
	if (pp.user.IdleDc.Action != pp_compare.user.IdleDc.Action)
	{
		printf("user.IdleDc.Action failed %i != %i\n",pp.user.IdleDc.Action,pp_compare.user.IdleDc.Action);
		ret = FALSE;
	}
	if (pp.user.IdleDc.Flags != pp_compare.user.IdleDc.Flags)
	{
		printf("user.IdleDc.Flags failed %lu != %lu\n",pp.user.IdleDc.Flags,pp_compare.user.IdleDc.Flags);
		ret = FALSE;
	}
	if (pp.user.IdleDc.EventCode != pp_compare.user.IdleDc.EventCode)
	{
		printf("user.IdleDc.EventCode failed %lu != %lu\n",pp.user.IdleDc.EventCode,pp_compare.user.IdleDc.EventCode);
		ret = FALSE;
	}
	if (pp.user.IdleTimeoutAc != pp_compare.user.IdleTimeoutAc)
	{
		printf("user.IdleTimeoutAc failed %lu != %lu\n",pp.user.IdleTimeoutAc,pp_compare.user.IdleTimeoutAc);
		ret = FALSE;
	}
	if (pp.user.IdleTimeoutDc != pp_compare.user.IdleTimeoutDc)
	{
		printf("user.IdleTimeoutDc failed %lu != %lu\n",pp.user.IdleTimeoutDc,pp_compare.user.IdleTimeoutDc);
		ret = FALSE;
	}
	if (pp.user.IdleSensitivityAc != pp_compare.user.IdleSensitivityAc)
	{
		printf("user.IdleSensitivityAc failed %i != %i\n",pp.user.IdleSensitivityAc,pp_compare.user.IdleSensitivityAc);
		ret = FALSE;
	}
	if (pp.user.IdleSensitivityDc != pp_compare.user.IdleSensitivityDc)
	{
		printf("user.IdleSensitivityDc failed %i != %i\n",pp.user.IdleSensitivityDc,pp_compare.user.IdleSensitivityDc);
		ret = FALSE;
	}
	if (pp.user.ThrottlePolicyAc != pp_compare.user.ThrottlePolicyAc)
	{
		printf("user.ThrottlePolicyAc failed %i != %i\n",pp.user.ThrottlePolicyAc,pp_compare.user.ThrottlePolicyAc);
		ret = FALSE;
	}
	if (pp.user.ThrottlePolicyDc != pp_compare.user.ThrottlePolicyDc)
	{
		printf("user.ThrottlePolicyDc failed %i != %i\n",pp.user.ThrottlePolicyDc,pp_compare.user.ThrottlePolicyDc);
		ret = FALSE;
	}
	if (pp.user.MaxSleepAc != pp_compare.user.MaxSleepAc)
	{
		printf("user.MaxSleepAc failed %i != %i\n",pp.user.MaxSleepAc,pp_compare.user.MaxSleepAc);
		ret = FALSE;
	}
	if (pp.user.MaxSleepDc != pp_compare.user.MaxSleepDc)
	{
		printf("user.MaxSleepDc failed %i != %i\n",pp.user.MaxSleepDc,pp_compare.user.MaxSleepDc);
		ret = FALSE;
	}
	if (pp.user.Reserved[0] != pp_compare.user.Reserved[0])
	{
		printf("user.Reserved[0] failed %lu != %lu\n",pp.user.Reserved[0],pp_compare.user.Reserved[0]);
		ret = FALSE;
	}
	if (pp.user.Reserved[1] != pp_compare.user.Reserved[1])
	{
		printf("user.Reserved[1] failed %lu != %lu\n",pp.user.Reserved[1],pp_compare.user.Reserved[1]);
		ret = FALSE;
	}
	if (pp.user.Reserved[2] != pp_compare.user.Reserved[2])
	{
		printf("user.Reserved[2] failed %lu != %lu\n",pp.user.Reserved[2],pp_compare.user.Reserved[2]);
		ret = FALSE;
	}
	if (pp.user.VideoTimeoutAc != pp_compare.user.VideoTimeoutAc)
	{
		printf("user.VideoTimeoutAc failed %lu != %lu\n",pp.user.VideoTimeoutAc,pp_compare.user.VideoTimeoutAc);
		ret = FALSE;
	}
	if (pp.user.VideoTimeoutDc != pp_compare.user.VideoTimeoutDc)
	{
		printf("user.VideoTimeoutDc failed %lu != %lu\n",pp.user.VideoTimeoutDc,pp_compare.user.VideoTimeoutDc);
		ret = FALSE;
	}

	if (pp.user.SpindownTimeoutAc != pp_compare.user.SpindownTimeoutAc)
	{
		printf("user.SpindownTimeoutAc failed %lu != %lu\n",pp.user.SpindownTimeoutAc,pp_compare.user.SpindownTimeoutAc);
		ret = FALSE;
	}
	if (pp.user.SpindownTimeoutDc != pp_compare.user.SpindownTimeoutDc)
	{
		printf("user.SpindownTimeoutDc failed %lu != %lu\n",pp.user.SpindownTimeoutDc,pp_compare.user.SpindownTimeoutDc);
		ret = FALSE;
	}
	if (pp.user.OptimizeForPowerAc != pp_compare.user.OptimizeForPowerAc)
	{
		printf("user.OptimizeForPowerAc failed %i != %i\n",pp.user.OptimizeForPowerAc,pp_compare.user.OptimizeForPowerAc);
		ret = FALSE;
	}
	if (pp.user.OptimizeForPowerDc != pp_compare.user.OptimizeForPowerDc)
	{
		printf("user.OptimizeForPowerDc failed %i != %i\n",pp.user.OptimizeForPowerDc,pp_compare.user.OptimizeForPowerDc);
		ret = FALSE;
	}
	if (pp.user.FanThrottleToleranceAc != pp_compare.user.FanThrottleToleranceAc)
	{
		printf("user.FanThrottleToleranceAc failed %i != %i\n",pp.user.FanThrottleToleranceAc,pp_compare.user.FanThrottleToleranceAc);
		ret = FALSE;
	}
	if (pp.user.FanThrottleToleranceDc != pp_compare.user.FanThrottleToleranceDc)
	{
		printf("user.FanThrottleToleranceDc failed %i != %i\n",pp.user.FanThrottleToleranceDc,pp_compare.user.FanThrottleToleranceDc);
		ret = FALSE;
	}
	if (pp.user.ForcedThrottleAc != pp_compare.user.ForcedThrottleAc)
	{
		printf("user.ForcedThrottleAc failed %i != %i\n",pp.user.ForcedThrottleAc,pp_compare.user.ForcedThrottleAc);
		ret = FALSE;
	}
	if (pp.user.ForcedThrottleDc != pp_compare.user.ForcedThrottleDc)
	{
		printf("user.ForcedThrottleDc failed %i != %i\n",pp.user.ForcedThrottleDc,pp_compare.user.ForcedThrottleDc);
		ret = FALSE;
	}
	if (pp.user.Revision != pp_compare.user.Revision)
	{
		printf("user.Revision failed %lu != %lu\n",pp.user.Revision,pp_compare.user.Revision);
		ret = FALSE;
	}

	return ret;
}

void test_ValidatePowerPolicies_Old(void)
{
   GLOBAL_POWER_POLICY gpp;
   POWER_POLICY pp;
   BOOLEAN ret;

   RtlZeroMemory(&gpp, sizeof(GLOBAL_POWER_POLICY));
   RtlZeroMemory(&pp, sizeof(POWER_POLICY));

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
   ok(GetLastError() == ERROR_GEN_FAILURE,"function was expected to fail with ERROR_GEN_FAILURE, but error :%i\n",(UINT)GetLastError());

   gpp.mach.LidOpenWakeAc = PowerSystemWorking;
   gpp.mach.LidOpenWakeDc = PowerSystemWorking;

   ret = ValidatePowerPolicies(&gpp,0);
   ok(ret, "function was expected to succeed return %i\n",(UINT)GetLastError());

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
		ok(GetLastError() == ERROR_GEN_FAILURE,"function was expected to fail with ERROR_GEN_FAILURE, but error :%i\n",(UINT)GetLastError());
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

void test_ValidatePowerPolicies(void)
{
   GLOBAL_POWER_POLICY gpp, gpp_compare, gpp_original;
   POWER_POLICY pp, pp_compare, pp_original;
   BOOLEAN ret;

   RtlZeroMemory(&gpp_original, sizeof(GLOBAL_POWER_POLICY));
   RtlZeroMemory(&pp_original, sizeof(POWER_POLICY));

 	memcpy(&pp, &pp_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, &pp_original, sizeof(POWER_POLICY));
   ok(compare(pp,pp_compare),"Difference Found\n");

    memcpy(&gpp, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");

   SetLastError(0);
   ret = ValidatePowerPolicies(0,0);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());

    memcpy(&gpp, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,0);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_REVISION_MISMATCH,"function was expected to fail with ERROR_REVISION_MISMATCH(%i,%i), but error :%i\n",(UINT)gpp.user.Revision,(UINT)gpp.mach.Revision,(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");

	memcpy(&pp, &pp_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, &pp_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(0,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_REVISION_MISMATCH,"function was expected to fail with ERROR_REVISION_MISMATCH, but error :%i\n",(UINT)GetLastError());
   ok(compare(pp,pp_compare),"Difference Found\n");

    memcpy(&gpp, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, &pp_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, &pp_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_REVISION_MISMATCH,"function was expected to fail with ERROR_REVISION_MISMATCH, but error :%i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   gpp_original.user.Revision = 1;
    memcpy(&gpp, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,0);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_REVISION_MISMATCH,"function was expected to fail with ERROR_REVISION_MISMATCH, but error :%i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");

   gpp_original.mach.Revision = 1;
    memcpy(&gpp, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,0);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_GEN_FAILURE,"function was expected to fail with ERROR_GEN_FAILURE, but error :%i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");

   gpp_original.mach.LidOpenWakeAc = PowerSystemWorking;
    memcpy(&gpp, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,0);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_GEN_FAILURE,"function was expected to fail with ERROR_GEN_FAILURE, but error :%i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");

   gpp_original.mach.LidOpenWakeDc = PowerSystemWorking;
    memcpy(&gpp, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,0);
   ok(ret, "function was expected to succeed, error %x\n",(UINT)GetLastError());
   gpp_compare.mach.BroadcastCapacityResolution=100;
   gpp_compare.user.DischargePolicy[1].PowerPolicy.EventCode = 65536;
   gpp_compare.user.DischargePolicy[2].PowerPolicy.EventCode = 131072;
   gpp_compare.user.DischargePolicy[3].PowerPolicy.EventCode = 196608;
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");

   gpp_original.user.PowerButtonAc.Action = PowerActionNone;
    memcpy(&gpp, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,0);
   ok(ret, "function was expected to succeed return %i\n",(UINT)GetLastError());
   gpp_compare.mach.BroadcastCapacityResolution=100;
   gpp_compare.user.DischargePolicy[1].PowerPolicy.EventCode = 65536;
   gpp_compare.user.DischargePolicy[2].PowerPolicy.EventCode = 131072;
   gpp_compare.user.DischargePolicy[3].PowerPolicy.EventCode = 196608;
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");

   gpp_original.user.PowerButtonDc.Action = PowerActionNone;
    memcpy(&gpp, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,0);
   ok(ret, "function was expected to succeed return %i\n",(UINT)GetLastError());
   gpp_compare.mach.BroadcastCapacityResolution=100;
   gpp_compare.user.DischargePolicy[1].PowerPolicy.EventCode = 65536;
   gpp_compare.user.DischargePolicy[2].PowerPolicy.EventCode = 131072;
   gpp_compare.user.DischargePolicy[3].PowerPolicy.EventCode = 196608;
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");

   gpp_original.user.SleepButtonAc.Action = PowerActionNone;
    memcpy(&gpp, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,0);
   ok(ret, "function was expected to succeed return %i\n",(UINT)GetLastError());
   gpp_compare.mach.BroadcastCapacityResolution=100;
   gpp_compare.user.DischargePolicy[1].PowerPolicy.EventCode = 65536;
   gpp_compare.user.DischargePolicy[2].PowerPolicy.EventCode = 131072;
   gpp_compare.user.DischargePolicy[3].PowerPolicy.EventCode = 196608;
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");

   gpp_original.user.SleepButtonDc.Action = PowerActionNone;
    memcpy(&gpp, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,0);
   ok(ret, "function was expected to succeed return %i\n",(UINT)GetLastError());
   gpp_compare.mach.BroadcastCapacityResolution=100;
   gpp_compare.user.DischargePolicy[1].PowerPolicy.EventCode = 65536;
   gpp_compare.user.DischargePolicy[2].PowerPolicy.EventCode = 131072;
   gpp_compare.user.DischargePolicy[3].PowerPolicy.EventCode = 196608;
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");

   gpp_original.user.LidCloseAc.Action = PowerActionNone;
    memcpy(&gpp, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,0);
   ok(ret, "function was expected to succeed return %i\n",(UINT)GetLastError());
   gpp_compare.mach.BroadcastCapacityResolution=100;
   gpp_compare.user.DischargePolicy[1].PowerPolicy.EventCode = 65536;
   gpp_compare.user.DischargePolicy[2].PowerPolicy.EventCode = 131072;
   gpp_compare.user.DischargePolicy[3].PowerPolicy.EventCode = 196608;
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");

   gpp_original.user.LidCloseDc.Action = PowerActionNone;
    memcpy(&gpp, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,0);
   ok(ret, "function was expected to succeed return %i\n",(UINT)GetLastError());
   gpp_compare.mach.BroadcastCapacityResolution=100;
   gpp_compare.user.DischargePolicy[1].PowerPolicy.EventCode = 65536;
   gpp_compare.user.DischargePolicy[2].PowerPolicy.EventCode = 131072;
   gpp_compare.user.DischargePolicy[3].PowerPolicy.EventCode = 196608;
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");

   gpp_original.user.DischargePolicy[0].Enable=FALSE;
    memcpy(&gpp, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,0);
   ok(ret, "function was expected to succeed return %i\n",(UINT)GetLastError());
   gpp_compare.mach.BroadcastCapacityResolution=100;
   gpp_compare.user.DischargePolicy[1].PowerPolicy.EventCode = 65536;
   gpp_compare.user.DischargePolicy[2].PowerPolicy.EventCode = 131072;
   gpp_compare.user.DischargePolicy[3].PowerPolicy.EventCode = 196608;
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");

   gpp_original.user.DischargePolicy[1].Enable=FALSE;
    memcpy(&gpp, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,0);
   ok(ret, "function was expected to succeed return %i\n",(UINT)GetLastError());
   gpp_compare.mach.BroadcastCapacityResolution=100;
   gpp_compare.user.DischargePolicy[1].PowerPolicy.EventCode = 65536;
   gpp_compare.user.DischargePolicy[2].PowerPolicy.EventCode = 131072;
   gpp_compare.user.DischargePolicy[3].PowerPolicy.EventCode = 196608;
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");

   gpp_original.user.DischargePolicy[2].Enable=FALSE;
    memcpy(&gpp, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,0);
   ok(ret, "function was expected to succeed return %i\n",(UINT)GetLastError());
   gpp_compare.mach.BroadcastCapacityResolution=100;
   gpp_compare.user.DischargePolicy[1].PowerPolicy.EventCode = 65536;
   gpp_compare.user.DischargePolicy[2].PowerPolicy.EventCode = 131072;
   gpp_compare.user.DischargePolicy[3].PowerPolicy.EventCode = 196608;
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");

   gpp_original.user.DischargePolicy[3].Enable=FALSE;
    memcpy(&gpp, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,0);
   ok(ret, "function was expected to succeed return %i\n",(UINT)GetLastError());
   gpp_compare.mach.BroadcastCapacityResolution=100;
   gpp_compare.user.DischargePolicy[1].PowerPolicy.EventCode = 65536;
   gpp_compare.user.DischargePolicy[2].PowerPolicy.EventCode = 131072;
   gpp_compare.user.DischargePolicy[3].PowerPolicy.EventCode = 196608;
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");

   pp_original.user.Revision = 1;
	memcpy(&pp, &pp_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, &pp_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(0,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() ==  ERROR_REVISION_MISMATCH,"function was expected to fail with  ERROR_REVISION_MISMATCH, but error :%i\n",(UINT)GetLastError());
   ok(compare(pp,pp_compare),"Difference Found\n");

   pp_original.mach.Revision = 1;
	memcpy(&pp, &pp_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, &pp_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(0,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_GEN_FAILURE,"function was expected to fail with ERROR_GEN_FAILURE, but error :%i\n",(UINT)GetLastError());
   ok(compare(pp,pp_compare),"Difference Found\n");

   pp_original.mach.MinSleepAc = PowerSystemWorking;
	memcpy(&pp, &pp_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, &pp_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(0,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_GEN_FAILURE,"function was expected to fail with ERROR_GEN_FAILURE, but error :%i\n",(UINT)GetLastError());
   ok(compare(pp,pp_compare),"Difference Found\n");

   pp_original.mach.MinSleepDc = PowerSystemWorking;
	memcpy(&pp, &pp_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, &pp_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(0,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_GEN_FAILURE,"function was expected to fail with ERROR_GEN_FAILURE, but error :%i\n",(UINT)GetLastError());
   ok(compare(pp,pp_compare),"Difference Found\n");

   pp_original.mach.ReducedLatencySleepAc = PowerSystemWorking;
	memcpy(&pp, &pp_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, &pp_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(0,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_GEN_FAILURE,"function was expected to fail with ERROR_GEN_FAILURE, but error :%i\n",(UINT)GetLastError());
   ok(compare(pp,pp_compare),"Difference Found\n");

   pp_original.mach.ReducedLatencySleepDc = PowerSystemWorking;
	memcpy(&pp, &pp_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, &pp_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(0,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_GEN_FAILURE,"function was expected to fail with ERROR_GEN_FAILURE, but error :%i\n",(UINT)GetLastError());
   ok(compare(pp,pp_compare),"Difference Found\n");

   pp_original.mach.OverThrottledAc.Action = PowerActionNone;
	memcpy(&pp, &pp_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, &pp_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(0,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_GEN_FAILURE,"function was expected to fail with ERROR_GEN_FAILURE, but error :%i\n",(UINT)GetLastError());
   ok(compare(pp,pp_compare),"Difference Found\n");

   pp_original.mach.OverThrottledDc.Action = PowerActionNone;
	memcpy(&pp, &pp_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, &pp_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(0,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_GEN_FAILURE,"function was expected to fail with ERROR_GEN_FAILURE, but error :%i\n",(UINT)GetLastError());
   ok(compare(pp,pp_compare),"Difference Found\n");

   pp_original.user.IdleAc.Action = PowerActionWarmEject+1;
	memcpy(&pp, &pp_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, &pp_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(0,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_INVALID_DATA,"function was expected to fail with ERROR_INVALID_DATA, but error :%i\n",(UINT)GetLastError());
   ok(compare(pp,pp_compare),"Difference Found\n");

   pp_original.user.IdleDc.Action = PowerActionNone-1;
	memcpy(&pp, &pp_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, &pp_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(0,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_INVALID_DATA,"function was expected to fail with ERROR_INVALID_DATA, but error :%i\n",(UINT)GetLastError());
   ok(compare(pp,pp_compare),"Difference Found\n");

   pp_original.user.MaxSleepAc = PowerSystemMaximum+1;
	memcpy(&pp, &pp_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, &pp_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(0,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_INVALID_DATA,"function was expected to fail with ERROR_INVALID_DATA, but error :%i\n",(UINT)GetLastError());
   ok(compare(pp,pp_compare),"Difference Found\n");

   pp_original.user.MaxSleepDc = PowerSystemUnspecified;
	memcpy(&pp, &pp_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, &pp_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(0,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_INVALID_DATA,"function was expected to fail with ERROR_INVALID_DATA, but error :%i\n",(UINT)GetLastError());
   ok(compare(pp,pp_compare),"Difference Found\n");

   pp_original.user.IdleAc.Action = PowerActionNone;
	memcpy(&pp, &pp_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, &pp_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(0,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_INVALID_DATA,"function was expected to fail with ERROR_INVALID_DATA, but error :%i\n",(UINT)GetLastError());
   ok(compare(pp,pp_compare),"Difference Found\n");

   pp_original.user.IdleDc.Action = PowerActionNone;
	memcpy(&pp, &pp_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, &pp_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(0,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_GEN_FAILURE,"function was expected to fail with ERROR_GEN_FAILURE, but error :%i\n",(UINT)GetLastError());
   ok(compare(pp,pp_compare),"Difference Found\n");

   pp_original.user.MaxSleepAc = PowerSystemWorking;
	memcpy(&pp, &pp_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, &pp_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(0,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_GEN_FAILURE,"function was expected to fail with ERROR_GEN_FAILURE, but error :%i\n",(UINT)GetLastError());
   ok(compare(pp,pp_compare),"Difference Found\n");

   pp_original.user.MaxSleepDc = PowerSystemWorking;
	memcpy(&pp, &pp_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, &pp_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(0,&pp);
   ok(ret, "function was expected to succeed, error %i\n",(UINT)GetLastError());
   pp_compare.mach.MinSleepAc=4;
   pp_compare.mach.MinSleepDc=4;
   pp_compare.user.MaxSleepAc=4;
   pp_compare.user.MaxSleepDc=4;
   pp_compare.user.OptimizeForPowerAc=1;
   pp_compare.user.OptimizeForPowerDc=1;
   ok(compare(pp,pp_compare),"Difference Found\n");

    memcpy(&gpp, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, &pp_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, &pp_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed, error %i\n",(UINT)GetLastError());
   gpp_compare.mach.BroadcastCapacityResolution=100;
   gpp_compare.user.DischargePolicy[1].PowerPolicy.EventCode = 65536;
   gpp_compare.user.DischargePolicy[2].PowerPolicy.EventCode = 131072;
   gpp_compare.user.DischargePolicy[3].PowerPolicy.EventCode = 196608;
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   pp_compare.mach.MinSleepAc=4;
   pp_compare.mach.MinSleepDc=4;
   pp_compare.user.MaxSleepAc=4;
   pp_compare.user.MaxSleepDc=4;
   pp_compare.user.OptimizeForPowerAc=1;
   pp_compare.user.OptimizeForPowerDc=1;
   ok(compare(pp,pp_compare),"Difference Found\n");


   gpp_original.mach.BroadcastCapacityResolution=95;
    memcpy(&gpp, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,0);
   ok(ret, "function was expected to succeed return %i\n",(UINT)GetLastError());
   gpp_compare.user.DischargePolicy[1].PowerPolicy.EventCode = 65536;
   gpp_compare.user.DischargePolicy[2].PowerPolicy.EventCode = 131072;
   gpp_compare.user.DischargePolicy[3].PowerPolicy.EventCode = 196608;
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");

   gpp_original.user.DischargePolicy[1].PowerPolicy.EventCode = 256;
    memcpy(&gpp, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,0);
   ok(ret, "function was expected to succeed return %i\n",(UINT)GetLastError());
   gpp_compare.user.DischargePolicy[1].PowerPolicy.EventCode = 65792;
   gpp_compare.user.DischargePolicy[2].PowerPolicy.EventCode = 131072;
   gpp_compare.user.DischargePolicy[3].PowerPolicy.EventCode = 196608;
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");

   gpp_original.user.DischargePolicy[2].PowerPolicy.EventCode = 256;
    memcpy(&gpp, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,0);
   ok(ret, "function was expected to succeed return %i\n",(UINT)GetLastError());
   gpp_compare.user.DischargePolicy[1].PowerPolicy.EventCode = 65792;
   gpp_compare.user.DischargePolicy[2].PowerPolicy.EventCode = 131328;
   gpp_compare.user.DischargePolicy[3].PowerPolicy.EventCode = 196608;
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");

   gpp_original.user.DischargePolicy[3].PowerPolicy.EventCode = 256;
    memcpy(&gpp, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,0);
   ok(ret, "function was expected to succeed return %i\n",(UINT)GetLastError());
   gpp_compare.user.DischargePolicy[1].PowerPolicy.EventCode = 65792;
   gpp_compare.user.DischargePolicy[2].PowerPolicy.EventCode = 131328;
   gpp_compare.user.DischargePolicy[3].PowerPolicy.EventCode = 196864;
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");


   gpp_original.user.DischargePolicy[1].PowerPolicy.EventCode = 65792;
   gpp_original.user.DischargePolicy[2].PowerPolicy.EventCode = 131328;
   gpp_original.user.DischargePolicy[3].PowerPolicy.EventCode = 196864;
   gpp_original.mach.LidOpenWakeAc=PowerSystemUnspecified;
    memcpy(&gpp, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,0);
   ok(!ret, "function was expected to succeed return %i\n",(UINT)GetLastError());
   ok(GetLastError() == ERROR_GEN_FAILURE,"function was expected to fail with ERROR_GEN_FAILURE, but error :%i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");

   gpp_original.mach.LidOpenWakeAc=PowerSystemWorking;
   gpp_original.mach.LidOpenWakeDc=PowerSystemUnspecified;
    memcpy(&gpp, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,0);
   ok(!ret, "function was expected to succeed return %i\n",(UINT)GetLastError());
   ok(GetLastError() == ERROR_GEN_FAILURE,"function was expected to fail with ERROR_GEN_FAILURE, but error :%i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");

   gpp_original.mach.LidOpenWakeDc=PowerSystemWorking;
   gpp_original.user.LidCloseAc.Action = PowerActionWarmEject+1;
    memcpy(&gpp, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,0);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_INVALID_DATA,"function was expected to fail with ERROR_INVALID_DATA, but error :%i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");

   gpp_original.user.LidCloseAc.Action = PowerActionWarmEject;
   gpp_original.user.LidCloseDc.Action = PowerActionNone;
    memcpy(&gpp, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,0);
   ok(ret, "function was expected to succeed return %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");

   gpp_original.user.LidCloseDc.Action = PowerActionWarmEject;
   gpp_original.user.PowerButtonAc.Action = PowerActionNone-1;
    memcpy(&gpp, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,0);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_INVALID_DATA,"function was expected to fail with ERROR_INVALID_DATA, but error :%i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");

   gpp_original.user.PowerButtonAc.Action = PowerActionNone;
   gpp_original.user.PowerButtonDc.Action = PowerActionWarmEject;
    memcpy(&gpp, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,0);
   ok(ret, "function was expected to succeed return %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");

   gpp_original.user.SleepButtonAc.Action = PowerActionWarmEject;
    memcpy(&gpp, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,0);
   ok(ret, "function was expected to succeed return %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");

   gpp_original.user.SleepButtonDc.Action = PowerActionWarmEject;
    memcpy(&gpp, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,0);
   ok(ret, "function was expected to succeed return %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");


   pp_original.mach.MinSleepAc=PowerSystemUnspecified-1;
    memcpy(&gpp, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, &pp_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, &pp_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(0,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   pp_compare.mach.MinSleepAc=4;
   pp_compare.mach.MinSleepDc=4;
   pp_compare.user.MaxSleepAc=4;
   pp_compare.user.MaxSleepDc=4;
   pp_compare.user.OptimizeForPowerAc=1;
   pp_compare.user.OptimizeForPowerDc=1;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pp_original.mach.MinSleepDc=PowerSystemUnspecified-1;
    memcpy(&gpp, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, &pp_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, &pp_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(0,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   pp_compare.mach.MinSleepAc=4;
   pp_compare.mach.MinSleepDc=4;
   pp_compare.user.MaxSleepAc=4;
   pp_compare.user.MaxSleepDc=4;
   pp_compare.user.OptimizeForPowerAc=1;
   pp_compare.user.OptimizeForPowerDc=1;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pp_original.user.MaxSleepAc=PowerSystemUnspecified-1;
    memcpy(&gpp, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, &pp_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, &pp_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(0,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   pp_compare.mach.MinSleepAc=4;
   pp_compare.mach.MinSleepDc=4;
   pp_compare.user.MaxSleepAc=4;
   pp_compare.user.MaxSleepDc=4;
   pp_compare.user.OptimizeForPowerAc=1;
   pp_compare.user.OptimizeForPowerDc=1;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pp_original.user.MaxSleepDc=PowerSystemUnspecified-1;
    memcpy(&gpp, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, &pp_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, &pp_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(0,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   pp_compare.mach.MinSleepAc=4;
   pp_compare.mach.MinSleepDc=4;
   pp_compare.user.MaxSleepAc=4;
   pp_compare.user.MaxSleepDc=4;
   pp_compare.user.OptimizeForPowerAc=1;
   pp_compare.user.OptimizeForPowerDc=1;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pp_original.mach.MinSleepAc=PowerSystemUnspecified;
    memcpy(&gpp, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, &pp_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, &pp_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(0,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   pp_compare.mach.MinSleepAc=4;
   pp_compare.mach.MinSleepDc=4;
   pp_compare.user.MaxSleepAc=4;
   pp_compare.user.MaxSleepDc=4;
   pp_compare.user.OptimizeForPowerAc=1;
   pp_compare.user.OptimizeForPowerDc=1;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pp_original.mach.MinSleepDc=PowerSystemUnspecified;
    memcpy(&gpp, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, &pp_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, &pp_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(0,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   pp_compare.mach.MinSleepAc=4;
   pp_compare.mach.MinSleepDc=4;
   pp_compare.user.MaxSleepAc=4;
   pp_compare.user.MaxSleepDc=4;
   pp_compare.user.OptimizeForPowerAc=1;
   pp_compare.user.OptimizeForPowerDc=1;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pp_original.user.MaxSleepAc=PowerSystemUnspecified;
    memcpy(&gpp, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, &pp_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, &pp_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(0,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_GEN_FAILURE,"function was expected to fail with ERROR_GEN_FAILURE, but error :%i\n",(UINT)GetLastError());
   ok(compare(pp,pp_compare),"Difference Found\n");

   pp_original.user.MaxSleepDc=PowerSystemUnspecified;
    memcpy(&gpp, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, &pp_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, &pp_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(0,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_GEN_FAILURE,"function was expected to fail with ERROR_GEN_FAILURE, but error :%i\n",(UINT)GetLastError());
   ok(compare(pp,pp_compare),"Difference Found\n");

   pp_original.mach.MinSleepAc=PowerSystemWorking;
    memcpy(&gpp, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, &pp_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, &pp_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(0,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_GEN_FAILURE,"function was expected to fail with ERROR_GEN_FAILURE, but error :%i\n",(UINT)GetLastError());
   //pp_compare.mach.MinSleepAc=4;
   //pp_compare.mach.MinSleepDc=4;
   //pp_compare.user.MaxSleepAc=4;
   //pp_compare.user.MaxSleepDc=4;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pp_original.mach.MinSleepDc=PowerSystemWorking;
    memcpy(&gpp, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, &pp_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, &pp_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(0,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_GEN_FAILURE,"function was expected to fail with ERROR_GEN_FAILURE, but error :%i\n",(UINT)GetLastError());
   //pp_compare.mach.MinSleepAc=4;
   //pp_compare.mach.MinSleepDc=4;
   //pp_compare.user.MaxSleepAc=4;
   //pp_compare.user.MaxSleepDc=4;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pp_original.user.MaxSleepAc=PowerSystemWorking;
    memcpy(&gpp, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, &pp_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, &pp_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(0,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_GEN_FAILURE,"function was expected to fail with ERROR_GEN_FAILURE, but error :%i\n",(UINT)GetLastError());
   //pp_compare.mach.MinSleepAc=4;
   //pp_compare.mach.MinSleepDc=4;
   //pp_compare.user.MaxSleepAc=4;
   //pp_compare.user.MaxSleepDc=4;
   //pp_compare.user.OptimizeForPowerAc=1;
   //pp_compare.user.OptimizeForPowerDc=1;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pp_original.user.MaxSleepDc=PowerSystemWorking;
    memcpy(&gpp, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, &pp_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, &pp_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(0,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   pp_compare.mach.MinSleepAc=4;
   pp_compare.mach.MinSleepDc=4;
   pp_compare.user.MaxSleepAc=4;
   pp_compare.user.MaxSleepDc=4;
   pp_compare.user.OptimizeForPowerAc=1;
   pp_compare.user.OptimizeForPowerDc=1;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pp_original.mach.MinSleepAc=PowerSystemSleeping1;
    memcpy(&gpp, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, &pp_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, &pp_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(0,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   pp_compare.mach.MinSleepAc=4;
   pp_compare.mach.MinSleepDc=4;
   pp_compare.user.MaxSleepAc=4;
   pp_compare.user.MaxSleepDc=4;
   pp_compare.user.OptimizeForPowerAc=1;
   pp_compare.user.OptimizeForPowerDc=1;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pp_original.mach.MinSleepDc=PowerSystemSleeping1;
    memcpy(&gpp, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, &pp_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, &pp_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(0,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   pp_compare.mach.MinSleepAc=4;
   pp_compare.mach.MinSleepDc=4;
   pp_compare.user.MaxSleepAc=4;
   pp_compare.user.MaxSleepDc=4;
   pp_compare.user.OptimizeForPowerAc=1;
   pp_compare.user.OptimizeForPowerDc=1;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pp_original.user.MaxSleepAc=PowerSystemSleeping1;
    memcpy(&gpp, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, &pp_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, &pp_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(0,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   pp_compare.mach.MinSleepAc=4;
   pp_compare.mach.MinSleepDc=4;
   pp_compare.user.MaxSleepAc=4;
   pp_compare.user.MaxSleepDc=4;
   pp_compare.user.OptimizeForPowerAc=1;
   pp_compare.user.OptimizeForPowerDc=1;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pp_original.user.MaxSleepDc=PowerSystemSleeping1;
    memcpy(&gpp, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, &pp_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, &pp_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(0,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   pp_compare.mach.MinSleepAc=4;
   pp_compare.mach.MinSleepDc=4;
   pp_compare.user.MaxSleepAc=4;
   pp_compare.user.MaxSleepDc=4;
   pp_compare.user.OptimizeForPowerAc=1;
   pp_compare.user.OptimizeForPowerDc=1;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pp_original.mach.MinSleepAc=PowerSystemSleeping2;
    memcpy(&gpp, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, &pp_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, &pp_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(0,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   pp_compare.mach.MinSleepAc=4;
   pp_compare.mach.MinSleepDc=4;
   pp_compare.user.MaxSleepAc=4;
   pp_compare.user.MaxSleepDc=4;
   pp_compare.user.OptimizeForPowerAc=1;
   pp_compare.user.OptimizeForPowerDc=1;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pp_original.mach.MinSleepDc=PowerSystemSleeping2;
    memcpy(&gpp, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, &pp_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, &pp_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(0,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   pp_compare.mach.MinSleepAc=4;
   pp_compare.mach.MinSleepDc=4;
   pp_compare.user.MaxSleepAc=4;
   pp_compare.user.MaxSleepDc=4;
   pp_compare.user.OptimizeForPowerAc=1;
   pp_compare.user.OptimizeForPowerDc=1;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pp_original.user.MaxSleepAc=PowerSystemSleeping2;
    memcpy(&gpp, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, &pp_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, &pp_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(0,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   pp_compare.mach.MinSleepAc=4;
   pp_compare.mach.MinSleepDc=4;
   pp_compare.user.MaxSleepAc=4;
   pp_compare.user.MaxSleepDc=4;
   pp_compare.user.OptimizeForPowerAc=1;
   pp_compare.user.OptimizeForPowerDc=1;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pp_original.user.MaxSleepDc=PowerSystemSleeping2;
    memcpy(&gpp, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, &pp_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, &pp_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(0,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   pp_compare.mach.MinSleepAc=4;
   pp_compare.mach.MinSleepDc=4;
   pp_compare.user.MaxSleepAc=4;
   pp_compare.user.MaxSleepDc=4;
   pp_compare.user.OptimizeForPowerAc=1;
   pp_compare.user.OptimizeForPowerDc=1;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pp_original.mach.MinSleepAc=PowerSystemSleeping3;
    memcpy(&gpp, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, &pp_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, &pp_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(0,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   pp_compare.mach.MinSleepDc=4;
   pp_compare.user.MaxSleepAc=4;
   pp_compare.user.MaxSleepDc=4;
   pp_compare.user.OptimizeForPowerAc=1;
   pp_compare.user.OptimizeForPowerDc=1;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pp_original.mach.MinSleepDc=PowerSystemSleeping3;
    memcpy(&gpp, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, &pp_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, &pp_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(0,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   pp_compare.user.MaxSleepAc=4;
   pp_compare.user.MaxSleepDc=4;
   pp_compare.user.OptimizeForPowerAc=1;
   pp_compare.user.OptimizeForPowerDc=1;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pp_original.user.MaxSleepAc=PowerSystemSleeping3;
    memcpy(&gpp, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, &pp_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, &pp_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(0,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   pp_compare.user.MaxSleepDc=4;
   pp_compare.user.OptimizeForPowerAc=1;
   pp_compare.user.OptimizeForPowerDc=1;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pp_original.user.MaxSleepDc=PowerSystemSleeping3;
    memcpy(&gpp, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, &pp_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, &pp_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(0,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   pp_compare.user.OptimizeForPowerAc=1;
   pp_compare.user.OptimizeForPowerDc=1;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pp_original.mach.MinSleepAc=PowerSystemHibernate;
    memcpy(&gpp, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, &pp_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, &pp_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(0,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   pp_compare.mach.MinSleepAc=4;
   pp_compare.mach.MinSleepDc=4;
   pp_compare.user.MaxSleepAc=4;
   pp_compare.user.MaxSleepDc=4;
   pp_compare.user.OptimizeForPowerAc=1;
   pp_compare.user.OptimizeForPowerDc=1;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pp_original.mach.MinSleepDc=PowerSystemHibernate;
    memcpy(&gpp, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, &pp_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, &pp_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(0,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   pp_compare.mach.MinSleepAc=4;
   pp_compare.mach.MinSleepDc=4;
   pp_compare.user.MaxSleepAc=4;
   pp_compare.user.MaxSleepDc=4;
   pp_compare.user.OptimizeForPowerAc=1;
   pp_compare.user.OptimizeForPowerDc=1;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pp_original.user.MaxSleepAc=PowerSystemHibernate;
    memcpy(&gpp, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, &pp_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, &pp_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(0,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   pp_compare.mach.MinSleepAc=4;
   pp_compare.mach.MinSleepDc=4;
   pp_compare.user.MaxSleepAc=4;
   pp_compare.user.MaxSleepDc=4;
   pp_compare.user.OptimizeForPowerAc=1;
   pp_compare.user.OptimizeForPowerDc=1;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pp_original.user.MaxSleepDc=PowerSystemHibernate;
    memcpy(&gpp, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, &pp_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, &pp_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(0,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   pp_compare.mach.MinSleepAc=4;
   pp_compare.mach.MinSleepDc=4;
   pp_compare.user.MaxSleepAc=4;
   pp_compare.user.MaxSleepDc=4;
   pp_compare.user.OptimizeForPowerAc=1;
   pp_compare.user.OptimizeForPowerDc=1;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pp_original.mach.MinSleepAc=PowerSystemShutdown;
    memcpy(&gpp, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, &pp_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, &pp_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(0,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_GEN_FAILURE,"function was expected to fail with ERROR_GEN_FAILURE, but error :%i\n",(UINT)GetLastError());
   ok(compare(pp,pp_compare),"Difference Found\n");

   pp_original.mach.MinSleepDc=PowerSystemShutdown;
    memcpy(&gpp, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, &pp_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, &pp_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(0,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_GEN_FAILURE,"function was expected to fail with ERROR_GEN_FAILURE, but error :%i\n",(UINT)GetLastError());
   ok(compare(pp,pp_compare),"Difference Found\n");

   pp_original.user.MaxSleepAc=PowerSystemShutdown;
    memcpy(&gpp, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, &pp_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, &pp_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(0,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_GEN_FAILURE,"function was expected to fail with ERROR_GEN_FAILURE, but error :%i\n",(UINT)GetLastError());
   ok(compare(pp,pp_compare),"Difference Found\n");

   pp_original.user.MaxSleepDc=PowerSystemShutdown;
    memcpy(&gpp, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, &pp_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, &pp_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(0,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_GEN_FAILURE,"function was expected to fail with ERROR_GEN_FAILURE, but error :%i\n",(UINT)GetLastError());
   ok(compare(pp,pp_compare),"Difference Found\n");

   pp_original.mach.MinSleepAc=PowerSystemMaximum;
    memcpy(&gpp, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, &pp_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, &pp_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(0,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_GEN_FAILURE,"function was expected to fail with ERROR_GEN_FAILURE, but error :%i\n",(UINT)GetLastError());
   ok(compare(pp,pp_compare),"Difference Found\n");

   pp_original.mach.MinSleepDc=PowerSystemMaximum;
    memcpy(&gpp, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, &pp_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, &pp_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(0,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_GEN_FAILURE,"function was expected to fail with ERROR_GEN_FAILURE, but error :%i\n",(UINT)GetLastError());
   ok(compare(pp,pp_compare),"Difference Found\n");

   pp_original.user.MaxSleepAc=PowerSystemMaximum;
    memcpy(&gpp, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, &pp_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, &pp_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(0,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_GEN_FAILURE,"function was expected to fail with ERROR_GEN_FAILURE, but error :%i\n",(UINT)GetLastError());
   ok(compare(pp,pp_compare),"Difference Found\n");

   pp_original.user.MaxSleepDc=PowerSystemMaximum;
    memcpy(&gpp, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, &pp_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, &pp_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(0,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_GEN_FAILURE,"function was expected to fail with ERROR_GEN_FAILURE, but error :%i\n",(UINT)GetLastError());
   ok(compare(pp,pp_compare),"Difference Found\n");

   pp_original.mach.MinSleepAc=PowerSystemMaximum+1;
    memcpy(&gpp, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, &pp_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, &pp_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(0,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_GEN_FAILURE,"function was expected to fail with ERROR_GEN_FAILURE, but error :%i\n",(UINT)GetLastError());
   ok(compare(pp,pp_compare),"Difference Found\n");

   pp_original.mach.MinSleepDc=PowerSystemMaximum+1;
    memcpy(&gpp, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, &pp_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, &pp_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(0,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_GEN_FAILURE,"function was expected to fail with ERROR_GEN_FAILURE, but error :%i\n",(UINT)GetLastError());
   ok(compare(pp,pp_compare),"Difference Found\n");

   pp_original.user.MaxSleepAc=PowerSystemMaximum+1;
    memcpy(&gpp, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, &pp_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, &pp_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(0,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_GEN_FAILURE,"function was expected to fail with ERROR_GEN_FAILURE, but error :%i\n",(UINT)GetLastError());
   ok(compare(pp,pp_compare),"Difference Found\n");

   pp_original.user.MaxSleepDc=PowerSystemMaximum+1;
    memcpy(&gpp, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, &pp_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, &pp_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(0,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_GEN_FAILURE,"function was expected to fail with ERROR_GEN_FAILURE, but error :%i\n",(UINT)GetLastError());
   ok(compare(pp,pp_compare),"Difference Found\n");

   pp_original.mach.MinSleepAc=PowerSystemWorking;
   pp_original.mach.MinSleepDc=PowerSystemWorking;
   pp_original.user.MaxSleepAc=PowerSystemWorking;
   pp_original.user.MaxSleepDc=PowerSystemWorking;


   test_ValidatePowerPolicies_Next(&gpp_original,&pp_original);


 //   memcpy(&gpp, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
 //   memcpy(&gpp_compare, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
	//memcpy(&pp, &pp_original, sizeof(POWER_POLICY));
 //   memcpy(&pp_compare, &pp_original, sizeof(POWER_POLICY));
   ret = GetCurrentPowerPolicies(&gpp_original,&pp_original);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   //gpp_compare.mach.BroadcastCapacityResolution = 3;
   //gpp_compare.user.PowerButtonAc.Action = 2;
   //gpp_compare.user.PowerButtonAc.Flags=3;
   //gpp_compare.user.PowerButtonDc.EventCode=16;
   //gpp_compare.user.PowerButtonDc.Flags=3;
   //gpp_compare.user.SleepButtonAc.Action=2;
   //gpp_compare.user.SleepButtonAc.Flags=3;
   //gpp_compare.user.SleepButtonDc.Action=2;
   //gpp_compare.user.SleepButtonDc.Flags=3;
   //gpp_compare.user.LidCloseAc.EventCode=-2147483648;
   //gpp_compare.user.LidCloseAc.Flags=1;
   //gpp_compare.user.LidCloseDc.EventCode=-2147483648;
   //gpp_compare.user.LidCloseDc.Flags=1;

   //gpp_compare.user.DischargePolicy[0].Enable=1;
   ////gpp_compare.user.DischargePolicy[0].Spare[0]=3;
   //gpp_compare.user.DischargePolicy[0].Spare[3]=3;
   //gpp_compare.user.DischargePolicy[0].BatteryLevel=3;
   //gpp_compare.user.DischargePolicy[0].PowerPolicy.Action=2;
   //gpp_compare.user.DischargePolicy[0].PowerPolicy.Flags=-1073741820;
   //gpp_compare.user.DischargePolicy[0].PowerPolicy.EventCode=1;
   //gpp_compare.user.DischargePolicy[0].MinSystemState=4;

   //gpp_compare.user.DischargePolicy[1].Enable=1;
   ////gpp_compare.user.DischargePolicy[1].Spare[0]=3;
   //gpp_compare.user.DischargePolicy[1].Spare[3]=10;
   //gpp_compare.user.DischargePolicy[1].BatteryLevel=10;
   ////gpp_compare.user.DischargePolicy[1].PowerPolicy.Action=3;
   //gpp_compare.user.DischargePolicy[1].PowerPolicy.Flags=3;
   //gpp_compare.user.DischargePolicy[1].PowerPolicy.EventCode=65537;
   //gpp_compare.user.DischargePolicy[1].MinSystemState=1;
   //
   //gpp_compare.user.DischargePolicy[2].PowerPolicy.EventCode=131072;
   //gpp_compare.user.DischargePolicy[3].PowerPolicy.EventCode=196608;
   //gpp_compare.user.GlobalFlags=20;

   //ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   //pp_compare.mach.MinSleepAc=4;
   //pp_compare.mach.MinSleepDc=4;
   //pp_compare.mach.ReducedLatencySleepAc=4;
   //pp_compare.mach.ReducedLatencySleepDc=4;
   //pp_compare.mach.OverThrottledAc.Action=2;
   //pp_compare.mach.OverThrottledAc.Flags=-1073741820;
   //pp_compare.mach.OverThrottledDc.Action=2;
   //pp_compare.mach.OverThrottledDc.Flags=-1073741820;
   //pp_compare.mach.pad1[2]=2;
   //pp_compare.user.IdleAc.Flags=1;
   //pp_compare.user.IdleDc.Flags=1;
   //pp_compare.user.IdleSensitivityAc=50;
   //pp_compare.user.IdleSensitivityDc=50;
   //pp_compare.user.ThrottlePolicyAc=3;
   //pp_compare.user.ThrottlePolicyDc=3;
   //pp_compare.user.Reserved[2]=1200;
   //pp_compare.user.VideoTimeoutAc=1200;
   //pp_compare.user.VideoTimeoutDc=600;
   //pp_compare.user.SpindownTimeoutAc=2700;
   //pp_compare.user.SpindownTimeoutDc=600;
   //pp_compare.user.FanThrottleToleranceAc=100;
   //pp_compare.user.FanThrottleToleranceDc=80;
   //pp_compare.user.ForcedThrottleAc=100;
   //pp_compare.user.ForcedThrottleDc=100;
   //pp_compare.user.MaxSleepAc=4;
   //pp_compare.user.MaxSleepDc=4;
   //pp_compare.user.OptimizeForPowerAc=1;
   //pp_compare.user.OptimizeForPowerDc=1;
   //ok(compare(pp,pp_compare),"Difference Found\n");

    memcpy(&gpp, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, &pp_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, &pp_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,0);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   //gpp_compare.mach.BroadcastCapacityResolution=100;
   //gpp_compare.user.DischargePolicy[1].PowerPolicy.EventCode = 65536;
   //gpp_compare.user.DischargePolicy[2].PowerPolicy.EventCode = 131072;
   //gpp_compare.user.DischargePolicy[3].PowerPolicy.EventCode = 196608;
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");

    memcpy(&gpp, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, &gpp_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, &pp_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, &pp_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(0,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   //pp_compare.mach.MinSleepAc=4;
   //pp_compare.mach.MinSleepDc=4;
   //pp_compare.user.MaxSleepAc=4;
   //pp_compare.user.MaxSleepDc=4;
   //pp_compare.user.OptimizeForPowerAc=1;
   //pp_compare.user.OptimizeForPowerDc=1;
   ok(compare(pp,pp_compare),"Difference Found\n");

	memcpy(&pp, &pp_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, &pp_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   //gpp_compare.mach.BroadcastCapacityResolution=100;
   //gpp_compare.user.DischargePolicy[1].PowerPolicy.EventCode = 65536;
   //gpp_compare.user.DischargePolicy[2].PowerPolicy.EventCode = 131072;
   //gpp_compare.user.DischargePolicy[3].PowerPolicy.EventCode = 196608;
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   pp_compare.mach.MinSleepAc=4;
   pp_compare.mach.MinSleepDc=4;
   pp_compare.user.MaxSleepAc=4;
   pp_compare.user.MaxSleepDc=4;
   //pp_compare.user.OptimizeForPowerAc=1;
   //pp_compare.user.OptimizeForPowerDc=1;
   ok(compare(pp,pp_compare),"Difference Found\n");

}

void test_ValidatePowerPolicies_Next(PGLOBAL_POWER_POLICY pGPP_original,PPOWER_POLICY pPP_original)
{
   GLOBAL_POWER_POLICY gpp, gpp_compare;
   POWER_POLICY pp, pp_compare;
   BOOLEAN ret;

   //printf("Old: %i\n",pGPP_original->mach.LidOpenWakeAc);//1
   //printf("Old: %i\n",pGPP_original->mach.LidOpenWakeDc);//1
   //printf("Old: %i,%i,%i \n",pGPP_original->user.LidCloseAc.Action,pGPP_original->user.LidCloseAc.EventCode,pGPP_original->user.LidCloseAc.Flags);//7
   //printf("Old: %i,%i,%i \n",pGPP_original->user.LidCloseDc.Action,pGPP_original->user.LidCloseDc.EventCode,pGPP_original->user.LidCloseDc.Flags);//7
   //printf("Old: %i,%i,%i \n",pGPP_original->user.PowerButtonAc.Action,pGPP_original->user.PowerButtonAc.EventCode,pGPP_original->user.PowerButtonAc.Flags);//0,0,0
   //printf("Old: %i,%i,%i \n",pGPP_original->user.PowerButtonDc.Action,pGPP_original->user.PowerButtonDc.EventCode,pGPP_original->user.PowerButtonDc.Flags);//7,0,0
   //printf("Old: %i,%i,%i \n",pGPP_original->user.SleepButtonAc.Action,pGPP_original->user.SleepButtonAc.EventCode,pGPP_original->user.SleepButtonAc.Flags);//7,0,0
   //printf("Old: %i,%i,%i \n",pGPP_original->user.SleepButtonDc.Action,pGPP_original->user.SleepButtonDc.EventCode,pGPP_original->user.SleepButtonDc.Flags);//7,0,0
   //printf("Old: %i \n",pPP_original->mach.DozeS4TimeoutAc);//0
   //printf("Old: %i \n",pPP_original->mach.DozeS4TimeoutDc);//0
   //printf("Old: %i \n",pPP_original->mach.DozeTimeoutAc);//0
   //printf("Old: %i \n",pPP_original->mach.DozeTimeoutDc);//0
   //printf("Old: %i \n",pPP_original->mach.MinSleepAc);//1
   //printf("Old: %i \n",pPP_original->mach.MinSleepDc);//1
   //printf("Old: %i \n",pPP_original->mach.MinThrottleAc);//0
   //printf("Old: %i \n",pPP_original->mach.MinThrottleDc);//0
   //printf("Old: %i,%i,%i \n",pPP_original->mach.OverThrottledAc.Action,pPP_original->mach.OverThrottledAc.EventCode,pPP_original->mach.OverThrottledAc.Flags);//0,0,0
   //printf("Old: %i,%i,%i \n",pPP_original->mach.OverThrottledDc.Action,pPP_original->mach.OverThrottledDc.EventCode,pPP_original->mach.OverThrottledDc.Flags);//0,0,0
   //printf("Old: %i \n",pPP_original->mach.ReducedLatencySleepAc);//1
   //printf("Old: %i \n",pPP_original->mach.ReducedLatencySleepDc);//1
   //printf("Old: %i \n",pPP_original->user.FanThrottleToleranceAc);//0
   //printf("Old: %i \n",pPP_original->user.FanThrottleToleranceDc);//0
   //printf("Old: %i \n",pPP_original->user.ForcedThrottleAc);//0
   //printf("Old: %i \n",pPP_original->user.ForcedThrottleDc);//0
   //printf("Old: %i,%i,%i \n",pPP_original->user.IdleAc.Action,pPP_original->user.IdleAc.EventCode,pPP_original->user.IdleAc.Flags);//0,0,0
   //printf("Old: %i,%i,%i \n",pPP_original->user.IdleDc.Action,pPP_original->user.IdleDc.EventCode,pPP_original->user.IdleDc.Flags);//0,0,0
   //printf("Old: %i \n",pPP_original->user.IdleSensitivityAc);//0
   //printf("Old: %i \n",pPP_original->user.IdleSensitivityDc);//0
   //printf("Old: %i \n",pPP_original->user.IdleTimeoutAc);//0
   //printf("Old: %i \n",pPP_original->user.IdleTimeoutDc);//0
   //printf("Old: %i \n",pPP_original->user.MaxSleepAc);//1
   //printf("Old: %i \n",pPP_original->user.MaxSleepDc);//1
   //printf("Old: %i \n",pPP_original->user.OptimizeForPowerAc);//0
   //printf("Old: %i \n",pPP_original->user.OptimizeForPowerDc);//0
   //printf("Old: %i \n",pPP_original->user.SpindownTimeoutAc);//0
   //printf("Old: %i \n",pPP_original->user.SpindownTimeoutDc);//0
   //printf("Old: %i \n",pPP_original->user.ThrottlePolicyAc);//0
   //printf("Old: %i \n",pPP_original->user.ThrottlePolicyDc);//0
   //printf("Old: %i \n",pPP_original->user.VideoTimeoutAc);//0
   //printf("Old: %i \n",pPP_original->user.VideoTimeoutDc);//0

   pPP_original->mach.MinSleepAc=4;
pPP_original->mach.MinSleepDc=4;
pPP_original->user.MaxSleepAc=4;
pPP_original->user.MaxSleepDc=4;
pPP_original->user.OptimizeForPowerAc=1;
pPP_original->user.OptimizeForPowerDc=1;

    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");

	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ok(compare(pp,pp_compare),"Difference Found\n");

   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->mach.LidOpenWakeAc=PowerSystemUnspecified-2;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->mach.LidOpenWakeAc=PowerSystemUnspecified-1;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->mach.LidOpenWakeAc=PowerSystemUnspecified;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_GEN_FAILURE, "expected ERROR_GEN_FAILURE but got %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->mach.LidOpenWakeAc=PowerSystemWorking;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->mach.LidOpenWakeAc=PowerSystemSleeping1;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   gpp_compare.mach.LidOpenWakeAc=4;
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->mach.LidOpenWakeAc=PowerSystemSleeping2;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   gpp_compare.mach.LidOpenWakeAc=4;
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->mach.LidOpenWakeAc=PowerSystemSleeping3;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->mach.LidOpenWakeAc=PowerSystemHibernate;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   gpp_compare.mach.LidOpenWakeAc=4;
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->mach.LidOpenWakeAc=PowerSystemShutdown;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_GEN_FAILURE, "expected ERROR_GEN_FAILURE but got %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->mach.LidOpenWakeAc=PowerSystemMaximum;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_GEN_FAILURE, "expected ERROR_GEN_FAILURE but got %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->mach.LidOpenWakeAc=PowerSystemMaximum+1;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_GEN_FAILURE, "expected ERROR_GEN_FAILURE but got %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->mach.LidOpenWakeAc=PowerSystemMaximum+2;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_GEN_FAILURE, "expected ERROR_GEN_FAILURE but got %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");
   pGPP_original->mach.LidOpenWakeAc=PowerSystemWorking;


   pGPP_original->mach.LidOpenWakeDc=PowerSystemUnspecified-2;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->mach.LidOpenWakeDc=PowerSystemUnspecified-1;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->mach.LidOpenWakeDc=PowerSystemUnspecified;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_GEN_FAILURE, "expected ERROR_GEN_FAILURE but got %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->mach.LidOpenWakeDc=PowerSystemWorking;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->mach.LidOpenWakeDc=PowerSystemSleeping1;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   gpp_compare.mach.LidOpenWakeDc=4;
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->mach.LidOpenWakeDc=PowerSystemSleeping2;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   gpp_compare.mach.LidOpenWakeDc=4;
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->mach.LidOpenWakeDc=PowerSystemSleeping3;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->mach.LidOpenWakeDc=PowerSystemHibernate;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   gpp_compare.mach.LidOpenWakeDc=4;
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->mach.LidOpenWakeDc=PowerSystemShutdown;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_GEN_FAILURE, "expected ERROR_GEN_FAILURE but got %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->mach.LidOpenWakeDc=PowerSystemMaximum;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_GEN_FAILURE, "expected ERROR_GEN_FAILURE but got %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->mach.LidOpenWakeDc=PowerSystemMaximum+1;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_GEN_FAILURE, "expected ERROR_GEN_FAILURE but got %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->mach.LidOpenWakeDc=PowerSystemMaximum+2;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_GEN_FAILURE, "expected ERROR_GEN_FAILURE but got %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");
   pGPP_original->mach.LidOpenWakeDc=PowerSystemWorking;

   pGPP_original->user.LidCloseAc.Action=PowerActionNone-2;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_INVALID_DATA, "expected ERROR_INVALID_DATA but got %x\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->user.LidCloseAc.Action=PowerActionNone-1;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_INVALID_DATA, "expected ERROR_INVALID_DATA but got %x\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->user.LidCloseAc.Action=PowerActionNone;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->user.LidCloseAc.Action=PowerActionReserved;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   gpp_compare.user.LidCloseAc.Action=2;
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->user.LidCloseAc.Action=PowerActionSleep;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->user.LidCloseAc.Action=PowerActionHibernate;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   gpp_compare.user.LidCloseAc.Action=2;
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->user.LidCloseAc.Action=PowerActionShutdown;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->user.LidCloseAc.Action=PowerActionShutdownReset;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->user.LidCloseAc.Action=PowerActionShutdownOff;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->user.LidCloseAc.Action=PowerActionWarmEject;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->user.LidCloseAc.Action=PowerActionWarmEject+1;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_INVALID_DATA, "expected ERROR_INVALID_DATA but got %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->user.LidCloseAc.Action=PowerActionWarmEject+2;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_INVALID_DATA, "expected ERROR_INVALID_DATA but got %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");
  pGPP_original->user.LidCloseAc.Action=PowerActionWarmEject;


   pGPP_original->user.LidCloseDc.Action=PowerActionNone-2;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_INVALID_DATA, "expected ERROR_INVALID_DATA but got %x\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->user.LidCloseDc.Action=PowerActionNone-1;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_INVALID_DATA, "expected ERROR_INVALID_DATA but got %x\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->user.LidCloseDc.Action=PowerActionNone;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->user.LidCloseDc.Action=PowerActionReserved;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   gpp_compare.user.LidCloseDc.Action=2;
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->user.LidCloseDc.Action=PowerActionSleep;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->user.LidCloseDc.Action=PowerActionHibernate;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   gpp_compare.user.LidCloseDc.Action=2;
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->user.LidCloseDc.Action=PowerActionShutdown;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->user.LidCloseDc.Action=PowerActionShutdownReset;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->user.LidCloseDc.Action=PowerActionShutdownOff;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->user.LidCloseDc.Action=PowerActionWarmEject;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->user.LidCloseDc.Action=PowerActionWarmEject+1;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_INVALID_DATA, "expected ERROR_INVALID_DATA but got %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->user.LidCloseDc.Action=PowerActionWarmEject+2;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_INVALID_DATA, "expected ERROR_INVALID_DATA but got %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");
  pGPP_original->user.LidCloseDc.Action=PowerActionWarmEject;


   pGPP_original->user.PowerButtonAc.Action=PowerActionNone-2;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_INVALID_DATA, "expected ERROR_INVALID_DATA but got %x\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->user.PowerButtonAc.Action=PowerActionNone-1;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_INVALID_DATA, "expected ERROR_INVALID_DATA but got %x\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->user.PowerButtonAc.Action=PowerActionNone;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->user.PowerButtonAc.Action=PowerActionReserved;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   gpp_compare.user.PowerButtonAc.Action=2;
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->user.PowerButtonAc.Action=PowerActionSleep;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->user.PowerButtonAc.Action=PowerActionHibernate;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   gpp_compare.user.PowerButtonAc.Action=2;
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->user.PowerButtonAc.Action=PowerActionShutdown;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->user.PowerButtonAc.Action=PowerActionShutdownReset;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->user.PowerButtonAc.Action=PowerActionShutdownOff;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->user.PowerButtonAc.Action=PowerActionWarmEject;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->user.PowerButtonAc.Action=PowerActionWarmEject+1;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_INVALID_DATA, "expected ERROR_INVALID_DATA but got %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->user.PowerButtonAc.Action=PowerActionWarmEject+2;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_INVALID_DATA, "expected ERROR_INVALID_DATA but got %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");
  pGPP_original->user.PowerButtonAc.Action=PowerActionNone;


   pGPP_original->user.PowerButtonDc.Action=PowerActionNone-2;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_INVALID_DATA, "expected ERROR_INVALID_DATA but got %x\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->user.PowerButtonDc.Action=PowerActionNone-1;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_INVALID_DATA, "expected ERROR_INVALID_DATA but got %x\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->user.PowerButtonDc.Action=PowerActionNone;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->user.PowerButtonDc.Action=PowerActionReserved;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   gpp_compare.user.PowerButtonDc.Action=2;
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->user.PowerButtonDc.Action=PowerActionSleep;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->user.PowerButtonDc.Action=PowerActionHibernate;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   gpp_compare.user.PowerButtonDc.Action=2;
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->user.PowerButtonDc.Action=PowerActionShutdown;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->user.PowerButtonDc.Action=PowerActionShutdownReset;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->user.PowerButtonDc.Action=PowerActionShutdownOff;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->user.PowerButtonDc.Action=PowerActionWarmEject;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->user.PowerButtonDc.Action=PowerActionWarmEject+1;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_INVALID_DATA, "expected ERROR_INVALID_DATA but got %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->user.PowerButtonDc.Action=PowerActionWarmEject+2;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_INVALID_DATA, "expected ERROR_INVALID_DATA but got %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");
  pGPP_original->user.PowerButtonDc.Action=PowerActionWarmEject;


   pGPP_original->user.SleepButtonAc.Action=PowerActionNone-2;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_INVALID_DATA, "expected ERROR_INVALID_DATA but got %x\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->user.SleepButtonAc.Action=PowerActionNone-1;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_INVALID_DATA, "expected ERROR_INVALID_DATA but got %x\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->user.SleepButtonAc.Action=PowerActionNone;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->user.SleepButtonAc.Action=PowerActionReserved;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   gpp_compare.user.SleepButtonAc.Action=2;
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->user.SleepButtonAc.Action=PowerActionSleep;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->user.SleepButtonAc.Action=PowerActionHibernate;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   gpp_compare.user.SleepButtonAc.Action=2;
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->user.SleepButtonAc.Action=PowerActionShutdown;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->user.SleepButtonAc.Action=PowerActionShutdownReset;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->user.SleepButtonAc.Action=PowerActionShutdownOff;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->user.SleepButtonAc.Action=PowerActionWarmEject;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->user.SleepButtonAc.Action=PowerActionWarmEject+1;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_INVALID_DATA, "expected ERROR_INVALID_DATA but got %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->user.SleepButtonAc.Action=PowerActionWarmEject+2;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_INVALID_DATA, "expected ERROR_INVALID_DATA but got %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");
  pGPP_original->user.SleepButtonAc.Action=PowerActionWarmEject;


   pGPP_original->user.SleepButtonDc.Action=PowerActionNone-2;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_INVALID_DATA, "expected ERROR_INVALID_DATA but got %x\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->user.SleepButtonDc.Action=PowerActionNone-1;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_INVALID_DATA, "expected ERROR_INVALID_DATA but got %x\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->user.SleepButtonDc.Action=PowerActionNone;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->user.SleepButtonDc.Action=PowerActionReserved;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   gpp_compare.user.SleepButtonDc.Action=2;
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->user.SleepButtonDc.Action=PowerActionSleep;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->user.SleepButtonDc.Action=PowerActionHibernate;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   gpp_compare.user.SleepButtonDc.Action=2;
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->user.SleepButtonDc.Action=PowerActionShutdown;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->user.SleepButtonDc.Action=PowerActionShutdownReset;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->user.SleepButtonDc.Action=PowerActionShutdownOff;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->user.SleepButtonDc.Action=PowerActionWarmEject;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->user.SleepButtonDc.Action=PowerActionWarmEject+1;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_INVALID_DATA, "expected ERROR_INVALID_DATA but got %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pGPP_original->user.SleepButtonDc.Action=PowerActionWarmEject+2;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_INVALID_DATA, "expected ERROR_INVALID_DATA but got %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");
  pGPP_original->user.SleepButtonDc.Action=PowerActionWarmEject;


   pPP_original->mach.MinSleepAc=PowerSystemUnspecified-2;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   pp_compare.mach.MinSleepAc=4;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->mach.MinSleepAc=PowerSystemUnspecified-1;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   pp_compare.mach.MinSleepAc=4;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->mach.MinSleepAc=PowerSystemUnspecified;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   pp_compare.mach.MinSleepAc=4;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->mach.MinSleepAc=PowerSystemWorking;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   pp_compare.mach.MinSleepAc=4;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->mach.MinSleepAc=PowerSystemSleeping1;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   pp_compare.mach.MinSleepAc=4;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->mach.MinSleepAc=PowerSystemSleeping2;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   pp_compare.mach.MinSleepAc=4;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->mach.MinSleepAc=PowerSystemSleeping3;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->mach.MinSleepAc=PowerSystemHibernate;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   pp_compare.mach.MinSleepAc=4;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->mach.MinSleepAc=PowerSystemShutdown;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_GEN_FAILURE, "expected ERROR_GEN_FAILURE but got %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->mach.MinSleepAc=PowerSystemMaximum;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_GEN_FAILURE, "expected ERROR_GEN_FAILURE but got %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->mach.MinSleepAc=PowerSystemMaximum+1;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_GEN_FAILURE, "expected ERROR_GEN_FAILURE but got %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->mach.MinSleepAc=PowerSystemMaximum+2;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_GEN_FAILURE, "expected ERROR_GEN_FAILURE but got %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");
  pPP_original->mach.MinSleepAc=PowerSystemSleeping3;


   pPP_original->mach.MinSleepDc=PowerSystemUnspecified-2;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   pp_compare.mach.MinSleepDc=4;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->mach.MinSleepDc=PowerSystemUnspecified-1;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   pp_compare.mach.MinSleepDc=4;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->mach.MinSleepDc=PowerSystemUnspecified;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   pp_compare.mach.MinSleepDc=4;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->mach.MinSleepDc=PowerSystemWorking;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   pp_compare.mach.MinSleepDc=4;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->mach.MinSleepDc=PowerSystemSleeping1;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   pp_compare.mach.MinSleepDc=4;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->mach.MinSleepDc=PowerSystemSleeping2;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   pp_compare.mach.MinSleepDc=4;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->mach.MinSleepDc=PowerSystemSleeping3;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->mach.MinSleepDc=PowerSystemHibernate;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   pp_compare.mach.MinSleepDc=4;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->mach.MinSleepDc=PowerSystemShutdown;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_GEN_FAILURE, "expected ERROR_GEN_FAILURE but got %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->mach.MinSleepDc=PowerSystemMaximum;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_GEN_FAILURE, "expected ERROR_GEN_FAILURE but got %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->mach.MinSleepDc=PowerSystemMaximum+1;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_GEN_FAILURE, "expected ERROR_GEN_FAILURE but got %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->mach.MinSleepDc=PowerSystemMaximum+2;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_GEN_FAILURE, "expected ERROR_GEN_FAILURE but got %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");
  pPP_original->mach.MinSleepDc=PowerSystemSleeping3;


   pPP_original->mach.OverThrottledAc.Action=PowerActionNone-2;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_INVALID_DATA, "expected ERROR_INVALID_DATA but got %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   //pp_compare.mach.OverThrottledAc.Action=4;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->mach.OverThrottledAc.Action=PowerActionNone-1;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_INVALID_DATA, "expected ERROR_INVALID_DATA but got %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   //pp_compare.mach.OverThrottledAc.Action=4;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->mach.OverThrottledAc.Action=PowerActionNone;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   //pp_compare.mach.OverThrottledAc.Action=4;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->mach.OverThrottledAc.Action=PowerActionReserved;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   //pp_compare.mach.OverThrottledAc.Action=4;
   pp_compare.mach.OverThrottledAc.Action=2;
   pp_compare.mach.pad1[2]=2;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->mach.OverThrottledAc.Action=PowerActionSleep;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   //pp_compare.mach.OverThrottledAc.Action=4;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->mach.OverThrottledAc.Action=PowerActionHibernate;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   pp_compare.mach.OverThrottledAc.Action=2;
   pp_compare.mach.pad1[2]=2;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->mach.OverThrottledAc.Action=PowerActionShutdown;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->mach.OverThrottledAc.Action=PowerActionShutdownReset;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   //pp_compare.mach.OverThrottledAc.Action=4;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->mach.OverThrottledAc.Action=PowerActionShutdownOff;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->mach.OverThrottledAc.Action=PowerActionWarmEject;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->mach.OverThrottledAc.Action=PowerActionWarmEject+1;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_INVALID_DATA, "expected ERROR_INVALID_DATA but got %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->mach.OverThrottledAc.Action=PowerActionWarmEject+2;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_INVALID_DATA, "expected ERROR_INVALID_DATA but got %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");
  pPP_original->mach.OverThrottledAc.Action=PowerActionNone;


   pPP_original->mach.OverThrottledDc.Action=PowerActionNone-2;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_INVALID_DATA, "expected ERROR_INVALID_DATA but got %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   //pp_compare.mach.OverThrottledDc.Action=4;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->mach.OverThrottledDc.Action=PowerActionNone-1;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_INVALID_DATA, "expected ERROR_INVALID_DATA but got %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   //pp_compare.mach.OverThrottledDc.Action=4;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->mach.OverThrottledDc.Action=PowerActionNone;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   //pp_compare.mach.OverThrottledDc.Action=4;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->mach.OverThrottledDc.Action=PowerActionReserved;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   //pp_compare.mach.OverThrottledDc.Action=4;
   pp_compare.mach.OverThrottledDc.Action=2;
   pp_compare.mach.OverThrottledAc.Action=0;
   pp_compare.mach.pad1[2]=0;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->mach.OverThrottledDc.Action=PowerActionSleep;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   //pp_compare.mach.OverThrottledDc.Action=4;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->mach.OverThrottledDc.Action=PowerActionHibernate;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   pp_compare.mach.OverThrottledDc.Action=2;
   pp_compare.mach.OverThrottledAc.Action=0;
   pp_compare.mach.pad1[2]=0;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->mach.OverThrottledDc.Action=PowerActionShutdown;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->mach.OverThrottledDc.Action=PowerActionShutdownReset;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   //pp_compare.mach.OverThrottledDc.Action=4;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->mach.OverThrottledDc.Action=PowerActionShutdownOff;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->mach.OverThrottledDc.Action=PowerActionWarmEject;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->mach.OverThrottledDc.Action=PowerActionWarmEject+1;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_INVALID_DATA, "expected ERROR_INVALID_DATA but got %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->mach.OverThrottledDc.Action=PowerActionWarmEject+2;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_INVALID_DATA, "expected ERROR_INVALID_DATA but got %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");
  pPP_original->mach.OverThrottledDc.Action=PowerActionNone;


     pPP_original->mach.ReducedLatencySleepAc=PowerSystemUnspecified-2;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   //pp_compare.mach.ReducedLatencySleepAc=4;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->mach.ReducedLatencySleepAc=PowerSystemUnspecified-1;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   //pp_compare.mach.ReducedLatencySleepAc=4;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->mach.ReducedLatencySleepAc=PowerSystemUnspecified;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_GEN_FAILURE, "expected ERROR_GEN_FAILURE but got %x\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   //pp_compare.mach.ReducedLatencySleepAc=4;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->mach.ReducedLatencySleepAc=PowerSystemWorking;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   //pp_compare.mach.ReducedLatencySleepAc=4;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->mach.ReducedLatencySleepAc=PowerSystemSleeping1;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   pp_compare.mach.ReducedLatencySleepAc=4;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->mach.ReducedLatencySleepAc=PowerSystemSleeping2;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   pp_compare.mach.ReducedLatencySleepAc=4;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->mach.ReducedLatencySleepAc=PowerSystemSleeping3;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->mach.ReducedLatencySleepAc=PowerSystemHibernate;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   pp_compare.mach.ReducedLatencySleepAc=4;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->mach.ReducedLatencySleepAc=PowerSystemShutdown;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_GEN_FAILURE, "expected ERROR_GEN_FAILURE but got %x\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->mach.ReducedLatencySleepAc=PowerSystemMaximum;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_GEN_FAILURE, "expected ERROR_GEN_FAILURE but got %x\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->mach.ReducedLatencySleepAc=PowerSystemMaximum+1;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_GEN_FAILURE, "expected ERROR_GEN_FAILURE but got %x\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->mach.ReducedLatencySleepAc=PowerSystemMaximum+2;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_GEN_FAILURE, "expected ERROR_GEN_FAILURE but got %x\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");
  pPP_original->mach.ReducedLatencySleepAc=PowerSystemWorking;


     pPP_original->mach.ReducedLatencySleepDc=PowerSystemUnspecified-2;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   //pp_compare.mach.ReducedLatencySleepDc=4;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->mach.ReducedLatencySleepDc=PowerSystemUnspecified-1;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   //pp_compare.mach.ReducedLatencySleepDc=4;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->mach.ReducedLatencySleepDc=PowerSystemUnspecified;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_GEN_FAILURE, "expected ERROR_GEN_FAILURE but got %x\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   //pp_compare.mach.ReducedLatencySleepDc=4;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->mach.ReducedLatencySleepDc=PowerSystemWorking;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   //pp_compare.mach.ReducedLatencySleepDc=4;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->mach.ReducedLatencySleepDc=PowerSystemSleeping1;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   pp_compare.mach.ReducedLatencySleepDc=4;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->mach.ReducedLatencySleepDc=PowerSystemSleeping2;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   pp_compare.mach.ReducedLatencySleepDc=4;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->mach.ReducedLatencySleepDc=PowerSystemSleeping3;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->mach.ReducedLatencySleepDc=PowerSystemHibernate;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   pp_compare.mach.ReducedLatencySleepDc=4;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->mach.ReducedLatencySleepDc=PowerSystemShutdown;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_GEN_FAILURE, "expected ERROR_GEN_FAILURE but got %x\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->mach.ReducedLatencySleepDc=PowerSystemMaximum;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_GEN_FAILURE, "expected ERROR_GEN_FAILURE but got %x\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->mach.ReducedLatencySleepDc=PowerSystemMaximum+1;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_GEN_FAILURE, "expected ERROR_GEN_FAILURE but got %x\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->mach.ReducedLatencySleepDc=PowerSystemMaximum+2;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_GEN_FAILURE, "expected ERROR_GEN_FAILURE but got %x\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");
  pPP_original->mach.ReducedLatencySleepDc=PowerSystemWorking;


   pPP_original->user.IdleAc.Action=PowerActionNone-2;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_INVALID_DATA, "expected ERROR_INVALID_DATA but got %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   //pp_compare.user.IdleAc.Action=4;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->user.IdleAc.Action=PowerActionNone-1;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_INVALID_DATA, "expected ERROR_INVALID_DATA but got %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   //pp_compare.user.IdleAc.Action=4;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->user.IdleAc.Action=PowerActionNone;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   //pp_compare.user.IdleAc.Action=4;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->user.IdleAc.Action=PowerActionReserved;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   //pp_compare.user.IdleAc.Action=4;
   pp_compare.user.IdleAc.Action=2;
   //pp_compare.user.pad1[2]=2;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->user.IdleAc.Action=PowerActionSleep;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   //pp_compare.user.IdleAc.Action=4;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->user.IdleAc.Action=PowerActionHibernate;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   pp_compare.user.IdleAc.Action=2;
   //pp_compare.user.pad1[2]=2;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->user.IdleAc.Action=PowerActionShutdown;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->user.IdleAc.Action=PowerActionShutdownReset;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   //pp_compare.user.IdleAc.Action=4;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->user.IdleAc.Action=PowerActionShutdownOff;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->user.IdleAc.Action=PowerActionWarmEject;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->user.IdleAc.Action=PowerActionWarmEject+1;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_INVALID_DATA, "expected ERROR_INVALID_DATA but got %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->user.IdleAc.Action=PowerActionWarmEject+2;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_INVALID_DATA, "expected ERROR_INVALID_DATA but got %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");
  pPP_original->user.IdleAc.Action=PowerActionNone;



   pPP_original->user.IdleDc.Action=PowerActionNone-2;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_INVALID_DATA, "expected ERROR_INVALID_DATA but got %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   //pp_compare.user.IdleDc.Action=4;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->user.IdleDc.Action=PowerActionNone-1;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_INVALID_DATA, "expected ERROR_INVALID_DATA but got %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   //pp_compare.user.IdleDc.Action=4;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->user.IdleDc.Action=PowerActionNone;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   //pp_compare.user.IdleDc.Action=4;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->user.IdleDc.Action=PowerActionReserved;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   //pp_compare.user.IdleDc.Action=4;
   pp_compare.user.IdleDc.Action=2;
//   pp_compare.user.pad1[2]=2;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->user.IdleDc.Action=PowerActionSleep;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   //pp_compare.user.IdleDc.Action=4;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->user.IdleDc.Action=PowerActionHibernate;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   pp_compare.user.IdleDc.Action=2;
//   pp_compare.user.pad1[2]=2;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->user.IdleDc.Action=PowerActionShutdown;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->user.IdleDc.Action=PowerActionShutdownReset;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   //pp_compare.user.IdleDc.Action=4;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->user.IdleDc.Action=PowerActionShutdownOff;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->user.IdleDc.Action=PowerActionWarmEject;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->user.IdleDc.Action=PowerActionWarmEject+1;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_INVALID_DATA, "expected ERROR_INVALID_DATA but got %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->user.IdleDc.Action=PowerActionWarmEject+2;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_INVALID_DATA, "expected ERROR_INVALID_DATA but got %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");
  pPP_original->user.IdleDc.Action=PowerActionNone;


   pPP_original->user.MaxSleepAc=PowerSystemUnspecified-2;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   pp_compare.user.MaxSleepAc=4;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->user.MaxSleepAc=PowerSystemUnspecified-1;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   pp_compare.user.MaxSleepAc=4;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->user.MaxSleepAc=PowerSystemUnspecified;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_GEN_FAILURE, "expected ERROR_GEN_FAILURE but got %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   //pp_compare.user.MaxSleepAc=4;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->user.MaxSleepAc=PowerSystemWorking;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   pp_compare.user.MaxSleepAc=4;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->user.MaxSleepAc=PowerSystemSleeping1;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   pp_compare.user.MaxSleepAc=4;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->user.MaxSleepAc=PowerSystemSleeping2;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   pp_compare.user.MaxSleepAc=4;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->user.MaxSleepAc=PowerSystemSleeping3;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->user.MaxSleepAc=PowerSystemHibernate;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   pp_compare.user.MaxSleepAc=4;
   pp_compare.user.MaxSleepAc=4;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->user.MaxSleepAc=PowerSystemShutdown;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   pp_compare.user.MaxSleepAc=4;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->user.MaxSleepAc=PowerSystemMaximum;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   pp_compare.user.MaxSleepAc=4;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->user.MaxSleepAc=PowerSystemMaximum+1;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   pp_compare.user.MaxSleepAc=4;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->user.MaxSleepAc=PowerSystemMaximum+2;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   pp_compare.user.MaxSleepAc=4;
   ok(compare(pp,pp_compare),"Difference Found\n");
  pPP_original->user.MaxSleepAc=PowerSystemSleeping3;


   pPP_original->user.MaxSleepDc=PowerSystemUnspecified-2;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   pp_compare.user.MaxSleepDc=4;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->user.MaxSleepDc=PowerSystemUnspecified-1;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   pp_compare.user.MaxSleepDc=4;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->user.MaxSleepDc=PowerSystemUnspecified;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(!ret, "function was expected to fail\n");
   ok(GetLastError() == ERROR_GEN_FAILURE, "expected ERROR_GEN_FAILURE but got %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   //pp_compare.user.MaxSleepDc=4;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->user.MaxSleepDc=PowerSystemWorking;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   pp_compare.user.MaxSleepDc=4;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->user.MaxSleepDc=PowerSystemSleeping1;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   pp_compare.user.MaxSleepDc=4;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->user.MaxSleepDc=PowerSystemSleeping2;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   pp_compare.user.MaxSleepDc=4;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->user.MaxSleepDc=PowerSystemSleeping3;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->user.MaxSleepDc=PowerSystemHibernate;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   pp_compare.user.MaxSleepDc=4;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->user.MaxSleepDc=PowerSystemShutdown;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   pp_compare.user.MaxSleepDc=4;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->user.MaxSleepDc=PowerSystemMaximum;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   pp_compare.user.MaxSleepDc=4;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->user.MaxSleepDc=PowerSystemMaximum+1;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   pp_compare.user.MaxSleepDc=4;
   ok(compare(pp,pp_compare),"Difference Found\n");

   pPP_original->user.MaxSleepDc=PowerSystemMaximum+2;
    memcpy(&gpp, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
    memcpy(&gpp_compare, pGPP_original, sizeof(GLOBAL_POWER_POLICY));
	memcpy(&pp, pPP_original, sizeof(POWER_POLICY));
    memcpy(&pp_compare, pPP_original, sizeof(POWER_POLICY));
   ret = ValidatePowerPolicies(&gpp,&pp);
   ok(ret, "function was expected to succeed error %i\n",(UINT)GetLastError());
   ok(globalcompare(gpp,gpp_compare),"Difference Found\n");
   pp_compare.user.MaxSleepDc=4;
   ok(compare(pp,pp_compare),"Difference Found\n");
  pPP_original->user.MaxSleepDc=PowerSystemSleeping3;

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
   HKEY hSubKey;
   LONG lSize;
   LONG Err;
   WCHAR szPath[MAX_PATH];
   static const WCHAR szTestSchemeName[] = {'P','o','w','r','p','r','o','f',0};
   static const WCHAR szTestSchemeDesc[] = {'P','o','w','r','p','r','o','f',' ','S','c','h','e','m','e',0};

   /*
    * create a temporarly profile, will be deleted in test_DeletePwrScheme
    */

   retval = WritePwrScheme(&g_TempPwrScheme, (LPWSTR)szTestSchemeName, (LPWSTR)szTestSchemeDesc, &g_PowerPolicy);
   ok(retval, "Warning: function should have succeeded\n");
   if (RegOpenKeyW(HKEY_LOCAL_MACHINE, szMachPowerPoliciesSubKey, &hSubKey) == ERROR_SUCCESS)
   {
	   lSize = MAX_PATH * sizeof(WCHAR);
	   Err = RegQueryValueW(hSubKey, szTempPwrScheme, szPath, &lSize);
	   if (Err != STATUS_SUCCESS)
         printf("#1 failed to query subkey %i (testentry)\n", g_TempPwrScheme);
      RegCloseKey(hSubKey);
   }

}

void func_power(void)
{
   if (1)
      skip("CallNtPowerInformation test is broken and fails on Windows\n");
   else
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
   if (1)
      skip("ValidatePowerPolicies tests are broken and fail on Windows\n");
   else
   {
      test_ValidatePowerPolicies_Old();
      test_ValidatePowerPolicies();
   }
   test_WriteGlobalPwrPolicy();
   test_WriteProcessorPwrScheme();

}
