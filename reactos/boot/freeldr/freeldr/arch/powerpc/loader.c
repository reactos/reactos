/*
 *  FreeLoader
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
#define _NTSYSTEM_
#include <freeldr.h>

#define NDEBUG
#include <debug.h>
#undef DbgPrint

extern PVOID KernelMemory;

PVOID
NTAPI
LdrPEGetExportByName(PVOID BaseAddress,
                     PUCHAR SymbolName,
                     USHORT Hint);

/* FUNCTIONS *****************************************************************/

PLOADER_MODULE
NTAPI
LdrGetModuleObject(PCHAR ModuleName)
{
    ULONG i;

    for (i = 0; i < LoaderBlock.ModsCount; i++)
    {
        if (strstr(_strupr((PCHAR)reactos_modules[i].String), _strupr(ModuleName)))
        {
            return &reactos_modules[i];
        }
    }

    return NULL;
}

PVOID
NTAPI
LdrPEFixupForward(IN PCHAR ForwardName)
{
    CHAR NameBuffer[128];
    PCHAR p;
    PLOADER_MODULE ModuleObject;

    strcpy(NameBuffer, ForwardName);
    p = strchr(NameBuffer, '.');
    if (p == NULL) return NULL;
    *p = 0;

    ModuleObject = LdrGetModuleObject(NameBuffer);
    if (!ModuleObject)
    {
        DbgPrint("LdrPEFixupForward: failed to find module %s\n", NameBuffer);
        return NULL;
    }

    return LdrPEGetExportByName((PVOID)ModuleObject->ModStart, (PUCHAR)(p + 1), 0xffff);
}

PVOID
NTAPI
LdrPEGetExportByName(PVOID BaseAddress,
                     PUCHAR SymbolName,
                     USHORT Hint)
{
    PIMAGE_EXPORT_DIRECTORY ExportDir;
    PULONG * ExFunctions;
    PULONG * ExNames;
    USHORT * ExOrdinals;
    PVOID ExName;
    ULONG Ordinal;
    PVOID Function;
    LONG minn, maxn, mid, res;
    ULONG ExportDirSize;

    /* HAL and NTOS use a virtual address, switch it to physical mode */
    if ((ULONG_PTR)BaseAddress & 0x80000000)
    {
        BaseAddress = (PVOID)((ULONG_PTR)BaseAddress - KSEG0_BASE + (ULONG)KernelMemory);
    }

    DbgPrint("Exports: RtlImageDirectoryEntryToData\n");
    ExportDir = (PIMAGE_EXPORT_DIRECTORY)
        RtlImageDirectoryEntryToData(BaseAddress,
                                     TRUE,
                                     IMAGE_DIRECTORY_ENTRY_EXPORT,
                                     &ExportDirSize);
    DbgPrint("RtlImageDirectoryEntryToData done\n");
    if (!ExportDir)
    {
        DbgPrint("LdrPEGetExportByName(): no export directory!\n");
        return NULL;
    }

    /* The symbol names may be missing entirely */
    if (!ExportDir->AddressOfNames)
    {
        DbgPrint("LdrPEGetExportByName(): symbol names missing entirely\n");
        return NULL;
    }

    /*
    * Get header pointers
    */
    ExNames = (PULONG *)RVA(BaseAddress, ExportDir->AddressOfNames);
    ExOrdinals = (USHORT *)RVA(BaseAddress, ExportDir->AddressOfNameOrdinals);
    ExFunctions = (PULONG *)RVA(BaseAddress, ExportDir->AddressOfFunctions);

    /*
    * Check the hint first
    */
    if (Hint < ExportDir->NumberOfNames)
    {
        ExName = RVA(BaseAddress, ExNames[Hint]);
        if (strcmp(ExName, (PCHAR)SymbolName) == 0)
        {
            Ordinal = ExOrdinals[Hint];
            Function = RVA(BaseAddress, ExFunctions[Ordinal]);
            if ((ULONG_PTR)Function >= (ULONG_PTR)ExportDir &&
                (ULONG_PTR)Function < (ULONG_PTR)ExportDir + ExportDirSize)
            {
                Function = LdrPEFixupForward((PCHAR)Function);
                if (Function == NULL)
                {
                    DbgPrint("LdrPEGetExportByName(): failed to find %s\n", Function);
                }
                return Function;
            }

            if (Function != NULL) return Function;
        }
    }

    /*
    * Binary search
    */
    minn = 0;
    maxn = ExportDir->NumberOfNames - 1;
    while (minn <= maxn)
    {
        mid = (minn + maxn) / 2;

        ExName = RVA(BaseAddress, ExNames[mid]);
        res = strcmp(ExName, (PCHAR)SymbolName);
        if (res == 0)
        {
            Ordinal = ExOrdinals[mid];
            Function = RVA(BaseAddress, ExFunctions[Ordinal]);
            if ((ULONG_PTR)Function >= (ULONG_PTR)ExportDir &&
                (ULONG_PTR)Function < (ULONG_PTR)ExportDir + ExportDirSize)
            {
                Function = LdrPEFixupForward((PCHAR)Function);
                if (Function == NULL)
                {
                    DbgPrint("1: failed to find %s\n", Function);
                }
                return Function;
            }
            if (Function != NULL)
            {
                return Function;
            }
        }
        else if (res > 0)
        {
            maxn = mid - 1;
        }
        else
        {
            minn = mid + 1;
        }
    }

    /* Fall back on unsorted */
    minn = 0;
    maxn = ExportDir->NumberOfNames - 1;
    while (minn <= maxn)
    {
        ExName = RVA(BaseAddress, ExNames[minn]);
        res = strcmp(ExName, (PCHAR)SymbolName);
        if (res == 0)
        {
            Ordinal = ExOrdinals[minn];
            Function = RVA(BaseAddress, ExFunctions[Ordinal]);
            if ((ULONG_PTR)Function >= (ULONG_PTR)ExportDir &&
                (ULONG_PTR)Function < (ULONG_PTR)ExportDir + ExportDirSize)
            {
                DbgPrint("Forward: %s\n", (PCHAR)Function);
                Function = LdrPEFixupForward((PCHAR)Function);
                if (Function == NULL)
                {
                    DbgPrint("LdrPEGetExportByName(): failed to find %s\n",SymbolName);
                }
                return Function;
            }
	    if (Function != NULL)
	    {
		return Function;
	    }
	    DbgPrint("Failed to get function %s\n", SymbolName);
	}
	minn++;
    }

    DbgPrint("2: failed to find %s\n",SymbolName);
    return (PVOID)NULL;
}

NTSTATUS
NTAPI
LdrPEProcessImportDirectoryEntry(PVOID DriverBase,
                                 PLOADER_MODULE LoaderModule,
                                 PIMAGE_IMPORT_DESCRIPTOR ImportModuleDirectory)
{
    PVOID* ImportAddressList;
    PULONG FunctionNameList;

    if (ImportModuleDirectory == NULL || ImportModuleDirectory->Name == 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    /* Get the import address list. */
    ImportAddressList = (PVOID*)RVA(DriverBase, ImportModuleDirectory->FirstThunk);

    /* Get the list of functions to import. */
    if (ImportModuleDirectory->OriginalFirstThunk != 0)
    {
        FunctionNameList = (PULONG)RVA(DriverBase, ImportModuleDirectory->OriginalFirstThunk);
    }
    else
    {
        FunctionNameList = (PULONG)RVA(DriverBase, ImportModuleDirectory->FirstThunk);
    }

    /* Walk through function list and fixup addresses. */
    while (*FunctionNameList != 0L)
    {
        if ((*FunctionNameList) & 0x80000000)
        {
            DbgPrint("Failed to import ordinal from %s\n", LoaderModule->String);
            return STATUS_UNSUCCESSFUL;
        }
        else
        {
            IMAGE_IMPORT_BY_NAME *pe_name;
            pe_name = RVA(DriverBase, *FunctionNameList);
            *ImportAddressList = LdrPEGetExportByName((PVOID)LoaderModule->ModStart, pe_name->Name, pe_name->Hint);

            /* Fixup the address to be virtual */
            *ImportAddressList = (PVOID)((ULONG_PTR)*ImportAddressList + (KSEG0_BASE - (ULONG_PTR)KernelMemory));

            //DbgPrint("Looked for: %s and found: %p\n", pe_name->Name, *ImportAddressList);
            if ((*ImportAddressList) == NULL)
            {
                DbgPrint("Failed to import %s from %s\n", pe_name->Name, LoaderModule->String);
                return STATUS_UNSUCCESSFUL;
            }
        }
        ImportAddressList++;
        FunctionNameList++;
    }
    return STATUS_SUCCESS;
}

extern BOOLEAN FrLdrLoadDriver(PCHAR szFileName, INT nPos);

NTSTATUS
NTAPI
LdrPEGetOrLoadModule(IN PCHAR ModuleName,
                     IN PCHAR ImportedName,
                     IN PLOADER_MODULE* ImportedModule)
{
    NTSTATUS Status = STATUS_SUCCESS;

    *ImportedModule = LdrGetModuleObject(ImportedName);
    if (*ImportedModule == NULL)
    {
        /*
         * For now, we only support import-loading the HAL.
         * Later, FrLdrLoadDriver should be made to share the same
         * code, and we'll just call it instead.
         */
	FrLdrLoadDriver(ImportedName, 0);
	
	/* Return the new module */
	*ImportedModule = LdrGetModuleObject(ImportedName);
	if (*ImportedModule == NULL)
	{
	    DbgPrint("Error loading import: %s\n", ImportedName);
	    return STATUS_UNSUCCESSFUL;
	}
    }

    return Status;
}

NTSTATUS
NTAPI
LdrPEFixupImports(IN PVOID DllBase,
                  IN PCHAR DllName)
{
    PIMAGE_IMPORT_DESCRIPTOR ImportModuleDirectory;
    PCHAR ImportedName;
    NTSTATUS Status;
    PLOADER_MODULE ImportedModule;
    ULONG Size;

    printf("Fixing up %x (%s)\n", DllBase, DllName);

    /*  Process each import module  */
    DbgPrint("FixupImports: RtlImageDirectoryEntryToData\n");
    ImportModuleDirectory = (PIMAGE_IMPORT_DESCRIPTOR)
        RtlImageDirectoryEntryToData(DllBase,
                                     TRUE,
                                     IMAGE_DIRECTORY_ENTRY_IMPORT,
                                     &Size);
    DbgPrint("RtlImageDirectoryEntryToData done\n");
    while (ImportModuleDirectory && ImportModuleDirectory->Name)
    {
        /*  Check to make sure that import lib is kernel  */
        ImportedName = (PCHAR) DllBase + ImportModuleDirectory->Name;
        //DbgPrint("Processing imports for file: %s into file: %s\n", DllName, ImportedName);

        Status = LdrPEGetOrLoadModule(DllName, ImportedName, &ImportedModule);
        if (!NT_SUCCESS(Status)) return Status;

        Status = LdrPEProcessImportDirectoryEntry(DllBase, ImportedModule, ImportModuleDirectory);
        if (!NT_SUCCESS(Status)) return Status;

        //DbgPrint("Imports for file: %s into file: %s complete\n", DllName, ImportedName);
        ImportModuleDirectory++;
    }

    return STATUS_SUCCESS;
}
