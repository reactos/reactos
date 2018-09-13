//================================================================================
//		File:	utiles .H
//		Date: 	6/27/98
//		Desc:	contains definition utility prototypes 
//================================================================================

#ifndef __UTILS_H__
#define __UTILS_H__

#define ARRAY_SIZE(x)   (sizeof(x) / sizeof(x[0]))


HRESULT GetLastWin32Error( );
HRESULT GetResourceStringValue(UINT idr, BSTR * pbstr );

#endif