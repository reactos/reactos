#ifndef _SHTYPES_H
#define _SHTYPES_H
#if __GNUC__ >= 3
#pragma GCC system_header
#endif

#ifdef __cplusplus
extern "C" {
#endif
#pragma pack(push,1)

typedef struct _SHITEMID {
	USHORT	cb;
	BYTE	abID[1];
} SHITEMID, * LPSHITEMID;
typedef const SHITEMID *LPCSHITEMID;
typedef struct _ITEMIDLIST {
	SHITEMID mkid;
} ITEMIDLIST,*LPITEMIDLIST;
typedef const ITEMIDLIST *LPCITEMIDLIST;
typedef struct _STRRET {
	UINT uType;
	_ANONYMOUS_UNION union {
		LPWSTR pOleStr;
		UINT uOffset;
		char cStr[MAX_PATH];
	} DUMMYUNIONNAME;
} STRRET,*LPSTRRET;
typedef struct _SHELLDETAILS
{
	int fmt;
	int cxChar;
	STRRET str;
} SHELLDETAILS, *LPSHELLDETAILS;

#pragma pack(pop)
#ifdef __cplusplus
}
#endif


#endif /* _SHLOBJ_H */
