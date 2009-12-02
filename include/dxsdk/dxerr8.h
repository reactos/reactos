

#ifndef _DXERR8_H_
#define _DXERR8_H_

#ifdef __cplusplus
extern "C" {
#endif

const char*  WINAPI DXGetErrorString8A(HRESULT hr);
const char*  WINAPI DXGetErrorDescription8A(HRESULT hr);
HRESULT WINAPI DXTraceA( const char* strFile, DWORD dwLine, HRESULT hr, const char* strMsg, BOOL bPopMsgBox );

const WCHAR* WINAPI DXGetErrorString8W(HRESULT hr);
const WCHAR* WINAPI DXGetErrorDescription8W(HRESULT hr);
HRESULT WINAPI DXTraceW( const char* strFile, DWORD dwLine, HRESULT hr, const WCHAR* strMsg, BOOL bPopMsgBox );


#ifdef UNICODE
  #define DXGetErrorString8 DXGetErrorString8W
  #define DXGetErrorDescription8 DXGetErrorDescription8W
  #define DXTrace DXTraceW
#else
  #define DXGetErrorString8 DXGetErrorString8A
  #define DXGetErrorDescription8 DXGetErrorDescription8A
  #define DXTrace DXTraceA
#endif

#if defined(DEBUG) | defined(_DEBUG)
  #define DXTRACE_MSG(str)                  DXTrace( __FILE__, (DWORD)__LINE__, 0, str, FALSE )
  #define DXTRACE_ERR(str,hr)               DXTrace( __FILE__, (DWORD)__LINE__, hr, str, TRUE )
  #define DXTRACE_ERR_NOMSGBOX(str,hr)      DXTrace( __FILE__, (DWORD)__LINE__, hr, str, FALSE )
#else
  #define DXTRACE_MSG(str)                  (0L)
  #define DXTRACE_ERR(str,hr)               (hr)
  #define DXTRACE_ERR_NOMSGBOX(str,hr)      (hr)
#endif


#ifdef __cplusplus
}
#endif

#endif

