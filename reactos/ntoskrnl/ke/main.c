/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id: main.c,v 1.158 2003/05/20 14:37:05 ekohl Exp $
 *
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
#include <internal/ifs.h>
#include <internal/module.h>
#include <internal/ldr.h>
#include <internal/ex.h>
#include <internal/ps.h>
#include <internal/ke.h>
#include <internal/io.h>
#include <internal/po.h>
#include <internal/cc.h>
#include <internal/se.h>
#include <internal/v86m.h>
#include <internal/kd.h>
#include <internal/trap.h>
#include "../dbg/kdb.h"
#include <internal/registry.h>
#include <internal/nls.h>
#include <reactos/bugcodes.h>

#ifdef HALDBG
#include <internal/ntosdbg.h>
#else
#define ps(args...)
#endif

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

ULONG EXPORTED NtBuildNumber = KERNEL_VERSION_BUILD;
ULONG EXPORTED NtGlobalFlag = 0;
CHAR  EXPORTED KeNumberProcessors;
LOADER_PARAMETER_BLOCK EXPORTED KeLoaderBlock;
ULONG EXPORTED KeDcacheFlushCount = 0;
ULONG EXPORTED KeIcacheFlushCount = 0;
static LOADER_MODULE KeLoaderModules[64];
static UCHAR KeLoaderModuleStrings[64][256];
static UCHAR KeLoaderCommandLine[256];
static ADDRESS_RANGE KeMemoryMap[64];
static ULONG KeMemoryMapRangeCount;
static ULONG FirstKrnlPhysAddr;
static ULONG LastKrnlPhysAddr;
static ULONG LastKernelAddress;
volatile BOOLEAN Initialized = FALSE;

extern PVOID Ki386InitialStackArray[MAXIMUM_PROCESSORS];


/* FUNCTIONS ****************************************************************/

static BOOLEAN
RtlpCheckFileNameExtension(PCHAR FileName,
			   PCHAR Extension)
{
  PCHAR Ext;

  Ext = strrchr(FileName, '.');
  if (Ext == NULL)
    {
      if ((Extension == NULL) || (*Extension == 0))
	return TRUE;
      else
	return FALSE;
    }

  if (*Extension != '.')
    Ext++;

  if (_stricmp(Ext, Extension) == 0)
    return TRUE;
  else
    return FALSE;
}


static VOID
InitSystemSharedUserPage (PCSZ ParameterLine)
{
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

   /*
    * NOTE:
    *   The shared user page has been zeroed-out right after creation.
    *   There is NO need to do this again.
    */

   SharedUserData->NtProductType = NtProductWinNt;

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
			       OBJ_OPENLINK,
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
	CPRINT("NtOpenSymbolicLinkObject() failed (Status %x)\n",
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
	CPRINT("NtQuerySymbolicObject() failed (Status %x)\n",
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
				    OBJ_OPENLINK,
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
	     swprintf(SharedUserData->NtSystemRoot,
		      L"%C:%wZ", 'A' + i, &BootPath);
	     
	     BootDriveFound = TRUE;
	  }

	NtClose (Handle);
     }

   RtlFreeUnicodeString (&BootPath);
   RtlFreeUnicodeString (&DriveDeviceName);
   RtlFreeUnicodeString (&ArcDeviceName);

   DPRINT("DosDeviceMap: 0x%x\n", SharedUserData->DosDeviceMap);

   if (BootDriveFound == FALSE)
     {
	DbgPrint("No system drive found!\n");
	KeBugCheck (0x0);
     }
}

VOID STATIC
MiFreeBootDriverMemory(PVOID StartAddress, ULONG Length)
{
  PHYSICAL_ADDRESS Page;
  ULONG i;

  for (i = 0; i < PAGE_ROUND_UP(Length)/PAGE_SIZE; i++)
  {
     Page = MmGetPhysicalAddressForProcess(NULL, StartAddress + i * PAGE_SIZE);
     MmDeleteVirtualMapping(NULL, StartAddress + i * PAGE_SIZE, FALSE, NULL, NULL);
     MmDereferencePage(Page);
  }
}

VOID
ExpInitializeExecutive(VOID)
{
  LARGE_INTEGER Timeout;
  HANDLE ProcessHandle;
  HANDLE ThreadHandle;
  ULONG BootDriverCount;
  ULONG i;
  ULONG start;
  ULONG length;
  PCHAR name;
  CHAR str[50];
  NTSTATUS Status;
  BOOLEAN SetupBoot;

  /*
   * Fail at runtime if someone has changed various structures without
   * updating the offsets used for the assembler code.
   */
  assert(FIELD_OFFSET(KTHREAD, InitialStack) == KTHREAD_INITIAL_STACK);
  assert(FIELD_OFFSET(KTHREAD, Teb) == KTHREAD_TEB);
  assert(FIELD_OFFSET(KTHREAD, KernelStack) == KTHREAD_KERNEL_STACK);
  assert(FIELD_OFFSET(KTHREAD, PreviousMode) == KTHREAD_PREVIOUS_MODE);
  assert(FIELD_OFFSET(KTHREAD, TrapFrame) == KTHREAD_TRAP_FRAME);
  assert(FIELD_OFFSET(KTHREAD, CallbackStack) == KTHREAD_CALLBACK_STACK);
  assert(FIELD_OFFSET(ETHREAD, ThreadsProcess) == ETHREAD_THREADS_PROCESS);
  assert(FIELD_OFFSET(KPROCESS, DirectoryTableBase) == 
	 KPROCESS_DIRECTORY_TABLE_BASE);
  assert(FIELD_OFFSET(KTRAP_FRAME, Reserved9) == KTRAP_FRAME_RESERVED9);
  assert(FIELD_OFFSET(KV86M_TRAP_FRAME, regs) == TF_REGS);
  assert(FIELD_OFFSET(KV86M_TRAP_FRAME, orig_ebp) == TF_ORIG_EBP);
  
  assert(FIELD_OFFSET(KPCR, ExceptionList) == KPCR_EXCEPTION_LIST);
  assert(FIELD_OFFSET(KPCR, Self) == KPCR_SELF);
  assert(FIELD_OFFSET(KPCR, CurrentThread) == KPCR_CURRENT_THREAD);

  LdrInit1();

  KeLowerIrql(DISPATCH_LEVEL);
  
  NtEarlyInitVdm();
  
  MmInit1(FirstKrnlPhysAddr,
	  LastKrnlPhysAddr,
	  LastKernelAddress,
	  (PADDRESS_RANGE)&KeMemoryMap,
	  KeMemoryMapRangeCount);

  /* Import ANSI code page table */
  for (i = 1; i < KeLoaderBlock.ModsCount; i++)
    {
      start = KeLoaderModules[i].ModStart;
      length = KeLoaderModules[i].ModEnd - start;

      name = strrchr((PCHAR)KeLoaderModules[i].String, '\\');
      if (name == NULL)
	{
	  name = (PCHAR)KeLoaderModules[i].String;
	}
      else
	{
	  name++;
	}

      if (!_stricmp (name, "ansi.nls"))
	{
	  RtlpImportAnsiCodePage((PUSHORT)start, length);
	}
    }

  /* Import OEM code page table */
  for (i = 1; i < KeLoaderBlock.ModsCount; i++)
    {
      start = KeLoaderModules[i].ModStart;
      length = KeLoaderModules[i].ModEnd - start;

      name = strrchr((PCHAR)KeLoaderModules[i].String, '\\');
      if (name == NULL)
	{
	  name = (PCHAR)KeLoaderModules[i].String;
	}
      else
	{
	  name++;
	}

      if (!_stricmp (name, "oem.nls"))
	{
	  RtlpImportOemCodePage((PUSHORT)start, length);
	}
    }

  /* Import Unicode casemap table */
  for (i = 1; i < KeLoaderBlock.ModsCount; i++)
    {
      start = KeLoaderModules[i].ModStart;
      length = KeLoaderModules[i].ModEnd - start;

      name = strrchr((PCHAR)KeLoaderModules[i].String, '\\');
      if (name == NULL)
	{
	  name = (PCHAR)KeLoaderModules[i].String;
	}
      else
	{
	  name++;
	}

      if (!_stricmp (name, "casemap.nls"))
	{
	  RtlpImportUnicodeCasemap((PUSHORT)start, length);
	}
    }

  /* Create initial NLS tables */
  RtlpCreateInitialNlsTables();

  /*
   * Initialize the kernel debugger
   */
  KdInitSystem (0, (PLOADER_PARAMETER_BLOCK)&KeLoaderBlock);

  MmInit2();
  KeInit2();
  
  KeLowerIrql(PASSIVE_LEVEL);

  if (!SeInit1())
    KeBugCheck(SECURITY_INITIALIZATION_FAILED);

  ObInit();

  if (!SeInit2())
    KeBugCheck(SECURITY1_INITIALIZATION_FAILED);

  PiInitProcessManager();

  KdInit1();

  if (KdPollBreakIn ())
    {
      DbgBreakPointWithStatus (DBG_STATUS_CONTROL_C);
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
  HalDisplayString("conditions. There is absolutely no warranty for "
		   "ReactOS.\n\n");

  /* Initialize all processors */
  KeNumberProcessors = 0;

  while (!HalAllProcessorsStarted())
    {
      PVOID ProcessorStack;

      if (KeNumberProcessors != 0)
	{
	  KePrepareForApplicationProcessorInit(KeNumberProcessors);
	  PsPrepareForApplicationProcessorInit(KeNumberProcessors);
	}
      /* Allocate a stack for use when booting the processor */
      /* FIXME: The nonpaged memory for the stack is not released after use */
      ProcessorStack = 
	ExAllocatePool(NonPagedPool, MM_STACK_SIZE) + MM_STACK_SIZE;
      Ki386InitialStackArray[((int)KeNumberProcessors)] = 
	(PVOID)(ProcessorStack - MM_STACK_SIZE);
      HalInitializeProcessor(KeNumberProcessors, ProcessorStack);
      KeNumberProcessors++;
    }

  if (KeNumberProcessors > 1)
    {
      sprintf(str,
	      "Found %d system processors. [%lu MB Memory]\n",
	      KeNumberProcessors,
	      (KeLoaderBlock.MemHigher + 1088)/ 1024);
    }
  else
    {
      sprintf(str,
	      "Found 1 system processor. [%lu MB Memory]\n",
	      (KeLoaderBlock.MemHigher + 1088)/ 1024);
    }
  HalDisplayString(str);

  /*
   * Initialize various critical subsystems
   */
  HalInitSystem(1, (PLOADER_PARAMETER_BLOCK)&KeLoaderBlock);

  ExInit();
  IoInit();
  PoInit();
  LdrInitModuleManagement();
  CmInitializeRegistry();
  NtInit();
  MmInit3();
  CcInit();
  KdInit2();
  FsRtlpInitFileLockingImplementation();

  /* Report all resources used by hal */
  HalReportResourceUsage();

  /* Create the NLS section */
  RtlpCreateNlsSection();

  /*
   * Initalize services loaded at boot time
   */
  DPRINT("%d files loaded\n",KeLoaderBlock.ModsCount);
  for (i=0; i < KeLoaderBlock.ModsCount; i++)
    {
      CPRINT("Module: '%s' at %08lx, length 0x%08lx\n",
       KeLoaderModules[i].String,
       KeLoaderModules[i].ModStart,
       KeLoaderModules[i].ModEnd - KeLoaderModules[i].ModStart);
    }

  /* Pass 1: import system hive registry chunk */
  SetupBoot = TRUE;
  for (i = 1; i < KeLoaderBlock.ModsCount; i++)
    {
      start = KeLoaderModules[i].ModStart;
      length = KeLoaderModules[i].ModEnd - start;

      DPRINT("Module: '%s'\n", (PCHAR)KeLoaderModules[i].String);
      name = strrchr((PCHAR)KeLoaderModules[i].String, '\\');
      if (name == NULL)
	{
	  name = (PCHAR)KeLoaderModules[i].String;
	}
      else
	{
	  name++;
	}

      if (!_stricmp (name, "system") ||
	  !_stricmp (name, "system.hiv"))
	{
	  CPRINT("Process system hive registry chunk at %08lx\n", start);
	  SetupBoot = FALSE;
	  CmImportSystemHive((PCHAR)start, length);
	}
    }

  /* Pass 2: import hardware hive registry chunk */
  for (i = 1; i < KeLoaderBlock.ModsCount; i++)
    {
      start = KeLoaderModules[i].ModStart;
      length = KeLoaderModules[i].ModEnd - start;
      name = (PCHAR)KeLoaderModules[i].String;
      if (!_stricmp (name, "hardware") ||
	  !_stricmp (name, "hardware.hiv"))
	{
	  CPRINT("Process hardware hive registry chunk at %08lx\n", start);
	  CmImportHardwareHive((PCHAR)start, length);
	}
    }

  /* Create dummy keys if no hardware hive was found */
  CmImportHardwareHive (NULL, 0);

  /* Initialize volatile registry settings */
  if (SetupBoot == FALSE)
    {
      CmInit2((PCHAR)KeLoaderBlock.CommandLine);
    }

  /*
   * Enter the kernel debugger before starting up the boot drivers
   */
#ifdef KDBG
  KdbEnter();
#endif /* KDBG */

  IoCreateDriverList();

  IoInit2();

  /* Pass 3: process boot loaded drivers */
  BootDriverCount = 0;
  for (i=1; i < KeLoaderBlock.ModsCount; i++)
    {
      start = KeLoaderModules[i].ModStart;
      length = KeLoaderModules[i].ModEnd - start;
      name = (PCHAR)KeLoaderModules[i].String;
      if (RtlpCheckFileNameExtension(name, ".sys") ||
	  RtlpCheckFileNameExtension(name, ".sym"))
	{
	  CPRINT("Initializing driver '%s' at %08lx, length 0x%08lx\n",
	         name, start, length);
	  LdrInitializeBootStartDriver((PVOID)start, name, length);
	}
      if (RtlpCheckFileNameExtension(name, ".sys"))
	BootDriverCount++;
    }

  /* Pass 4: free memory for all boot files, except ntoskrnl.exe and hal.dll */
  for (i = 2; i < KeLoaderBlock.ModsCount; i++)
    {
       MiFreeBootDriverMemory((PVOID)KeLoaderModules[i].ModStart,
			      KeLoaderModules[i].ModEnd - KeLoaderModules[i].ModStart);
    }

  if (BootDriverCount == 0)
    {
      DbgPrint("No boot drivers available.\n");
      KeBugCheck(0);
    }

  /* Create ARC names for boot devices */
  IoCreateArcNames();

  /* Create the SystemRoot symbolic link */
  CPRINT("CommandLine: %s\n", (PUCHAR)KeLoaderBlock.CommandLine);
  Status = IoCreateSystemRootLink((PUCHAR)KeLoaderBlock.CommandLine);
  if (!NT_SUCCESS(Status))
    KeBugCheck(INACCESSIBLE_BOOT_DEVICE);

#ifdef DBGPRINT_FILE_LOG
  /* On the assumption that we can now access disks start up the debug
     logger thread */
  DebugLogInit2();
#endif /* DBGPRINT_FILE_LOG */

#ifdef KDBG
  KdbInitProfiling2();
#endif /* KDBG */


  PiInitDefaultLocale();

  /*
   * Start the motherboard enumerator (the HAL)
   */
  HalInitSystem(2, (PLOADER_PARAMETER_BLOCK)&KeLoaderBlock);
#if 0
  /*
   * Load boot start drivers
   */
  IopLoadBootStartDrivers();
#else
  /*
   * Load Auto configured drivers
   */
  LdrLoadAutoConfigDrivers();
#endif

  IoDestroyDriverList();

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
  Status = LdrLoadInitialProcess(&ProcessHandle,
				 &ThreadHandle);
  if (!NT_SUCCESS(Status))
    {
      KeBugCheckEx(SESSION4_INITIALIZATION_FAILED, Status, 0, 0, 0);
    }

  /*
   * Crash the system if the initial process terminates within 5 seconds.
   */
  Timeout.QuadPart = -50000000LL;
  Status = NtWaitForSingleObject(ProcessHandle,
				 FALSE,
				 &Timeout);
  if (Status != STATUS_TIMEOUT)
    {
      KeBugCheckEx(SESSION5_INITIALIZATION_FAILED, Status, 0, 0, 0);
    }

  NtClose(ThreadHandle);
  NtClose(ProcessHandle);

  PsTerminateSystemThread(STATUS_SUCCESS);
}


VOID
KiSystemStartup(BOOLEAN BootProcessor)
{
  HalInitSystem (0, (PLOADER_PARAMETER_BLOCK)&KeLoaderBlock);

  if (BootProcessor)
    {
      /* Never returns */
      ExpInitializeExecutive();
      KeBugCheck(0);
    }
  /* Do application processor initialization */
  KeApplicationProcessorInit();
  PsApplicationProcessorInit();
  KeLowerIrql(PASSIVE_LEVEL);
  PsIdleThreadMain(NULL);
  KeBugCheck(0);
  for(;;);
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
  ULONG size;
  ULONG last_kernel_address;
  extern ULONG _bss_end__;
  ULONG HalBase;
  ULONG DriverBase;
  ULONG DriverSize;

  /* Low level architecture specific initialization */
  KeInit1();

  /*
   * Copy the parameters to a local buffer because lowmem will go away
   */
  memcpy(&KeLoaderBlock, _LoaderBlock, sizeof(LOADER_PARAMETER_BLOCK));
  memcpy(&KeLoaderModules[1], (PVOID)KeLoaderBlock.ModsAddr,
	 sizeof(LOADER_MODULE) * KeLoaderBlock.ModsCount);
  KeLoaderBlock.ModsCount++;
  KeLoaderBlock.ModsAddr = (ULONG)&KeLoaderModules;

  /*
   * Convert a path specification in the grub format to one understood by the
   * rest of the kernel.
   */
  if (((PUCHAR)_LoaderBlock->CommandLine)[0] == '(')
    {
      ULONG DiskNumber = 0, PartNumber = 0;
      PCH p;
      CHAR Temp[256];
      PCH options;
      PCH s1;

      if (((PUCHAR)_LoaderBlock->CommandLine)[1] == 'h' &&
	  ((PUCHAR)_LoaderBlock->CommandLine)[2] == 'd')
	{
	  DiskNumber = ((PUCHAR)_LoaderBlock->CommandLine)[3] - '0';
	  PartNumber = ((PUCHAR)_LoaderBlock->CommandLine)[5] - '0';
	}
      strcpy(Temp, &((PUCHAR)_LoaderBlock->CommandLine)[7]);
      if ((options = strchr(Temp, ' ')) != NULL)
	{
	  *options = 0;
	  options++;
	}
      else
	{
	  options = "";
	}
      if ((s1 = strrchr(Temp, '/')) != NULL)
	{
	  *s1 = 0;
	  if ((s1 = strrchr(Temp, '/')) != NULL)
	    {
	      *s1 = 0;
	    }
	}
      sprintf(KeLoaderCommandLine, 
	      "multi(0)disk(0)rdisk(%lu)partition(%lu)%s %s",
	      DiskNumber, PartNumber + 1, Temp, options);

      p = KeLoaderCommandLine;
      while (*p != 0 && *p != ' ')
	{
	  if ((*p) == '/')
	    {
	      (*p) = '\\';
	    }
	  p++;
	}
      DPRINT1("Command Line: %s\n", KeLoaderCommandLine);
    }
  else
    {
      strcpy(KeLoaderCommandLine, (PUCHAR)_LoaderBlock->CommandLine);
    }
  KeLoaderBlock.CommandLine = (ULONG)KeLoaderCommandLine;
  
  strcpy(KeLoaderModuleStrings[0], "ntoskrnl.exe");
  KeLoaderModules[0].String = (ULONG)KeLoaderModuleStrings[0];
  KeLoaderModules[0].ModStart = 0xC0000000;
  KeLoaderModules[0].ModEnd = PAGE_ROUND_UP((ULONG)&_bss_end__);
  for (i = 1; i < KeLoaderBlock.ModsCount; i++)
    {      
      CHAR* s;
      if ((s = strrchr((PUCHAR)KeLoaderModules[i].String, '/')) != 0)
	{
	  strcpy(KeLoaderModuleStrings[i], s + 1);
	}
      else
	{
	  strcpy(KeLoaderModuleStrings[i], (PUCHAR)KeLoaderModules[i].String);
	}
      KeLoaderModules[i].ModStart -= 0x200000;
      KeLoaderModules[i].ModStart += 0xc0000000;
      KeLoaderModules[i].ModEnd -= 0x200000;
      KeLoaderModules[i].ModEnd += 0xc0000000;
      KeLoaderModules[i].String = (ULONG)KeLoaderModuleStrings[i];
    }

#ifdef HAL_DBG
  HalnInitializeDisplay((PLOADER_PARAMETER_BLOCK)&KeLoaderBlock);
#endif

  HalBase = KeLoaderModules[1].ModStart;
  DriverBase = 
    PAGE_ROUND_UP(KeLoaderModules[KeLoaderBlock.ModsCount - 1].ModEnd);

  /*
   * Process hal.dll
   */
  LdrSafePEProcessModule((PVOID)HalBase, (PVOID)DriverBase, (PVOID)0xC0000000, &DriverSize);

  LdrHalBase = (ULONG_PTR)DriverBase;
  last_kernel_address = DriverBase + DriverSize;

  /*
   * Process ntoskrnl.exe
   */
  LdrSafePEProcessModule((PVOID)0xC0000000, (PVOID)0xC0000000, (PVOID)DriverBase, &DriverSize);

  FirstKrnlPhysAddr = KeLoaderModules[0].ModStart - 0xc0000000 + 0x200000;
  LastKrnlPhysAddr = last_kernel_address - 0xc0000000 + 0x200000;
  LastKernelAddress = last_kernel_address;

#ifndef ACPI
  /* FIXME: VMware does not like it when ReactOS is using the BIOS memory map */
  KeLoaderBlock.Flags &= ~MB_FLAGS_MMAP_INFO;
#endif

  KeMemoryMapRangeCount = 0;
  if (KeLoaderBlock.Flags & MB_FLAGS_MMAP_INFO)
    {
      /* We have a memory map from the nice BIOS */
      size = *((PULONG)(KeLoaderBlock.MmapAddr - sizeof(ULONG)));
      i = 0;
      while (i < KeLoaderBlock.MmapLength)
        {
          memcpy (&KeMemoryMap[KeMemoryMapRangeCount],
            (PVOID)(KeLoaderBlock.MmapAddr + i),
	          sizeof(ADDRESS_RANGE));
          KeMemoryMapRangeCount++;
          i += size;
        }
    }
  
  KiSystemStartup(1);
}

/* EOF */

