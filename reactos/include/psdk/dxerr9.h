/*

	dxerr9.h - Header file for the DirectX 9 Error API

	Written by Filip Navara <xnavara@volny.cz>

	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

*/

#ifndef _DXERR9_H
#define _DXERR9_H
#if __GNUC__ >=3
#pragma GCC system_header
#endif

#ifdef __cplusplus
extern "C" {
#endif

const char *WINAPI DXGetErrorString9A(HRESULT);
const WCHAR *WINAPI DXGetErrorString9W(HRESULT);
const char* WINAPI DXGetErrorDescription9A(HRESULT);
const WCHAR* WINAPI DXGetErrorDescription9W(HRESULT);
HRESULT WINAPI DXTraceA(const char*,DWORD,HRESULT,const char*,BOOL);
HRESULT WINAPI DXTraceW(const char*,DWORD,HRESULT,const WCHAR*,BOOL);

#ifdef UNICODE
#define DXGetErrorString9 DXGetErrorString9W
#define DXGetErrorDescription9 DXGetErrorDescription9W
#define DXTrace DXTraceW
#else
#define DXGetErrorString9 DXGetErrorString9A
#define DXGetErrorDescription9 DXGetErrorDescription9A
#define DXTrace DXTraceA
#endif 

#if defined(DEBUG) || defined(_DEBUG)
#define DXTRACE_MSG(str)	DXTrace(__FILE__,(DWORD)__LINE__,0,str,FALSE)
#define DXTRACE_ERR(str,hr)	DXTrace(__FILE__,(DWORD)__LINE__,hr,str,FALSE)
#define DXTRACE_ERR_MSGBOX(str,hr)	DXTrace(__FILE__,(DWORD)__LINE__,hr,str,TRUE)
#else
#define DXTRACE_MSG(str)	(0L)
#define DXTRACE_ERR(str,hr)	(hr)
#define DXTRACE_ERR_MSGBOX(str,hr)	(hr)
#endif

#ifdef __cplusplus
}
#endif
#endif
