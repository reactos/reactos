#include <direct.h>
#include <windows.h>
#include <ctype.h>


unsigned int _getdiskfree(unsigned int _drive, struct _diskfree_t *_diskspace)
{
	char RootPathName[10];
	RootPathName[0] = toupper(_drive +'@');
	RootPathName[1] = ':';
	RootPathName[2] = '\\';
	RootPathName[3] = 0;
	if ( _diskspace == NULL )
		return 0;

	if ( !GetDiskFreeSpaceA(RootPathName,&_diskspace->sectors_per_cluster,&_diskspace->bytes_per_sector,&_diskspace->avail_clusters,&_diskspace->total_clusters ) )
		return 0;
	return _diskspace->avail_clusters;
}