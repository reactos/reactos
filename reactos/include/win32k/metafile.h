#ifndef __WIN32K_METAFILE_H
#define __WIN32K_METAFILE_H

HENHMETAFILE
STDCALL
NtGdiCloseEnhMetaFile (
	HDC	hDC
	);
HMETAFILE
STDCALL
NtGdiCloseMetaFile (
	HDC	hDC
	);
HENHMETAFILE
STDCALL
NtGdiCopyEnhMetaFile (
	HENHMETAFILE	Src,
	LPCWSTR		File
	);
HMETAFILE
STDCALL
NtGdiCopyMetaFile (
	HMETAFILE	Src,
	LPCWSTR		File
	);
HDC
STDCALL
NtGdiCreateEnhMetaFile (
	HDC		hDCRef,
	LPCWSTR		File,
	CONST LPRECT	Rect,
	LPCWSTR		Description
	);
HDC
STDCALL
NtGdiCreateMetaFile (
	LPCWSTR		File
	);
BOOL
STDCALL
NtGdiDeleteEnhMetaFile (
	HENHMETAFILE	emf
	);
BOOL
STDCALL
NtGdiDeleteMetaFile (
	HMETAFILE	mf
	);
BOOL
STDCALL
NtGdiEnumEnhMetaFile (
	HDC		hDC,
	HENHMETAFILE	emf,
	ENHMFENUMPROC	EnhMetaFunc,
	LPVOID		Data,
	CONST LPRECT	Rect
	);
BOOL
STDCALL
NtGdiEnumMetaFile (
	HDC		hDC,
	HMETAFILE	mf,
	MFENUMPROC	MetaFunc,
	LPARAM		lParam
	);
BOOL
STDCALL
NtGdiGdiComment (
	HDC		hDC,
	UINT		Size,
	CONST LPBYTE	Data
	);
HENHMETAFILE
STDCALL
NtGdiGetEnhMetaFile (
	LPCWSTR	MetaFile
	);
UINT
STDCALL
NtGdiGetEnhMetaFileBits (
	HENHMETAFILE	hemf,
	UINT		BufSize,
	LPBYTE		Buffer
	);
UINT
STDCALL
NtGdiGetEnhMetaFileDescription (
	HENHMETAFILE	hemf,
	UINT		BufSize,
	LPWSTR		Description
	);
UINT
STDCALL
NtGdiGetEnhMetaFileHeader (
	HENHMETAFILE	hemf,
	UINT		BufSize,
	LPENHMETAHEADER	emh
	);
UINT
STDCALL
NtGdiGetEnhMetaFilePaletteEntries (
	HENHMETAFILE	hemf,
	UINT		Entries,
	LPPALETTEENTRY	pe
	);
HMETAFILE
STDCALL
NtGdiGetMetaFile (
	LPCWSTR	MetaFile
	);
UINT
STDCALL
NtGdiGetMetaFileBitsEx (
	HMETAFILE	hmf,
	UINT		Size,
	LPVOID		Data
	);
UINT
STDCALL
NtGdiGetWinMetaFileBits (
	HENHMETAFILE	hemf,
	UINT		BufSize,
	LPBYTE		Buffer,
	INT		MapMode,
	HDC		Ref
	);
BOOL
STDCALL
NtGdiPlayEnhMetaFile (
	HDC		hDC,
	HENHMETAFILE	hemf,
	CONST PRECT	Rect
	);
BOOL
STDCALL
NtGdiPlayEnhMetaFileRecord (
	HDC			hDC,
	LPHANDLETABLE		Handletable,
	CONST ENHMETARECORD	* EnhMetaRecord,
	UINT			Handles
	);
BOOL
STDCALL
NtGdiPlayMetaFile (
	HDC		hDC,
	HMETAFILE	hmf
	);
BOOL
STDCALL
NtGdiPlayMetaFileRecord (
	HDC		hDC,
	LPHANDLETABLE	Handletable,
	LPMETARECORD	MetaRecord,
	UINT		Handles
	);
HENHMETAFILE
STDCALL
NtGdiSetEnhMetaFileBits (
	UINT		BufSize,
	CONST PBYTE	Data
	);
HMETAFILE
STDCALL
NtGdiSetMetaFileBitsEx (
	UINT		Size,
	CONST PBYTE	Data
	);
#if 0
HENHMETAFILE
STDCALL
NtGdiSetWinMetaFileBits (
	UINT			BufSize,
	CONST PBYTE		Buffer,
	HDC			Ref,
	CONST METAFILEPICT	* mfp
	);
#endif

#endif

