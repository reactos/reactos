/* $Id$
 * 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/main.c
 * PURPOSE:         Initalizes the kernel
 *
 * PROGRAMMERS:     David Welch (welch@cwcom.net)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#include "../dbg/kdb.h"
#include <ntos/bootvid.h>
#include <napi/core.h>

#ifdef HALDBG
#include <internal/ntosdbg.h>
#else
#if defined(_MSC_VER) && (_MSC_VER <= 1200)
#define ps
#else
#define ps(args...)
#endif /* HALDBG */

#endif

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

#define BUILD_OSCSDVERSION(major, minor) (((major & 0xFF) << 8) | (minor & 0xFF))
ULONG NtMajorVersion = 4;
ULONG NtMinorVersion = 0;
ULONG NtOSCSDVersion = BUILD_OSCSDVERSION(6, 0);
#ifdef  __GNUC__
ULONG EXPORTED NtBuildNumber = KERNEL_VERSION_BUILD;
ULONG EXPORTED NtGlobalFlag = 0;
CHAR  EXPORTED KeNumberProcessors;
KAFFINITY  EXPORTED KeActiveProcessors;
LOADER_PARAMETER_BLOCK EXPORTED KeLoaderBlock;
ULONG EXPORTED KeDcacheFlushCount = 0;
ULONG EXPORTED KeIcacheFlushCount = 0;
ULONG EXPORTED KiDmaIoCoherency = 0; /* RISC Architectures only */
ULONG EXPORTED InitSafeBootMode = 0; /* KB83764 */
#else
/* Microsoft-style declarations */
EXPORTED ULONG NtBuildNumber = KERNEL_VERSION_BUILD;
EXPORTED ULONG NtGlobalFlag = 0;
EXPORTED CHAR  KeNumberProcessors;
EXPORTED KAFFINITY KeActiveProcessors;
EXPORTED LOADER_PARAMETER_BLOCK KeLoaderBlock;
EXPORTED ULONG KeDcacheFlushCount = 0;
EXPORTED ULONG KeIcacheFlushCount = 0;
EXPORTED ULONG KiDmaIoCoherency = 0; /* RISC Architectures only */
EXPORTED ULONG InitSafeBootMode = 0; /* KB83764 */
#endif	/* __GNUC__ */

static LOADER_MODULE KeLoaderModules[64];
static CHAR KeLoaderModuleStrings[64][256];
static CHAR KeLoaderCommandLine[256];
static ADDRESS_RANGE KeMemoryMap[64];
static ULONG KeMemoryMapRangeCount;
static ULONG_PTR FirstKrnlPhysAddr;
static ULONG_PTR LastKrnlPhysAddr;
static ULONG_PTR LastKernelAddress;
volatile BOOLEAN Initialized = FALSE;
extern ULONG MmCoreDumpType;
extern CHAR KiTimerSystemAuditing;

extern PVOID Ki386InitialStackArray[MAXIMUM_PROCESSORS];

/* We allocate 4 pages, but we only use 3. The 4th is to guarantee page alignment */
ULONG kernel_stack[4096];
ULONG double_trap_stack[4096];

/* These point to the aligned 3 pages */
ULONG init_stack;
ULONG init_stack_top;
ULONG trap_stack;
ULONG trap_stack_top;

/* FUNCTIONS ****************************************************************/

static VOID INIT_FUNCTION
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

   Ki386SetProcessorFeatures();

   SharedUserData->NtProductType = NtProductWinNt;
   SharedUserData->ProductTypeIsValid = TRUE;
   SharedUserData->NtMajorVersion = 5;
   SharedUserData->NtMinorVersion = 0;

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

	KEBUGCHECK (0x0);
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

	KEBUGCHECK (0x0);
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

   if (BootDriveFound == FALSE)
     {
	DbgPrint("No system drive found!\n");
	KEBUGCHECK (NO_BOOT_DEVICE);
     }
}

VOID INIT_FUNCTION
ExpInitializeExecutive(VOID)
{
  LARGE_INTEGER Timeout;
  HANDLE ProcessHandle;
  HANDLE ThreadHandle;
  ULONG i;
  ULONG start;
  ULONG length;
  PCHAR name;
  CHAR str[50];
  NTSTATUS Status;
  BOOLEAN SetupBoot;
  PCHAR p1, p2;
  ULONG MaxMem;
  BOOLEAN NoGuiBoot = FALSE;
  UNICODE_STRING Name;
  HANDLE InitDoneEventHandle;
  OBJECT_ATTRIBUTES ObjectAttributes;

  /*
   * Fail at runtime if someone has changed various structures without
   * updating the offsets used for the assembler code.
   */
  ASSERT(FIELD_OFFSET(KTHREAD, InitialStack) == KTHREAD_INITIAL_STACK);
  ASSERT(FIELD_OFFSET(KTHREAD, Teb) == KTHREAD_TEB);
  ASSERT(FIELD_OFFSET(KTHREAD, KernelStack) == KTHREAD_KERNEL_STACK);
  ASSERT(FIELD_OFFSET(KTHREAD, NpxState) == KTHREAD_NPX_STATE);
  ASSERT(FIELD_OFFSET(KTHREAD, ServiceTable) == KTHREAD_SERVICE_TABLE);
  ASSERT(FIELD_OFFSET(KTHREAD, PreviousMode) == KTHREAD_PREVIOUS_MODE);
  ASSERT(FIELD_OFFSET(KTHREAD, TrapFrame) == KTHREAD_TRAP_FRAME);
  ASSERT(FIELD_OFFSET(KTHREAD, CallbackStack) == KTHREAD_CALLBACK_STACK);
  ASSERT(FIELD_OFFSET(KTHREAD, ApcState.Process) == KTHREAD_APCSTATE_PROCESS);
  ASSERT(FIELD_OFFSET(KPROCESS, DirectoryTableBase) == 
	 KPROCESS_DIRECTORY_TABLE_BASE);
  ASSERT(FIELD_OFFSET(KPROCESS, IopmOffset) == KPROCESS_IOPM_OFFSET);
  ASSERT(FIELD_OFFSET(KPROCESS, LdtDescriptor) == KPROCESS_LDT_DESCRIPTOR0);
  ASSERT(FIELD_OFFSET(KTRAP_FRAME, Reserved9) == KTRAP_FRAME_RESERVED9);
  ASSERT(FIELD_OFFSET(KV86M_TRAP_FRAME, SavedExceptionStack) == TF_SAVED_EXCEPTION_STACK);
  ASSERT(FIELD_OFFSET(KV86M_TRAP_FRAME, regs) == TF_REGS);
  ASSERT(FIELD_OFFSET(KV86M_TRAP_FRAME, orig_ebp) == TF_ORIG_EBP);

  ASSERT(FIELD_OFFSET(KPCR, Tib.ExceptionList) == KPCR_EXCEPTION_LIST);
  ASSERT(FIELD_OFFSET(KPCR, Self) == KPCR_SELF);
  ASSERT(FIELD_OFFSET(KPCR, PrcbData) + FIELD_OFFSET(KPRCB, CurrentThread) == KPCR_CURRENT_THREAD);  
  ASSERT(FIELD_OFFSET(KPCR, PrcbData) + FIELD_OFFSET(KPRCB, NpxThread) == KPCR_NPX_THREAD);

  ASSERT(FIELD_OFFSET(KTSS, Esp0) == KTSS_ESP0);
  ASSERT(FIELD_OFFSET(KTSS, Eflags) == KTSS_EFLAGS);
  ASSERT(FIELD_OFFSET(KTSS, IoMapBase) == KTSS_IOMAPBASE);

  ASSERT(sizeof(FX_SAVE_AREA) == SIZEOF_FX_SAVE_AREA);

  LdrInit1();

  KeLowerIrql(DISPATCH_LEVEL);
  
  NtEarlyInitVdm();

  p1 = (PCHAR)KeLoaderBlock.CommandLine;

  MaxMem = 0;
  while(*p1 && (p2 = strchr(p1, '/')))
  {
     p2++;
     if (!_strnicmp(p2, "MAXMEM", 6))
     {
        p2 += 6;
        while (isspace(*p2)) p2++;
	if (*p2 == '=')
	{
	   p2++;
	   while(isspace(*p2)) p2++;
	   if (isdigit(*p2))
	   {
	      while (isdigit(*p2))
	      {
	         MaxMem = MaxMem * 10 + *p2 - '0';
		 p2++;
	      }
	      break;
	   }
	}
     }
     else if (!_strnicmp(p2, "NOGUIBOOT", 9))
     {
       p2 += 9;
       NoGuiBoot = TRUE;
     }
     else if (!_strnicmp(p2, "CRASHDUMP", 9))
     {
       p2 += 9;
       if (*p2 == ':')
	 {
	   p2++;
	   if (!_strnicmp(p2, "FULL", 4))
	     {
	       MmCoreDumpType = MM_CORE_DUMP_TYPE_FULL;
	     }
	   else
	     {
	       MmCoreDumpType = MM_CORE_DUMP_TYPE_NONE;
	     }
	 }
     }
     p1 = p2;
  }

  MmInit1(FirstKrnlPhysAddr,
	  LastKrnlPhysAddr,
	  LastKernelAddress,
	  (PADDRESS_RANGE)&KeMemoryMap,
	  KeMemoryMapRangeCount,
	  MaxMem > 8 ? MaxMem : 4096);

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
  KdInitSystem (1, (PLOADER_PARAMETER_BLOCK)&KeLoaderBlock);

  KeInit2();
  
#if 0
  if (KeMemoryMapRangeCount > 0)
    {
      DPRINT1("MemoryMap:\n");
      for (i = 0; i < KeMemoryMapRangeCount; i++)
        {
          switch(KeMemoryMap[i].Type)
            {
              case 1:
	        strcpy(str, "(usable)");
	        break;
	      case 2:
	        strcpy(str, "(reserved)");
	        break;
	      case 3:
	        strcpy(str, "(ACPI data)");
	        break;
	      case 4:
	        strcpy(str, "(ACPI NVS)");
	        break;
	      default:
	        sprintf(str, "type %lu", KeMemoryMap[i].Type);
            }
          DPRINT1("%08x - %08x %s\n", KeMemoryMap[i].BaseAddrLow, KeMemoryMap[i].BaseAddrLow + KeMemoryMap[i].LengthLow, str);
	}
    }
#endif

  KeLowerIrql(PASSIVE_LEVEL);

  if (!SeInit1())
    KEBUGCHECK(SECURITY_INITIALIZATION_FAILED);

  ObInit();
  ExInit2();
  MmInit2();

  if (!SeInit2())
    KEBUGCHECK(SECURITY1_INITIALIZATION_FAILED);

  KeNumberProcessors = 1;

  PiInitProcessManager();

  if (KdPollBreakIn ())
    {
      DbgBreakPointWithStatus (DBG_STATUS_CONTROL_C);
    }

  /* Initialize all processors */
  while (!HalAllProcessorsStarted())
    {
      PVOID ProcessorStack;

      KePrepareForApplicationProcessorInit(KeNumberProcessors);
      PsPrepareForApplicationProcessorInit(KeNumberProcessors);

      /* Allocate a stack for use when booting the processor */
      ProcessorStack = Ki386InitialStackArray[((int)KeNumberProcessors)] + MM_STACK_SIZE;

      HalStartNextProcessor(0, (ULONG)ProcessorStack - 2*sizeof(FX_SAVE_AREA));
      KeNumberProcessors++;
    }

  /*
   * Initialize various critical subsystems
   */
  HalInitSystem(1, (PLOADER_PARAMETER_BLOCK)&KeLoaderBlock);

  ExInit3();
  KdInit1();
  IoInit();
  PoInit(&KeLoaderBlock, FALSE);
  CmInitializeRegistry();
  MmInit3();
  CcInit();
  KdInit2();
  FsRtlpInitFileLockingImplementation();

  /* Report all resources used by hal */
  HalReportResourceUsage();  

  /*
   * Clear the screen to blue
   */
  HalInitSystem(2, (PLOADER_PARAMETER_BLOCK)&KeLoaderBlock);

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

  KdInit3();


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

  /* Initialize the time zone information from the registry */
  ExpInitTimeZoneInfo();

  /*
   * Enter the kernel debugger before starting up the boot drivers
   */
#ifdef KDBG
  KdbEnter();
#endif /* KDBG */

  IoCreateDriverList();

  IoInit2();

  /* Initialize Callbacks before drivers */
  ExpInitializeCallbacks();

  /* Start boot logging */
  IopInitBootLog();
  p1 = (PCHAR)KeLoaderBlock.CommandLine;
  while (*p1 && (p2 = strchr(p1, '/')))
  {
    p2++;
    if (!_strnicmp(p2, "BOOTLOG", 7))
    {
      p2 += 7;
      IopStartBootLog();
    }

    p1 = p2;
  }

  /*
   * Load boot start drivers
   */
  IopInitializeBootDrivers();

  /* Display the boot screen image if not disabled */
  if (!NoGuiBoot)
    {
      InbvEnableBootDriver(TRUE);
    }

  /* Create ARC names for boot devices */
  IoCreateArcNames();

  /* Create the SystemRoot symbolic link */
  CPRINT("CommandLine: %s\n", (PCHAR)KeLoaderBlock.CommandLine);
  DPRINT1("MmSystemRangeStart: 0x%x PageDir: 0x%x\n", MmSystemRangeStart, KeLoaderBlock.PageDirectoryStart);
  Status = IoCreateSystemRootLink((PCHAR)KeLoaderBlock.CommandLine);
  if (!NT_SUCCESS(Status))
  {
    DbgPrint ( "IoCreateSystemRootLink FAILED: (0x%x) - ", Status );
    DbgPrintErrorMessage ( Status );
    KEBUGCHECK(INACCESSIBLE_BOOT_DEVICE);
  }

#if defined(KDBG)
  KdbInit();
#endif /* KDBG */
#if defined(KDBG) || defined(DBG)
  KdbInitProfiling2();
#endif /* KDBG || DBG */

  /* On the assumption that we can now access disks start up the debug
   * logger thread */
  if ((KdDebuggerEnabled == TRUE) && (KdDebugState & KD_DEBUG_FILELOG))
    {
      DebugLogInit2();
    }

  PiInitDefaultLocale();

  /*
   * Load services for devices found by PnP manager
   */
  IopInitializePnpServices(IopRootDeviceNode, FALSE);

  /*
   * Load system start drivers
   */
  IopInitializeSystemDrivers();

  IoDestroyDriverList();

  /* Stop boot logging */
  IopStopBootLog();

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
  InitSystemSharedUserPage ((PCHAR)KeLoaderBlock.CommandLine);

  /* Create 'ReactOSInitDone' event */
  RtlInitUnicodeString(&Name, L"\\ReactOSInitDone");
  InitializeObjectAttributes(&ObjectAttributes,
    &Name,
    0,
    NULL,
    NULL);
  Status = ZwCreateEvent(&InitDoneEventHandle,
    EVENT_ALL_ACCESS,
    &ObjectAttributes,
    SynchronizationEvent,
    FALSE);             /* Not signalled */
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("Failed to create 'ReactOSInitDone' event (Status 0x%x)\n", Status);
      InitDoneEventHandle = INVALID_HANDLE_VALUE;
    }

  /*
   *  Launch initial process
   */
  Status = LdrLoadInitialProcess(&ProcessHandle,
				 &ThreadHandle);
  if (!NT_SUCCESS(Status))
    {
      KEBUGCHECKEX(SESSION4_INITIALIZATION_FAILED, Status, 0, 0, 0);
    }

  if (InitDoneEventHandle != INVALID_HANDLE_VALUE)
    {
      HANDLE Handles[2]; /* Init event, Initial process */

      Handles[0] = InitDoneEventHandle;
      Handles[1] = ProcessHandle;

      /* Wait for the system to be initialized */
      Timeout.QuadPart = (LONGLONG)-1200000000;  /* 120 second timeout */
      Status = ZwWaitForMultipleObjects(((LONG) sizeof(Handles) / sizeof(HANDLE)),
        Handles,
        WaitAny,
        FALSE,    /* Non-alertable */
        &Timeout);
      if (!NT_SUCCESS(Status))
        {
          DPRINT1("NtWaitForMultipleObjects failed with status 0x%x!\n", Status);
        }
      else if (Status == STATUS_TIMEOUT)
        {
          DPRINT1("WARNING: System not initialized after 120 seconds.\n");
        }
      else if (Status == STATUS_WAIT_0 + 1)
        {
          /*
           * Crash the system if the initial process was terminated.
           */
          KEBUGCHECKEX(SESSION5_INITIALIZATION_FAILED, Status, 0, 0, 0);
        }

      if (!NoGuiBoot)
        {
          InbvEnableBootDriver(FALSE);
        }

      ZwSetEvent(InitDoneEventHandle, NULL);

      ZwClose(InitDoneEventHandle);
    }
  else
    {
      /* On failure to create 'ReactOSInitDone' event, go to text mode ASAP */
      if (!NoGuiBoot)
        {
          InbvEnableBootDriver(FALSE);
        }

      /*
       * Crash the system if the initial process terminates within 5 seconds.
       */
      Timeout.QuadPart = (LONGLONG)-50000000;  /* 5 second timeout */
      Status = ZwWaitForSingleObject(ProcessHandle,
    				 FALSE,
    				 &Timeout);
      if (Status != STATUS_TIMEOUT)
        {
          KEBUGCHECKEX(SESSION5_INITIALIZATION_FAILED, Status, 1, 0, 0);
        }
    }
/*
 * Tell ke/timer.c it's okay to run.
 */

  KiTimerSystemAuditing = 1;

  ZwClose(ThreadHandle);
  ZwClose(ProcessHandle);
}

VOID __attribute((noinline))
KiSystemStartup(BOOLEAN BootProcessor)
{
  DPRINT1("KiSystemStartup(%d)\n", BootProcessor);
  if (BootProcessor)
  {
  }
  else
  {
     KeApplicationProcessorInit();
  }

  HalInitializeProcessor(KeNumberProcessors, (PLOADER_PARAMETER_BLOCK)&KeLoaderBlock);

  if (BootProcessor)
  {
     ExpInitializeExecutive();
     MiFreeInitMemory();
     /* Never returns */
     PsTerminateSystemThread(STATUS_SUCCESS);
  }
  else
  {
     /* Do application processor initialization */
     PsApplicationProcessorInit();
     KeLowerIrql(PASSIVE_LEVEL);
     PsIdleThreadMain(NULL);
  }
  KEBUGCHECK(0);
  for(;;);
}

VOID INIT_FUNCTION
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
  ULONG HalBase;
  ULONG DriverBase;
  ULONG DriverSize;

  /* Set up the Stacks */
  trap_stack = PAGE_ROUND_UP(&double_trap_stack);
  trap_stack_top = trap_stack + 3 * PAGE_SIZE;
  init_stack = PAGE_ROUND_UP(&kernel_stack);
  init_stack_top = init_stack + 3 * PAGE_SIZE;
            
  /*
   * Copy the parameters to a local buffer because lowmem will go away
   */
  memcpy(&KeLoaderBlock, _LoaderBlock, sizeof(LOADER_PARAMETER_BLOCK));
  memcpy(&KeLoaderModules[1], (PVOID)KeLoaderBlock.ModsAddr,
	 sizeof(LOADER_MODULE) * KeLoaderBlock.ModsCount);
  KeLoaderBlock.ModsCount++;
  KeLoaderBlock.ModsAddr = (ULONG)&KeLoaderModules;

  /* Save the Base Address */
  MmSystemRangeStart = (PVOID)KeLoaderBlock.KernelBase;
  
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
	  DiskNumber = ((PCHAR)_LoaderBlock->CommandLine)[3] - '0';
	  PartNumber = ((PCHAR)_LoaderBlock->CommandLine)[5] - '0';
	}
      strcpy(Temp, &((PCHAR)_LoaderBlock->CommandLine)[7]);
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
      strcpy(KeLoaderCommandLine, (PCHAR)_LoaderBlock->CommandLine);
    }
  KeLoaderBlock.CommandLine = (ULONG)KeLoaderCommandLine;
  
  strcpy(KeLoaderModuleStrings[0], "ntoskrnl.exe");
  KeLoaderModules[0].String = (ULONG)KeLoaderModuleStrings[0];
  KeLoaderModules[0].ModStart = KERNEL_BASE;
  /* Take this value from the PE... */
    PIMAGE_NT_HEADERS      NtHeader = RtlImageNtHeader((PVOID)KeLoaderModules[0].ModStart);
    PIMAGE_OPTIONAL_HEADER OptHead  = &NtHeader->OptionalHeader;
    KeLoaderModules[0].ModEnd = KeLoaderModules[0].ModStart + PAGE_ROUND_UP((ULONG)OptHead->SizeOfImage);
  for (i = 1; i < KeLoaderBlock.ModsCount; i++)
    {      
      CHAR* s;
      if ((s = strrchr((PCHAR)KeLoaderModules[i].String, '/')) != 0)
	{
	  strcpy(KeLoaderModuleStrings[i], s + 1);
	}
      else
	{
	  strcpy(KeLoaderModuleStrings[i], (PCHAR)KeLoaderModules[i].String);
	}
      KeLoaderModules[i].ModStart -= 0x200000;
      KeLoaderModules[i].ModStart += KERNEL_BASE;
      KeLoaderModules[i].ModEnd -= 0x200000;
      KeLoaderModules[i].ModEnd += KERNEL_BASE;
      KeLoaderModules[i].String = (ULONG)KeLoaderModuleStrings[i];
    }

  LastKernelAddress = PAGE_ROUND_UP(KeLoaderModules[KeLoaderBlock.ModsCount - 1].ModEnd);

  /* Low level architecture specific initialization */
  KeInit1((PCHAR)KeLoaderBlock.CommandLine, &LastKernelAddress);

  HalBase = KeLoaderModules[1].ModStart;
  DriverBase = LastKernelAddress;
  LdrHalBase = (ULONG_PTR)DriverBase;

  LdrInitModuleManagement();

  /*
   * Process hal.dll
   */
  LdrSafePEProcessModule((PVOID)HalBase, (PVOID)DriverBase, (PVOID)KERNEL_BASE, &DriverSize);

  LastKernelAddress += PAGE_ROUND_UP(DriverSize);

  /*
   * Process ntoskrnl.exe
   */
  LdrSafePEProcessModule((PVOID)KERNEL_BASE, (PVOID)KERNEL_BASE, (PVOID)DriverBase, &DriverSize);

  /* Now our imports from HAL are fixed. This is the first */
  /* time in the boot process that we can use HAL          */

  FirstKrnlPhysAddr = KeLoaderModules[0].ModStart - KERNEL_BASE + 0x200000;
  LastKrnlPhysAddr = LastKernelAddress - KERNEL_BASE + 0x200000;

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
      KeLoaderBlock.MmapLength = KeMemoryMapRangeCount * sizeof(ADDRESS_RANGE);
      KeLoaderBlock.MmapAddr = (ULONG)KeMemoryMap;
    }
  else
    {
      KeLoaderBlock.MmapLength = 0;
      KeLoaderBlock.MmapAddr = (ULONG)KeMemoryMap;
    }

  KdInitSystem (0, (PLOADER_PARAMETER_BLOCK)&KeLoaderBlock);
  HalInitSystem (0, (PLOADER_PARAMETER_BLOCK)&KeLoaderBlock);

  DPRINT1("_main (%x, %x)\n", MultiBootMagic, _LoaderBlock);


  KiSystemStartup(1);
}

/* EOF */

