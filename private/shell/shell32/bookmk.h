#ifndef _BOOKMK_H_
#define _BOOKMK_H_

#include "idlcomm.h"

STDAPI SHCreateStdEnumFmtEtc(UINT cfmt, const FORMATETC afmt[], IEnumFORMATETC **ppenumFormatEtc);
STDAPI SHCreateStdEnumFmtEtcEx(UINT cfmt, const FORMATETC afmt[], IDataObject *pdtInner, IEnumFORMATETC **ppenumFormatEtc);
STDAPI FS_CreateBookMark(HWND hwnd, LPCTSTR pszPath, IDataObject *pDataObj, POINTL pt, DWORD *pdwEffect);

#endif // _BOOKMK_H_
