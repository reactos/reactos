#ifndef __ICW_h__
#define __ICW_h__
/**************************************************************************

   File:          icw.h
   
   Description:   

**************************************************************************/
typedef HRESULT (WINAPI *INETGETPROXY)(LPBOOL lpfEnable, LPSTR lpszServer, DWORD cbServer, LPSTR lpszOverride, DWORD cbOverride);
typedef HRESULT (WINAPI *INETCONFIGSYSTEM) ( HWND hWndParent, DWORD dwfOptions, LPBOOL lpfNeedsRestart);



#ifdef __cplusplus
extern "C" 
{
#endif

	BOOL ICWGetProxy(PTCHAR szProxy, DWORD dwBufferLength);

#ifdef __cplusplus
}
#endif	

#endif	// __ICW_h__
