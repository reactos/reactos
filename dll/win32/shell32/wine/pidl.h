/*
 * internal pidl functions
 *
 * Copyright 1998 Juergen Schmied
 * Copyright 2004 Juan Lang
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 * NOTES:
 *
 * DO NOT use these definitions outside the shell32.dll!
 *
 * The contents of a pidl should never be used directly from an application.
 *
 * Undocumented:
 * MS says: the abID of SHITEMID should be treated as binary data and not
 * be interpreted by applications. Applies to everyone but MS itself.
 * Word95 interprets the contents of abID (Filesize/Date) so we have to go
 * for binary compatibility here.
 */

#ifndef __WINE_PIDL_H
#define __WINE_PIDL_H

#ifdef __cplusplus
extern "C" {
#endif

/*
* the pidl does cache fileattributes to speed up SHGetAttributes when
* displaying a big number of files.
*
* a pidl of NULL means the desktop
*
* The structure of the pidl seems to be a union. The first byte of the
* PIDLDATA describes the type of pidl.
*
*	object        ! first byte /  ! format       ! living space
*	              ! size
*	----------------------------------------------------------------
*	my computer	0x1F/20		guid (2)	(usual)
*	network		0x1F		guid
*	bitbucket	0x1F		guid
*	drive		0x23/25		drive		(usual)
*	drive		0x25/25		drive		(lnk/persistent)
*	drive		0x29/25		drive
*	shell extension	0x2E		guid
*	drive		0x2F		drive		(lnk/persistent)
*	folder/file	0x30		folder/file (1)	(lnk/persistent)
*	folder		0x31		folder		(usual)
*	valueA		0x32		file		(ANSI file name) 
*	valueW		0x34		file		(Unicode file name)
*	workgroup	0x41		network (3)
*	computer	0x42		network (4)
*	net provider	0x46		network
*	whole network	0x47		network (5)
*	MSITStore	0x61		htmlhlp (7)
*	printers/ras connections 	0x70		guid
*	history/favorites 0xb1		file
*	share		0xc3		network (6)
*
* guess: the persistent elements are non tracking
*
* (1) dummy byte is used, attributes are empty
* (2) IID_MyComputer = 20D04FE0L-3AEA-1069-A2D8-08002B30309D
* (3) two strings	"workgroup" "Microsoft Network"
* (4) two strings	"\\sirius" "Microsoft Network"
* (5) one string	"Entire Network"
* (6) two strings	"\\sirius\c" "Microsoft Network"
* (7) contains string   "mk:@MSITStore:C:\path\file.chm::/path/filename.htm"
*		GUID	871C5380-42A0-1069-A2EA-08002B30309D
*/

#define PT_CPLAPPLET	0x00
#define PT_GUID		0x1F
#define PT_DRIVE	0x23
#define PT_DRIVE2	0x25
#define PT_DRIVE3	0x29
#define PT_SHELLEXT	0x2E
#define PT_DRIVE1	0x2F
#define PT_FOLDER1	0x30
#define PT_FOLDER	0x31
#define PT_VALUE	0x32
#define PT_VALUEW	0x34
#define PT_FOLDERW	0x35
#define PT_WORKGRP	0x41
#define PT_COMP		0x42
#define PT_NETPROVIDER	0x46
#define PT_NETWORK	0x47
#define PT_IESPECIAL1	0x61
#define PT_YAGUID	0x70 /* yet another guid.. */
#define PT_IESPECIAL2	0xb1
#define PT_SHARE	0xc3

#ifdef __REACTOS__
#define PT_FOLDERTYPEMASK       0x70
#define PT_DESKTOP_REGITEM      0x1F // => SHDID_ROOT_REGITEM
#define PT_COMPUTER_REGITEM     0x2E // => SHDID_COMPUTER_?
#define PT_FS                   0x30 // Win95 SHSimpleIDListFromPath
#define PT_FS_FOLDER_FLAG       0x01
#define PT_FS_FILE_FLAG         0x02
#define PT_FS_UNICODE_FLAG      0x04
//      PT_NET_REGITEM          0x4? // => SHDID_NET_OTHER
#define PT_CONTROLS_OLDREGITEM  0x70
#define PT_CONTROLS_NEWREGITEM  0x71
#endif

static inline BYTE _ILGetType(LPCITEMIDLIST pidl)
{
    return pidl && pidl->mkid.cb >= 3 ? pidl->mkid.abID[0] : 0;
}

static inline BYTE _ILGetFSType(LPCITEMIDLIST pidl)
{
    const BYTE type = _ILGetType(pidl);
    return (type & PT_FOLDERTYPEMASK) == PT_FS ? type : 0;
}

#include "pshpack1.h"
typedef BYTE PIDLTYPE;

typedef struct tagPIDLCPanelStruct
{ 
    BYTE dummy;			/*01 is 0x00 */
    DWORD iconIdx;		/*02 negative icon ID */
    WORD offsDispName;		/*06*/
    WORD offsComment;		/*08*/
    WCHAR szName[1];		/*10*/ /* terminated by 0x00, followed by display name and comment string */
} PIDLCPanelStruct;

#ifdef __REACTOS__

typedef struct tagPIDLFontStruct
{
    BYTE dummy;
    WORD offsFile;
    WCHAR szName[1];
} PIDLFontStruct;

typedef struct tagPIDLPrinterStruct
{
    BYTE dummy;
    DWORD Attributes;
    WORD offsServer;
    WCHAR szName[1];
} PIDLPrinterStruct;

#endif /* !__REACTOS__ */

typedef struct tagGUIDStruct
{
    BYTE dummy; /* offset 01 is unknown */
    GUID guid;  /* offset 02 */
} GUIDStruct;

typedef struct tagDriveStruct
{
    CHAR szDriveName[20];	/*01*/
    WORD unknown;		/*21*/
} DriveStruct;

typedef struct tagFileStruct
{
    BYTE dummy;			/*01 is 0x00 for files or dirs */
    DWORD dwFileSize;		/*02*/
    WORD uFileDate;		/*06*/
    WORD uFileTime;		/*08*/
    WORD uFileAttribs;		/*10*/
    CHAR szNames[1];		/*12*/
    /* Here are coming two strings. The first is the long name.
    The second the dos name when needed or just 0x00 */
} FileStruct;

/* At least on WinXP, this struct is appended with 2-byte-alignment to FileStruct. There follows 
 * a WORD member after the wszName string, which gives the offset from the beginning of the PIDL 
 * to the FileStructW member. */
typedef struct tagFileStructW {
    WORD cbLen;
    BYTE dummy1[6];
    WORD uCreationDate;
    WORD uCreationTime;
    WORD uLastAccessDate;
    WORD uLastAccessTime;
    BYTE dummy2[4];
    WCHAR wszName[1];
} FileStructW;

typedef struct tagValueW
{
    WCHAR name[1];
} ValueWStruct;

typedef struct tagPIDLDATA
{	PIDLTYPE type;			/*00*/
	union
	{
	  struct tagGUIDStruct guid;
	  struct tagDriveStruct drive;
	  struct tagFileStruct file;
	  struct
	  { WORD dummy;		/*01*/
	    CHAR szNames[1];	/*03*/
	  } network;
	  struct
	  { WORD dummy;		/*01*/
	    DWORD dummy1;	/*02*/
	    CHAR szName[1];	/*06*/ /* terminated by 0x00 0x00 */
	  } htmlhelp;
	  struct tagPIDLCPanelStruct cpanel;
          struct tagValueW valueW;
#ifdef __REACTOS__
          struct tagPIDLFontStruct cfont;
          struct tagPIDLPrinterStruct cprinter;
#endif
	}u;
} PIDLDATA, *LPPIDLDATA;
#include "poppack.h"

/*
 * getting special values from simple pidls
 */
DWORD   _ILSimpleGetTextW   (LPCITEMIDLIST pidl, LPWSTR pOut, UINT uOutSize) DECLSPEC_HIDDEN;
DWORD   _ILGetFileSize      (LPCITEMIDLIST pidl, LPWSTR pOut, UINT uOutSize) DECLSPEC_HIDDEN;
BOOL    _ILGetExtension     (LPCITEMIDLIST pidl, LPWSTR pOut, UINT uOutSize) DECLSPEC_HIDDEN;
DWORD   _ILGetFileAttributes(LPCITEMIDLIST pidl, LPWSTR pOut, UINT uOutSize) DECLSPEC_HIDDEN;
BOOL    _ILGetFileDateTime  (LPCITEMIDLIST pidl, FILETIME *ft) DECLSPEC_HIDDEN;
DWORD   _ILGetDrive         (LPCITEMIDLIST, LPWSTR, UINT) DECLSPEC_HIDDEN;

/*
 * testing simple pidls
 */
BOOL	_ILIsUnicode		(LPCITEMIDLIST pidl) DECLSPEC_HIDDEN;
BOOL	_ILIsDesktop		(LPCITEMIDLIST pidl) DECLSPEC_HIDDEN;
BOOL	_ILIsMyComputer		(LPCITEMIDLIST pidl) DECLSPEC_HIDDEN;
#ifdef __REACTOS__
BOOL	_ILIsMyDocuments	(LPCITEMIDLIST pidl);
BOOL	_ILIsBitBucket		(LPCITEMIDLIST pidl);
BOOL	_ILIsNetHood		(LPCITEMIDLIST pidl);
BOOL    _ILIsControlPanel   (LPCITEMIDLIST pidl);
#define _ILIsFolderOrFile   _ILGetFSType
#endif
BOOL	_ILIsDrive		(LPCITEMIDLIST pidl) DECLSPEC_HIDDEN;
BOOL	_ILIsFolder		(LPCITEMIDLIST pidl) DECLSPEC_HIDDEN;
BOOL	_ILIsValue		(LPCITEMIDLIST pidl) DECLSPEC_HIDDEN;
BOOL	_ILIsSpecialFolder	(LPCITEMIDLIST pidl) DECLSPEC_HIDDEN;
BOOL	_ILIsPidlSimple		(LPCITEMIDLIST pidl) DECLSPEC_HIDDEN;
BOOL	_ILIsCPanelStruct	(LPCITEMIDLIST pidl) DECLSPEC_HIDDEN;
static inline BOOL _ILIsEmpty(LPCITEMIDLIST pidl) { return _ILIsDesktop(pidl); }
UINT _ILGetDepth(LPCITEMIDLIST pidl);

/*
 * simple pidls
 */

/* Creates a PIDL with guid format and type type, which must be one of PT_GUID,
 * PT_SHELLEXT, or PT_YAGUID.
 */
LPITEMIDLIST	_ILCreateGuid(PIDLTYPE type, REFIID guid) DECLSPEC_HIDDEN;

#ifndef __REACTOS__
/* Like _ILCreateGuid, but using the string szGUID. */
LPITEMIDLIST	_ILCreateGuidFromStrA(LPCSTR szGUID) DECLSPEC_HIDDEN;
LPITEMIDLIST	_ILCreateGuidFromStrW(LPCWSTR szGUID) DECLSPEC_HIDDEN;
#endif

/* Commonly used PIDLs representing file system objects. */
LPITEMIDLIST	_ILCreateDesktop	(void) DECLSPEC_HIDDEN;
LPITEMIDLIST	_ILCreateFromFindDataW(const WIN32_FIND_DATAW *stffile) DECLSPEC_HIDDEN;
HRESULT		_ILCreateFromPathW	(LPCWSTR szPath, LPITEMIDLIST* ppidl) DECLSPEC_HIDDEN;
LPITEMIDLIST SHELL32_CreateSimpleIDListFromPath(LPCWSTR pszPath, DWORD dwAttributes);

/* Other helpers */
LPITEMIDLIST	_ILCreateMyComputer	(void) DECLSPEC_HIDDEN;
LPITEMIDLIST	_ILCreateMyDocuments	(void) DECLSPEC_HIDDEN;
LPITEMIDLIST	_ILCreateIExplore	(void) DECLSPEC_HIDDEN;
LPITEMIDLIST	_ILCreateControlPanel	(void) DECLSPEC_HIDDEN;
LPITEMIDLIST	_ILCreatePrinters	(void) DECLSPEC_HIDDEN;
LPITEMIDLIST	_ILCreateNetwork	(void) DECLSPEC_HIDDEN;
LPITEMIDLIST	_ILCreateNetHood	(void) DECLSPEC_HIDDEN;
#ifdef __REACTOS__
LPITEMIDLIST	_ILCreateAdminTools	(void);
#endif
LPITEMIDLIST	_ILCreateBitBucket	(void) DECLSPEC_HIDDEN;
LPITEMIDLIST	_ILCreateDrive		(LPCWSTR) DECLSPEC_HIDDEN;
LPITEMIDLIST    _ILCreateEntireNetwork  (void) DECLSPEC_HIDDEN;

/*
 * helper functions (getting struct-pointer)
 */
LPPIDLDATA	_ILGetDataPointer	(LPCITEMIDLIST) DECLSPEC_HIDDEN;
LPSTR		_ILGetTextPointer	(LPCITEMIDLIST) DECLSPEC_HIDDEN;
IID		*_ILGetGUIDPointer	(LPCITEMIDLIST pidl) DECLSPEC_HIDDEN;
FileStructW     *_ILGetFileStructW      (LPCITEMIDLIST pidl) DECLSPEC_HIDDEN;

/*
 * debug helper
 */
void	pdump	(LPCITEMIDLIST pidl) DECLSPEC_HIDDEN;
BOOL	pcheck	(LPCITEMIDLIST pidl) DECLSPEC_HIDDEN;

/*
 * aPidl helper
 */
void _ILFreeaPidl(LPITEMIDLIST * apidl, UINT cidl) DECLSPEC_HIDDEN;
PITEMID_CHILD* _ILCopyaPidl(PCUITEMID_CHILD_ARRAY apidlsrc, UINT cidl) DECLSPEC_HIDDEN;
LPITEMIDLIST * _ILCopyCidaToaPidl(LPITEMIDLIST* pidl, const CIDA * cida) DECLSPEC_HIDDEN;

BOOL ILGetDisplayNameExW(LPSHELLFOLDER psf, LPCITEMIDLIST pidl, LPWSTR path, DWORD type) DECLSPEC_HIDDEN;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __WINE_PIDL_H */
