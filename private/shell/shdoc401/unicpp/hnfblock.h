#ifndef HNFBLOCK_H_
#define HNFBLOCK_H_

#include <iethread.h>

DECLARE_HANDLE(HNFBLOCK);

HNFBLOCK ConvertNFItoHNFBLOCK(IETHREADPARAM* pInfo, LPCTSTR pszPath, DWORD dwProcId);
IETHREADPARAM *ConvertHNFBLOCKtoNFI(HNFBLOCK hBlock);
BOOL DesktopOnCommandLine(HNFBLOCK hnf);

#endif
