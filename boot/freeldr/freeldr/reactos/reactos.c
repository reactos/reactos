/*
 *  FreeLoader
 *
 *  Copyright (C) 1998-2003  Brian Palmer  <brianp@sginet.com>
 *  Copyright (C) 2005       Alex Ionescu  <alex@relsoft.net>
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

#include <freeldr.h>
#include <debug.h>

extern ULONG PageDirectoryStart;
extern ULONG PageDirectoryEnd;

ROS_LOADER_PARAMETER_BLOCK LoaderBlock;
char					reactos_kernel_cmdline[255];	// Command line passed to kernel
LOADER_MODULE			reactos_modules[64];		// Array to hold boot module info loaded for the kernel
char					reactos_module_strings[64][256];	// Array to hold module names
// Make this a single struct to guarantee that these elements are nearby in
// memory.  
reactos_mem_data_t reactos_mem_data;
ARC_DISK_SIGNATURE      reactos_arc_disk_info[32]; // ARC Disk Information
char                    reactos_arc_strings[32][256];
unsigned long           reactos_disk_count = 0;
char reactos_arc_hardware_data[HW_MAX_ARC_HEAP_SIZE] = {0};

CHAR szHalName[255];
CHAR szBootPath[255];
CHAR SystemRoot[255];
static CHAR szLoadingMsg[] = "ReactOS is loading files...";
BOOLEAN FrLdrBootType;
ULONG_PTR KernelBase;
ROS_KERNEL_ENTRY_POINT KernelEntryPoint;

BOOLEAN
FrLdrLoadDriver(PCHAR szFileName,
                INT nPos)
{
    PFILE FilePointer;
    CHAR value[256], *FinalSlash;
    LPSTR p;

    if (!_stricmp(szFileName, "hal.dll"))
    {
        /* Use the boot.ini name instead */
        szFileName = szHalName;
    }

    FinalSlash = strrchr(szFileName, '\\');
    if(FinalSlash)
	szFileName = FinalSlash + 1;

    /* Open the Driver */
    FilePointer = FsOpenFile(szFileName);

    /* Try under the system root in the main dir and drivers */
    if (FilePointer == NULL)
    {
	strcpy(value, SystemRoot);
	if(value[strlen(value)-1] != '\\')
	    strcat(value, "\\");
	strcat(value, szFileName);
	FilePointer = FsOpenFile(value);
    }

    if (FilePointer == NULL)
    {
	strcpy(value, SystemRoot);
	if(value[strlen(value)-1] != '\\')
	    strcat(value, "\\");
	strcat(value, "SYSTEM32\\");
	strcat(value, szFileName);
	FilePointer = FsOpenFile(value);
    }

    if (FilePointer == NULL)
    {
	strcpy(value, SystemRoot);
	if(value[strlen(value)-1] != '\\')
	    strcat(value, "\\");
	strcat(value, "SYSTEM32\\DRIVERS\\");
	strcat(value, szFileName);
	FilePointer = FsOpenFile(value);
    }

    /* Make sure we did */
    if (FilePointer == NULL) {

        /* Fail if file wasn't opened */
        strcpy(value, szFileName);
        strcat(value, " not found.");
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
    FrLdrReadAndMapImage(FilePointer, szFileName, 0);

    /* Update status and return */
    UiDrawProgressBarCenter(nPos, 100, szLoadingMsg);
    return(TRUE);
}

PVOID
NTAPI
FrLdrLoadImage(IN PCHAR szFileName,
               IN INT nPos,
               IN ULONG ImageType)
{
    PFILE FilePointer;
    PCHAR szShortName;
    CHAR szBuffer[256], szFullPath[256];
    PVOID LoadBase;

    /* Extract filename without path */
    szShortName = strrchr(szFileName, '\\');
    if (!szShortName)
    {
        /* No path, leave it alone */
        szShortName = szFileName;

        /* Which means we need to build a path now */
        strcpy(szBuffer, szFileName);
        strcpy(szFullPath, szBootPath);
        if (!FrLdrBootType)
        {
            strcat(szFullPath, "SYSTEM32\\DRIVERS\\");
        }
        else
        {
            strcat(szFullPath, "\\");
        }
        strcat(szFullPath, szBuffer);
        szFileName = szFullPath;
    }
    else
    {
        /* Skip the path */
        szShortName = szShortName + 1;
    }

    /* Open the image */
    FilePointer = FsOpenFile(szFileName);
    if (!FilePointer)
    {
        /* Return failure on the short name */
        strcpy(szBuffer, szShortName);
        strcat(szBuffer, " not found.");
        UiMessageBox(szBuffer);
        return FALSE;
    }

    /* Update the status bar with the current file */
    strcpy(szBuffer, "Reading ");
    strcat(szBuffer, szShortName);
    UiDrawStatusText(szBuffer);

    /* Do the actual loading */
    LoadBase = FrLdrReadAndMapImage(FilePointer, szShortName, ImageType);

    /* Update Processbar and return success */
    if (!FrLdrBootType) UiDrawProgressBarCenter(nPos, 100, szLoadingMsg);
    return LoadBase;
}

BOOLEAN
FrLdrLoadNlsFile(PCSTR szFileName,
                 PCSTR szModuleName)
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

static BOOLEAN
FrLdrLoadNlsFiles(PCHAR szSystemRoot,
                  PCHAR szErrorOut)
{
    LONG rc = ERROR_SUCCESS;
    FRLDRHKEY hKey;
    WCHAR szIdBuffer[80];
    WCHAR szNameBuffer[80];
    CHAR szFileName[256];
    ULONG BufferSize;

    /* open the codepage key */
    rc = RegOpenKey(NULL,
                    L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Control\\NLS\\CodePage",
                    &hKey);
    if (rc != ERROR_SUCCESS) {

        strcpy(szErrorOut, "Couldn't open CodePage registry key");
        return(FALSE);
    }

    /* get ANSI codepage */
    BufferSize = sizeof(szIdBuffer);
    rc = RegQueryValue(hKey, L"ACP", NULL, (PUCHAR)szIdBuffer, &BufferSize);
    if (rc != ERROR_SUCCESS) {

        strcpy(szErrorOut, "Couldn't get ACP NLS setting");
        return(FALSE);
    }

    BufferSize = sizeof(szNameBuffer);
    rc = RegQueryValue(hKey, szIdBuffer, NULL, (PUCHAR)szNameBuffer, &BufferSize);
    if (rc != ERROR_SUCCESS) {

        strcpy(szErrorOut, "ACP NLS Setting exists, but isn't readable");
        return(FALSE);
    }

    /* load ANSI codepage table */
    sprintf(szFileName,"%ssystem32\\%S", szSystemRoot, szNameBuffer);
    DbgPrint((DPRINT_REACTOS, "ANSI file: %s\n", szFileName));
    if (!FrLdrLoadNlsFile(szFileName, "ansi.nls")) {

        strcpy(szErrorOut, "Couldn't load ansi.nls");
        return(FALSE);
    }

    /* get OEM codepage */
    BufferSize = sizeof(szIdBuffer);
    rc = RegQueryValue(hKey, L"OEMCP", NULL, (PUCHAR)szIdBuffer, &BufferSize);
    if (rc != ERROR_SUCCESS) {

        strcpy(szErrorOut, "Couldn't get OEMCP NLS setting");
        return(FALSE);
    }

    BufferSize = sizeof(szNameBuffer);
    rc = RegQueryValue(hKey, szIdBuffer, NULL, (PUCHAR)szNameBuffer, &BufferSize);
    if (rc != ERROR_SUCCESS) {

        strcpy(szErrorOut, "OEMCP NLS setting exists, but isn't readable");
        return(FALSE);
    }

    /* load OEM codepage table */
    sprintf(szFileName, "%ssystem32\\%S", szSystemRoot, szNameBuffer);
    DbgPrint((DPRINT_REACTOS, "Oem file: %s\n", szFileName));
    if (!FrLdrLoadNlsFile(szFileName, "oem.nls")) {

        strcpy(szErrorOut, "Couldn't load oem.nls");
        return(FALSE);
    }

    /* open the language key */
    rc = RegOpenKey(NULL,
                    L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Control\\NLS\\Language",
                    &hKey);
    if (rc != ERROR_SUCCESS) {

        strcpy(szErrorOut, "Couldn't open Language registry key");
        return(FALSE);
    }

    /* get the Unicode case table */
    BufferSize = sizeof(szIdBuffer);
    rc = RegQueryValue(hKey, L"Default", NULL, (PUCHAR)szIdBuffer, &BufferSize);
    if (rc != ERROR_SUCCESS) {

        strcpy(szErrorOut, "Couldn't get Language Default setting");
        return(FALSE);
    }

    BufferSize = sizeof(szNameBuffer);
    rc = RegQueryValue(hKey, szIdBuffer, NULL, (PUCHAR)szNameBuffer, &BufferSize);
    if (rc != ERROR_SUCCESS) {

        strcpy(szErrorOut, "Language Default setting exists, but isn't readable");
        return(FALSE);
    }

    /* load Unicode case table */
    sprintf(szFileName, "%ssystem32\\%S", szSystemRoot, szNameBuffer);
    DbgPrint((DPRINT_REACTOS, "Casemap file: %s\n", szFileName));
    if (!FrLdrLoadNlsFile(szFileName, "casemap.nls")) {

        strcpy(szErrorOut, "casemap.nls");
        return(FALSE);
    }

    return(TRUE);
}

static VOID
FrLdrLoadBootDrivers(PCHAR szSystemRoot,
                     INT nPos)
{
    LONG rc = 0;
    FRLDRHKEY hGroupKey, hOrderKey, hServiceKey, hDriverKey;
    WCHAR GroupNameBuffer[512];
    WCHAR ServiceName[256];
    ULONG OrderList[128];
    ULONG BufferSize;
    ULONG Index;
    ULONG TagIndex;
    LPWSTR GroupName;

    ULONG ValueSize;
    ULONG ValueType;
    ULONG StartValue;
    ULONG TagValue;
    WCHAR DriverGroup[256];
    ULONG DriverGroupSize;

    CHAR ImagePath[256];
    WCHAR TempImagePath[256];

    /* get 'service group order' key */
    rc = RegOpenKey(NULL,
                    L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Control\\ServiceGroupOrder",
                    &hGroupKey);
    if (rc != ERROR_SUCCESS) {

        DbgPrint((DPRINT_REACTOS, "Failed to open the 'ServiceGroupOrder' key (rc %d)\n", (int)rc));
        return;
    }

    /* get 'group order list' key */
    rc = RegOpenKey(NULL,
                    L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Control\\GroupOrderList",
                    &hOrderKey);
    if (rc != ERROR_SUCCESS) {

        DbgPrint((DPRINT_REACTOS, "Failed to open the 'GroupOrderList' key (rc %d)\n", (int)rc));
        return;
    }

    /* enumerate drivers */
    rc = RegOpenKey(NULL,
                    L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Services",
                    &hServiceKey);
    if (rc != ERROR_SUCCESS)  {

        DbgPrint((DPRINT_REACTOS, "Failed to open the 'Services' key (rc %d)\n", (int)rc));
        return;
    }

    /* Get the Name Group */
    BufferSize = sizeof(GroupNameBuffer);
    rc = RegQueryValue(hGroupKey, L"List", NULL, (PUCHAR)GroupNameBuffer, &BufferSize);
    DbgPrint((DPRINT_REACTOS, "RegQueryValue(): rc %d\n", (int)rc));
    if (rc != ERROR_SUCCESS) return;
    DbgPrint((DPRINT_REACTOS, "BufferSize: %d \n", (int)BufferSize));
    DbgPrint((DPRINT_REACTOS, "GroupNameBuffer: '%S' \n", GroupNameBuffer));

    /* Loop through each group */
    GroupName = GroupNameBuffer;
    while (*GroupName) {
        DbgPrint((DPRINT_REACTOS, "Driver group: '%S'\n", GroupName));

        /* Query the Order */
        BufferSize = sizeof(OrderList);
        rc = RegQueryValue(hOrderKey, GroupName, NULL, (PUCHAR)OrderList, &BufferSize);
        if (rc != ERROR_SUCCESS) OrderList[0] = 0;

        /* enumerate all drivers */
        for (TagIndex = 1; TagIndex <= SWAPD(OrderList[0]); TagIndex++) {

            Index = 0;

            while (TRUE) {

                /* Get the Driver's Name */
                ValueSize = sizeof(ServiceName);
                rc = RegEnumKey(hServiceKey, Index, ServiceName, &ValueSize);
                DbgPrint((DPRINT_REACTOS, "RegEnumKey(): rc %d\n", (int)rc));

                /* Makre sure it's valid, and check if we're done */
                if (rc == ERROR_NO_MORE_ITEMS) break;
                if (rc != ERROR_SUCCESS) return;
                DbgPrint((DPRINT_REACTOS, "Service %d: '%S'\n", (int)Index, ServiceName));

                /* open driver Key */
                rc = RegOpenKey(hServiceKey, ServiceName, &hDriverKey);
                if (rc == ERROR_SUCCESS)
				{
                    /* Read the Start Value */
                    ValueSize = sizeof(ULONG);
                    rc = RegQueryValue(hDriverKey, L"Start", &ValueType, (PUCHAR)&StartValue, &ValueSize);
                    if (rc != ERROR_SUCCESS) StartValue = (ULONG)-1;
                    DbgPrint((DPRINT_REACTOS, "  Start: %x  \n", (int)StartValue));

                    /* Read the Tag */
                    ValueSize = sizeof(ULONG);
                    rc = RegQueryValue(hDriverKey, L"Tag", &ValueType, (PUCHAR)&TagValue, &ValueSize);
                    if (rc != ERROR_SUCCESS) TagValue = (ULONG)-1;
                    DbgPrint((DPRINT_REACTOS, "  Tag:   %x  \n", (int)TagValue));

                    /* Read the driver's group */
                    DriverGroupSize = sizeof(DriverGroup);
                    rc = RegQueryValue(hDriverKey, L"Group", NULL, (PUCHAR)DriverGroup, &DriverGroupSize);
                    DbgPrint((DPRINT_REACTOS, "  Group: '%S'  \n", DriverGroup));

                    /* Make sure it should be started */
                    if ((StartValue == 0) &&
                        (TagValue == OrderList[TagIndex]) &&
                        (_wcsicmp(DriverGroup, GroupName) == 0)) {

                        /* Get the Driver's Location */
                        ValueSize = sizeof(TempImagePath);
                        rc = RegQueryValue(hDriverKey, L"ImagePath", NULL, (PUCHAR)TempImagePath, &ValueSize);

                        /* Write the whole path if it suceeded, else prepare to fail */
                        if (rc != ERROR_SUCCESS) {
                            DbgPrint((DPRINT_REACTOS, "  ImagePath: not found\n"));
                            sprintf(ImagePath, "%s\\system32\\drivers\\%S.sys", szSystemRoot, ServiceName);
                        } else if (TempImagePath[0] != L'\\') {
                            sprintf(ImagePath, "%s%S", szSystemRoot, TempImagePath);
                        } else {
                            sprintf(ImagePath, "%S", TempImagePath);
                            DbgPrint((DPRINT_REACTOS, "  ImagePath: '%s'\n", ImagePath));
                        }

                        DbgPrint((DPRINT_REACTOS, "  Loading driver: '%s'\n", ImagePath));

                        /* Update the position if needed */
                        if (nPos < 100) nPos += 5;

                        FrLdrLoadImage(ImagePath, nPos, 2);

                    } else {

                        DbgPrint((DPRINT_REACTOS, "  Skipping driver '%S' with Start %d, Tag %d and Group '%S' (Current Tag %d, current group '%S')\n",
                                 ServiceName, StartValue, TagValue, DriverGroup, OrderList[TagIndex], GroupName));
                    }
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
            DbgPrint((DPRINT_REACTOS, "Service %d: '%S'\n", (int)Index, ServiceName));

            /* open driver Key */
            rc = RegOpenKey(hServiceKey, ServiceName, &hDriverKey);
            if (rc == ERROR_SUCCESS)
            {
                /* Read the Start Value */
                ValueSize = sizeof(ULONG);
                rc = RegQueryValue(hDriverKey, L"Start", &ValueType, (PUCHAR)&StartValue, &ValueSize);
                if (rc != ERROR_SUCCESS) StartValue = (ULONG)-1;
                DbgPrint((DPRINT_REACTOS, "  Start: %x  \n", (int)StartValue));

                /* Read the Tag */
                ValueSize = sizeof(ULONG);
                rc = RegQueryValue(hDriverKey, L"Tag", &ValueType, (PUCHAR)&TagValue, &ValueSize);
                if (rc != ERROR_SUCCESS) TagValue = (ULONG)-1;
                DbgPrint((DPRINT_REACTOS, "  Tag:   %x  \n", (int)TagValue));

                /* Read the driver's group */
                DriverGroupSize = sizeof(DriverGroup);
                rc = RegQueryValue(hDriverKey, L"Group", NULL, (PUCHAR)DriverGroup, &DriverGroupSize);
                DbgPrint((DPRINT_REACTOS, "  Group: '%S'  \n", DriverGroup));

                for (TagIndex = 1; TagIndex <= OrderList[0]; TagIndex++) {
                    if (TagValue == OrderList[TagIndex]) break;
                }

                if ((StartValue == 0) &&
                    (TagIndex > OrderList[0]) &&
                    (_wcsicmp(DriverGroup, GroupName) == 0)) {

                        ValueSize = sizeof(TempImagePath);
                        rc = RegQueryValue(hDriverKey, L"ImagePath", NULL, (PUCHAR)TempImagePath, &ValueSize);
                        if (rc != ERROR_SUCCESS) {
                            DbgPrint((DPRINT_REACTOS, "  ImagePath: not found\n"));
                            sprintf(ImagePath, "%ssystem32\\drivers\\%S.sys", szSystemRoot, ServiceName);
                        } else if (TempImagePath[0] != L'\\') {
                            sprintf(ImagePath, "%s%S", szSystemRoot, TempImagePath);
                        } else {
                            sprintf(ImagePath, "%S", TempImagePath);
                            DbgPrint((DPRINT_REACTOS, "  ImagePath: '%s'\n", ImagePath));
                        }
                    DbgPrint((DPRINT_REACTOS, "  Loading driver: '%s'\n", ImagePath));

                    if (nPos < 100) nPos += 5;

                    FrLdrLoadImage(ImagePath, nPos, 2);

                } else {

                    DbgPrint((DPRINT_REACTOS, "  Skipping driver '%S' with Start %d, Tag %d and Group '%S' (Current group '%S')\n",
                    ServiceName, StartValue, TagValue, DriverGroup, GroupName));
                }
            }

            Index++;
        }

        /* Move to the next group name */
        GroupName = GroupName + wcslen(GroupName) + 1;
    }
}

VOID
LoadAndBootReactOS(PCSTR OperatingSystemName)
{
	PFILE FilePointer;
	CHAR name[255];
	CHAR value[255];
	CHAR SystemPath[255];
	CHAR szKernelName[255];
	CHAR szFileName[255];
	CHAR  MsgBuffer[256];
	ULONG_PTR SectionId;
    PIMAGE_NT_HEADERS NtHeader;
    PVOID LoadBase;
	ULONG_PTR Base;
	ULONG Size;
    
    //
    // Backdrop
    //
    UiDrawBackdrop();

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

    //
    // Read the command line
    //
	if (IniReadSettingByName(SectionId, "Options", value, sizeof(value)))
	{
        //
        // Check if a ramdisk file was given
        //
        PCHAR File;
        File = strstr(value, "/RDIMAGEPATH=");
        if (File)
        {
            //
            // Copy the file name and everything else after it
            //
            strcpy(szFileName, File + 13);
            
            //
            // Null-terminate
            //
            *strstr(szFileName, " ") = ANSI_NULL;
            
            //
            // Load the ramdisk
            //
            RamDiskLoadVirtualFile(szFileName);
        }
	}

	/*
	 * Setup multiboot information structure
	 */
    UiDrawProgressBarCenter(1, 100, szLoadingMsg);
	UiDrawStatusText("Detecting Hardware...");
	LoaderBlock.CommandLine = reactos_kernel_cmdline;
	LoaderBlock.PageDirectoryStart = (ULONG_PTR)&PageDirectoryStart;
	LoaderBlock.PageDirectoryEnd = (ULONG_PTR)&PageDirectoryEnd;
	LoaderBlock.ModsCount = 0;
	LoaderBlock.ModsAddr = reactos_modules;
    LoaderBlock.DrivesAddr = reactos_arc_disk_info;
    LoaderBlock.RdAddr = (ULONG_PTR)gRamDiskBase;
    LoaderBlock.RdLength = gRamDiskSize;
    LoaderBlock.MmapLength = (SIZE_T)MachGetMemoryMap((PBIOS_MEMORY_MAP)reactos_memory_map, 32) * sizeof(memory_map_t);
    if (LoaderBlock.MmapLength)
    {
        ULONG i;
        LoaderBlock.Flags |= MB_FLAGS_MEM_INFO | MB_FLAGS_MMAP_INFO;
        LoaderBlock.MmapAddr = (ULONG_PTR)&reactos_memory_map;
        reactos_memory_map_descriptor_size = sizeof(memory_map_t); // GetBiosMemoryMap uses a fixed value of 24
        for (i=0; i<(LoaderBlock.MmapLength/sizeof(memory_map_t)); i++)
        {
#ifdef _M_PPC
            ULONG tmp;
            /* Also swap from long long to high/low
             * We also have unusable memory that will be available to kernel
             * land.  Mark it here.
             */
            if (BiosMemoryAcpiReclaim == reactos_memory_map[i].type)
            {
                reactos_memory_map[i].type = BiosMemoryUsable;
            }

            tmp = reactos_memory_map[i].base_addr_low;
            reactos_memory_map[i].base_addr_low = reactos_memory_map[i].base_addr_high;
            reactos_memory_map[i].base_addr_high = tmp;
            tmp = reactos_memory_map[i].length_low;
            reactos_memory_map[i].length_low = reactos_memory_map[i].length_high;
            reactos_memory_map[i].length_high = tmp;
#endif

            if (BiosMemoryUsable == reactos_memory_map[i].type &&
                0 == reactos_memory_map[i].base_addr_low)
            {
                LoaderBlock.MemLower = (reactos_memory_map[i].base_addr_low + reactos_memory_map[i].length_low) / 1024;
                if (640 < LoaderBlock.MemLower)
                {
                    LoaderBlock.MemLower = 640;
                }
            }
            if (BiosMemoryUsable == reactos_memory_map[i].type &&
                reactos_memory_map[i].base_addr_low <= 1024 * 1024 &&
                1024 * 1024 <= reactos_memory_map[i].base_addr_low + reactos_memory_map[i].length_low)
            {
                LoaderBlock.MemHigher = (reactos_memory_map[i].base_addr_low + reactos_memory_map[i].length_low) / 1024 - 1024;
            }
        }
    }

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
	if (!_stricmp(SystemPath, "LiveCD"))
	{
		/* Normalize */
		MachDiskGetBootPath(SystemPath, sizeof(SystemPath));
		strcat(SystemPath, "\\reactos");
		strcat(strcpy(reactos_kernel_cmdline, SystemPath),
		       " /MININT");
	}
	else
	{
		if (! MachDiskNormalizeSystemPath(SystemPath,
		                                  sizeof(SystemPath)))
		{
			UiMessageBox("Invalid system path");
			return;
		}
		/* copy system path into kernel command line */
		strcpy(reactos_kernel_cmdline, SystemPath);
	}

	/*
	 * Read the optional kernel parameters (if any)
	 */
	if (IniReadSettingByName(SectionId, "Options", value, sizeof(value)))
	{
		strcat(reactos_kernel_cmdline, " ");
		strcat(reactos_kernel_cmdline, value);
	}

	/*
	 * Detect hardware
	 */
	LoaderBlock.ArchExtra = (ULONG_PTR)MachHwDetect();
    UiDrawProgressBarCenter(5, 100, szLoadingMsg);

    LoaderBlock.DrivesCount = reactos_disk_count;

	UiDrawStatusText("Loading...");

	//
	// If we have a ramdisk, this will switch to the ramdisk disk routines
	// which read from memory instead of using the firmware. This has to be done
	// after hardware detection, since hardware detection will require using the
	// real routines in order to perform disk-detection (just because we're on a
	// ram-boot doesn't mean the user doesn't have actual disks installed too!)
	//
	RamDiskSwitchFromBios();

	/*
	 * Try to open system drive
	 */
	if (!FsOpenSystemVolume(SystemPath, szBootPath, &LoaderBlock.BootDevice))
	{
		UiMessageBox("Failed to open system drive.");
		return;
	}

	/* append a backslash */
	if ((strlen(szBootPath)==0) ||
	    szBootPath[strlen(szBootPath)] != '\\')
		strcat(szBootPath, "\\");

	DbgPrint((DPRINT_REACTOS,"SystemRoot: '%s'\n", szBootPath));
	strcpy(SystemRoot, szBootPath);

	/*
	 * Find the kernel image name
	 * and try to load the kernel off the disk
	 */
	if(IniReadSettingByName(SectionId, "Kernel", value, sizeof(value)))
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

	/*
	 * Find the HAL image name
	 * and try to load the kernel off the disk
	 */
	if(IniReadSettingByName(SectionId, "Hal", value, sizeof(value)))
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

	/* Load the kernel */
	LoadBase = FrLdrLoadImage(szKernelName, 5, 1);
	if (!LoadBase) return;

	/* Get the NT header, kernel base and kernel entry */
	NtHeader = RtlImageNtHeader(LoadBase);
	KernelBase = SWAPD(NtHeader->OptionalHeader.ImageBase);
	KernelEntryPoint = (ROS_KERNEL_ENTRY_POINT)(KernelBase + SWAPD(NtHeader->OptionalHeader.AddressOfEntryPoint));
	LoaderBlock.KernelBase = KernelBase;

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

	UiDrawProgressBarCenter(15, 100, szLoadingMsg);

	UiDrawProgressBarCenter(20, 100, szLoadingMsg);

	/*
	 * Load NLS files
	 */
	if (!FrLdrLoadNlsFiles(szBootPath, MsgBuffer))
	{
		UiMessageBox(MsgBuffer);
		return;
	}
	UiDrawProgressBarCenter(30, 100, szLoadingMsg);

	/*
	 * Load boot drivers
	 */
	FrLdrLoadBootDrivers(szBootPath, 40);
	//UiUnInitialize("Booting ReactOS...");

    //
    // Perform architecture-specific pre-boot configuration
    //
    MachPrepareForReactOS(FALSE);

    //
    // Setup paging and jump to kernel
    //
	FrLdrStartup(0x2badb002);
}

#undef DbgPrint
ULONG
DbgPrint(const char *Format, ...)
{
	va_list ap;
	CHAR Buffer[512];
	ULONG Length;

	va_start(ap, Format);

	/* Construct a string */
	Length = _vsnprintf(Buffer, 512, Format, ap);

	/* Check if we went past the buffer */
	if (Length == -1)
	{
		/* Terminate it if we went over-board */
		Buffer[sizeof(Buffer) - 1] = '\n';

		/* Put maximum */
		Length = sizeof(Buffer);
	}

	/* Show it as a message box */
	UiMessageBox(Buffer);

	/* Cleanup and exit */
	va_end(ap);
	return 0;
}

/* EOF */
