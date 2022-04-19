/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Windows-compatible NT OS Setup Loader.
 * COPYRIGHT:   Copyright 2009-2019 Aleksey Bragin <aleksey@reactos.org>
 */

#include <freeldr.h>
#include <ndk/ldrtypes.h>
#include <arc/setupblk.h>
#include "winldr.h"
#include "inffile.h"
#include "ntldropts.h"

#include <debug.h>
DBG_DEFAULT_CHANNEL(WINDOWS);

// TODO: Move to .h
VOID
AllocateAndInitLPB(
    IN USHORT VersionToBoot,
    OUT PLOADER_PARAMETER_BLOCK* OutLoaderBlock);

static VOID
SetupLdrLoadNlsData(
    _Inout_ PLOADER_PARAMETER_BLOCK LoaderBlock,
    _In_ HINF InfHandle,
    _In_ PCSTR SearchPath)
{
    BOOLEAN Success;
    INFCONTEXT InfContext;
    PCSTR AnsiData;
    UNICODE_STRING AnsiFileName = {0};
    UNICODE_STRING OemFileName = {0};
    UNICODE_STRING LangFileName = {0}; // CaseTable
    UNICODE_STRING OemHalFileName = {0};

    /* Get ANSI codepage file */
    if (!InfFindFirstLine(InfHandle, "NLS", "AnsiCodepage", &InfContext) ||
        !InfGetDataField(&InfContext, 1, &AnsiData) ||
        !RtlCreateUnicodeStringFromAsciiz(&AnsiFileName, AnsiData))
    {
        ERR("Failed to find or get 'NLS/AnsiCodepage'\n");
        return;
    }

    /* Get OEM codepage file */
    if (!InfFindFirstLine(InfHandle, "NLS", "OemCodepage", &InfContext) ||
        !InfGetDataField(&InfContext, 1, &AnsiData) ||
        !RtlCreateUnicodeStringFromAsciiz(&OemFileName, AnsiData))
    {
        ERR("Failed to find or get 'NLS/OemCodepage'\n");
        goto Quit;
    }

    /* Get the Unicode case table file */
    if (!InfFindFirstLine(InfHandle, "NLS", "UnicodeCasetable", &InfContext) ||
        !InfGetDataField(&InfContext, 1, &AnsiData) ||
        !RtlCreateUnicodeStringFromAsciiz(&LangFileName, AnsiData))
    {
        ERR("Failed to find or get 'NLS/UnicodeCasetable'\n");
        goto Quit;
    }

    /* Get OEM HAL font file */
    if (!InfFindFirstLine(InfHandle, "NLS", "OemHalFont", &InfContext) ||
        !InfGetData(&InfContext, NULL, &AnsiData) ||
        !RtlCreateUnicodeStringFromAsciiz(&OemHalFileName, AnsiData))
    {
        WARN("Failed to find or get 'NLS/OemHalFont'\n");
        /* Ignore, this is an optional file */
        RtlInitEmptyUnicodeString(&OemHalFileName, NULL, 0);
    }

    TRACE("NLS data: '%wZ' '%wZ' '%wZ' '%wZ'\n",
          &AnsiFileName, &OemFileName, &LangFileName, &OemHalFileName);

    /* Load NLS data */
    Success = WinLdrLoadNLSData(LoaderBlock,
                                SearchPath,
                                &AnsiFileName,
                                &OemFileName,
                                &LangFileName,
                                &OemHalFileName);
    TRACE("NLS data loading %s\n", Success ? "successful" : "failed");
    (VOID)Success;

Quit:
    RtlFreeUnicodeString(&OemHalFileName);
    RtlFreeUnicodeString(&LangFileName);
    RtlFreeUnicodeString(&OemFileName);
    RtlFreeUnicodeString(&AnsiFileName);
}

static
BOOLEAN
SetupLdrInitErrataInf(
    IN OUT PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN HINF InfHandle,
    IN PCSTR SystemRoot)
{
    INFCONTEXT InfContext;
    PCSTR FileName;
    ULONG FileSize;
    PVOID PhysicalBase;
    CHAR ErrataFilePath[MAX_PATH];

    /* Retrieve the INF file name value */
    if (!InfFindFirstLine(InfHandle, "BiosInfo", "InfName", &InfContext))
    {
        WARN("Failed to find 'BiosInfo/InfName'\n");
        return FALSE;
    }
    if (!InfGetDataField(&InfContext, 1, &FileName))
    {
        WARN("Failed to read 'InfName' value\n");
        return FALSE;
    }

    RtlStringCbCopyA(ErrataFilePath, sizeof(ErrataFilePath), SystemRoot);
    RtlStringCbCatA(ErrataFilePath, sizeof(ErrataFilePath), FileName);

    /* Load the INF file */
    PhysicalBase = WinLdrLoadModule(ErrataFilePath, &FileSize, LoaderRegistryData);
    if (!PhysicalBase)
    {
        WARN("Could not load '%s'\n", ErrataFilePath);
        return FALSE;
    }

    LoaderBlock->Extension->EmInfFileImage = PaToVa(PhysicalBase);
    LoaderBlock->Extension->EmInfFileSize  = FileSize;

    return TRUE;
}

static VOID
SetupLdrScanBootDrivers(
    _Inout_ PLIST_ENTRY BootDriverListHead,
    _In_ HINF InfHandle,
    _In_ PCSTR SearchPath)
{
    INFCONTEXT InfContext, dirContext;
    PCSTR Media, DriverName, dirIndex, ImagePath;
    BOOLEAN Success;
    WCHAR ImagePathW[MAX_PATH];
    WCHAR DriverNameW[256];

    UNREFERENCED_PARAMETER(SearchPath);

    /* Open INF section */
    if (!InfFindFirstLine(InfHandle, "SourceDisksFiles", NULL, &InfContext))
        goto Quit;

    /* Load all listed boot drivers */
    do
    {
        if (InfGetDataField(&InfContext, 7, &Media) &&
            InfGetDataField(&InfContext, 0, &DriverName) &&
            InfGetDataField(&InfContext, 13, &dirIndex))
        {
            if ((strcmp(Media, "x") == 0) &&
                InfFindFirstLine(InfHandle, "Directories", dirIndex, &dirContext) &&
                InfGetDataField(&dirContext, 1, &ImagePath))
            {
                /* Prepare image path */
                RtlStringCbPrintfW(ImagePathW, sizeof(ImagePathW),
                                   L"%S\\%S", ImagePath, DriverName);

                /* Convert name to unicode and remove .sys extension */
                RtlStringCbPrintfW(DriverNameW, sizeof(DriverNameW),
                                   L"%S", DriverName);
                DriverNameW[wcslen(DriverNameW) - 4] = UNICODE_NULL;

                /* Add it to the list */
                Success = WinLdrAddDriverToList(BootDriverListHead,
                                                FALSE,
                                                DriverNameW,
                                                ImagePathW,
                                                NULL,
                                                SERVICE_ERROR_NORMAL,
                                                -1);
                if (!Success)
                {
                    ERR("Could not add boot driver '%s'\n", DriverName);
                    /* Ignore and continue adding other drivers */
                }
            }
        }
    } while (InfFindNextLine(&InfContext, &InfContext));

Quit:
    /* Finally, add the boot filesystem driver to the list */
    if (BootFileSystem)
    {
        TRACE("Adding filesystem driver %S\n", BootFileSystem);
        Success = WinLdrAddDriverToList(BootDriverListHead,
                                        FALSE,
                                        BootFileSystem,
                                        NULL,
                                        L"Boot File System",
                                        SERVICE_ERROR_CRITICAL,
                                        -1);
        if (!Success)
            ERR("Failed to add filesystem driver %S\n", BootFileSystem);
    }
    else
    {
        TRACE("No required filesystem driver\n");
    }
}


/* SETUP STARTER **************************************************************/

/*
 * Update the options in the buffer pointed by LoadOptions, of maximum size
 * BufferSize, by first removing any specified options, and then adding any
 * other ones.
 *
 * OptionsToAdd is a NULL-terminated array of string buffer pointers that
 *    specify the options to be added into LoadOptions. Whether they are
 *    prepended or appended to LoadOptions is controlled via the Append
 *    parameter. The options are added in the order specified by the array.
 *
 * OptionsToRemove is a NULL-terminated array of string buffer pointers that
 *    specify the options to remove from LoadOptions. Specifying also there
 *    any options to add, has the effect of removing from LoadOptions any
 *    duplicates of the options to be added, before adding them later into
 *    LoadOptions. The options are removed in the order specified by the array.
 *
 * The options string buffers in the OptionsToRemove array have the format:
 *    "/option1 /option2[=] ..."
 *
 * An option in the OptionsToRemove list with a trailing '=' or ':' designates
 * an option in LoadOptions with user-specific data appended after the sign.
 * When such an option is being removed from LoadOptions, all the appended
 * data is also removed until the next option.
 */
VOID
NtLdrUpdateLoadOptions(
    IN OUT PSTR LoadOptions,
    IN ULONG BufferSize,
    IN BOOLEAN Append,
    IN PCSTR OptionsToAdd[] OPTIONAL,
    IN PCSTR OptionsToRemove[] OPTIONAL)
{
    PCSTR NextOptions, NextOpt;
    PSTR Options, Option;
    ULONG NextOptLength;
    ULONG OptionLength;

    if (!LoadOptions || (BufferSize == 0))
        return;
    // ASSERT(strlen(LoadOptions) + 1 <= BufferSize);

    /* Loop over the options to remove */
    for (; OptionsToRemove && *OptionsToRemove; ++OptionsToRemove)
    {
        NextOptions = *OptionsToRemove;
        while ((NextOpt = NtLdrGetNextOption(&NextOptions, &NextOptLength)))
        {
            /* Scan the load options */
            Options = LoadOptions;
            while ((Option = (PSTR)NtLdrGetNextOption((PCSTR*)&Options, &OptionLength)))
            {
                /*
                 * Check whether the option to find exactly matches the current
                 * load option, or is a prefix thereof if this is an option with
                 * appended data.
                 */
                if ((OptionLength >= NextOptLength) &&
                    (_strnicmp(Option, NextOpt, NextOptLength) == 0))
                {
                    if ((OptionLength == NextOptLength) ||
                        (NextOpt[NextOptLength-1] == '=') ||
                        (NextOpt[NextOptLength-1] == ':'))
                    {
                        /* Eat any skipped option or whitespace separators */
                        while ((Option > LoadOptions) &&
                               (Option[-1] == '/' ||
                                Option[-1] == ' ' ||
                                Option[-1] == '\t'))
                        {
                            --Option;
                        }

                        /* If the option was not preceded by a whitespace
                         * separator, insert one and advance the pointer. */
                        if ((Option > LoadOptions) &&
                            (Option[-1] != ' ') &&
                            (Option[-1] != '\t') &&
                            (*Options != '\0') /* &&
                            ** Not necessary since NtLdrGetNextOption() **
                            ** stripped any leading separators.         **
                            (*Options != ' ') &&
                            (*Options != '\t') */)
                        {
                            *Option++ = ' ';
                        }

                        /* Move the remaining options back, erasing the current one */
                        ASSERT(Option <= Options);
                        RtlMoveMemory(Option,
                                      Options,
                                      (strlen(Options) + 1) * sizeof(CHAR));

                        /* Reset the iterator */
                        Options = Option;
                    }
                }
            }
        }
    }

    /* Now loop over the options to add */
    for (; OptionsToAdd && *OptionsToAdd; ++OptionsToAdd)
    {
        NtLdrAddOptions(LoadOptions,
                        BufferSize,
                        Append,
                        *OptionsToAdd);
    }
}


/*
 * List of options and their corresponding higher priority ones,
 * that are either checked before any other ones, or whose name
 * includes another option name as a subset (e.g. NODEBUG vs. DEBUG).
 * See also https://geoffchappell.com/notes/windows/boot/editoptions.htm
 */
static const struct
{
    PCSTR Options;
    PCSTR ExtraOptions;
    PCSTR HigherPriorOptions;
} HighPriorOptionsMap[] =
{
    /* NODEBUG has a higher precedence than DEBUG */
    {"/DEBUG/DEBUG=", NULL, "/NODEBUG"},

    /* When using SCREEN debug port, we need boot video */
    {"/DEBUGPORT=SCREEN", NULL, "/NOGUIBOOT"},

    /* DETECTHAL has a higher precedence than HAL= or KERNEL= */
    {"/HAL=/KERNEL=", NULL, "/DETECTHAL"},

    /* NOPAE has a higher precedence than PAE */
    {"/PAE", NULL, "/NOPAE"},

    /* NOEXECUTE(=) has a higher precedence than EXECUTE */
    {"/EXECUTE", "/NOEXECUTE=ALWAYSOFF", "/NOEXECUTE/NOEXECUTE="},
    /* NOEXECUTE(=) options are self-excluding and
     * some have higher precedence than others. */
    {"/NOEXECUTE/NOEXECUTE=", NULL, "/NOEXECUTE/NOEXECUTE="},

    /* SAFEBOOT(:) options are self-excluding */
    {"/SAFEBOOT/SAFEBOOT:", NULL, "/SAFEBOOT/SAFEBOOT:"},
};

#define TAG_BOOT_OPTIONS 'pOtB'

VOID
NtLdrGetHigherPriorityOptions(
    IN PCSTR BootOptions,
    OUT PSTR* ExtraOptions,
    OUT PSTR* HigherPriorityOptions)
{
    ULONG i;
    PCSTR NextOptions, NextOpt;
    ULONG NextOptLength;
    SIZE_T ExtraOptsSize = 0;
    SIZE_T HighPriorOptsSize = 0;

    /* Masks specifying the presence (TRUE) or absence (FALSE) of the options */
    BOOLEAN Masks[RTL_NUMBER_OF(HighPriorOptionsMap)];

    /* Just return if we cannot return anything */
    if (!ExtraOptions && !HigherPriorityOptions)
        return;

    if (ExtraOptions)
        *ExtraOptions = NULL;
    if (HigherPriorityOptions)
        *HigherPriorityOptions = NULL;

    /* Just return if no initial options were given */
    if (!BootOptions || !*BootOptions)
        return;

    /* Determine the presence of the colliding options, and the
     * maximum necessary sizes for the pointers to be allocated. */
    RtlZeroMemory(Masks, sizeof(Masks));
    for (i = 0; i < RTL_NUMBER_OF(HighPriorOptionsMap); ++i)
    {
        /* Loop over the given options to search for */
        NextOptions = HighPriorOptionsMap[i].Options;
        while ((NextOpt = NtLdrGetNextOption(&NextOptions, &NextOptLength)))
        {
            /* If any of these options are present... */
            if (NtLdrGetOptionExN(BootOptions, NextOpt, NextOptLength, NULL))
            {
                /* ... set the mask, retrieve the sizes and stop looking for these options */
                Masks[i] = TRUE;
                if (ExtraOptions && HighPriorOptionsMap[i].ExtraOptions)
                {
                    ExtraOptsSize += strlen(HighPriorOptionsMap[i].ExtraOptions) * sizeof(CHAR);
                }
                if (HigherPriorityOptions && HighPriorOptionsMap[i].HigherPriorOptions)
                {
                    HighPriorOptsSize += strlen(HighPriorOptionsMap[i].HigherPriorOptions) * sizeof(CHAR);
                }
                break;
            }
        }
    }
    /* Count the NULL-terminator */
    if (ExtraOptions)
        ExtraOptsSize += sizeof(ANSI_NULL);
    if (HigherPriorityOptions)
        HighPriorOptsSize += sizeof(ANSI_NULL);

    /* Allocate the string pointers */
    if (ExtraOptions)
    {
        *ExtraOptions = FrLdrHeapAlloc(ExtraOptsSize, TAG_BOOT_OPTIONS);
        if (!*ExtraOptions)
            return;
    }
    if (HigherPriorityOptions)
    {
        *HigherPriorityOptions = FrLdrHeapAlloc(HighPriorOptsSize, TAG_BOOT_OPTIONS);
        if (!*HigherPriorityOptions)
        {
            if (ExtraOptions)
            {
                FrLdrHeapFree(*ExtraOptions, TAG_BOOT_OPTIONS);
                *ExtraOptions = NULL;
            }
            return;
        }
    }

    /* Initialize the strings */
    if (ExtraOptions)
        *(*ExtraOptions) = '\0';
    if (HigherPriorityOptions)
        *(*HigherPriorityOptions) = '\0';

    /* Go through the masks that determine the options to check */
    for (i = 0; i < RTL_NUMBER_OF(HighPriorOptionsMap); ++i)
    {
        if (Masks[i])
        {
            /* Retrieve the strings */
            if (ExtraOptions && HighPriorOptionsMap[i].ExtraOptions)
            {
                RtlStringCbCatA(*ExtraOptions,
                                ExtraOptsSize,
                                HighPriorOptionsMap[i].ExtraOptions);
            }
            if (HigherPriorityOptions && HighPriorOptionsMap[i].HigherPriorOptions)
            {
                RtlStringCbCatA(*HigherPriorityOptions,
                                HighPriorOptsSize,
                                HighPriorOptionsMap[i].HigherPriorOptions);
            }
        }
    }
}


ARC_STATUS
LoadReactOSSetup(
    IN ULONG Argc,
    IN PCHAR Argv[],
    IN PCHAR Envp[])
{
    ARC_STATUS Status;
    PCSTR ArgValue;
    PCSTR SystemPartition;
    PCSTR SystemPath;
    PSTR FileName;
    ULONG FileNameLength;
    BOOLEAN BootFromFloppy;
    BOOLEAN Success;
    HINF InfHandle;
    INFCONTEXT InfContext;
    ULONG i, ErrorLine;
    PLOADER_PARAMETER_BLOCK LoaderBlock;
    PSETUP_LOADER_BLOCK SetupBlock;
    CHAR BootPath[MAX_PATH];
    CHAR FilePath[MAX_PATH];
    CHAR UserBootOptions[256];
    PCSTR BootOptions;

    static PCSTR SourcePaths[] =
    {
        "", /* Only for floppy boot */
#if defined(_M_IX86)
        "I386\\",
#elif defined(_M_MPPC)
        "PPC\\",
#elif defined(_M_MRX000)
        "MIPS\\",
#endif
        "reactos\\",
        NULL
    };

    /* Retrieve the (mandatory) boot type */
    ArgValue = GetArgumentValue(Argc, Argv, "BootType");
    if (!ArgValue || !*ArgValue)
    {
        ERR("No 'BootType' value, aborting!\n");
        return EINVAL;
    }
    if (_stricmp(ArgValue, "ReactOSSetup") != 0)
    {
        ERR("Unknown 'BootType' value '%s', aborting!\n", ArgValue);
        return EINVAL;
    }

    /* Retrieve the (mandatory) system partition */
    SystemPartition = GetArgumentValue(Argc, Argv, "SystemPartition");
    if (!SystemPartition || !*SystemPartition)
    {
        ERR("No 'SystemPartition' specified, aborting!\n");
        return EINVAL;
    }

    /* Let the user know we started loading */
    UiDrawBackdrop();
    UiDrawStatusText("Setup is loading...");
    UiDrawProgressBarCenter("Loading ReactOS Setup...");

    /* Retrieve the system path */
    *BootPath = ANSI_NULL;
    ArgValue = GetArgumentValue(Argc, Argv, "SystemPath");
    if (ArgValue)
    {
        RtlStringCbCopyA(BootPath, sizeof(BootPath), ArgValue);
    }
    else
    {
        /*
         * IMPROVE: I don't want to use the SystemPartition here as a
         * default choice because I can do it after (see few lines below).
         * Instead I reset BootPath here so that we can build the full path
         * using the general code from below.
         */
        // RtlStringCbCopyA(BootPath, sizeof(BootPath), SystemPartition);
        *BootPath = ANSI_NULL;
    }

    /*
     * Check whether BootPath is a full path
     * and if not, create a full boot path.
     *
     * See FsOpenFile for the technique used.
     */
    if (strrchr(BootPath, ')') == NULL)
    {
        /* Temporarily save the boot path */
        RtlStringCbCopyA(FilePath, sizeof(FilePath), BootPath);

        /* This is not a full path: prepend the SystemPartition */
        RtlStringCbCopyA(BootPath, sizeof(BootPath), SystemPartition);

        /* Append a path separator if needed */
        if (*FilePath != '\\' && *FilePath != '/')
            RtlStringCbCatA(BootPath, sizeof(BootPath), "\\");

        /* Append the remaining path */
        RtlStringCbCatA(BootPath, sizeof(BootPath), FilePath);
    }

    /* Append a path separator if needed */
    if (!*BootPath || BootPath[strlen(BootPath) - 1] != '\\')
        RtlStringCbCatA(BootPath, sizeof(BootPath), "\\");

    TRACE("BootPath: '%s'\n", BootPath);

    /*
     * Retrieve the boot options. Any options present here will supplement or
     * override those that will be specified in TXTSETUP.SIF's OsLoadOptions.
     */
    BootOptions = GetArgumentValue(Argc, Argv, "Options");
    if (!BootOptions)
        BootOptions = "";

    TRACE("BootOptions: '%s'\n", BootOptions);

    /* Check if a RAM disk file was given */
    FileName = (PSTR)NtLdrGetOptionEx(BootOptions, "RDPATH=", &FileNameLength);
    if (FileName && (FileNameLength > 7))
    {
        /* Load the RAM disk */
        Status = RamDiskInitialize(FALSE, BootOptions, SystemPartition);
        if (Status != ESUCCESS)
        {
            FileName += 7; FileNameLength -= 7;
            UiMessageBox("Failed to load RAM disk file '%.*s'",
                         FileNameLength, FileName);
            return Status;
        }
    }

    /* Check if we booted from floppy */
    BootFromFloppy = strstr(BootPath, "fdisk") != NULL;

    /* Open 'txtsetup.sif' from any of the source paths */
    FileName = BootPath + strlen(BootPath);
    for (i = BootFromFloppy ? 0 : 1; ; i++)
    {
        SystemPath = SourcePaths[i];
        if (!SystemPath)
        {
            UiMessageBox("Failed to open txtsetup.sif");
            return ENOENT;
        }
        FileNameLength = (ULONG)(sizeof(BootPath) - (FileName - BootPath)*sizeof(CHAR));
        RtlStringCbCopyA(FileName, FileNameLength, SystemPath);
        RtlStringCbCopyA(FilePath, sizeof(FilePath), BootPath);
        RtlStringCbCatA(FilePath, sizeof(FilePath), "txtsetup.sif");
        if (InfOpenFile(&InfHandle, FilePath, &ErrorLine))
        {
            break;
        }
    }

    TRACE("BootPath: '%s', SystemPath: '%s'\n", BootPath, SystemPath);

    // UseLocalSif = NtLdrGetOption(BootOptions, "USELOCALSIF");

    if (NtLdrGetOption(BootOptions, "SIFOPTIONSOVERRIDE"))
    {
        PCSTR OptionsToRemove[2] = {"SIFOPTIONSOVERRIDE", NULL};

        /* Do not use any load options from TXTSETUP.SIF, but
         * use instead those passed from the command line. */
        RtlStringCbCopyA(UserBootOptions, sizeof(UserBootOptions), BootOptions);

        /* Remove the private switch from the options */
        NtLdrUpdateLoadOptions(UserBootOptions,
                               sizeof(UserBootOptions),
                               FALSE,
                               NULL,
                               OptionsToRemove);

        BootOptions = UserBootOptions;
    }
    else // if (!*BootOptions || NtLdrGetOption(BootOptions, "SIFOPTIONSADD"))
    {
        PCSTR LoadOptions = NULL;
        PCSTR DbgLoadOptions = NULL;
        PSTR ExtraOptions, HigherPriorityOptions;
        PSTR OptionsToAdd[3];
        PSTR OptionsToRemove[4];

        /* Load the options from TXTSETUP.SIF */
        if (InfFindFirstLine(InfHandle, "SetupData", "OsLoadOptions", &InfContext))
        {
            if (!InfGetDataField(&InfContext, 1, &LoadOptions))
                WARN("Failed to get load options\n");
        }

#if !DBG
        /* Non-debug mode: get the debug load options only if /DEBUG was specified
         * in the Argv command-line options (was e.g. added to the options when
         * the user selected "Debugging Mode" in the advanced boot menu). */
        if (NtLdrGetOption(BootOptions, "DEBUG") ||
            NtLdrGetOption(BootOptions, "DEBUG="))
        {
#else
        /* Debug mode: always get the debug load options */
#endif
        if (InfFindFirstLine(InfHandle, "SetupData", "SetupDebugOptions", &InfContext))
        {
            if (!InfGetDataField(&InfContext, 1, &DbgLoadOptions))
                WARN("Failed to get debug load options\n");
        }
        /* If none was found, default to enabling debugging */
        if (!DbgLoadOptions)
            DbgLoadOptions = "/DEBUG";
#if !DBG
        }
#endif

        /* Initialize the load options with those from TXTSETUP.SIF */
        *UserBootOptions = ANSI_NULL;
        if (LoadOptions && *LoadOptions)
            RtlStringCbCopyA(UserBootOptions, sizeof(UserBootOptions), LoadOptions);

        /* Merge the debug load options if any */
        if (DbgLoadOptions)
        {
            RtlZeroMemory(OptionsToAdd, sizeof(OptionsToAdd));
            RtlZeroMemory(OptionsToRemove, sizeof(OptionsToRemove));

            /*
             * Retrieve any option patterns that we should remove from the
             * SIF load options because they are of higher precedence than
             * those specified in the debug load options to be added.
             * Also always remove NODEBUG (even if the debug load options
             * do not contain explicitly the DEBUG option), since we want
             * to have debugging enabled if possible.
             */
            OptionsToRemove[0] = "/NODEBUG";
            NtLdrGetHigherPriorityOptions(DbgLoadOptions,
                                          &ExtraOptions,
                                          &HigherPriorityOptions);
            OptionsToAdd[1] = (ExtraOptions ? ExtraOptions : "");
            OptionsToRemove[1] = (HigherPriorityOptions ? HigherPriorityOptions : "");

            /*
             * Prepend the debug load options, so that in case it contains
             * redundant options with respect to the SIF load options, the
             * former can take precedence over the latter.
             */
            OptionsToAdd[0] = (PSTR)DbgLoadOptions;
            OptionsToRemove[2] = (PSTR)DbgLoadOptions;
            NtLdrUpdateLoadOptions(UserBootOptions,
                                   sizeof(UserBootOptions),
                                   FALSE,
                                   (PCSTR*)OptionsToAdd,
                                   (PCSTR*)OptionsToRemove);

            if (ExtraOptions)
                FrLdrHeapFree(ExtraOptions, TAG_BOOT_OPTIONS);
            if (HigherPriorityOptions)
                FrLdrHeapFree(HigherPriorityOptions, TAG_BOOT_OPTIONS);
        }

        RtlZeroMemory(OptionsToAdd, sizeof(OptionsToAdd));
        RtlZeroMemory(OptionsToRemove, sizeof(OptionsToRemove));

        /*
         * Retrieve any option patterns that we should remove from the
         * SIF load options because they are of higher precedence than
         * those specified in the options to be added.
         */
        NtLdrGetHigherPriorityOptions(BootOptions,
                                      &ExtraOptions,
                                      &HigherPriorityOptions);
        OptionsToAdd[1] = (ExtraOptions ? ExtraOptions : "");
        OptionsToRemove[0] = (HigherPriorityOptions ? HigherPriorityOptions : "");

        /* Finally, prepend the user-specified options that
         * take precedence over those from TXTSETUP.SIF. */
        OptionsToAdd[0] = (PSTR)BootOptions;
        OptionsToRemove[1] = (PSTR)BootOptions;
        NtLdrUpdateLoadOptions(UserBootOptions,
                               sizeof(UserBootOptions),
                               FALSE,
                               (PCSTR*)OptionsToAdd,
                               (PCSTR*)OptionsToRemove);

        if (ExtraOptions)
            FrLdrHeapFree(ExtraOptions, TAG_BOOT_OPTIONS);
        if (HigherPriorityOptions)
            FrLdrHeapFree(HigherPriorityOptions, TAG_BOOT_OPTIONS);

        BootOptions = UserBootOptions;
    }

    TRACE("BootOptions: '%s'\n", BootOptions);

    /* Handle the SOS option */
    SosEnabled = !!NtLdrGetOption(BootOptions, "SOS");
    if (SosEnabled)
        UiResetForSOS();

    /* Allocate and minimally-initialize the Loader Parameter Block */
    AllocateAndInitLPB(_WIN32_WINNT_WS03, &LoaderBlock);

    /* Allocate and initialize the setup loader block */
    SetupBlock = &WinLdrSystemBlock->SetupBlock;
    LoaderBlock->SetupLdrBlock = SetupBlock;

    /* Set textmode setup flag */
    SetupBlock->Flags = SETUPLDR_TEXT_MODE;

    /* Load the "setupreg.hiv" setup system hive */
    UiUpdateProgressBar(15, "Loading setup system hive...");
    Success = WinLdrInitSystemHive(LoaderBlock, BootPath, TRUE);
    TRACE("Setup SYSTEM hive %s\n", (Success ? "loaded" : "not loaded"));
    /* Bail out if failure */
    if (!Success)
        return ENOEXEC;

    /* Load NLS data, they are in the System32 directory of the installation medium */
    RtlStringCbCopyA(FilePath, sizeof(FilePath), BootPath);
    RtlStringCbCatA(FilePath, sizeof(FilePath), "system32\\");
    SetupLdrLoadNlsData(LoaderBlock, InfHandle, FilePath);

    /* Load the Firmware Errata file from the installation medium */
    Success = SetupLdrInitErrataInf(LoaderBlock, InfHandle, BootPath);
    TRACE("Firmware Errata file %s\n", (Success ? "loaded" : "not loaded"));
    /* Not necessarily fatal if not found - carry on going */

    // UiDrawStatusText("Press F6 if you need to install a 3rd-party SCSI or RAID driver...");

    /* Get a list of boot drivers */
    SetupLdrScanBootDrivers(&LoaderBlock->BootDriverListHead, InfHandle, BootPath);

    /* Close the inf file */
    InfCloseFile(InfHandle);

    UiDrawStatusText("The Setup program is starting...");

    /* Finish loading */
    return LoadAndBootWindowsCommon(_WIN32_WINNT_WS03,
                                    LoaderBlock,
                                    BootOptions,
                                    BootPath);
}
