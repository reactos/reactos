/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/test1/test.c
 * PURPOSE:          Bug demonstration
 * PROGRAMMER:       David Welch (welch@mcmail.com)
 * UPDATE HISTORY: 
 *              ??/??/??: Created
 *              18/06/98: Made more NT like
 */

/* FUNCTIONS **************************************************************/

#include <windows.h>
#include <internal/kernel.h>
#include <internal/halio.h>
#include <ddk/ntddk.h>
#include <string.h>
#include <internal/debug.h>

#define IDE_NT_ROOTDIR_NAME "\\Device\\"
#define IDE_NT_DEVICE_NAME "\\HardDrive"

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
/*
 * FUNCTION: Called by the system to initalize the driver
 * ARGUMENTS:
 *           DriverObject = object describing this driver
 *           RegistryPath = path to our configuration entries
 * RETURNS: Success or failure
 */
{
   char DeviceDirName[255];
   UNICODE_STRING UnicodeDeviceDirName;
   ANSI_STRING AnsiDeviceDirName;
   OBJECT_ATTRIBUTES DeviceDirAttributes;
   HANDLE Handle;
   NTSTATUS RC;
   ULONG HarddiskIdx = 0;
   
   strcpy(DeviceDirName,IDE_NT_ROOTDIR_NAME);
   strcat(DeviceDirName,IDE_NT_DEVICE_NAME);
   DeviceDirName[strlen(DeviceDirName)+1]='\0';
   DeviceDirName[strlen(DeviceDirName)]= '0' + HarddiskIdx;
   printk("DeviceDirName %s\n",DeviceDirName);
   RtlInitAnsiString(&AnsiDeviceDirName,DeviceDirName);
   RC = RtlAnsiStringToUnicodeString(&UnicodeDeviceDirName,
				     &AnsiDeviceDirName,
				     TRUE);
   if (!NT_SUCCESS(RC))
     {
	DPRINT("Could not convert ansi to unicode for device dir\n",0);
	return(STATUS_UNSUCCESSFUL);
     }
   InitializeObjectAttributes(&DeviceDirAttributes,&UnicodeDeviceDirName,
			      0,NULL,NULL);
   RC = ZwCreateDirectoryObject(&Handle,0,&DeviceDirAttributes);
   if (!NT_SUCCESS(RC))
     {
	DPRINT("Could not create device dir\n",0);
	return(STATUS_UNSUCCESSFUL);       
     }
   RtlFreeUnicodeString(&UnicodeDeviceDirName);
}

