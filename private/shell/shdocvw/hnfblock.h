#ifndef HNFBLOCK_H_
#define HNFBLOCK_H_
#include <iethread.h>

#ifdef __cplusplus
extern "C" {                        /* Assume C declarations for C++. */
#endif   /* __cplusplus */

DECLARE_HANDLE(HNFBLOCK);

HNFBLOCK ConvertNFItoHNFBLOCK(IETHREADPARAM* pInfo, LPCTSTR pszPath, DWORD dwProcId);
IETHREADPARAM *ConvertHNFBLOCKtoNFI(HNFBLOCK hBlock);

#ifdef __cplusplus
}                                   /* End of extern "C" {. */
#endif   /* __cplusplus */

#endif // HNFBLOCK_H_
