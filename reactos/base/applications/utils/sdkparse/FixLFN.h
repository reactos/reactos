//
// FixLFN.h
//

#ifndef __FIXLFN_H
#define __FIXLFN_H

#include <string.h>
#include <tchar.h>
#include <shellapi.h>

inline int FixLFN ( const TCHAR* pBadFileName, TCHAR* pGoodFileName )
{
	SHFILEINFO sfi;
	TCHAR* p;

	DWORD dwResult = SHGetFileInfo ( pBadFileName, 0, &sfi, sizeof(sfi), SHGFI_DISPLAYNAME );
	if ( dwResult )
	{
		if ( pGoodFileName != pBadFileName )
			_tcscpy ( pGoodFileName, pBadFileName );
		if ( (p = _tcsrchr ( pGoodFileName, '\\' )) )
			_tcscpy ( p+1, sfi.szDisplayName );
		else if ( (p = _tcsrchr ( pGoodFileName, '/' )) )
			_tcscpy ( p+1, sfi.szDisplayName );
		else
			_tcscpy ( pGoodFileName, sfi.szDisplayName );
	}
	return dwResult;
}

#endif//__FIXLFN_H
