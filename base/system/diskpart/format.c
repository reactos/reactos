/*
 * PROJECT:         ReactOS DiskPart
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/system/diskpart/format.c
 * PURPOSE:         Manages all the partitions of the OS in an interactive way.
 * PROGRAMMERS:     Lee Schroeder
 */

#include "diskpart.h"

#include <fmifs/fmifs.h>

#define NDEBUG
#include <debug.h>

static
BOOL
GetFsModule(
    _In_ PWSTR pszFileSystem,
    _Out_ HMODULE *phModule)
{
    WCHAR szDllNameBuffer[32];
    HKEY hKey;
    DWORD dwLength, dwErr;

    *phModule = NULL;

    dwErr = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                          L"SOFTWARE\\ReactOS\\ReactOS\\CurrentVersion\\IFS",
                          0,
                          KEY_READ,
                          &hKey);
    if (dwErr != ERROR_SUCCESS)
    {
        DPRINT1("Failed to open!\n");
        return FALSE;
    }

    dwLength = sizeof(szDllNameBuffer);
    dwErr = RegQueryValueExW(hKey,
                             pszFileSystem,
                             NULL,
                             NULL,
                             (PBYTE)szDllNameBuffer,
                             &dwLength);

    RegCloseKey(hKey);

    if (dwErr != ERROR_SUCCESS)
    {
        DPRINT1("Failed to query!\n");
        return FALSE;
    }

    *phModule = LoadLibraryW(szDllNameBuffer);
    if (*phModule == NULL)
    {
        DPRINT1("Failed to load!\n");
        return FALSE;
    }

    return TRUE;
}


BOOLEAN
WINAPI
FormatCallback(
    CALLBACKCOMMAND Command,
    ULONG Size,
    PVOID Argument)
{
    PDWORD      percent;

    switch (Command)
    {
        case PROGRESS:
            percent = (PDWORD)Argument;
            ConResPrintf(StdOut, IDS_FORMAT_PROGRESS, *percent);
            break;

        case DONE:
            break;

        default:
            DPRINT1("Callback (%u %lu %p)\n", Command, Size, Argument);
            break;
    }

    return TRUE;
}


EXIT_CODE
format_main(
    _In_ INT argc,
    _In_ PWSTR *argv)
{
    UNICODE_STRING usDriveRoot;
    UNICODE_STRING LabelString;
    PWSTR pszSuffix = NULL;
    PWSTR pszFileSystem = NULL;
    PWSTR pszLabel = NULL;
    BOOLEAN bQuickFormat = FALSE;
    ULONG ulClusterSize = 0;
    HMODULE hModule = NULL;
    PULIB_FORMAT pFormat = NULL;
//    FMIFS_MEDIA_FLAG MediaType = FMIFS_HARDDISK;
    INT i;
    BOOLEAN Success = FALSE;


    if (CurrentVolume == NULL)
    {
        ConResPuts(StdOut, IDS_SELECT_NO_VOLUME);
        return EXIT_SUCCESS;
    }

    for (i = 1; i < argc; i++)
    {
        if (_wcsicmp(argv[i], L"noerr") == 0)
        {
            /* noerr */
            DPRINT("NoErr\n", pszSuffix);
            ConPuts(StdOut, L"The NOERR option is not supported yet!\n");
#if 0
            bNoErr = TRUE;
#endif
        }
    }

    for (i = 1; i < argc; i++)
    {
        ConPrintf(StdOut, L"%s\n", argv[i]);

        if (HasPrefix(argv[i], L"fs=", &pszSuffix))
        {
            /* fs=<fs> */
            pszFileSystem = pszSuffix;
        }
        else if (HasPrefix(argv[i], L"revision=", &pszSuffix))
        {
            /* revision=<X.XX> */
            ConPuts(StdOut, L"The REVISION option is not supported yet!\n");
        }
        else if (HasPrefix(argv[i], L"label=", &pszSuffix))
        {
            /* label=<"label"> */
            pszLabel = pszSuffix;
        }
        else if (HasPrefix(argv[i], L"unit=", &pszSuffix))
        {
            /* unit=<N> */
            ulClusterSize = wcstoul(pszSuffix, NULL, 0);
            if ((ulClusterSize == 0) && (errno == ERANGE))
            {
                ConResPuts(StdErr, IDS_ERROR_INVALID_ARGS);
                goto done;
            }
        }
        else if (_wcsicmp(argv[i], L"recommended") == 0)
        {
            /* recommended */
            ConPuts(StdOut, L"The RECOMMENDED option is not supported yet!\n");
        }
        else if (_wcsicmp(argv[i], L"quick") == 0)
        {
            /* quick */
            bQuickFormat = TRUE;
        }
        else if (_wcsicmp(argv[i], L"compress") == 0)
        {
            /* compress */
            ConPuts(StdOut, L"The COMPRESS option is not supported yet!\n");
        }
        else if (_wcsicmp(argv[i], L"override") == 0)
        {
            /* override */
            ConPuts(StdOut, L"The OVERRIDE option is not supported yet!\n");
        }
        else if (_wcsicmp(argv[i], L"duplicate") == 0)
        {
            /* duplicate */
            ConPuts(StdOut, L"The DUPLICATE option is not supported yet!\n");
        }
        else if (_wcsicmp(argv[i], L"nowait") == 0)
        {
            /* nowait */
            ConPuts(StdOut, L"The NOWAIT option is not supported yet!\n");
        }
        else if (_wcsicmp(argv[i], L"noerr") == 0)
        {
            /* noerr - Already handled above */
        }
        else
        {
            ConResPuts(StdErr, IDS_ERROR_INVALID_ARGS);
            return EXIT_SUCCESS;
        }
    }

    DPRINT("VolumeName    : %S\n", CurrentVolume->VolumeName);
    DPRINT("DeviceName    : %S\n", CurrentVolume->DeviceName);
    DPRINT("DriveLetter   : %C\n", CurrentVolume->DriveLetter);
#if 0
    switch (CurrentVolume->VolumeType)
    {
        case VOLUME_TYPE_CDROM:
            MediaType = FMIFS_REMOVABLE; // ???
            break;

        case VOLUME_TYPE_PARTITION:
            MediaType = FMIFS_HARDDISK;
            break;

        case VOLUME_TYPE_REMOVABLE:
            MediaType = FMIFS_REMOVABLE; // ???
            break;

        case VOLUME_TYPE_UNKNOWN:
        default:
            MediaType = FMIFS_REMOVABLE; // ???
            break;
    }
#endif

    DPRINT("FileSystem: %S\n", pszFileSystem);
    DPRINT("Label: %S\n", pszLabel);
    DPRINT("Quick: %d\n", bQuickFormat);
    DPRINT("ClusterSize: %lu\n", ulClusterSize);

    RtlDosPathNameToNtPathName_U(CurrentVolume->VolumeName, &usDriveRoot, NULL, NULL);

    /* Remove trailing backslash */
    if (usDriveRoot.Buffer[(usDriveRoot.Length / sizeof(WCHAR)) - 1] == L'\\')
    {
        usDriveRoot.Buffer[(usDriveRoot.Length / sizeof(WCHAR)) - 1] = UNICODE_NULL;
        usDriveRoot.Length -= sizeof(WCHAR);
    }

    DPRINT("DriveRoot: %wZ\n", &usDriveRoot);

    /* Use the FAT filesystem as default */
    if (pszFileSystem == NULL)
        pszFileSystem = L"FAT";

    BOOLEAN bBackwardCompatible = FALSE; // Default to latest FS versions.
    if (_wcsicmp(pszFileSystem, L"FAT") == 0)
        bBackwardCompatible = TRUE;
    // else if (wcsicmp(pszFileSystem, L"FAT32") == 0)
        // bBackwardCompatible = FALSE;

    if (pszLabel == NULL)
        pszLabel = L"";

    RtlInitUnicodeString(&LabelString, pszLabel);

    if (!GetFsModule(pszFileSystem, &hModule))
    {
        DPRINT1("GetFsModule() failed\n");
        goto done;
    }

    pFormat = (PULIB_FORMAT)GetProcAddress(hModule, "Format");
    if (pFormat)
    {
        Success = (pFormat)(&usDriveRoot,
                            FormatCallback,
                            bQuickFormat,
                            bBackwardCompatible,
                            FMIFS_HARDDISK, //MediaType,
                            &LabelString,
                            ulClusterSize);
    }

done:
    ConPuts(StdOut, L"\n");
    if (Success)
        ConResPrintf(StdOut, IDS_FORMAT_SUCCESS);
    else
        ConResPrintf(StdOut, IDS_FORMAT_FAIL);

    if (hModule)
        FreeLibrary(hModule);

    RtlFreeUnicodeString(&usDriveRoot);

    return EXIT_SUCCESS;
}
