//
//  LoadImag.h
//
//  routines to load and decompress a graphics file using a MS Office
//  graphic import filter.
//

#define GIF_SUPPORT
#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

//
//  LoadDIBFromFile
//
//  load a image file using a image import filter.
//

LPBITMAPINFOHEADER LoadDIBFromFileA(LPCSTR szFileName);
LPBITMAPINFOHEADER LoadDIBFromFileW(LPCWSTR szFileName);
#ifdef UNICODE
#define LoadDIBFromFile LoadDIBFromFileW
#else
#define LoadDIBFromFile LoadDIBFromFileA
#endif
void               FreeDIB(LPBITMAPINFOHEADER lpbi);

//
// GetFilterInfo
//
BOOL GetInstalledFilters (BOOL bOpenFileDialog,int i,
                          LPTSTR szName, UINT cbName,
                          LPTSTR szExt, UINT cbExt,
                          LPTSTR szHandler, UINT cbHandler,
                          BOOL& bImageAPI);


#ifdef __cplusplus
}
#endif  /* __cplusplus */
