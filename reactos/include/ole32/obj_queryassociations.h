/************************************************************
 *    IQueryAssociations
 */

#ifndef __WINE_WINE_OBJ_QUERYASSOCIATIONS_H
#define __WINE_WINE_OBJ_QUERYASSOCIATIONS_H

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

DEFINE_GUID(IID_IQueryAssociations, 0xc46ca590, 0x3c3f, 0x11d2, 0xbe, 0xe6, 0x00, 0x00, 0xf8, 0x05, 0xca, 0x57);

typedef struct IQueryAssociations IQueryAssociations,*LPQUERYASSOCIATIONS;

#define ASSOCF_INIT_BYEXENAME		0x00000002
#define ASSOCF_OPEN_BYEXENAME		0x00000002
#define ASSOCF_INIT_DEFAULTTOSTAR	0x00000004
#define ASSOCF_INIT_DEFAULTTOFOLDER	0x00000008
#define ASSOCF_NOUSERSETTINGS		0x00000010
#define ASSOCF_NOTRUNCATE		0x00000020
#define ASSOCF_VERIFY			0x00000040
#define ASSOCF_REMAPRUNDLL		0x00000080
#define ASSOCF_NOFIXUPS			0x00000100
#define ASSOCF_IGNOREBASECLASS		0x00000200

typedef DWORD ASSOCF;

typedef enum
{
	ASSOCSTR_COMMAND      = 1,
	ASSOCSTR_EXECUTABLE,
	ASSOCSTR_FRIENDLYDOCNAME,
	ASSOCSTR_FRIENDLYAPPNAME,
	ASSOCSTR_NOOPEN,
	ASSOCSTR_SHELLNEWVALUE,
	ASSOCSTR_DDECOMMAND,
	ASSOCSTR_DDEIFEXEC,
	ASSOCSTR_DDEAPPLICATION,
	ASSOCSTR_DDETOPIC,
	ASSOCSTR_MAX 
} ASSOCSTR;

typedef enum
{
	ASSOCKEY_SHELLEXECCLASS = 1,
	ASSOCKEY_APP,  
	ASSOCKEY_CLASS,
	ASSOCKEY_BASECLASS,
	ASSOCKEY_MAX   
} ASSOCKEY;

typedef enum
{
	ASSOCDATA_MSIDESCRIPTOR = 1,
	ASSOCDATA_NOACTIVATEHANDLER,
	ASSOCDATA_QUERYCLASSSTORE,
	ASSOCDATA_HASPERUSERASSOC,
	ASSOCDATA_MAX
} ASSOCDATA;

typedef enum
{
	ASSOCENUM_NONE
} ASSOCENUM;

#define ICOM_INTERFACE IQueryAssociations
#define IQueryAssociations_METHODS \
    ICOM_METHOD4 (HRESULT, Init, ASSOCF, flags, LPCWSTR, pszAssoc, HKEY, hkProgid, HWND, hwnd) \
    ICOM_METHOD5 (HRESULT, GetString, ASSOCF, flags, ASSOCSTR, str, LPCWSTR, pszExtra, LPWSTR, pszOut, DWORD*, pcchOut) \
    ICOM_METHOD4 (HRESULT, GetKey, ASSOCF, flags, ASSOCKEY, key, LPCWSTR, pszExtra, HKEY*, phkeyOut) \
    ICOM_METHOD5 (HRESULT, GetData, ASSOCF, flags, ASSOCDATA, data, LPCWSTR, pszExtra, LPVOID, pvOut, DWORD*, pcbOut) \
    ICOM_METHOD5 (HRESULT, GetEnum, ASSOCF, flags, ASSOCENUM, assocenum, LPCWSTR, pszExtra, REFIID, riid, LPVOID*, ppvOut)
#define IQueryAssociations_IMETHODS \
	IUnknown_IMETHODS \
	IQueryAssociations_METHODS
ICOM_DEFINE(IQueryAssociations,IUnknown)
#undef ICOM_INTERFACE

#define IQueryAssociations_QueryInterface(p,a,b)	ICOM_CALL2(QueryInterface,p,a,b)
#define IQueryAssociations_AddRef(p)			ICOM_CALL(AddRef,p)
#define IQueryAssociations_Release(p)			ICOM_CALL(Release,p)
#define IQueryAssociations_Init(p,a,b,c,d)		ICOM_CALL4(Init,p,a,b,c,d)
#define IQueryAssociations_GetString(p,a,b,c,d,e)	ICOM_CALL5(GetString,p,a,b,c,d,e)
#define IQueryAssociations_GetKey(p,a,b,c,d)		ICOM_CALL4(GetKey,p,a,b,c,d)
#define IQueryAssociations_GetData(p,a,b,c,d,e)		ICOM_CALL5(GetData,p,a,b,c,d,e)
#define IQueryAssociations_GetEnum(p,a,b,c,d,e)		ICOM_CALL5(GetEnum,p,a,b,c,d,e)

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* __WINE_WINE_OBJ_QUERYASSOCIATIONS_H */

