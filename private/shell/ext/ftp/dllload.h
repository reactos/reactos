#ifndef _DLLLOAD_H_
#define _DLLLOAD_H_

#include <wininet.h>
#include <winineti.h>

// MLANG.DLL
HRESULT ConvertINetMultiByteToUnicode(LPDWORD lpdwMode, DWORD dwEncoding, LPCSTR lpSrcStr, LPINT lpnMultiCharCount, LPWSTR lpDstStr, LPINT lpnWideCharCount);
HRESULT ConvertINetUnicodeToMultiByte(LPDWORD lpdwMode, DWORD dwEncoding, LPCWSTR lpSrcStr, LPINT lpnWideCharCount, LPSTR lpDstStr, LPINT lpnMultiCharCount);

HRESULT __stdcall _SHCreateShellFolderView(const SFV_CREATE* pcsfv, LPSHELLVIEW FAR* ppsv);
STDAPI _SHPathPrepareForWriteW(HWND hwnd, IUnknown *punkEnableModless, LPCWSTR pwzPath, DWORD dwFlags);


#endif // _DLLLOAD_H_

