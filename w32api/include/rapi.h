/* rapi.h - main header file for the RAPI API

        NOTE: This strictly does not belong in the Win32 API since it's
        really part of Platform SDK.

*/

#ifndef _RAPI_H
#define _RAPI_H
#if __GNUC__ >=3
#pragma GCC system_header
#endif

typedef struct IRAPIStream
{
  struct IRAPIStreamVtbl * lpVtbl;
} IRAPIStream;

typedef struct IRAPIStreamVtbl IRAPIStreamVtbl;

typedef enum tagRAPISTREAMFLAG
{
  STREAM_TIMEOUT_READ
} RAPISTREAMFLAG;

struct IRAPIStreamVtbl
{
  HRESULT (__stdcall * SetRapiStat)( IRAPIStream *, RAPISTREAMFLAG, DWORD);
  HRESULT (__stdcall * GetRapiStat)( IRAPIStream *, RAPISTREAMFLAG, DWORD *);
};

typedef  HRESULT (STDAPICALLTYPE RAPIEXT)(DWORD, BYTE, DWORD, BYTE, IRAPIStream	*);

typedef struct _RAPIINIT
{
  DWORD cbSize;
  HANDLE heRapiInit;
  HRESULT hrRapiInit;
} RAPIINIT;

STDAPI CeRapiInit (void);
STDAPI CeRapiInitEx (RAPIINIT*);
STDAPI_(BOOL) CeCreateProcess (LPCWSTR, LPCWSTR, LPSECURITY_ATTRIBUTES, LPSECURITY_ATTRIBUTES,
			       BOOL, DWORD, LPVOID, LPWSTR, LPSTARTUPINFO, LPPROCESS_INFORMATION);
STDAPI CeRapiUninit (void);

STDAPI_(BOOL) CeWriteFile (HANDLE, LPCVOID, DWORD, LPDWORD, LPOVERLAPPED);
STDAPI_(HANDLE) CeCreateFile (LPCWSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
STDAPI_(BOOL) CeCreateDirectory (LPCWSTR, LPSECURITY_ATTRIBUTES);
STDAPI_(DWORD) CeGetLastError (void);
STDAPI_(BOOL) CeGetFileTime (HANDLE, LPFILETIME, LPFILETIME, LPFILETIME);
STDAPI_(BOOL) CeCloseHandle (HANDLE);

#endif /* _RAPI_H */
