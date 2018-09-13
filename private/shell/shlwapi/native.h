#pragma once


STDAPI _CreateStreamOnFileA(LPCSTR pszFile, DWORD grfMode, IStream **ppstm);
STDAPI _CreateStreamOnFileW(LPCWSTR pszFile, DWORD grfMode, IStream **ppstm);

#ifdef UNICODE
#define _CreateStreamOnFile _CreateStreamOnFileW
#else
#define _CreateStreamOnFile _CreateStreamOnFileA
#endif
