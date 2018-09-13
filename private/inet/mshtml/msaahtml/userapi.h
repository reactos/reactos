//================================================================================
//		File:	USERAPI.H
//		Date: 	11-12-97
//		Desc:	Declares the structures, macros, and variables needed to
//				dynamically link to the API exposed in USER32.DLL.
//================================================================================

#ifndef __USERAPI__
#define __USERAPI__


//--------------------------------------------------
//	Structure to hold the function pointer and
//	name of an exposed USER32.DLL API used in
//	MSAAHTML.DLL.
//--------------------------------------------------

struct USERAPI
{
	FARPROC		lpfn;
	LPCSTR		lpszAPIName;
};



#define	INIT_USERAPI_STRUCTURE() \
struct USERAPI g_UserAPI[] = { \
	{ NULL, "SetWinEventHook" }, \
	{ NULL, "UnhookWinEvent" }, \
};

#define	USERAPI_LPFN(i)	(g_UserAPI[i].lpfn)
#define	USERAPI_NAME(i)	(g_UserAPI[i].lpszAPIName)

#define	SIZE_USERAPI								(sizeof(g_UserAPI)/sizeof(USERAPI))

#define IDX_USERAPI_SETWINEVENTHOOK				0
#define	IDX_USERAPI_UNHOOKWINEVENT				1

typedef HWINEVENTHOOK (WINAPI  *LPFNSETWINEVENTHOOK)(DWORD,DWORD,HMODULE,WINEVENTPROC,DWORD,DWORD,DWORD);
typedef BOOL (WINAPI * LPFNUNHOOKWINEVENT)(HWINEVENTHOOK);


#undef SetWinEventHook
#define	SetWinEventHook			((LPFNSETWINEVENTHOOK)(g_UserAPI[IDX_USERAPI_SETWINEVENTHOOK].lpfn))

#undef UnhookWinEvent
#define	UnhookWinEvent			((LPFNUNHOOKWINEVENT)(g_UserAPI[IDX_USERAPI_UNHOOKWINEVENT].lpfn))

//--------------------------------------------------
//	External declaration of USERAPI structure
//	defined in PROXYMGR.CPP.
//--------------------------------------------------

extern	USERAPI	g_UserAPI[];



#endif // __USERAPI__
