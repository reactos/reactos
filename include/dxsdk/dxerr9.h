                                                                              #ifndef _DXERR9_H_
#define _DXERR9_H_

#ifdef __cplusplus
extern "C" {
#endif 


const char*  WINAPI DXGetErrorString9A(HRESULT hr);
const char*  WINAPI DXGetErrorDescription9A(HRESULT hr);
HRESULT WINAPI DXTraceA( const char* strFile, DWORD dwLine, HRESULT hr, const char* strMsg, BOOL bPopMsgBox );

const WCHAR* WINAPI DXGetErrorString9W(HRESULT hr);
const WCHAR* WINAPI DXGetErrorDescription9W(HRESULT hr);
HRESULT WINAPI DXTraceW( const char* strFile, DWORD dwLine, HRESULT hr, const WCHAR* strMsg, BOOL bPopMsgBox );

#ifdef UNICODE
  #define DXGetErrorString9 DXGetErrorString9W
  #define DXGetErrorDescription9 DXGetErrorDescription9W
  #define DXTrace DXTraceW
#else
  #define DXGetErrorString9 DXGetErrorString9A
  #define DXGetErrorDescription9 DXGetErrorDescription9A
  #define DXTrace DXTraceA
#endif 


#if defined(DEBUG) | defined(_DEBUG)
  #define DXTRACE_MSG(str)                  DXTrace( __FILE__, (DWORD)__LINE__, 0, str, FALSE )
  #define DXTRACE_ERR(str,hr)               DXTrace( __FILE__, (DWORD)__LINE__, hr, str, FALSE )
  #define DXTRACE_ERR_MSGBOX(str,hr)        DXTrace( __FILE__, (DWORD)__LINE__, hr, str, TRUE )
#else
  #define DXTRACE_MSG(str)                  (0L)
  #define DXTRACE_ERR(str,hr)               (hr)
  #define DXTRACE_ERR_MSGBOX(str,hr)        (hr)
#endif


#ifdef __cplusplus
}
#endif

#endif

