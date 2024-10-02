/*
 *  FreeLoader
 *  Copyright (C) 1998-2003  Brian Palmer  <brianp@sginet.com>
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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/* INCLUDES *******************************************************************/

#include <freeldr.h>

#include <debug.h>
DBG_DEFAULT_CHANNEL(WARNING);

/* GLOBALS ********************************************************************/

#define TOSTRING_(X) #X
#define TOSTRING(X) TOSTRING_(X)

const PCSTR FrLdrVersionString =
#if (FREELOADER_PATCH_VERSION == 0)
    "FreeLoader v" TOSTRING(FREELOADER_MAJOR_VERSION) "." TOSTRING(FREELOADER_MINOR_VERSION);
#else
    "FreeLoader v" TOSTRING(FREELOADER_MAJOR_VERSION) "." TOSTRING(FREELOADER_MINOR_VERSION) "." TOSTRING(FREELOADER_PATCH_VERSION);
#endif

CCHAR FrLdrBootPath[MAX_PATH] = "";

/* FUNCTIONS ******************************************************************/

static
BOOLEAN
LoadRosload(
    _In_ PCSTR RosloadPath,
    _Out_ PCHAR FullPath,
    _Out_ PVOID* ImageBase)
{
    BOOLEAN Success;

    /* Create full rosload.exe path */
    strcpy(FullPath, FrLdrBootPath);
    strcat(FullPath, "\\");
    strcat(FullPath, RosloadPath);

    TRACE("Loading second stage loader '%s'\n", FullPath);

    /* Load rosload.exe as a bootloader image */
    Success = PeLdrLoadImageEx(FullPath,
                               LoaderLoadedProgram,
                               ImageBase,
                               FALSE);
    if (!Success)
    {
        ERR("Failed to load second stage loader '%s'\n", FullPath);
    }

    return Success;
}

static
ULONG
LaunchSecondStageLoader(VOID)
{
    CHAR SecondStageLdrPath[MAX_PATH];
    PLDR_DATA_TABLE_ENTRY FreeldrDTE, RosloadDTE;
    LIST_ENTRY ModuleListHead;
    PVOID ImageBase;
    LONG (*EntryPoint)(VOID);
    BOOLEAN Success;

    /* Initialize the loaded module list */
    InitializeListHead(&ModuleListHead);

    /* Load the second stage loader */
    if (!LoadRosload("rosload.exe", SecondStageLdrPath, &ImageBase))
    {
        /* Try in loader directory */
        if (!LoadRosload("loader\\rosload.exe", SecondStageLdrPath, &ImageBase))
        {
            return ENOENT;
        }
    }

    /* Allocate a DTE for rosload.exe */
    Success = PeLdrAllocateDataTableEntry(&ModuleListHead,
                                          "rosload.exe",
                                          SecondStageLdrPath,
                                          ImageBase,
                                          &RosloadDTE);
    if (!Success)
    {
        /* Cleanup and bail out */
        ERR("Failed to allocate DTE for rosload.exe\n");
        MmFreeMemory(ImageBase);
        return ENOMEM;
    }

    /* Add the PE part of freeldr.sys to the list of loaded executables, it
       contains ScsiPort* exports, imported by ntbootdd.sys */
    Success = PeLdrAllocateDataTableEntry(&ModuleListHead,
                                          "freeldr_pe.exe",
                                          "freeldr_pe.exe",
                                          &__ImageBase,
                                          &FreeldrDTE);
    if (!Success)
    {
        /* Cleanup and bail out */
        ERR("Failed to allocate DTE for freeldr\n");
        PeLdrFreeDataTableEntry(RosloadDTE);
        MmFreeMemory(ImageBase);
        return ENOMEM;
    }

    /* Resolve imports */
    Success = PeLdrScanImportDescriptorTable(&ModuleListHead, "", RosloadDTE);
    if (!Success)
    {
        /* Cleanup and bail out */
        ERR("Failed to resolve imports for rosload.exe\n");
        PeLdrFreeDataTableEntry(FreeldrDTE);
        PeLdrFreeDataTableEntry(RosloadDTE);
        MmFreeMemory(ImageBase);
        return EIO;
    }

    /* Call the entrypoint */
    printf("Launching rosload.exe...\n");
    EntryPoint = VaToPa(RosloadDTE->EntryPoint);
    return (*EntryPoint)();
}

VOID __cdecl BootMain(IN PCCH CmdLine)
{
    /* Load the default settings from the command-line */
    LoadSettings(CmdLine);

    /* Debugger pre-initialization */
    DebugInit(BootMgrInfo.DebugString);

    MachInit(CmdLine);

    TRACE("BootMain() called.\n");

#ifndef UEFIBOOT
    /* Check if the CPU is new enough */
    FrLdrCheckCpuCompatibility(); // FIXME: Should be done inside MachInit!
#endif

    /* UI pre-initialization */
    if (!UiInitialize(FALSE))
    {
        UiMessageBoxCritical("Unable to initialize UI.");
        goto Quit;
    }

    /* Initialize memory manager */
    if (!MmInitializeMemoryManager())
    {
        UiMessageBoxCritical("Unable to initialize memory manager.");
        goto Quit;
    }

    /* Initialize I/O subsystem */
    FsInit();

    if (!MachInitializeBootDevices())
    {
        UiMessageBoxCritical("Error when detecting hardware.");
        goto Quit;
    }

    /* Launch second stage loader */
    if (LaunchSecondStageLoader() != ESUCCESS)
    {
        UiMessageBoxCritical("Unable to load second stage loader.");
    }

Quit:
    /* If we reach this point, something went wrong before, therefore reboot */
    Reboot();
}

// We need to emulate these, because the original ones don't work in freeldr
// These functions are here, because they need to be in the main compilation unit
// and cannot be in a library.
int __cdecl wctomb(char *mbchar, wchar_t wchar)
{
    *mbchar = (char)wchar;
    return 1;
}

int __cdecl mbtowc(wchar_t *wchar, const char *mbchar, size_t count)
{
    *wchar = (wchar_t)*mbchar;
    return 1;
}

// The wctype table is 144 KB, too much for poor freeldr
int __cdecl iswctype(wint_t wc, wctype_t wctypeFlags)
{
    return _isctype((char)wc, wctypeFlags);
}

#ifdef _MSC_VER
#pragma warning(disable:4164)
#pragma function(pow)
#pragma function(log)
#pragma function(log10)
#endif

// Stubs to avoid pulling in data from CRT
double pow(double x, double y)
{
    __debugbreak();
    return 0.0;
}

double log(double x)
{
    __debugbreak();
    return 0.0;
}

double log10(double x)
{
    __debugbreak();
    return 0.0;
}

PCCHAR FrLdrGetBootPath(VOID)
{
    return FrLdrBootPath;
}

UCHAR FrldrGetBootDrive(VOID)
{
    return FrldrBootDrive;
}

ULONG FrldrGetBootPartition(VOID)
{
    return FrldrBootPartition;
}
