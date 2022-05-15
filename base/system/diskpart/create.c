/*
 * PROJECT:         ReactOS DiskPart
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/system/diskpart/create.c
 * PURPOSE:         Manages all the partitions of the OS in an interactive way.
 * PROGRAMMERS:     Lee Schroeder
 */

#include "diskpart.h"

BOOL
CreateExtendedPartition(
    INT argc,
    PWSTR *argv)
{
    if (CurrentDisk == NULL)
    {
        ConResPuts(StdOut, IDS_SELECT_NO_DISK);
        return TRUE;
    }

    ConPrintf(StdOut, L"Not implemented yet!\n");

    return TRUE;
}


BOOL
CreateLogicalPartition(
    INT argc,
    PWSTR *argv)
{
    if (CurrentDisk == NULL)
    {
        ConResPuts(StdOut, IDS_SELECT_NO_DISK);
        return TRUE;
    }

    ConPrintf(StdOut, L"Not implemented yet!\n");

    return TRUE;
}


BOOL
CreatePrimaryPartition(
    INT argc,
    PWSTR *argv)
{
    LARGE_INTEGER liSize, liOffset;
    INT i;
//    BOOL bNoErr = FALSE;
    PWSTR pszSuffix = NULL;

    liSize.QuadPart = -1;
    liOffset.QuadPart = -1;

/*
    if (CurrentDisk == NULL)
    {
        ConResPuts(StdOut, IDS_SELECT_NO_DISK);
        return TRUE;
    }
*/

    for (i = 3; i < argc; i++)
    {
        if (HasPrefix(argv[i], L"size=", &pszSuffix))
        {
            /* size=<N> (MB) */
            ConPrintf(StdOut, L"Size : %s\n", pszSuffix);

            liSize.QuadPart = _wcstoui64(pszSuffix, NULL, 10);
            if (((liSize.QuadPart == 0) && (errno == ERANGE)) ||
                (liSize.QuadPart < 0))
            {
                ConResPuts(StdErr, IDS_ERROR_INVALID_ARGS);
                return TRUE;
            }
        }
        else if (HasPrefix(argv[i], L"offset=", &pszSuffix))
        {
            /* offset=<N> (KB) */
            ConPrintf(StdOut, L"Offset : %s\n", pszSuffix);

            liOffset.QuadPart = _wcstoui64(pszSuffix, NULL, 10);
            if (((liOffset.QuadPart == 0) && (errno == ERANGE)) ||
                (liOffset.QuadPart < 0))
            {
                ConResPuts(StdErr, IDS_ERROR_INVALID_ARGS);
                return TRUE;
            }
        }
        else if (HasPrefix(argv[i], L"id=", &pszSuffix))
        {
            /* id=<Byte>|<GUID> */
            ConPrintf(StdOut, L"Id : %s\n", pszSuffix);
        }
        else if (HasPrefix(argv[i], L"align=", &pszSuffix))
        {
            /* align=<N> */
            ConPrintf(StdOut, L"Align : %s\n", pszSuffix);
        }
        else if (_wcsicmp(argv[i], L"noerr") == 0)
        {
            /* noerr */
            ConPrintf(StdOut, L"NoErr\n", pszSuffix);
//            bNoErr = TRUE;
        }
        else
        {
            ConResPuts(StdErr, IDS_ERROR_INVALID_ARGS);
            return TRUE;
        }
    }

    ConPrintf(StdOut, L"Size: %I64d\n", liSize.QuadPart);
    ConPrintf(StdOut, L"Offset: %I64d\n", liOffset.QuadPart);

    return TRUE;
}
