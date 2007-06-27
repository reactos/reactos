
#ifndef _WINPPI_
#define _WINPPI_

typedef int (CALLBACK* EMFPLAYPROC)( HDC, INT, HANDLE );

#define  EMF_PP_COLOR_OPTIMIZATION  0x01

HANDLE WINAPI
GdiGetSpoolFileHandle(
    LPWSTR pwszPrinterName,
    LPDEVMODEW pDevmode,
    LPWSTR pwszDocName);

BOOL WINAPI
GdiDeleteSpoolFileHandle(HANDLE SpoolFileHandle);

DWORD WINAPI
GdiGetPageCount(HANDLE SpoolFileHandle);

HDC WINAPI
GdiGetDC(HANDLE SpoolFileHandle);

HANDLE WINAPI
GdiGetPageHandle(
    HANDLE SpoolFileHandle,
    DWORD Page,
    LPDWORD pdwPageType);

BOOL WINAPI
GdiStartDocEMF(
    HANDLE SpoolFileHandle,
    DOCINFOW *pDocInfo);

BOOL WINAPI
GdiStartPageEMF(
    HANDLE SpoolFileHandle);

BOOL WINAPI
GdiPlayPageEMF(
    HANDLE SpoolFileHandle,
    HANDLE hemf,
    RECT *prectDocument,
    RECT *prectBorder,
    RECT *prectClip);

BOOL WINAPI
GdiEndPageEMF(
    HANDLE SpoolFileHandle,
    DWORD dwOptimization);

BOOL WINAPI
GdiEndDocEMF(
    HANDLE SpoolFileHandle);

BOOL WINAPI
GdiGetDevmodeForPage(
    HANDLE SpoolFileHandle,
    DWORD dwPageNumber,
    PDEVMODEW *pCurrDM,
    PDEVMODEW *pLastDM);

BOOL WINAPI
GdiResetDCEMF(
    HANDLE SpoolFileHandle,
    PDEVMODEW pCurrDM);

#endif

