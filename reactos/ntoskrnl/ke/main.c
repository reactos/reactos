/* $Id: main.c,v 1.62 2000/09/13 10:29:48 jean Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/main.c
 * PURPOSE:         Initalizes the kernel
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                28/05/98: Created
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/ntoskrnl.h>
#include <reactos/buildno.h>
#include <internal/mm.h>
#include <string.h>
#include <internal/string.h>
#include <internal/module.h>
#include <internal/ldr.h>
#include <internal/ex.h>
#include <internal/ps.h>
#include <internal/hal.h>
#include <internal/ke.h>
#include <internal/io.h>

#include <internal/mmhal.h>
#include <internal/i386/segment.h>
#include <napi/shared_data.h>

#define NDEBUG
#include <internal/debug.h>

/* DATA *********************************************************************/

ULONG EXPORTED NtBuildNumber = KERNEL_VERSION_BUILD;
ULONG EXPORTED NtGlobalFlag = 0;
CHAR  EXPORTED KeNumberProcessors = 1;
LOADER_PARAMETER_BLOCK EXPORTED KeLoaderBlock;

/* FUNCTIONS ****************************************************************/

static VOID
CreateSystemRootLink (PCSZ ParameterLine)
{
	UNICODE_STRING LinkName;
	UNICODE_STRING DeviceName;
	UNICODE_STRING ArcName;
	UNICODE_STRING BootPath;
	PCHAR ParamBuffer;
	PWCHAR ArcNameBuffer;
	PCHAR p;
	NTSTATUS Status;
	ULONG Length;
	OBJECT_ATTRIBUTES ObjectAttributes;
	HANDLE Handle;

	/* create local parameter line copy */
	ParamBuffer = ExAllocatePool (PagedPool, 256);
	strcpy (ParamBuffer, (char *)ParameterLine);

	DPRINT("%s\n", ParamBuffer);
	/* Format: <arc_name>\<path> [options...] */

	/* cut options off */
	p = strchr (ParamBuffer, ' ');
	if (p)
		*p = 0;
	DPRINT("%s\n", ParamBuffer);

	/* extract path */
	p = strchr (ParamBuffer, '\\');
	if (p)
	{
		DPRINT("Boot path: %s\n", p);
		RtlCreateUnicodeStringFromAsciiz (&BootPath, p);
		*p = 0;
	}
	else
	{
		DPRINT("Boot path: %s\n", "\\");
		RtlCreateUnicodeStringFromAsciiz (&BootPath, "\\");
	}
	DPRINT("Arc name: %s\n", ParamBuffer);

	/* Only arc name left - build full arc name */
	ArcNameBuffer = ExAllocatePool (PagedPool, 256 * sizeof(WCHAR));
	swprintf (ArcNameBuffer,
	          L"\\ArcName\\%S", ParamBuffer);
	RtlInitUnicodeString (&ArcName, ArcNameBuffer);
	DPRINT("Arc name: %wZ\n", &ArcName);

	/* free ParamBuffer */
	ExFreePool (ParamBuffer);

	/* allocate device name string */
	DeviceName.Length = 0;
	DeviceName.MaximumLength = 256 * sizeof(WCHAR);
	DeviceName.Buffer = ExAllocatePool (PagedPool, 256 * sizeof(WCHAR));

	InitializeObjectAttributes (&ObjectAttributes,
	                            &ArcName,
	                            0,
	                            NULL,
	                            NULL);

	Status = NtOpenSymbolicLinkObject (&Handle,
	                                   SYMBOLIC_LINK_ALL_ACCESS,
	                                   &ObjectAttributes);
	RtlFreeUnicodeString (&ArcName);
	if (!NT_SUCCESS(Status))
	{
		RtlFreeUnicodeString (&BootPath);
		RtlFreeUnicodeString (&DeviceName);
		DbgPrint("NtOpenSymbolicLinkObject() failed (Status %x)\n",
		         Status);

		KeBugCheck (0x0);
	}

	Status = NtQuerySymbolicLinkObject (Handle,
	                                    &DeviceName,
	                                    &Length);
	NtClose (Handle);
	if (!NT_SUCCESS(Status))
	{
		RtlFreeUnicodeString (&BootPath);
		RtlFreeUnicodeString (&DeviceName);
		DbgPrint("NtQuerySymbolicObject() failed (Status %x)\n",
		         Status);

		KeBugCheck (0x0);
	}
	DPRINT("Length: %lu DeviceName: %wZ\n", Length, &DeviceName);

	RtlAppendUnicodeStringToString (&DeviceName,
					&BootPath);

	RtlFreeUnicodeString (&BootPath);
	DPRINT("DeviceName: %wZ\n", &DeviceName);

	/* create the '\SystemRoot' link */
	RtlInitUnicodeString (&LinkName,
			      L"\\SystemRoot");

	Status = IoCreateSymbolicLink (&LinkName,
				       &DeviceName);
	RtlFreeUnicodeString (&DeviceName);
	if (!NT_SUCCESS(Status))
	{
		DbgPrint("IoCreateSymbolicLink() failed (Status %x)\n",
		         Status);

		KeBugCheck (0x0);
	}

	/*
	 * FIXME: test if '\SystemRoot' (LinkName)can be opened,
	 * otherwise crash it!
	 */
}


static VOID
InitSystemSharedUserPage (PCSZ ParameterLine)
{
   PKUSER_SHARED_DATA SharedPage;

   UNICODE_STRING ArcDeviceName;
   UNICODE_STRING ArcName;
   UNICODE_STRING BootPath;
   UNICODE_STRING DriveDeviceName;
   UNICODE_STRING DriveName;
   WCHAR DriveNameBuffer[20];
   PCHAR ParamBuffer;
   PWCHAR ArcNameBuffer;
   PCHAR p;
   NTSTATUS Status;
   ULONG Length;
   OBJECT_ATTRIBUTES ObjectAttributes;
   HANDLE Handle;
   ULONG i;
   BOOLEAN BootDriveFound;

   SharedPage = (PKUSER_SHARED_DATA)KERNEL_SHARED_DATA_BASE;
   SharedPage->DosDeviceMap = 0;
   SharedPage->NtProductType = NtProductWinNt;
   for (i = 0; i < 32; i++)
     {
	SharedPage->DosDeviceDriveType[i] = 0;
     }

   BootDriveFound = FALSE;

   /*
    * Retrieve the current dos system path
    * (e.g.: C:\reactos) from the given arc path
    * (e.g.: multi(0)disk(0)rdisk(0)partititon(1)\reactos)
    * Format: "<arc_name>\<path> [options...]"
    */

   /* create local parameter line copy */
   ParamBuffer = ExAllocatePool (PagedPool, 256);
   strcpy (ParamBuffer, (char *)ParameterLine);
   DPRINT("%s\n", ParamBuffer);

   /* cut options off */
   p = strchr (ParamBuffer, ' ');
   if (p)
     {
	*p = 0;
     }
   DPRINT("%s\n", ParamBuffer);

   /* extract path */
   p = strchr (ParamBuffer, '\\');
   if (p)
     {
	DPRINT("Boot path: %s\n", p);
	RtlCreateUnicodeStringFromAsciiz (&BootPath, p);
	*p = 0;
     }
   else
     {
	DPRINT("Boot path: %s\n", "\\");
	RtlCreateUnicodeStringFromAsciiz (&BootPath, "\\");
     }
   DPRINT("Arc name: %s\n", ParamBuffer);

   /* Only arc name left - build full arc name */
   ArcNameBuffer = ExAllocatePool (PagedPool, 256 * sizeof(WCHAR));
   swprintf (ArcNameBuffer, L"\\ArcName\\%S", ParamBuffer);
   RtlInitUnicodeString (&ArcName, ArcNameBuffer);
   DPRINT("Arc name: %wZ\n", &ArcName);

   /* free ParamBuffer */
   ExFreePool (ParamBuffer);

   /* allocate arc device name string */
   ArcDeviceName.Length = 0;
   ArcDeviceName.MaximumLength = 256 * sizeof(WCHAR);
   ArcDeviceName.Buffer = ExAllocatePool (PagedPool, 256 * sizeof(WCHAR));

   InitializeObjectAttributes (&ObjectAttributes,
			       &ArcName,
			       0,
			       NULL,
			       NULL);

   Status = NtOpenSymbolicLinkObject (&Handle,
				      SYMBOLIC_LINK_ALL_ACCESS,
				      &ObjectAttributes);
   RtlFreeUnicodeString (&ArcName);
   if (!NT_SUCCESS(Status))
     {
	RtlFreeUnicodeString (&BootPath);
	RtlFreeUnicodeString (&ArcDeviceName);
	DbgPrint("NtOpenSymbolicLinkObject() failed (Status %x)\n",
	         Status);

	KeBugCheck (0x0);
     }

   Status = NtQuerySymbolicLinkObject (Handle,
				       &ArcDeviceName,
				       &Length);
   NtClose (Handle);
   if (!NT_SUCCESS(Status))
     {
	RtlFreeUnicodeString (&BootPath);
	RtlFreeUnicodeString (&ArcDeviceName);
	DbgPrint("NtQuerySymbolicObject() failed (Status %x)\n",
		 Status);

	KeBugCheck (0x0);
     }
   DPRINT("Length: %lu ArcDeviceName: %wZ\n", Length, &ArcDeviceName);


   /* allocate device name string */
   DriveDeviceName.Length = 0;
   DriveDeviceName.MaximumLength = 256 * sizeof(WCHAR);
   DriveDeviceName.Buffer = ExAllocatePool (PagedPool, 256 * sizeof(WCHAR));

   for (i = 0; i < 26; i++)
     {
	swprintf (DriveNameBuffer, L"\\??\\%C:", 'A' + i);
	RtlInitUnicodeString (&DriveName,
			      DriveNameBuffer);

	InitializeObjectAttributes (&ObjectAttributes,
				    &DriveName,
				    0,
				    NULL,
				    NULL);

	Status = NtOpenSymbolicLinkObject (&Handle,
					   SYMBOLIC_LINK_ALL_ACCESS,
					   &ObjectAttributes);
	if (!NT_SUCCESS(Status))
	  {
	     DPRINT("Failed to open link %wZ\n",
		    &DriveName);
	     continue;
	  }

	Status = NtQuerySymbolicLinkObject (Handle,
					    &DriveDeviceName,
					    &Length);
	if (!NT_SUCCESS(Status))
	  {
	     DPRINT("Failed query open link %wZ\n",
		    &DriveName);
	     continue;
	  }
	DPRINT("Opened link: %wZ ==> %wZ\n",
	       &DriveName, &DriveDeviceName);

	if (!RtlCompareUnicodeString (&ArcDeviceName, &DriveDeviceName, FALSE))
	  {
	     DPRINT("DOS Boot path: %c:%wZ\n", 'A' + i, &BootPath);
	     swprintf (SharedPage->NtSystemRoot,
		       L"%C:%wZ", 'A' + i, &BootPath);

		BootDriveFound = TRUE;
	  }

	NtClose (Handle);

	/* set bit in dos drives bitmap (drive available) */
	SharedPage->DosDeviceMap |= (1<<i);
     }

   RtlFreeUnicodeString (&BootPath);
   RtlFreeUnicodeString (&DriveDeviceName);
   RtlFreeUnicodeString (&ArcDeviceName);

   DPRINT("DosDeviceMap: 0x%x\n", SharedPage->DosDeviceMap);

   if (BootDriveFound == FALSE)
     {
	DbgPrint("No system drive found!\n");
	KeBugCheck (0x0);
     }
}


void _main (PLOADER_PARAMETER_BLOCK LoaderBlock)
/*
 * FUNCTION: Called by the boot loader to start the kernel
 * ARGUMENTS:
 *          LoaderBlock = Pointer to boot parameters initialized by the boot loader
 * NOTE: The boot parameters are stored in low memory which will become
 * invalid after the memory managment is initialized so we make a local copy.
 */
{
   unsigned int i;
   unsigned int last_kernel_address;
   ULONG start, start1;
   
   /*
    * Copy the parameters to a local buffer because lowmem will go away
    */
   memcpy (&KeLoaderBlock, LoaderBlock, sizeof(LOADER_PARAMETER_BLOCK));

   /*
    * FIXME: Preliminary hack!!!!
    * Initializes the kernel parameter line.
    * This should be done by the boot loader.
    */
   strcpy (KeLoaderBlock.kernel_parameters,
	   "multi(0)disk(0)rdisk(0)partition(1)\\reactos /DEBUGPORT=SCREEN");

   /*
    * Initialization phase 0
    */
   HalInitSystem (0, (PLOADER_PARAMETER_BLOCK)&KeLoaderBlock);
   KeInit1();
   KeLowerIrql(DISPATCH_LEVEL);

   /*
    * Display version number and copyright/warranty message
    */
   HalDisplayString("Starting ReactOS "KERNEL_VERSION_STR" (Build "KERNEL_VERSION_BUILD_STR")\n");
   HalDisplayString("Copyright 2000 (So who owns the copyright?).\n");
   HalDisplayString("ReactOS is free software, covered by the GNU General Public License, and you\n");
   HalDisplayString("are welcome to change it and/or distribute copies of it under certain\n"); 
   HalDisplayString("conditions.\n");
   HalDisplayString("There is absolutely no warranty for ReactOS.\n");
   
   last_kernel_address = KERNEL_BASE;
   for (i=0; i <= KeLoaderBlock.nr_files; i++)
     {
	last_kernel_address = last_kernel_address +
	  PAGE_ROUND_UP(KeLoaderBlock.module_length[i]);
     }

   MmInit1((PLOADER_PARAMETER_BLOCK)&KeLoaderBlock, last_kernel_address);

   /*
    * Initialize the kernel debugger
    */
   KdInitSystem (0, (PLOADER_PARAMETER_BLOCK)&KeLoaderBlock);
   if (KdPollBreakIn ())
     {
	DbgBreakPointWithStatus (DBG_STATUS_CONTROL_C);
     }

   /*
    * Initialization phase 1
    * Initalize various critical subsystems
    */
   HalInitSystem (1, (PLOADER_PARAMETER_BLOCK)&KeLoaderBlock);
   MmInit2();
   KeInit2();

   /*
    * Allow interrupts
    */
   KeLowerIrql(PASSIVE_LEVEL);

   ObInit();
   PiInitProcessManager();
   ExInit();
   IoInit();
   LdrInitModuleManagement();
   CmInitializeRegistry();
   NtInit();
   MmInit3();
   
   /* Report all resources used by hal */
   HalReportResourceUsage ();
   
   /*
    * Initalize services loaded at boot time
    */
   DPRINT1("%d files loaded\n",KeLoaderBlock.nr_files);

   /*  Pass 1: load registry chunks passed in  */
   start = KERNEL_BASE + PAGE_ROUND_UP(KeLoaderBlock.module_length[0]);
   for (i = 1; i < KeLoaderBlock.nr_files; i++)
     {
       if (!strcmp ((PCHAR) start, "REGEDIT4"))
         {
           DPRINT1("process registry chunk at %08lx\n", start);
           CmImportHive((PCHAR) start);
         }
       start = start + KeLoaderBlock.module_length[i];
     }

   /*  Pass 2: process boot loaded drivers  */
   start = KERNEL_BASE + PAGE_ROUND_UP(KeLoaderBlock.module_length[0]);
   start1 = start + KeLoaderBlock.module_length[1];
   for (i=1;i<KeLoaderBlock.nr_files;i++)
     {
       if (strcmp ((PCHAR) start, "REGEDIT4"))
         {
           DPRINT1("process module at %08lx\n", start);
           LdrProcessDriver((PVOID)start);
         }
       start = start + KeLoaderBlock.module_length[i];
     }
   
   /* Create the SystemRoot symbolic link */
   CreateSystemRootLink (KeLoaderBlock.kernel_parameters);
   
   CmInitializeRegistry2();
   /*
    * Load Auto configured drivers
    */
   LdrLoadAutoConfigDrivers();
   
   /*
    * Assign drive letters
    */
   IoAssignDriveLetters ((PLOADER_PARAMETER_BLOCK)&KeLoaderBlock,
                         NULL,
                         NULL,
                         NULL);

   /*
    * Initialize shared user page:
    *  - set dos system path, dos device map, etc.
    */
   InitSystemSharedUserPage (KeLoaderBlock.kernel_parameters);

  /*
   *  Launch initial process
   */
   LdrLoadInitialProcess();
   
   DbgPrint("Finished main()\n");
   PsTerminateSystemThread(STATUS_SUCCESS);
}

/* EOF */
