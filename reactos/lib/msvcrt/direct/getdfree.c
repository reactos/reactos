#include <windows.h>
#include <msvcrt/ctype.h>
#include <msvcrt/direct.h>


unsigned int _getdiskfree(unsigned int _drive, struct _diskfree_t* _diskspace)
{
    char RootPathName[10];

    RootPathName[0] = toupper(_drive +'@');
    RootPathName[1] = ':';
    RootPathName[2] = '\\';
    RootPathName[3] = 0;
    if (_diskspace == NULL)
        return 0;
    if (!GetDiskFreeSpaceA(RootPathName,(LPDWORD)&_diskspace->sectors_per_cluster,(LPDWORD)&_diskspace->bytes_per_sector,
            (LPDWORD )&_diskspace->avail_clusters,(LPDWORD )&_diskspace->total_clusters))
        return 0;
    return _diskspace->avail_clusters;
}
