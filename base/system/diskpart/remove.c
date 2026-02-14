/*
 * PROJECT:         ReactOS DiskPart
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/system/diskpart/remove.c
 * PURPOSE:         Manages all the partitions of the OS in an interactive way.
 * PROGRAMMERS:     Lee Schroeder
 */

#include "diskpart.h"

#define NDEBUG
#include <debug.h>

EXIT_CODE
remove_main(
    _In_ INT argc,
    _In_ PWSTR *argv)
{
    PWSTR pszSuffix = NULL;
    WCHAR DriveLetter = UNICODE_NULL;
    INT i, nExclusive = 0;
    BOOL bResult;

    DPRINT("remove_main()\n");

    if (CurrentVolume == NULL)
    {
        ConResPuts(StdOut, IDS_SELECT_NO_VOLUME);
        return EXIT_SUCCESS;
    }

    if (CurrentVolume->DriveLetter == UNICODE_NULL)
    {
        ConResPuts(StdOut, IDS_REMOVE_NO_LETTER);
        return EXIT_SUCCESS;
    }

    for (i = 1; i < argc; i++)
    {
        if (_wcsicmp(argv[i], L"noerr") == 0)
        {
            /* noerr */
            ConPuts(StdOut, L"The NOERR option is not supported yet!\n");
#if 0
            bNoErr = TRUE;
#endif
        }
    }

    for (i = 1; i < argc; i++)
    {
        if (HasPrefix(argv[i], L"letter=", &pszSuffix))
        {
            if (wcslen(pszSuffix) == 1)
            {
                DriveLetter = towupper(*pszSuffix);
                nExclusive++;
            }
            else
            {
                ConResPuts(StdErr, IDS_ERROR_INVALID_ARGS);
                return EXIT_SUCCESS; 
            }
        }
        else if (HasPrefix(argv[i], L"mount=", &pszSuffix))
        {
            DPRINT("Mount\n", pszSuffix);
            ConPuts(StdOut, L"The MOUNT option is not supported yet!\n");
            nExclusive++;
        }
        else if (_wcsicmp(argv[i], L"all") == 0)
        {
            ConPuts(StdOut, L"The ALL option is not supported yet!\n");
            nExclusive++;
        }
        else if (_wcsicmp(argv[i], L"dismount") == 0)
        {
            ConPuts(StdOut, L"The DISMOUNT option is not supported yet!\n");
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

    if (CurrentVolume->IsBoot || CurrentVolume->IsSystem)
    {
        ConResPuts(StdOut, IDS_ASSIGN_SYSTEM_VOLUME);
        return EXIT_SUCCESS;
    }

    if (nExclusive > 1)
    {
        ConResPuts(StdErr, IDS_ERROR_INVALID_ARGS);
        return EXIT_SUCCESS;
    }

    DPRINT("VolumeName: %S\n", CurrentVolume->VolumeName);
    DPRINT("DeviceName: %S\n", CurrentVolume->DeviceName);
    DPRINT("DriveLetter: %C\n", CurrentVolume->DriveLetter);

    if (DriveLetter != UNICODE_NULL)
    {
        DPRINT1("DriveLetter: %C\n", DriveLetter);

        if ((DriveLetter < L'C') || (DriveLetter > L'Z'))
        {
            ConResPuts(StdOut, IDS_ASSIGN_INVALID_LETTER);
            return EXIT_SUCCESS;
        }

        if (DriveLetter != CurrentVolume->DriveLetter)
        {
            ConResPuts(StdOut, IDS_REMOVE_WRONG_LETTER);
            return EXIT_SUCCESS;
        }
    }
    else
    {
        DriveLetter = CurrentVolume->DriveLetter;
    }

    bResult = DeleteDriveLetter(DriveLetter);
    if (bResult == FALSE)
    {
        ConResPuts(StdOut, IDS_REMOVE_FAIL);
        return EXIT_SUCCESS;
    }

    CurrentVolume->DriveLetter = UNICODE_NULL;
    ConResPuts(StdOut, IDS_REMOVE_SUCCESS);

    return EXIT_SUCCESS;
}
