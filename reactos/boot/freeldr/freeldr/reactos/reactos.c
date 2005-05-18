/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         Freeloader
 * FILE:            boot/freeldr/freeldr/reactos/rosboot.c
 * PURPOSE:         ReactOS Loader
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 */

#include <freeldr.h>
#include <internal/i386/ke.h>
#include <reactos/rossym.h>

#include "registry.h"

#define NDEBUG
#include <debug.h>

BOOL
STDCALL
FrLdrLoadKernel(PCHAR szFileName,
                INT nPos)
{
    PFILE FilePointer;
    PCHAR szShortName;
    CHAR szBuffer[256];

    /* Extract Kernel filename without path */
    szShortName = strrchr(szFileName, '\\');
    if (szShortName == NULL) {

        /* No path, leave it alone */
        szShortName = szFileName;

    } else {

        /* Skip the path */
        szShortName = szShortName + 1;
    }

    /* Open the Kernel */
    FilePointer = FsOpenFile(szFileName);

    /* Make sure it worked */
    if (FilePointer == NULL) {

        /* Return failure on the short name */
        strcpy(szBuffer, szShortName);
        strcat(szBuffer, " not found.");
        UiMessageBox(szBuffer);
        return(FALSE);
    }

    /* Update the status bar with the current file */
    strcpy(szBuffer, "Reading ");
    strcat(szBuffer, szShortName);
    UiDrawStatusText(szBuffer);

    /* Do the actual loading */
    FrLdrMapKernel(FilePointer);

    /* Update Processbar and return success */
    UiDrawProgressBarCenter(nPos, 100, "Loading ReactOS...");
    return(TRUE);
}

static VOID
FreeldrFreeMem(PVOID Area)
{
  MmFreeMemory(Area);
}

static PVOID
FreeldrAllocMem(ULONG_PTR Size)
{
  return MmAllocateMemory((ULONG) Size);
}

static BOOLEAN
FreeldrReadFile(PVOID FileContext, PVOID Buffer, ULONG Size)
{
  ULONG BytesRead;

  return FsReadFile((PFILE) FileContext, (ULONG) Size, &BytesRead, Buffer)
         && Size == BytesRead;
}

static BOOLEAN
FreeldrSeekFile(PVOID FileContext, ULONG_PTR Position)
{
  FsSetFilePointer((PFILE) FileContext, (ULONG) Position);
    return TRUE;
}

static BOOL
LoadKernelSymbols(PCHAR szKernelName, int nPos)
{
  static ROSSYM_CALLBACKS FreeldrCallbacks =
    {
      FreeldrAllocMem,
      FreeldrFreeMem,
      FreeldrReadFile,
      FreeldrSeekFile
    };
  PFILE FilePointer;
  PROSSYM_INFO RosSymInfo;
  ULONG Size;
  ULONG_PTR Base;

  RosSymInit(&FreeldrCallbacks);

  FilePointer = FsOpenFile(szKernelName);
  if (FilePointer == NULL)
    {
      return FALSE;
    }
  if (! RosSymCreateFromFile(FilePointer, &RosSymInfo))
    {
      return FALSE;
    }
  Base = FrLdrCreateModule("NTOSKRNL.SYM");
  Size = RosSymGetRawDataLength(RosSymInfo);
  RosSymGetRawData(RosSymInfo, (PVOID)Base);
  FrLdrCloseModule(Base, Size);
  RosSymDelete(RosSymInfo);
  return TRUE;
}

BOOL
FrLdrLoadNlsFile(PCHAR szFileName,
                 PCHAR szModuleName)
{
    PFILE FilePointer;
    CHAR value[256];
    LPSTR p;

    /* Open the Driver */
    FilePointer = FsOpenFile(szFileName);

    /* Make sure we did */
    if (FilePointer == NULL) {

        /* Fail if file wasn't opened */
        strcpy(value, szFileName);
        strcat(value, " not found.");
        UiMessageBox(value);
        return(FALSE);
    }

    /* Update the status bar with the current file */
    strcpy(value, "Reading ");
    p = strrchr(szFileName, '\\');
    if (p == NULL) {

        strcat(value, szFileName);

    } else {

        strcat(value, p + 1);
    }
    UiDrawStatusText(value);

    /* Load the driver */
    FrLdrLoadModule(FilePointer, szModuleName, NULL);
    return(TRUE);
}

BOOL
FrLdrLoadNlsFiles(PCHAR szSystemRoot,
                  PCHAR szErrorOut)
{
    LONG rc = ERROR_SUCCESS;
    FRLDRHKEY hKey;
    CHAR szIdBuffer[80];
    CHAR szNameBuffer[80];
    CHAR szFileName[256];
    ULONG BufferSize;

    /* open the codepage key */
    rc = RegOpenKey(NULL,
                    "\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Control\\NLS\\CodePage",
                    &hKey);
    if (rc != ERROR_SUCCESS) {

        strcpy(szErrorOut, "Couldn't open CodePage registry key");
        return(FALSE);
    }

    /* get ANSI codepage */
    BufferSize = 80;
    rc = RegQueryValue(hKey, "ACP", NULL, (PUCHAR)szIdBuffer, &BufferSize);
    if (rc != ERROR_SUCCESS) {

        strcpy(szErrorOut, "Couldn't get ACP NLS setting");
        return(FALSE);
    }

    BufferSize = 80;
    rc = RegQueryValue(hKey, szIdBuffer, NULL, (PUCHAR)szNameBuffer, &BufferSize);
    if (rc != ERROR_SUCCESS) {

        strcpy(szErrorOut, "ACP NLS Setting exists, but isn't readable");
        return(FALSE);
    }

    /* load ANSI codepage table */
    strcpy(szFileName, szSystemRoot);
    strcat(szFileName, "system32\\");
    strcat(szFileName, szNameBuffer);
    DbgPrint((DPRINT_REACTOS, "ANSI file: %s\n", szFileName));
    if (!FrLdrLoadNlsFile(szFileName, "ansi.nls")) {

        strcpy(szErrorOut, "Couldn't load ansi.nls");
        return(FALSE);
    }

    /* get OEM codepage */
    BufferSize = 80;
    rc = RegQueryValue(hKey, "OEMCP", NULL, (PUCHAR)szIdBuffer, &BufferSize);
    if (rc != ERROR_SUCCESS) {

        strcpy(szErrorOut, "Couldn't get OEMCP NLS setting");
        return(FALSE);
    }

    BufferSize = 80;
    rc = RegQueryValue(hKey, szIdBuffer, NULL, (PUCHAR)szNameBuffer, &BufferSize);
    if (rc != ERROR_SUCCESS) {

        strcpy(szErrorOut, "OEMCP NLS setting exists, but isn't readable");
        return(FALSE);
    }

    /* load OEM codepage table */
    strcpy(szFileName, szSystemRoot);
    strcat(szFileName, "system32\\");
    strcat(szFileName, szNameBuffer);
    DbgPrint((DPRINT_REACTOS, "Oem file: %s\n", szFileName));
    if (!FrLdrLoadNlsFile(szFileName, "oem.nls")) {

        strcpy(szErrorOut, "Couldn't load oem.nls");
        return(FALSE);
    }

    /* open the language key */
    rc = RegOpenKey(NULL,
                    "\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Control\\NLS\\Language",
                    &hKey);
    if (rc != ERROR_SUCCESS) {

        strcpy(szErrorOut, "Couldn't open Language registry key");
        return(FALSE);
    }

    /* get the Unicode case table */
    BufferSize = 80;
    rc = RegQueryValue(hKey, "Default", NULL, (PUCHAR)szIdBuffer, &BufferSize);
    if (rc != ERROR_SUCCESS) {

        strcpy(szErrorOut, "Couldn't get Language Default setting");
        return(FALSE);
    }

    BufferSize = 80;
    rc = RegQueryValue(hKey, szIdBuffer, NULL, (PUCHAR)szNameBuffer, &BufferSize);
    if (rc != ERROR_SUCCESS) {

        strcpy(szErrorOut, "Language Default setting exists, but isn't readable");
        return(FALSE);
    }

    /* load Unicode case table */
    strcpy(szFileName, szSystemRoot);
    strcat(szFileName, "system32\\");
    strcat(szFileName, szNameBuffer);
    DbgPrint((DPRINT_REACTOS, "Casemap file: %s\n", szFileName));
    if (!FrLdrLoadNlsFile(szFileName, "casemap.nls")) {

        strcpy(szErrorOut, "casemap.nls");
        return(FALSE);
    }

    return(TRUE);
}

BOOL
FrLdrLoadDriver(PCHAR szFileName,
                INT nPos)
{
    PFILE FilePointer;
    CHAR value[256];
    LPSTR p;

    /* Open the Driver */
    FilePointer = FsOpenFile(szFileName);

    /* Make sure we did */
    if (FilePointer == NULL) {

        /* Fail if file wasn't opened */
        strcpy(value, szFileName);
        strcat(value, " not found.");
        UiMessageBox(value);
        return(FALSE);
    }

    /* Update the status bar with the current file */
    strcpy(value, "Reading ");
    p = strrchr(szFileName, '\\');
    if (p == NULL) {

        strcat(value, szFileName);

    } else {

        strcat(value, p + 1);

    }
    UiDrawStatusText(value);

    /* Load the driver */
    FrLdrLoadModule(FilePointer, szFileName, NULL);

    /* Update status and return */
    UiDrawProgressBarCenter(nPos, 100, "Loading ReactOS...");
    return(TRUE);
}

VOID
FrLdrLoadBootDrivers(PCHAR szSystemRoot,
                     INT nPos)
{
    LONG rc = 0;
    FRLDRHKEY hGroupKey, hOrderKey, hServiceKey, hDriverKey;
    CHAR GroupNameBuffer[512];
    CHAR ServiceName[256];
    ULONG OrderList[128];
    ULONG BufferSize;
    ULONG Index;
    ULONG TagIndex;
    LPSTR GroupName;

    ULONG ValueSize;
    ULONG ValueType;
    ULONG StartValue;
    ULONG TagValue;
    CHAR DriverGroup[256];
    ULONG DriverGroupSize;

    CHAR ImagePath[256];
    CHAR TempImagePath[256];

    /* get 'service group order' key */
    rc = RegOpenKey(NULL,
                    "\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Control\\ServiceGroupOrder",
                    &hGroupKey);
    if (rc != ERROR_SUCCESS) {

        DbgPrint((DPRINT_REACTOS, "Failed to open the 'ServiceGroupOrder' key (rc %d)\n", (int)rc));
        return;
    }

    /* get 'group order list' key */
    rc = RegOpenKey(NULL,
                    "\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Control\\GroupOrderList",
                    &hOrderKey);
    if (rc != ERROR_SUCCESS) {

        DbgPrint((DPRINT_REACTOS, "Failed to open the 'GroupOrderList' key (rc %d)\n", (int)rc));
        return;
    }

    /* enumerate drivers */
    rc = RegOpenKey(NULL,
                    "\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Services",
                    &hServiceKey);
    if (rc != ERROR_SUCCESS)  {

        DbgPrint((DPRINT_REACTOS, "Failed to open the 'Services' key (rc %d)\n", (int)rc));
        return;
    }

    /* Get the Name Group */
    BufferSize = sizeof(GroupNameBuffer);
    rc = RegQueryValue(hGroupKey, "List", NULL, (PUCHAR)GroupNameBuffer, &BufferSize);
    DbgPrint((DPRINT_REACTOS, "RegQueryValue(): rc %d\n", (int)rc));
    if (rc != ERROR_SUCCESS) return;
    DbgPrint((DPRINT_REACTOS, "BufferSize: %d \n", (int)BufferSize));
    DbgPrint((DPRINT_REACTOS, "GroupNameBuffer: '%s' \n", GroupNameBuffer));

    /* Loop through each group */
    GroupName = GroupNameBuffer;
    while (*GroupName) {
        DbgPrint((DPRINT_REACTOS, "Driver group: '%s'\n", GroupName));

        /* Query the Order */
        BufferSize = sizeof(OrderList);
        rc = RegQueryValue(hOrderKey, GroupName, NULL, (PUCHAR)OrderList, &BufferSize);
        if (rc != ERROR_SUCCESS) OrderList[0] = 0;

        /* enumerate all drivers */
        for (TagIndex = 1; TagIndex <= OrderList[0]; TagIndex++) {

            Index = 0;

            while (TRUE) {

                /* Get the Driver's Name */
                ValueSize = sizeof(ServiceName);
                rc = RegEnumKey(hServiceKey, Index, ServiceName, &ValueSize);
                DbgPrint((DPRINT_REACTOS, "RegEnumKey(): rc %d\n", (int)rc));

                /* Makre sure it's valid, and check if we're done */
                if (rc == ERROR_NO_MORE_ITEMS) break;
                if (rc != ERROR_SUCCESS) return;
                DbgPrint((DPRINT_REACTOS, "Service %d: '%s'\n", (int)Index, ServiceName));

                /* open driver Key */
                rc = RegOpenKey(hServiceKey, ServiceName, &hDriverKey);

                /* Read the Start Value */
                ValueSize = sizeof(ULONG);
                rc = RegQueryValue(hDriverKey, "Start", &ValueType, (PUCHAR)&StartValue, &ValueSize);
                DbgPrint((DPRINT_REACTOS, "  Start: %x  \n", (int)StartValue));

                /* Read the Tag */
                ValueSize = sizeof(ULONG);
                rc = RegQueryValue(hDriverKey, "Tag", &ValueType, (PUCHAR)&TagValue, &ValueSize);
                if (rc != ERROR_SUCCESS) TagValue = (ULONG)-1;
                DbgPrint((DPRINT_REACTOS, "  Tag:   %x  \n", (int)TagValue));

                /* Read the driver's group */
                DriverGroupSize = 256;
                rc = RegQueryValue(hDriverKey, "Group", NULL, (PUCHAR)DriverGroup, &DriverGroupSize);
                DbgPrint((DPRINT_REACTOS, "  Group: '%s'  \n", DriverGroup));

                /* Make sure it should be started */
                if ((StartValue == 0) &&
                    (TagValue == OrderList[TagIndex]) &&
                    (stricmp(DriverGroup, GroupName) == 0)) {

                    /* Get the Driver's Location */
                    ValueSize = 256;
                    rc = RegQueryValue(hDriverKey, "ImagePath", NULL, (PUCHAR)TempImagePath, &ValueSize);

                    /* Write the whole path if it suceeded, else prepare to fail */
                    if (rc != ERROR_SUCCESS) {
                        DbgPrint((DPRINT_REACTOS, "  ImagePath: not found\n"));
                        strcpy(ImagePath, szSystemRoot);
                        strcat(ImagePath, "system32\\drivers\\");
                        strcat(ImagePath, ServiceName);
                        strcat(ImagePath, ".sys");
                    } else if (TempImagePath[0] != '\\') {
                        strcpy(ImagePath, szSystemRoot);
                        strcat(ImagePath, TempImagePath);
                    } else {
                        strcpy(ImagePath, TempImagePath);
                        DbgPrint((DPRINT_REACTOS, "  ImagePath: '%s'\n", ImagePath));
                    }

                    DbgPrint((DPRINT_REACTOS, "  Loading driver: '%s'\n", ImagePath));

                    /* Update the position if needed */
                    if (nPos < 100) nPos += 5;

                    FrLdrLoadDriver(ImagePath, nPos);

                } else {

                    DbgPrint((DPRINT_REACTOS, "  Skipping driver '%s' with Start %d, Tag %d and Group '%s' (Current Tag %d, current group '%s')\n",
                    ServiceName, StartValue, TagValue, DriverGroup, OrderList[TagIndex], GroupName));
                }

                Index++;
            }
        }

        Index = 0;
        while (TRUE) {

            /* Get the Driver's Name */
            ValueSize = sizeof(ServiceName);
            rc = RegEnumKey(hServiceKey, Index, ServiceName, &ValueSize);

            DbgPrint((DPRINT_REACTOS, "RegEnumKey(): rc %d\n", (int)rc));
            if (rc == ERROR_NO_MORE_ITEMS) break;
            if (rc != ERROR_SUCCESS) return;
            DbgPrint((DPRINT_REACTOS, "Service %d: '%s'\n", (int)Index, ServiceName));

            /* open driver Key */
            rc = RegOpenKey(hServiceKey, ServiceName, &hDriverKey);

            /* Read the Start Value */
            ValueSize = sizeof(ULONG);
            rc = RegQueryValue(hDriverKey, "Start", &ValueType, (PUCHAR)&StartValue, &ValueSize);
            DbgPrint((DPRINT_REACTOS, "  Start: %x  \n", (int)StartValue));

            /* Read the Tag */
            ValueSize = sizeof(ULONG);
            rc = RegQueryValue(hDriverKey, "Tag", &ValueType, (PUCHAR)&TagValue, &ValueSize);
            if (rc != ERROR_SUCCESS) TagValue = (ULONG)-1;
            DbgPrint((DPRINT_REACTOS, "  Tag:   %x  \n", (int)TagValue));

            /* Read the driver's group */
            DriverGroupSize = 256;
            rc = RegQueryValue(hDriverKey, "Group", NULL, (PUCHAR)DriverGroup, &DriverGroupSize);
            DbgPrint((DPRINT_REACTOS, "  Group: '%s'  \n", DriverGroup));

            for (TagIndex = 1; TagIndex <= OrderList[0]; TagIndex++) {
                if (TagValue == OrderList[TagIndex]) break;
            }

            if ((StartValue == 0) &&
                (TagIndex > OrderList[0]) &&
                (stricmp(DriverGroup, GroupName) == 0)) {

                    ValueSize = 256;
                    rc = RegQueryValue(hDriverKey, "ImagePath", NULL, (PUCHAR)TempImagePath, &ValueSize);
                    if (rc != ERROR_SUCCESS) {
                        DbgPrint((DPRINT_REACTOS, "  ImagePath: not found\n"));
                        strcpy(ImagePath, szSystemRoot);
                        strcat(ImagePath, "system32\\drivers\\");
                        strcat(ImagePath, ServiceName);
                        strcat(ImagePath, ".sys");
                    } else if (TempImagePath[0] != '\\') {
                        strcpy(ImagePath, szSystemRoot);
                        strcat(ImagePath, TempImagePath);
                    } else {
                        strcpy(ImagePath, TempImagePath);
                        DbgPrint((DPRINT_REACTOS, "  ImagePath: '%s'\n", ImagePath));
                    }
                DbgPrint((DPRINT_REACTOS, "  Loading driver: '%s'\n", ImagePath));

                if (nPos < 100) nPos += 5;

                FrLdrLoadDriver(ImagePath, nPos);

            } else {

                DbgPrint((DPRINT_REACTOS, "  Skipping driver '%s' with Start %d, Tag %d and Group '%s' (Current group '%s')\n",
                ServiceName, StartValue, TagValue, DriverGroup, GroupName));
            }

            Index++;
        }

        /* Move to the next group name */
        GroupName = GroupName + strlen(GroupName) + 1;
    }
}

VOID
LoadAndBootReactOS(PCHAR OperatingSystemName)
{
	PFILE FilePointer;
	CHAR  name[1024];
	CHAR  value[1024];
	CHAR  SystemPath[1024];
	CHAR  szKernelName[1024];
	CHAR  szHalName[1024];
	CHAR  szFileName[1024];
	CHAR  szBootPath[256];
	INT   i;
	CHAR  MsgBuffer[256];
	ULONG SectionId;

	ULONG_PTR Base;
	ULONG Size;

	extern ULONG PageDirectoryStart;
	extern ULONG PageDirectoryEnd;
	extern BOOLEAN AcpiPresent;

	//
	// Open the operating system section
	// specified in the .ini file
	//
	if (!IniOpenSection(OperatingSystemName, &SectionId))
	{
		sprintf(MsgBuffer,"Operating System section '%s' not found in freeldr.ini", OperatingSystemName);
		UiMessageBox(MsgBuffer);
		return;
	}

	/*
	 * Setup multiboot information structure
	 */
	LoaderBlock.Flags = MB_INFO_FLAG_MEM_SIZE | MB_INFO_FLAG_BOOT_DEVICE | MB_INFO_FLAG_COMMAND_LINE | MB_INFO_FLAG_MODULES;
	LoaderBlock.PageDirectoryStart = (ULONG)&PageDirectoryStart;
	LoaderBlock.PageDirectoryEnd = (ULONG)&PageDirectoryEnd;
	LoaderBlock.BootDevice = 0xffffffff;
	LoaderBlock.CommandLine = (unsigned long)multiboot_kernel_cmdline;
	LoaderBlock.ModsCount = 0;
	LoaderBlock.ModsAddr = (unsigned long)multiboot_modules;
	LoaderBlock.MmapLength = (unsigned long)MachGetMemoryMap((PBIOS_MEMORY_MAP)(PVOID)&multiboot_memory_map, 32) * sizeof(memory_map_t);
	if (LoaderBlock.MmapLength)
	{
		LoaderBlock.MmapAddr = (unsigned long)&multiboot_memory_map;
		LoaderBlock.Flags |= MB_INFO_FLAG_MEM_SIZE | MB_INFO_FLAG_MEMORY_MAP;
		multiboot_memory_map_descriptor_size = sizeof(memory_map_t); // GetBiosMemoryMap uses a fixed value of 24
		DbgPrint((DPRINT_REACTOS, "memory map length: %d\n", LoaderBlock.MmapLength));
		DbgPrint((DPRINT_REACTOS, "dumping memory map:\n"));
		for (i=0; i<(LoaderBlock.MmapLength/sizeof(memory_map_t)); i++)
		{
			if (MEMTYPE_USABLE == multiboot_memory_map[i].type &&
			    0 == multiboot_memory_map[i].base_addr_low)
			{
				LoaderBlock.MemLower = (multiboot_memory_map[i].base_addr_low + multiboot_memory_map[i].length_low) / 1024;
				if (640 < LoaderBlock.MemLower)
				{
					LoaderBlock.MemLower = 640;
				}
			}
			if (MEMTYPE_USABLE == multiboot_memory_map[i].type &&
			    multiboot_memory_map[i].base_addr_low <= 1024 * 1024 &&
			    1024 * 1024 <= multiboot_memory_map[i].base_addr_low + multiboot_memory_map[i].length_low)
			{
				LoaderBlock.MemHigher = (multiboot_memory_map[i].base_addr_low + multiboot_memory_map[i].length_low) / 1024 - 1024;
			}
			DbgPrint((DPRINT_REACTOS, "start: %x\t size: %x\t type %d\n",
			          multiboot_memory_map[i].base_addr_low,
				  multiboot_memory_map[i].length_low,
				  multiboot_memory_map[i].type));
		}
	}
	DbgPrint((DPRINT_REACTOS, "low_mem = %d\n", LoaderBlock.MemLower));
	DbgPrint((DPRINT_REACTOS, "high_mem = %d\n", LoaderBlock.MemHigher));

	/*
	 * Initialize the registry
	 */
	RegInitializeRegistry();

	/*
	 * Make sure the system path is set in the .ini file
	 */
	if (!IniReadSettingByName(SectionId, "SystemPath", SystemPath, sizeof(SystemPath)))
	{
		UiMessageBox("System path not specified for selected operating system.");
		return;
	}

	/*
	 * Special case for Live CD.
	 */
	if (!stricmp(SystemPath, "LiveCD"))
	{
		/* Normalize */
		MachDiskGetBootPath(SystemPath, sizeof(SystemPath));
		strcat(SystemPath, "\\reactos");
		strcat(strcpy(multiboot_kernel_cmdline, SystemPath),
		       " /MININT");
	}
	else
	{
		/* copy system path into kernel command line */
		strcpy(multiboot_kernel_cmdline, SystemPath);
	}

	/*
	 * Read the optional kernel parameters (if any)
	 */
	if (IniReadSettingByName(SectionId, "Options", value, 1024))
	{
		strcat(multiboot_kernel_cmdline, " ");
		strcat(multiboot_kernel_cmdline, value);
	}


	UiDrawBackdrop();
	UiDrawStatusText("Detecting Hardware...");

	/*
	 * Detect hardware
	 */
	MachHwDetect();

	if (AcpiPresent) LoaderBlock.Flags |= MB_INFO_FLAG_ACPI_TABLE;

	UiDrawStatusText("Loading...");
	UiDrawProgressBarCenter(0, 100, "Loading ReactOS...");

	/*
	 * Try to open system drive
	 */
	if (!FsOpenSystemVolume(SystemPath, szBootPath, &LoaderBlock.BootDevice))
	{
		UiMessageBox("Failed to open boot drive.");
		return;
	}

	/* append a backslash */
	if ((strlen(szBootPath)==0) ||
	    szBootPath[strlen(szBootPath)] != '\\')
		strcat(szBootPath, "\\");

	DbgPrint((DPRINT_REACTOS,"SystemRoot: '%s'\n", szBootPath));

	/*
	 * Find the kernel image name
	 * and try to load the kernel off the disk
	 */
	if(IniReadSettingByName(SectionId, "Kernel", value, 1024))
	{
		/*
		 * Set the name and
		 */
		if (value[0] == '\\')
		{
			strcpy(szKernelName, value);
		}
		else
		{
			strcpy(szKernelName, szBootPath);
			strcat(szKernelName, "SYSTEM32\\");
			strcat(szKernelName, value);
		}
	}
	else
	{
		strcpy(value, "NTOSKRNL.EXE");
		strcpy(szKernelName, szBootPath);
		strcat(szKernelName, "SYSTEM32\\");
		strcat(szKernelName, value);
	}

    if (!FrLdrLoadKernel(szKernelName, 5)) return;

	/*
	 * Find the HAL image name
	 * and try to load the kernel off the disk
	 */
	if(IniReadSettingByName(SectionId, "Hal", value, 1024))
	{
		/*
		 * Set the name and
		 */
		if (value[0] == '\\')
		{
			strcpy(szHalName, value);
		}
		else
		{
			strcpy(szHalName, szBootPath);
			strcat(szHalName, "SYSTEM32\\");
			strcat(szHalName, value);
		}
	}
	else
	{
		strcpy(value, "HAL.DLL");
		strcpy(szHalName, szBootPath);
		strcat(szHalName, "SYSTEM32\\");
		strcat(szHalName, value);
	}

	if (!FrLdrLoadDriver(szHalName, 10))
		return;

#if 0
    /* Load bootvid */
		strcpy(value, "INBV.DLL");
		strcpy(szHalName, szBootPath);
		strcat(szHalName, "SYSTEM32\\");
		strcat(szHalName, value);

	if (!FrLdrLoadDriver(szHalName, 10))
		return;
#endif
	/*
	 * Load the System hive from disk
	 */
	strcpy(szFileName, szBootPath);
	strcat(szFileName, "SYSTEM32\\CONFIG\\SYSTEM");

	DbgPrint((DPRINT_REACTOS, "SystemHive: '%s'", szFileName));

	FilePointer = FsOpenFile(szFileName);
	if (FilePointer == NULL)
	{
		UiMessageBox("Could not find the System hive!");
		return;
	}

	/*
	 * Update the status bar with the current file
	 */
	strcpy(name, "Reading ");
	strcat(name, value);
	while (strlen(name) < 80)
		strcat(name, " ");
	UiDrawStatusText(name);

	/*
	 * Load the System hive
	 */
	Base = FrLdrLoadModule(FilePointer, szFileName, &Size);
	if (Base == 0 || Size == 0)
	{
		UiMessageBox("Could not load the System hive!\n");
		return;
	}
	DbgPrint((DPRINT_REACTOS, "SystemHive loaded at 0x%x size %u", (unsigned)Base, (unsigned)Size));

	/*
	 * Import the loaded system hive
	 */
	RegImportBinaryHive((PCHAR)Base, Size);

	/*
	 * Initialize the 'CurrentControlSet' link
	 */
	RegInitCurrentControlSet(FALSE);

	UiDrawProgressBarCenter(15, 100, "Loading ReactOS...");

	/*
	 * Export the hardware hive
	 */
	Base = FrLdrCreateModule ("HARDWARE");
	RegExportBinaryHive ("\\Registry\\Machine\\HARDWARE", (PCHAR)Base, &Size);
	FrLdrCloseModule (Base, Size);

	UiDrawProgressBarCenter(20, 100, "Loading ReactOS...");

	/*
	 * Load NLS files
	 */
	if (!FrLdrLoadNlsFiles(szBootPath, MsgBuffer))
	{
	        UiMessageBox(MsgBuffer);
		return;
	}
	UiDrawProgressBarCenter(30, 100, "Loading ReactOS...");

	/*
	 * Load kernel symbols
	 */
	LoadKernelSymbols(szKernelName, 30);
	UiDrawProgressBarCenter(40, 100, "Loading ReactOS...");

	/*
	 * Load boot drivers
	 */
	FrLdrLoadBootDrivers(szBootPath, 40);
	UiUnInitialize("Booting ReactOS...");

	/*
	 * Now boot the kernel
	 */
	DiskStopFloppyMotor();
    MachVideoPrepareForReactOS();
    FrLdrStartup(0x2badb002);
}

#undef DbgPrint
ULONG
DbgPrint(char *Fmt, ...)
{
  UiMessageBox(Fmt);
  return 0;
}

/* EOF */
