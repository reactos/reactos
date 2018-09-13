//================================================================================
//		File:	OLEACAPI.H
//		Date: 	10-23-97
//		Desc:	Declares the structures, macros, and variables needed to
//				dynamically link to the API exposed in OLEACC.DLL.
//================================================================================

#ifndef __OLEACAPI__
#define __OLEACAPI__


//--------------------------------------------------
//	Structure to hold the function pointer and
//	name of an exposed OLEACC.DLL API used in
//	MSAAHTML.DLL.
//--------------------------------------------------

struct OLEACCAPI
{
	FARPROC		lpfn;
	LPCSTR		lpszAPIName;
};



#define	INIT_OLEACCAPI_STRUCTURE() \
struct OLEACCAPI g_OleAccAPI[] = { \
	{ NULL, "ObjectFromLresult" }, \
	{ NULL, "CreateStdAccessibleObject" }, \
};

#define	OLEACCAPI_LPFN(i)	(g_OleAccAPI[i].lpfn)
#define	OLEACCAPI_NAME(i)	(g_OleAccAPI[i].lpszAPIName)

#define	SIZE_OLEACCAPI								(sizeof(g_OleAccAPI)/sizeof(OLEACCAPI))

#define IDX_OLEACCAPI_OBJECTFROMLRESULT				0
#define	IDX_OLEACCAPI_CREATESTDACCESSIBLEOBJECT		1

#undef ObjectFromLresult
#define	ObjectFromLresult			((LPFNOBJECTFROMLRESULT)(g_OleAccAPI[IDX_OLEACCAPI_OBJECTFROMLRESULT].lpfn))

#undef CreateStdAccessibleObject
#define	CreateStdAccessibleObject	((LPFNCREATESTDACCESSIBLEOBJECT)(g_OleAccAPI[IDX_OLEACCAPI_CREATESTDACCESSIBLEOBJECT].lpfn))


//--------------------------------------------------
//	External declaration of OLEACCAPI structure
//	defined in MSAAHTML.CPP.
//--------------------------------------------------

extern	OLEACCAPI	g_OleAccAPI[];



#endif // __OLEACAPI__
