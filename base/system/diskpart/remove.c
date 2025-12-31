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

BOOL
remove_main(
    _In_ INT argc,
    _In_ LPWSTR *argv)
{
    WCHAR szMountPoint[4];
    PWSTR pszSuffix = NULL;
    WCHAR DriveLetter = UNICODE_NULL;
    INT i, nExclusive = 0;
    BOOL bResult;

    DPRINT("remove_main()\n");

    if (CurrentVolume == NULL)
    {
        ConResPuts(StdOut, IDS_SELECT_NO_VOLUME);
        return TRUE;
    }

    if (CurrentVolume->DriveLetter == UNICODE_NULL)
    {
        ConResPuts(StdOut, IDS_REMOVE_NO_LETTER);
        return TRUE;
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
                return TRUE; 
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
            return TRUE;
        }
    }

    if (nExclusive > 1)
    {
        ConResPuts(StdErr, IDS_ERROR_INVALID_ARGS);
        return TRUE;
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
            return TRUE;
        }

        if (DriveLetter != CurrentVolume->DriveLetter)
        {
            ConResPuts(StdOut, IDS_REMOVE_WRONG_LETTER);
            return TRUE;
        }

        szMountPoint[0] = DriveLetter;
        szMountPoint[1] = L':';
        szMountPoint[2] = L'\\';
        szMountPoint[3] = UNICODE_NULL;
    }
    else
    {
        szMountPoint[0] = CurrentVolume->DriveLetter;
        szMountPoint[1] = L':';
        szMountPoint[2] = L'\\';
        szMountPoint[3] = UNICODE_NULL;
    }

    DPRINT("MountPoint: %S\n", szMountPoint);
    bResult = DeleteVolumeMountPointW(szMountPoint);
    if (bResult == FALSE)
    {
        ConResPuts(StdOut, IDS_REMOVE_FAIL);
        return TRUE;
    }

    CurrentVolume->DriveLetter = UNICODE_NULL;
    ConResPuts(StdOut, IDS_REMOVE_SUCCESS);

    return TRUE;
}
