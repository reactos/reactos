/*****************************************************************************\
*                                                                             *
* wfext.h -     Windows File Manager Extensions definitions		      *
*                                                                             *
* Version 3.10								      *
*                                                                             *
* Copyright (c) 1991-1994, Microsoft Corp. All rights reserved. 	      *
*                                                                             *
*******************************************************************************/

#ifndef _INC_WFEXT
#define _INC_WFEXT    /* #defined if wfext.h has been included */

#ifndef RC_INVOKED
#pragma pack(1)         /* Assume byte packing throughout */
#endif  /* RC_INVOKED */

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif	/* __cplusplus */

#define MENU_TEXT_LEN		40

#define FMMENU_FIRST		1
#define FMMENU_LAST		99

#define FMEVENT_LOAD		100
#define FMEVENT_UNLOAD		101
#define FMEVENT_INITMENU	102
#define FMEVENT_USER_REFRESH	103
#define FMEVENT_SELCHANGE	104
#define FMEVENT_TOOLBARLOAD	105
#define FMEVENT_HELPSTRING	106

#define FMFOCUS_DIR		1
#define FMFOCUS_TREE		2
#define FMFOCUS_DRIVES		3
#define FMFOCUS_SEARCH		4

#define FM_GETFOCUS		(WM_USER + 0x0200)
#define FM_GETDRIVEINFO		(WM_USER + 0x0201)
#define FM_GETSELCOUNT		(WM_USER + 0x0202)
#define FM_GETSELCOUNTLFN	(WM_USER + 0x0203)	/* LFN versions are odd */
#define FM_GETFILESEL		(WM_USER + 0x0204)
#define FM_GETFILESELLFN	(WM_USER + 0x0205)	/* LFN versions are odd */
#define FM_REFRESH_WINDOWS	(WM_USER + 0x0206)
#define FM_RELOAD_EXTENSIONS	(WM_USER + 0x0207)

typedef struct tagFMS_GETFILESEL
{
        UINT wTime;
        UINT wDate;
	DWORD dwSize;
	BYTE bAttr;
        char szName[260];               /* always fully qualified */
} FMS_GETFILESEL, FAR *LPFMS_GETFILESEL;

typedef struct tagFMS_GETDRIVEINFO       /* for drive */
{
	DWORD dwTotalSpace;
	DWORD dwFreeSpace;
	char szPath[260];		/* current directory */
	char szVolume[14];		/* volume label */
	char szShare[128];		/* if this is a net drive */
} FMS_GETDRIVEINFO, FAR *LPFMS_GETDRIVEINFO;

typedef struct tagFMS_LOAD
{
	DWORD dwSize;				/* for version checks */
	char  szMenuName[MENU_TEXT_LEN];	/* output */
	HMENU hMenu;				/* output */
        UINT  wMenuDelta;                       /* input */
} FMS_LOAD, FAR *LPFMS_LOAD;

typedef struct tagEXT_BUTTON
{
	WORD idCommand;			/* menu command to trigger */
	WORD idsHelp;			/* help string ID */
	WORD fsStyle;			/* button style */
} EXT_BUTTON, FAR *LPEXT_BUTTON;

#define FMTB_BUTTON	0x00
#define FMTB_SEP	0x01

typedef struct tagFMS_TOOLBARLOAD
{
	DWORD dwSize;			/* for version checks */
	LPEXT_BUTTON lpButtons;		/* output */
	WORD cButtons;			/* output, 0==>no buttons */
	WORD cBitmaps;			/* number of non-sep buttons */
	WORD idBitmap;			/* output */
	HBITMAP hBitmap;		/* output if idBitmap==0 */
} FMS_TOOLBARLOAD, FAR *LPFMS_TOOLBARLOAD;

typedef struct tagFMS_HELPSTRING
{
	int idCommand;			/* input, -1==>the menu was selected */
	HMENU hMenu;			/* input, the extensions menu */
	char szHelp[128];		/* output, the help string */
} FMS_HELPSTRING, FAR *LPFMS_HELPSTRING;

typedef DWORD (CALLBACK *FM_EXT_PROC)(HWND, UINT, LONG);
typedef DWORD (CALLBACK *FM_UNDELETE_PROC)(HWND, LPSTR);

#ifndef RC_INVOKED
#pragma pack()          /* Revert to default packing */
#endif  /* RC_INVOKED */

#ifdef __cplusplus
}                       /* End of extern "C" { */
#endif	/* __cplusplus */

#endif  /* _INC_WFEXT */
	
