#ifndef __WIN32K_METAFILE_H
#define __WIN32K_METAFILE_H

HENHMETAFILE
STDCALL
W32kCloseEnhMetaFile (
	HDC	hDC
	);
HMETAFILE
STDCALL
W32kCloseMetaFile (
	HDC	hDC
	);
HENHMETAFILE
STDCALL
W32kCopyEnhMetaFile (
	HENHMETAFILE	Src,
	LPCWSTR		File
	);
HMETAFILE
STDCALL
W32kCopyMetaFile (
	HMETAFILE	Src,
	LPCWSTR		File
	);
HDC
STDCALL
W32kCreateEnhMetaFile (
	HDC		hDCRef,
	LPCWSTR		File,
	CONST LPRECT	Rect,
	LPCWSTR		Description
	);
HDC
STDCALL
W32kCreateMetaFile (
	LPCWSTR		File
	);
BOOL
STDCALL
W32kDeleteEnhMetaFile (
	HENHMETAFILE	emf
	);
BOOL
STDCALL
W32kDeleteMetaFile (
	HMETAFILE	mf
	);
BOOL
STDCALL
W32kEnumEnhMetaFile (
	HDC		hDC,
	HENHMETAFILE	emf,
	ENHMFENUMPROC	EnhMetaFunc,
	LPVOID		Data,
	CONST LPRECT	Rect
	);
BOOL
STDCALL
W32kEnumMetaFile (
	HDC		hDC,
	HMETAFILE	mf,
	MFENUMPROC	MetaFunc,
	LPARAM		lParam
	);
BOOL
STDCALL
W32kGdiComment (
	HDC		hDC,
	UINT		Size,
	CONST LPBYTE	Data
	);
HENHMETAFILE
STDCALL
W32kGetEnhMetaFile (
	LPCWSTR	MetaFile
	);
UINT
STDCALL
W32kGetEnhMetaFileBits (
	HENHMETAFILE	hemf,
	UINT		BufSize,
	LPBYTE		Buffer
	);
UINT
STDCALL
W32kGetEnhMetaFileDescription (
	HENHMETAFILE	hemf,
	UINT		BufSize,
	LPWSTR		Description
	);
UINT
STDCALL
W32kGetEnhMetaFileHeader (
	HENHMETAFILE	hemf,
	UINT		BufSize,
	LPENHMETAHEADER	emh
	);
UINT
STDCALL
W32kGetEnhMetaFilePaletteEntries (
	HENHMETAFILE	hemf,
	UINT		Entries,
	LPPALETTEENTRY	pe
	);
HMETAFILE
STDCALL
W32kGetMetaFile (
	LPCWSTR	MetaFile
	);
UINT
STDCALL
W32kGetMetaFileBitsEx (
	HMETAFILE	hmf,
	UINT		Size,
	LPVOID		Data
	);
UINT
STDCALL
W32kGetWinMetaFileBits (
	HENHMETAFILE	hemf,
	UINT		BufSize,
	LPBYTE		Buffer,
	INT		MapMode,
	HDC		Ref
	);
BOOL
STDCALL
W32kPlayEnhMetaFile (
	HDC		hDC,
	HENHMETAFILE	hemf,
	CONST PRECT	Rect
	);
BOOL
STDCALL
W32kPlayEnhMetaFileRecord (
	HDC			hDC,
	LPHANDLETABLE		Handletable,
	CONST ENHMETARECORD	* EnhMetaRecord,
	UINT			Handles
	);
BOOL
STDCALL
W32kPlayMetaFile (
	HDC		hDC,
	HMETAFILE	hmf
	);
BOOL
STDCALL
W32kPlayMetaFileRecord (
	HDC		hDC,
	LPHANDLETABLE	Handletable,
	LPMETARECORD	MetaRecord,
	UINT		Handles
	);
HENHMETAFILE
STDCALL
W32kSetEnhMetaFileBits (
	UINT		BufSize,
	CONST PBYTE	Data
	);
HMETAFILE
STDCALL
W32kSetMetaFileBitsEx (
	UINT		Size,
	CONST PBYTE	Data
	);
#if 0
HENHMETAFILE
STDCALL
W32kSetWinMetaFileBits (
	UINT			BufSize,
	CONST PBYTE		Buffer,
	HDC			Ref,
	CONST METAFILEPICT	* mfp
	);
#endif

#endif

