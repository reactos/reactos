/* $Id: main.c,v 1.75 2001/01/17 19:07:13 ea Exp $
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
#include <reactos/resource.h>
#include <internal/mm.h>
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
#include <internal/v86m.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

ULONG EXPORTED NtBuildNumber = KERNEL_VERSION_BUILD;
ULONG EXPORTED NtGlobalFlag = 0;
CHAR  EXPORTED KeNumberProcessors = 1;
LOADER_PARAMETER_BLOCK EXPORTED KeLoaderBlock;
static LOADER_MODULE KeLoaderModules[64];
static UCHAR KeLoaderModuleStrings[64][256];
static UCHAR KeLoaderCommandLine[256];

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
	DPRINT1("Arc name: %wZ\n", &ArcName);

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
	if (!NT_SUCCESS(Status))
	{
		RtlFreeUnicodeString (&BootPath);
		RtlFreeUnicodeString (&DeviceName);
		DbgPrint("NtOpenSymbolicLinkObject() '%wZ' failed (Status %x)\n",
		         &ArcName,
		         Status);
		RtlFreeUnicodeString (&ArcName);

		KeBugCheck (0x0);
	}
	RtlFreeUnicodeString (&ArcName);

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

	/* Check if '\SystemRoot'(LinkName) can be opened, otherwise crash it! */
	InitializeObjectAttributes (&ObjectAttributes,
	                            &LinkName,
	                            0,
	                            NULL,
	                            NULL);

	Status = NtOpenSymbolicLinkObject (&Handle,
	                                   SYMBOLIC_LINK_ALL_ACCESS,
	                                   &ObjectAttributes);
	if (!NT_SUCCESS(Status))
	{
		DbgPrint("NtOpenSymbolicLinkObject() failed to open '\\SystemRoot' (Status %x)\n",
		         Status);
		KeBugCheck (0x0);
	}
	NtClose(Handle);
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

extern NTSTATUS STDCALL
Ke386CallBios(UCHAR Int, KV86M_REGISTERS* Regs);

struct __attribute__((packed)) vesa_info
{
  UCHAR Signature[4];
  USHORT Version;
  ULONG OEMName;
  ULONG Capabilities;
  ULONG SupportedModes;
  USHORT TotalVideoMemory;
  USHORT OEMVersion;
  ULONG VendorName;
  ULONG ProductName;
  ULONG ProductRevisionString;
  UCHAR Reserved[478];
};

VOID
TestV86Mode(VOID)
{
  ULONG i;
  extern UCHAR OrigIVT[1024];
  KV86M_REGISTERS regs;
  NTSTATUS Status;
  struct vesa_info* vi;

  for (i = 0; i < (640 / 4); i++)
    {
      MmCreateVirtualMapping(NULL,
			     (PVOID)(i * 4096),
			     PAGE_EXECUTE_READWRITE,
			     (ULONG)MmAllocPage(0));
    }
  for (; i < (1024 / 4); i++)
    {
      MmCreateVirtualMapping(NULL,
			     (PVOID)(i * 4096),
			     PAGE_EXECUTE_READ,
			     i * 4096);
    }
  vi = (struct vesa_info*)0x20000;
  vi->Signature[0] = 'V';
  vi->Signature[1] = 'B';
  vi->Signature[2] = 'E';
  vi->Signature[3] = '2';
  memset(&regs, 0, sizeof(regs));
  regs.Eax = 0x4F00;  
  regs.Es = 0x2000;
  regs.Edi = 0x0;
  memcpy((PVOID)0x0, OrigIVT, 1024);
  Status = Ke386CallBios(0x10, &regs);
  DbgPrint("Finished (Status %x, CS:EIP %x:%x)\n", Status, regs.Cs,
	   regs.Eip);
  DbgPrint("Eax %x\n", regs.Eax);
  DbgPrint("Signature %.4s\n", vi->Signature);
  DbgPrint("TotalVideoMemory %dKB\n", vi->TotalVideoMemory * 64);
}

VOID
_main (ULONG MultiBootMagic, PLOADER_PARAMETER_BLOCK _LoaderBlock)
/*
 * FUNCTION: Called by the boot loader to start the kernel
 * ARGUMENTS:
 *          LoaderBlock = Pointer to boot parameters initialized by the boot 
 *                        loader
 * NOTE: The boot parameters are stored in low memory which will become
 * invalid after the memory managment is initialized so we make a local copy.
 */
{
   ULONG i;
   ULONG last_kernel_address;
   ULONG start;
   PCHAR name;
   extern ULONG _bss_end__;

   /*
    * Copy the parameters to a local buffer because lowmem will go away
    */
   memcpy (&KeLoaderBlock, _LoaderBlock, sizeof(LOADER_PARAMETER_BLOCK));
   memcpy (&KeLoaderModules[1], (PVOID)KeLoaderBlock.ModsAddr,
	   sizeof(LOADER_MODULE) * KeLoaderBlock.ModsCount);
   KeLoaderBlock.ModsCount++;
   KeLoaderBlock.ModsAddr = (ULONG)&KeLoaderModules;
   
   /*
    * FIXME: Preliminary hack!!!! Add boot device to beginning of command line.
    * This should be done by the boot loader.
    */
   strcpy (KeLoaderCommandLine,
	   "multi(0)disk(0)rdisk(0)partition(1)\\reactos ");
   strcat (KeLoaderCommandLine, (PUCHAR)KeLoaderBlock.CommandLine);   

   KeLoaderBlock.CommandLine = (ULONG)KeLoaderCommandLine;
   strcpy(KeLoaderModuleStrings[0], "ntoskrnl.exe");
   KeLoaderModules[0].String = (ULONG)KeLoaderModuleStrings[0];
   KeLoaderModules[0].ModStart = 0xC0000000;
   KeLoaderModules[0].ModEnd = PAGE_ROUND_UP((ULONG)&_bss_end__);
   for (i = 1; i < KeLoaderBlock.ModsCount; i++)
     {
	strcpy(KeLoaderModuleStrings[i], (PUCHAR)KeLoaderModules[i].String);
	KeLoaderModules[i].ModStart -= 0x200000;
	KeLoaderModules[i].ModStart += 0xc0000000;
	KeLoaderModules[i].ModEnd -= 0x200000;
	KeLoaderModules[i].ModEnd += 0xc0000000;
	KeLoaderModules[i].String = (ULONG)KeLoaderModuleStrings[i];
     }
   
   /*
    * Initialization phase 0
    */
   HalInitSystem (0, (PLOADER_PARAMETER_BLOCK)&KeLoaderBlock);
   KeInit1();
   KeLowerIrql(DISPATCH_LEVEL);
   
   {
     char tmpbuf[80];
     sprintf(tmpbuf,"system with %d/%d MB memory\n",
	     (unsigned int)(KeLoaderBlock.MemLower)/1024,
	     (unsigned int)(KeLoaderBlock.MemHigher)/1024);
     HalDisplayString(tmpbuf);
   }

   /*
    * Display version number and copyright/warranty message
    */
   HalDisplayString("Starting ReactOS "KERNEL_VERSION_STR" (Build "
		    KERNEL_VERSION_BUILD_STR")\n");
   HalDisplayString(RES_STR_LEGAL_COPYRIGHT);
   HalDisplayString("\n\nReactOS is free software, covered by the GNU General "
		    "Public License, and you\n");
   HalDisplayString("are welcome to change it and/or distribute copies of it "
		    "under certain\n"); 
   HalDisplayString("conditions. There is absolutely no warranty for ReactOS.\n");
   
   last_kernel_address = KeLoaderModules[KeLoaderBlock.ModsCount - 1].ModEnd;
   
   NtEarlyInitVdm();
   MmInit1(KeLoaderModules[0].ModStart - 0xc0000000 + 0x200000,
	   last_kernel_address - 0xc0000000 + 0x200000,
	   last_kernel_address);

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
   DPRINT1("%d files loaded\n",KeLoaderBlock.ModsCount);
   for (i=0; i < KeLoaderBlock.ModsCount; i++)
     {
	DPRINT1("module: %s\n", KeLoaderModules[i].String);
     }

   /*  Pass 1: load registry chunks passed in  */
   for (i = 1; i < KeLoaderBlock.ModsCount; i++)
     {
       start = KeLoaderModules[i].ModStart;
       if (strcmp ((PCHAR) start, "REGEDIT4") == 0)
	 {
	    DPRINT1("process registry chunk at %08lx\n", start);
	    CmImportHive((PCHAR) start);
	 }
     }

   /*  Pass 2: process boot loaded drivers  */
   for (i=1; i < KeLoaderBlock.ModsCount; i++)
     {
       start = KeLoaderModules[i].ModStart;
       name = (PCHAR)KeLoaderModules[i].String;
       if (strcmp ((PCHAR) start, "REGEDIT4") != 0)
	 {
	    DPRINT1("process module '%s' at %08lx\n", name, start);
	    LdrProcessDriver((PVOID)start);
	 }
     }
   
   /* Create the SystemRoot symbolic link */
   DbgPrint("CommandLine: %s\n", (PUCHAR)KeLoaderBlock.CommandLine);
   CreateSystemRootLink ((PUCHAR)KeLoaderBlock.CommandLine);
   
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
   InitSystemSharedUserPage ((PUCHAR)KeLoaderBlock.CommandLine);

   /*
    *  Launch initial process
    */
   LdrLoadInitialProcess();

   DbgPrint("Finished main()\n");
   PsTerminateSystemThread(STATUS_SUCCESS);
}

/* EOF */

