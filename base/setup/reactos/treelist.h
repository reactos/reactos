/*
 * PROJECT:     ReactOS GUI first stage setup application
 * LICENSE:     GPL-3.0-or-later (https://spdx.org/licenses/GPL-3.0-or-later)
 * PURPOSE:     Implements a TreeList control: a tree window with columns.
 * COPYRIGHT:   Copyright (C) Anton Zechner (az_software@inode.at) 2007
 *              Copyright (C) Sébastien Kirche (sebastien.kirche@free.fr) 2014
 *
 * NOTE: Taken from the TreeList code found at https://github.com/sebkirche/treelist
 */

//*****************************************************************************
//*
//*
//*		TreeListWnd.h
//*
//*
//*****************************************************************************
#ifndef 	__TREELISTWND_H__
#define 	__TREELISTWND_H__
#if _MSC_VER > 1000
#pragma once
#endif

#include	<commctrl.h>

/* Window Messaging */
#ifndef SNDMSG
#ifdef __cplusplus
#define SNDMSG ::SendMessage
#else
#define SNDMSG SendMessage
#endif
#endif // ifndef SNDMSG


typedef int (CALLBACK *PFNTVCOMPAREEX)(HWND hWnd,HTREEITEM hItem1,HTREEITEM hItem2,LPARAM lParam1,LPARAM lParam2,LPARAM lParam);
typedef int (CALLBACK *PFNTVSORTEX   )(HWND hWnd,HTREEITEM hItem ,LPCTSTR pTextItem,LPCTSTR pTextInsert,LPARAM lParamItem,LPARAM lParamInsert);

typedef struct _TVSORTEX {
    HTREEITEM       hParent;
    PFNTVCOMPAREEX  lpfnCompare;
    LPARAM          lParam;
} TVSORTEX, *LPTVSORTEX;

typedef struct _TVFIND {
	UINT			uFlags;
	UINT			uColumn;
	UINT			uState;
	UINT			uStateMask;
	LPARAM			lParam;
	LPCTSTR			pText;
} TVFIND, *LPTVFIND;

typedef struct _TV_KEYDOWN_EX {
    NMHDR			hdr;
    WORD			wVKey;
	WORD			wScan;
    UINT			flags;
} TV_KEYDOWN_EX, *LPTV_KEYDOWN_EX;

typedef struct _TV_STARTEDIT {
    NMHDR			hdr;
    TVITEM			item;
	UINT			uAction;
	UINT			uHeight;
	UINT			uMaxEntries;
	LPCTSTR			pTextEntries;
	LPCTSTR		   *pTextList;
	POINT			ptAction;
} TV_STARTEDIT, *LPTV_STARTEDIT;

typedef struct _TV_COLSIZE {
    NMHDR			hdr;
    UINT			uColumn;
	UINT			uIndex;
	UINT			uPosX;
	INT				iSize;
} TV_COLSIZE, *LPTV_COLSIZE;

typedef		TVSORTEX			   *LPTVSORTEX;
typedef		TVSORTEX			   *LPTV_SORTEX;
typedef		TVSORTEX				TV_SORTEX;


#define 	TVCOLUMN				LV_COLUMN
#define 	TV_COLUMN				LV_COLUMN
#define 	TV_FIND					TVFIND
#define 	TV_NOIMAGE				(-2)
#define 	TV_NOCOLOR				0xFFFFFFFF
#define 	TV_NOAUTOEXPAND			0x20000000
#define 	TV_SECONDICON			0x40000000
#define 	TVLE_DONOTIFY			0xF5A5A500
#define 	TVIF_TEXTPTR			0x80000000
#define 	TVIF_TOOLTIPTIME		0x40000000
#define		TVIF_TEXTCHANGED		0x20000000
#define		TVIF_RETURNEXIT			0x10000000
#define 	TVIF_CASE				0x08000000
#define 	TVIF_NEXT				0x04000000
#define 	TVIF_CHILD				0x02000000
#define 	TVIF_CANCELED			0x01000000
#define 	TVIF_ONLYFOCUS			0x00800000
#define		TVIF_SUBITEM	        0x8000
#define 	TVIF_SUBNUMBER			0x4000
#define 	TVIS_UNDERLINE			0x0001
#define 	TVSIL_CHECK				0x0003
#define 	TVSIL_SUBIMAGES			0x0004
#define		TVSIL_HEADER			0x0005
#define		TVN_COLUMNCLICK			HDN_ITEMCLICK
#define 	TVN_COLUMNDBLCLICK		HDN_ITEMDBLCLICK
#define 	TVE_EXPANDRECURSIVE		(0x80000000)
#define 	TVE_EXPANDFORCE			(0x40000000)
#define 	TVE_EXPANDNEXT			(0x20000000)
#define 	TVE_ALLCHILDS			(0x10000000)
#define 	TVE_ONLYCHILDS			(0x00000008)
#define 	TVI_SORTEX				((HTREEITEM)(ULONG_PTR)0xFFFF0007)
#define 	TVI_BEFORE				((HTREEITEM)(ULONG_PTR)0xFFFF0006)
#define 	TVI_AFTER				((HTREEITEM)(ULONG_PTR)0xFFFF0005)
#define		TVI_ROW(n)		        ((HTREEITEM)(ULONG_PTR)(0xFFE00000+(n)))
#ifndef		VK_DBLCLK
#define 	VK_DBLCLK				0x10000				// Edit with doubleclick
#endif
#ifndef		VK_ICONCLK
#define 	VK_ICONCLK				0x10001				// Edit with click on icon
#endif
#ifndef		VK_EDITCLK
#define 	VK_EDITCLK				0x10002				// Edit with click on augewähltes Element //visble element ?
#endif
#ifdef		UNICODE
#define		TV_UINICODE				1
#else		
#define		TV_UINICODE				0
#endif		

#define TVC_CLASSNAME "SysTreeList32"

// Color Constants (TVM_SETBKCOLOR)
#define 	TVC_BK					0					// Background
#define 	TVC_ODD					1					// alternate colors / odd    (see TVS_EX_ALTERNATECOLOR)
#define 	TVC_EVEN				2					// alternate colors / even	(see TVS_EX_ALTERNATECOLOR)
#define 	TVC_FRAME				3					// separator lines	(see TVS_EX_ITEMLINES)
#define 	TVC_TEXT				4					// text
#define 	TVC_LINE				5					// interior of the buttons
#define 	TVC_BOX					6					// exterior of the buttons
#define 	TVC_TRACK				7					// tracked item text
#define 	TVC_MARK				8					// selected line
#define 	TVC_MARKODD				8					// selected line (odd)
#define 	TVC_MARKEVEN			9					// selected line (even)
#define 	TVC_INSERT				10					// insertion point
#define 	TVC_BOXBG				11					// background of buttons 
#define 	TVC_COLBK				12					// background of marked column
#define 	TVC_COLODD				13					// alternate odd color of marked column
#define 	TVC_COLEVEN				14					// alternate even color of marked column
#define 	TVC_GRAYED				15					// background when disabled


// constants for GetNextItem (TVM_GETNEXTITEM)
#define 	TVGN_DROPHILITESUB		0x000C				// get selected column
#define 	TVGN_CARETSUB			0x000D				// Drophighilite Spalte holen
#ifndef		TVGN_NEXTSELECTED
#define 	TVGN_NEXTSELECTED		0x000E				// next selected entry
#endif
#define 	TVGN_FOCUS				0x000F				// entry that has focus
#define 	TVGN_FOCUSSUB			0x0010				// column that has focus
#define 	TVGN_NEXTSELCHILD		0x0011				// next selected child entry
#define 	TVGN_LASTCHILD			0x0012				// last child entry
#define 	TVGN_NEXTITEM			0x0013				// to enumerate the entries


// constants for InsertColumn (mask)
#define 	TVCF_FMT				LVCF_FMT			// set the text alignment
#define 	TVCF_IMAGE				LVCF_IMAGE			// set column image
#define 	TVCF_TEXT				LVCF_TEXT			// set column text
#define 	TVCF_WIDTH				LVCF_WIDTH			// set fixed width
#define 	TVCF_VWIDTH				LVCF_SUBITEM		// set variable width
#define 	TVCF_MIN				LVCF_ORDER			// set minimum width
#define 	TVCF_MARK				0x80000000			// to mark a column
#define 	TVCF_FIXED				0x40000000			// can the column width can be changed 
#define 	TVCF_LASTSIZE			0x44332211			// Breite vor dem Fixieren wieder herstellen


// constants for InsertColumn (format mask)
#define 	TVCFMT_BITMAP_ON_RIGHT	LVCFMT_BITMAP_ON_RIGHT
#define 	TVCFMT_COL_HAS_IMAGES	LVCFMT_COL_HAS_IMAGES
#define 	TVCFMT_CENTER			LVCFMT_CENTER
#define 	TVCFMT_IMAGE			LVCFMT_IMAGE
#define 	TVCFMT_LEFT				LVCFMT_LEFT
#define 	TVCFMT_RIGHT			LVCFMT_RIGHT
#define 	TVCFMT_FIXED			0x20000000			// flag for fixing the column
#define 	TVCFMT_MARK				0x10000000			// flag for marking the column


// constants for Column AutoEdit
#define 	TVAE_NONE				(0<<TVAE_MODEPOS)	// no automatic edit
#define 	TVAE_EDIT				(1<<TVAE_MODEPOS)	// automatic edit with edit box
#define 	TVAE_COMBO				(2<<TVAE_MODEPOS)	// automatic edit with ComboBox
#define 	TVAE_CBLIST				(3<<TVAE_MODEPOS)	// automatic edit with ComboListBox
#define 	TVAE_STEP				(4<<TVAE_MODEPOS)	// advance with enter
#define 	TVAE_STEPED				(5<<TVAE_MODEPOS)	// advance with enter
#define 	TVAE_CHECK				(6<<TVAE_MODEPOS)	// automatic edit with CheckBox
#define 	TVAE_CHECKED			(7<<TVAE_MODEPOS)	// automatic edit with CheckBox and Edit
#define 	TVAE_NEXTLINE			0x0001				// go to next line after edit
#define 	TVAE_DBLCLICK			0x0002				// edit with doubleclick
#define 	TVAE_FULLWIDTH			0x0004				// show combobox over the full width
#define 	TVAE_PTRLIST			0x0008				// data list is passed as a pointer list
#define 	TVAE_ONLYRETURN			0x0010				// edition can only be started with Return
#define 	TVAE_STATEENABLE		0x0020				// edition can be (un)locked with the TVIS_DISABLEBIT flag
#define 	TVAE_ICONCLICK			0x0040				// edition starts by clicking on the icon
#define 	TVAE_DROPDOWN			0x0080				// open the DropDownList when edition starts
#define 	TVAE_COL(c)				(((c)&0x3F)<<11)	// column for automatic edit
#define 	TVAE_CHAR(c)			(((c)&0xFF)<<17)	// delimiter for the data list
#define 	TVAE_COUNT(c)			(((c)&0x7F)<<25)	// number of entries in the data list (0=auto)
#define 	TVAE_MODEMASK			(7<<TVAE_MODEPOS)	// Mask for mode bits
#define 	TVAE_MODEPOS			0x0008				// Mask for mode bits
#define 	TVIS_DISABLEBIT			0x8000				// flag for locking auto edit


// constants for HitTest (flags)
#define 	TVHT_SUBTOCOL(u)		(((unsigned)u)>>24)	// Convert column to column numbers mask
#define 	TVHT_SUBMASK			0xFF000000			// Maske in der die Spalte gespeichert wird
#define 	TVHT_ONRIGHTSPACE		0x00800000			// Auf rechtem Rand nach den Exträgen
#define 	TVHT_ONSUBLABEL			0x00400000			// Koordinate ist auf dem Text eines Extraeintrages
#define 	TVHT_ONSUBICON			0x00200000			// Koordinate ist auf einem Extraeintrag
#define 	TVHT_ONSUBRIGHT			0x00100000			// Koordinate ist auf einem Extraeintrag rechts vom Text
#define 	TVHT_ONSUBITEM			(TVHT_ONSUBICON|TVHT_ONSUBLABEL)


// constants for GetItemRect (TVM_GETITEMRECT)
#define 	TVIR_COLTOSUB(u)		((u)<<24)			// specify column
#define 	TVIR_GETCOLUMN			0x00000080			// get only column
#define 	TVIR_TEXT				0x00000001			// get only text area


// constants for SelectChilds (TVM_SELECTCHILDS)
#define 	TVIS_WITHCHILDS			0x00000001			// get also childs
#define 	TVIS_DESELECT			0x00000002			// deselect all items

// constants for Options (TVM_GETSETOPTION)
#define 	TVOP_AUTOEXPANDOFF		0x00000001			// Icon Offset for TVS_EX_AUTOEXPANDICON
#define 	TVOP_WRITEOPTION		0x80000000			// write too


// constants for EditLabel (LVM_EDITLABEL)
#define 	TVIR_EDITCOMBOCHAR(n)	(((n)&0xFF)<<8)		// Separator of the combobox entries (only for Notify message)
#define 	TVIR_EDITCOMBODEL		0x00000080			// Clears the buffer for the entries (only for Notify message)
#define 	TVIR_EDITCOMBODOWN		0x10000000			// open the combobox (only for Notify message)
#define 	TVIR_EDITCOMBOBOX		0x20000000			// instead of an edit, display a combobox
#define 	TVIR_EDITCOMBOLIST		0x40000000			// instead of an edit, display a combobox with list selection
#define 	TVIR_EDITFULL			0x80000000			// the edit window takes the full width
#define 	TVIR_EDITTEXT			0x00000001			// show the edit window over the text (only for Notify message)
#define 	TVIR_EDITCOL(u)			((u)&0xFF)			// auto edit column
#define 	TVIR_SELALL				0x00000000			// select all
#ifndef __REACTOS__
#define 	TVIR_SELAREA(a,b)		((0x0C0000|(a&0x1FF)|((b&0x1FF)<<9))<<8)	// select text area
#define 	TVIR_SETCURSOR(a)		((0x080000|(a&0x3FFFF))<<8)					// Cursor auf Textstelle
#define 	TVIR_SETAT(a)			((0x040000|(a&0x3FFFF))<<8)					// Cursor auf Pixel-Offset
#else // __REACTOS__
#define 	TVIR_SELAREA(a,b)		((0x0C0000|((a)&0x1FF)|(((b)&0x1FF)<<9))<<8)	// select text area
#define 	TVIR_SETCURSOR(a)		((0x080000|((a)&0x3FFFF))<<8)					// Cursor auf Textstelle
#define 	TVIR_SETAT(a)			((0x040000|((a)&0x3FFFF))<<8)					// Cursor auf Pixel-Offset
#endif // __REACTOS__

// constants for uStyleEx
#define		TVS_EX_HEADEROWNIMGLIST 0x00000400			// the header has its own TVSIL_HEADER image list (else it is shared with TVSIL_NORMAL)
#define 	TVS_EX_HEADERCHGNOTIFY	0x00000800			// notify when a column has been resized
#define 	TVS_EX_HEADERDRAGDROP	0x00001000			// columns over 1 can be sorted using drag and drop
#define 	TVS_EX_SINGLECHECKBOX	0x00002000			// checkboxes with single selection
#define 	TVS_EX_STEPOUT			0x00004000			// can leave an edit control with the cursor buttons
#define 	TVS_EX_BITCHECKBOX		0x00008000			// change only the first bit of the state
#define 	TVS_EX_ITEMLINES		0x00010000			// draw the item separating lines
#define 	TVS_EX_ALTERNATECOLOR	0x00020000			// use alternate lines background color 
#define 	TVS_EX_SUBSELECT		0x00040000			// allow to select columns
#define 	TVS_EX_FULLROWMARK		0x00080000			// the row mark fill the entire row
#define 	TVS_EX_TOOLTIPNOTIFY	0x00100000			// send a TVN_ITEMTOOLTIP notify to query for tooltip
#define 	TVS_EX_AUTOEXPANDICON	0x00200000			// use automaticaly the next icon for expanded items
#define 	TVS_EX_NOCHARSELCET		0x00400000			// disable looping selection of items via keyboard 
#define 	TVS_EX_NOCOLUMNRESIZE	0x00800000			// user cannot change the columns width
#define 	TVS_EX_HIDEHEADERS		0x01000000			// hide the header
#define 	TVS_EX_GRAYEDDISABLE	0x02000000			// gray out the control when disabled
#define 	TVS_EX_FULLROWITEMS		0x04000000			// backgrounds and lines takes the whole line (use with TVS_EX_ALTERNATECOLOR and TVS_EX_ITEMLINES)
#define 	TVS_EX_FIXEDCOLSIZE		0x08000000			// the width of the whole columns stays constant when resizing columns (the right margin of last column do not change)
#define 	TVS_EX_HOMEENDSELECT	0x10000000			// move to first / last item with Home / End keys
#define 	TVS_EX_SHAREIMAGELISTS	0x20000000			// image list is not deleted when the control is destroyed
#define 	TVS_EX_EDITCLICK		0x40000000			// enter edit mode with a single click
#define 	TVS_EX_NOCURSORSET		0x80000000			// VK_EDITCLK always select the entire text. Don't set the cursor to the click point, if TVS_EX_EDITCLICK is used
#ifndef		TVS_EX_MULTISELECT
#define 	TVS_EX_MULTISELECT		0x00000002			// allow multiple selections
#endif
#ifndef		TVS_EX_AUTOHSCROLL
#define		TVS_EX_AUTOHSCROLL      0x00000020			// auto scroll to selected item (horizontal scrollbar stays hidden)
#endif


// constants for notify messages
#define		TVN_ITEMTOOLTIP			(TVN_FIRST-32)		// tooltip query
#define     TVN_CBSTATECHANGED      (TVN_FIRST-33)		// checkbox change
#define		TVN_STEPSTATECHANGED	(TVN_FIRST-34)		// autoedit state changed
#define		TVN_STARTEDIT			(TVN_FIRST-35)		// field edit
#define 	TVN_LBUTTONUP			(TVN_FIRST-36)		// left button released
#define 	TVN_RBUTTONUP			(TVN_FIRST-37)		// right button released
#define 	TVN_COLUMNCHANGED		(TVN_FIRST-38)		// a column modified


// constants for new messages
#define 	TVM_GETHEADER			(TV_FIRST+128-1)
#define 	TVM_GETCOLUMNCOUNT		(TV_FIRST+128-2)
#define 	TVM_DELETECOLUMN		(TV_FIRST+128-3)
#define 	TVM_INSERTCOLUMN		(TV_FIRST+128-4-TV_UINICODE)
#define 	TVM_SELECTSUBITEM		(TV_FIRST+128-6)
#define 	TVM_SELECTDROP			(TV_FIRST+128-7)
#define 	TVM_SETITEMBKCOLOR		(TV_FIRST+128-8)
#define 	TVM_GETITEMBKCOLOR		(TV_FIRST+128-9)
#define 	TVM_SETITEMTEXTCOLOR	(TV_FIRST+128-10)
#define 	TVM_GETITEMTEXTCOLOR	(TV_FIRST+128-11)
#define 	TVM_GETITEMOFROW		(TV_FIRST+128-12)
#define 	TVM_GETROWCOUNT			(TV_FIRST+128-13)
#define 	TVM_GETROWOFITEM		(TV_FIRST+128-14)
#define 	TVM_SETCOLUMN			(TV_FIRST+128-15-TV_UINICODE)
#define 	TVM_GETCOLUMN			(TV_FIRST+128-17-TV_UINICODE)
#define 	TVM_SETCOLUMNWIDTH		(TV_FIRST+128-19)
#define 	TVM_GETCOLUMNWIDTH		(TV_FIRST+128-20)
#define 	TVM_SETUSERDATASIZE		(TV_FIRST+128-21)
#define 	TVM_GETUSERDATASIZE		(TV_FIRST+128-22)
#define 	TVM_GETUSERDATA			(TV_FIRST+128-23)
#define 	TVM_SORTCHILDRENEX		(TV_FIRST+128-24)
#define 	TVM_COLUMNAUTOEDIT		(TV_FIRST+128-25-TV_UINICODE)
#define 	TVM_COLUMNAUTOICON		(TV_FIRST+128-27)
#define 	TVM_GETCOUNTPERPAGE		(TV_FIRST+128-28)
#define 	TVM_FINDITEM			(TV_FIRST+128-29-TV_UINICODE)
#define 	TVM_SELECTCHILDS		(TV_FIRST+128-31)
#define 	TVM_GETSETOPTION		(TV_FIRST+128-32)
#define 	TVM_ISITEMVISIBLE		(TV_FIRST+128-33)
#define 	TVM_SETFOCUSITEM		(TV_FIRST+128-34)
#define 	TVM_GETCOLUMNORDERARRAY	(TV_FIRST+128-35)
#define 	TVM_SETCOLUMNORDERARRAY	(TV_FIRST+128-36)
#ifndef		TVM_GETITEMSTATE					  
#define 	TVM_GETITEMSTATE		(TV_FIRST+39) 
#endif											  
#ifndef		TVM_GETEXTENDEDSTYLE				  
#define 	TVM_GETEXTENDEDSTYLE	(TV_FIRST+45) 
#endif
#ifndef		TVM_SETEXTENDEDSTYLE
#define 	TVM_SETEXTENDEDSTYLE	(TV_FIRST+44)
#endif
#ifndef		TVM_GETLINECOLOR
#define 	TVM_GETLINECOLOR		(TV_FIRST+41)
#endif
#ifndef		TVM_SETLINECOLOR
#define 	TVM_SETLINECOLOR		(TV_FIRST+40)
#endif


#ifndef		TVNRET_DEFAULT
#define		TVNRET_DEFAULT			0
#endif
#ifndef		TVNRET_SKIPOLD
#define		TVNRET_SKIPOLD			1
#endif
#ifndef		TVNRET_SKIPNEW
#define		TVNRET_SKIPNEW			2
#endif

#define 	TreeList_DeleteChildItems(h,i)				((BOOL      )SNDMSG(h,TVM_DELETEITEM,0x88,(LPARAM)i))
#define 	TreeList_DeleteAllItems(h)					((BOOL      )SNDMSG(h,TVM_DELETEITEM,0,(LPARAM)TVI_ROOT))
#define		TreeList_DeleteItem(h,i)					((BOOL      )SNDMSG(h,TVM_DELETEITEM,0,(LPARAM)(HTREEITEM)(i)))
#define		TreeList_Expand(h,i,c)						((BOOL      )SNDMSG(h,TVM_EXPAND,(WPARAM)(c),(LPARAM)(HTREEITEM)(i)))
#define		TreeList_GetHeader(h)						((HWND      )SNDMSG(h,TVM_GETHEADER,0,0))
#define		TreeList_DeleteColumn(h,i)     				((BOOL      )SNDMSG(h,TVM_DELETECOLUMN,(WPARAM)(int)(i),0))
#define		TreeList_InsertColumn(h,i,p)				((INT       )SNDMSG(h,TVM_INSERTCOLUMN,(WPARAM)(int)(i),(LPARAM)(const TV_COLUMN*)(p)))
#define 	TreeList_GetColumnCount(h)					((INT       )SNDMSG(h,TVM_GETCOLUMNCOUNT,0,0))
#define 	TreeList_HitTest(h,p)						((HTREEITEM )SNDMSG(h,TVM_HITTEST,0,(LPARAM)(LPTV_HITTESTINFO)(p)))
#define 	TreeList_GetItemOfRow(h,r)					((HTREEITEM )SNDMSG(h,TVM_GETITEMOFROW,0,r))
#define 	TreeList_GetRowOfItem(h,i)					((INT       )SNDMSG(h,TVM_GETROWOFITEM,0,(LPARAM)(i)))
#define 	TreeList_GetRowCount(h)						((INT       )SNDMSG(h,TVM_GETROWCOUNT ,0,0))
#define 	TreeList_GetCountPerPage(h)					((INT       )SNDMSG(h,TVM_GETCOUNTPERPAGE ,0,0))
#define		TreeList_GetExtendedStyle(h)				((DWORD     )SNDMSG(h,TVM_GETEXTENDEDSTYLE,0,0))
#define		TreeList_SetExtendedStyle(h,d)	  			((DWORD     )SNDMSG(h,TVM_SETEXTENDEDSTYLE,0,d))
#define 	TreeList_SetExtendedStyleEx(h,d,m)			((DWORD     )SNDMSG(h,TVM_SETEXTENDEDSTYLE,m,d))
#define		TreeList_GetColor(h,i)						((COLORREF  )SNDMSG(h,TVM_GETBKCOLOR,(WPARAM)(i),0))
#define		TreeList_SetColor(h,i,c) 					((COLORREF  )SNDMSG(h,TVM_SETBKCOLOR,(WPARAM)(i),c))
#define		TreeList_GetItemBkColor(h,i,s)				((COLORREF  )SNDMSG(h,TVM_GETITEMBKCOLOR,(WPARAM)(i),s))
#define		TreeList_SetItemBkColor(h,i,s,c) 			((COLORREF  )SNDMSG(h,TVM_SETITEMBKCOLOR,((UINT)(i))|((s)<<24),c))
#define		TreeList_GetItemTextColor(h,i,s)			((COLORREF  )SNDMSG(h,TVM_GETITEMTEXTCOLOR,(WPARAM)(i),s))
#define		TreeList_SetItemTextColor(h,i,s,c) 			((COLORREF  )SNDMSG(h,TVM_SETITEMTEXTCOLOR,((UINT)(i))|((s)<<24),c))
#define 	TreeList_IsItemVisible(h,i,s)				((INT       )SNDMSG(h,TVM_ISITEMVISIBLE,s,(LPARAM)(HTREEITEM)(i)))
#define 	TreeList_EnsureVisible(h,i)					((BOOL      )SNDMSG(h,TVM_ENSUREVISIBLE,0,(LPARAM)(HTREEITEM)(i)))
#define 	TreeList_EnsureVisibleEx(h,i,s)				((BOOL      )SNDMSG(h,TVM_ENSUREVISIBLE,s,(LPARAM)(HTREEITEM)(i)))
#define 	TreeList_SelectDropTargetEx(h,i,s)			((BOOL      )SNDMSG(h,TVM_SELECTDROP,(WPARAM)(s),(LPARAM)(HTREEITEM)(i)))
#define 	TreeList_SelectSubItem(h,i,s)				((BOOL      )SNDMSG(h,TVM_SELECTSUBITEM,(WPARAM)(s),(LPARAM)(HTREEITEM)(i)))
#define 	TreeList_SelectChilds(h,i,s)				((BOOL      )SNDMSG(h,TVM_SELECTCHILDS,(WPARAM)(s),(LPARAM)(HTREEITEM)(i)))
#define 	TreeList_Select(h,i,c)						((BOOL      )SNDMSG(h,TVM_SELECTITEM,(WPARAM)(c),(LPARAM)(HTREEITEM)(i)))
#define		TreeList_EditLabel(h,i,s)				    ((HWND      )SNDMSG(h,TVM_EDITLABEL,s,(LPARAM)(HTREEITEM)(i)))
#define		TreeList_StartEdit(h,i,s)				    ((BOOL      )SNDMSG(h,TVM_EDITLABEL,TVIR_EDITCOL(s)|TVLE_DONOTIFY,(LPARAM)(HTREEITEM)(i)))
#define		TreeList_EndEditLabelNow(h,c)				((BOOL      )SNDMSG(h,TVM_ENDEDITLABELNOW,c,0))
#define		TreeList_GetItem(h,p)						((BOOL      )SNDMSG(h,TVM_GETITEM,0,(LPARAM)(TV_ITEM*)(p)))
#define		TreeList_GetCount()							((BOOL      )SNDMSG(h,TVM_GETCOUNT,0,0))
#define		TreeList_GetEditControl(h)					((HWND      )SNDMSG(h,TVM_GETEDITCONTROL,0,0))
#define		TreeList_GetImageList(h,i)					((HIMAGELIST)SNDMSG(h,TVM_GETIMAGELIST,i,0))
#define		TreeList_GetUserData(h,i)					((LPVOID    )SNDMSG(h,TVM_GETUSERDATA,0,(LPARAM)(HTREEITEM)(i)))
#define		TreeList_GetUserDataSize(h)					((INT       )SNDMSG(h,TVM_GETUSERDATASIZE,0,0))
#define		TreeList_SetUserDataSize(h,s)				((INT       )SNDMSG(h,TVM_SETUSERDATASIZE,0,s))
#define		TreeList_GetIndent							((UINT      )SNDMSG(h,TVM_GETINDENT,0,0))
#define		TreeList_GetVisibleCount					((UINT      )SNDMSG(h,TVM_GETVISIBLECOUNT,0,0))
#define		TreeList_InsertItem(h,p)					((HTREEITEM )SNDMSG(h,TVM_INSERTITEM,0,(LPARAM)(LPTV_INSERTSTRUCT)(p)))
#define		TreeList_FindItem(h,p,f)					((HTREEITEM )SNDMSG(h,TVM_FINDITEM ,(WPARAM)p,(LPARAM)f))
#define		TreeList_CreateDragImage(h,i)				((HIMAGELIST)SNDMSG(h,TVM_CREATEDRAGIMAGE, 0, (LPARAM)(HTREEITEM)(i)))
#define		TreeList_CreateDragImageEx(h,i,s)			((HIMAGELIST)SNDMSG(h,TVM_CREATEDRAGIMAGE, s, (LPARAM)(HTREEITEM)(i)))
#define		TreeList_SetImageList(h,l,i)				((HIMAGELIST)SNDMSG(h,TVM_SETIMAGELIST,i,(LPARAM)(HIMAGELIST)(l)))
#define		TreeList_SetIndent(h,i)					    ((BOOL      )SNDMSG(h,TVM_SETINDENT,(WPARAM)(i),0))
#define		TreeList_SetItem(h,p)					    ((BOOL      )SNDMSG(h,TVM_SETITEM,0,(LPARAM)(const TV_ITEM*)(p)))
#define		TreeList_SortChildren(h,i,r)				((BOOL      )SNDMSG(h,TVM_SORTCHILDREN  ,(WPARAM)r,(LPARAM)(HTREEITEM)(i)))
#define		TreeList_SortChildrenCB(h,p,r)				((BOOL      )SNDMSG(h,TVM_SORTCHILDRENCB,(WPARAM)r,(LPARAM)(LPTV_SORTCB)(p)))
#define		TreeList_SortChildrenEX(h,p,r)				((BOOL      )SNDMSG(h,TVM_SORTCHILDRENEX,(WPARAM)r,(LPARAM)(LPTV_SORTEX)(p)))
#define		TreeList_SetColumn(h,i,p)					((BOOL      )SNDMSG(h,TVM_SETCOLUMN,i,(LPARAM)(const TV_COLUMN*)(p)))
#define		TreeList_GetColumn(h,i,p)					((BOOL      )SNDMSG(h,TVM_GETCOLUMN,i,(LPARAM)(TV_COLUMN*)(p)))
#define		TreeList_SetColumnWidth(h,i,w)				((BOOL      )SNDMSG(h,TVM_SETCOLUMNWIDTH,i,w))
#define		TreeList_GetColumnWidth(h,i)				((INT       )SNDMSG(h,TVM_GETCOLUMNWIDTH,i,0))
#define 	TreeList_SetColumnAutoEdit(h,i,f,p)			((BOOL      )SNDMSG(h,TVM_COLUMNAUTOEDIT,(WPARAM)((f)&~TVAE_COL(-1))|TVAE_COL(i),(LPARAM)(p)))
#define 	TreeList_SetColumnAutoIcon(h,i,n)			((BOOL      )SNDMSG(h,TVM_COLUMNAUTOICON,i,n))
#define 	TreeList_SetFocusItem(h,i,c)				((BOOL      )SNDMSG(h,TVM_SETFOCUSITEM,c,(LPARAM)(i)))
#define 	TreeList_SetOption(h,i,o)					((INT       )SNDMSG(h,TVM_GETSETOPTION,(i)|TVOP_WRITEOPTION,(LPARAM)(o)))
#define 	TreeList_GetOption(h,i)						((INT       )SNDMSG(h,TVM_GETSETOPTION,i,0))
#define 	TreeList_SetColumnOrderArray(h,n,p)			((BOOL      )SNDMSG(h,TVM_SETCOLUMNORDERARRAY,n,(LPARAM)(p)))
#define 	TreeList_GetColumnOrderArray(h,n,p)			((BOOL      )SNDMSG(h,TVM_GETCOLUMNORDERARRAY,n,(LPARAM)(p)))
#ifdef __cplusplus
#define		TreeList_GetStyle(h)						((DWORD     )::GetWindowLong(h,GWL_STYLE))
#define		TreeList_SetStyle(h,d)	  					((DWORD     )::SetWindowLong(h,GWL_STYLE,d))
#define 	TreeList_SetStyleEx(h,d,m)					((DWORD     )::SetWindowLong(h,GWL_STYLE,((d)&(m))|(::GetWindowLong(h,GWL_STYLE)&~(m))))
#else
#define		TreeList_GetStyle(h)						((DWORD     )GetWindowLong(h,GWL_STYLE))
#define		TreeList_SetStyle(h,d)	  					((DWORD     )SetWindowLong(h,GWL_STYLE,d))
#define 	TreeList_SetStyleEx(h,d,m)					((DWORD     )SetWindowLong(h,GWL_STYLE,((d)&(m))|(GetWindowLong(h,GWL_STYLE)&~(m))))
#endif
#define		TreeList_GetItemRect(h,i,s,p,c)			    (*(HTREEITEM*)p =(i),(BOOL)SNDMSG(h,TVM_GETITEMRECT,(WPARAM)((c)|(TVIR_COLTOSUB(s))),(LPARAM)(RECT*)(p)))


#define		TreeList_SelectItem(h,i)					TreeList_Select(h,i,TVGN_CARET)
#define 	TreeList_SelectDropTarget(h,i)				TreeList_Select(h,i,TVGN_DROPHILITE)
#define 	TreeList_SelectSetFirstVisible(h,i)			TreeList_Select(h,i,TVGN_FIRSTVISIBLE)

#define 	TreeList_GetNextItem(h,i,c)					TreeView_GetNextItem(h, i,     c)
#define		TreeList_GetChild(h,i)						TreeView_GetNextItem(h, i,     TVGN_CHILD)
#define 	TreeList_GetParent(h, i)         			TreeView_GetNextItem(h, i,     TVGN_PARENT)
#define 	TreeList_GetNextSibling(h,i)    			TreeView_GetNextItem(h, i,     TVGN_NEXT)
#define 	TreeList_GetPrevSibling(h,i)    			TreeView_GetNextItem(h, i,     TVGN_PREVIOUS)
#define 	TreeList_GetNextSelected(h,i)			    TreeView_GetNextItem(h, i,     TVGN_NEXTSELECTED)
#define		TreeList_GetNextSelectedChild(h,i)			TreeView_GetNextItem(h, i,	   TVGN_NEXTSELCHILD)
#define 	TreeList_GetNextVisible(h,i)    			TreeView_GetNextItem(h, i,     TVGN_NEXTVISIBLE)
#define 	TreeList_GetPrevVisible(h,i)    			TreeView_GetNextItem(h, i,     TVGN_PREVIOUSVISIBLE)
#define 	TreeList_GetLastChild(h,i)				    TreeView_GetNextItem(h, i,     TVGN_LASTCHILD)
#define 	TreeList_GetSelection(h)					TreeView_GetNextItem(h, NULL,  TVGN_CARET)
#define 	TreeList_GetDropHilight(h)					TreeView_GetNextItem(h, NULL,  TVGN_DROPHILITE)
#define 	TreeList_GetFirstVisible(h)				    TreeView_GetNextItem(h, NULL,  TVGN_FIRSTVISIBLE)
#define		TreeList_GetLastVisible(h)					TreeView_GetNextItem(h, NULL,  TVGN_LASTVISIBLE)
#define 	TreeList_GetRoot(h)							TreeView_GetNextItem(h, NULL,  TVGN_ROOT)
#define		TreeList_GetFocus(h)						TreeView_GetNextItem(h, NULL,  TVGN_FOCUS)
#define		TreeList_GetFocusColumn(h)					((int)TreeView_GetNextItem(h, NULL,  TVGN_FOCUSSUB))
#define		TreeList_GetSelectionColumn(h)				((int)TreeView_GetNextItem(h, NULL,  TVGN_CARETSUB))
#define		TreeList_GetDropHilightColumn(h)			((int)TreeView_GetNextItem(h, NULL,  TVGN_DROPHILITESUB))

extern int	TreeListRegister(HINSTANCE hInstance);
extern BOOL	TreeListUnregister(HINSTANCE hInstance);


/* Compat with my old code... */
#define TLCOLUMN TVCOLUMN
#define HTLITEM  HTREEITEM

#define TL_INSERTSTRUCTA TV_INSERTSTRUCTA
#define TLINSERTSTRUCTA  TVINSERTSTRUCTA
#define TL_INSERTSTRUCTW TV_INSERTSTRUCTW
#define TLINSERTSTRUCTW  TVINSERTSTRUCTW
#define TL_INSERTSTRUCT  TV_INSERTSTRUCT
#define TLINSERTSTRUCT   TVINSERTSTRUCT

#define TL_ITEMA TV_ITEMA
#define TLITEMA  TVITEMA
#define TL_ITEMW TV_ITEMW
#define TLITEMW  TVITEMW
#define TL_ITEM  TV_ITEM
#define TLITEM   TVITEM

/* New stuff */
#ifndef __REACTOS__
#define TreeList_SetItemText(hwndLV,hItem_,iSubItem_,pszText_) \
{ \
    TV_ITEM _ms_tvi; \
    _ms_tvi.mask        = TVIF_SUBITEM | TVIF_TEXT; \
    _ms_tvi.hItem       = (hItem_); \
    _ms_tvi.stateMask   = 0; \
    _ms_tvi.pszText     = (pszText_); \
    _ms_tvi.cchTextMax  = (pszText_) ? 256 : 0; \
    _ms_tvi.cChildren   = (iSubItem_); \
    TreeList_SetItem((hwndLV), &_ms_tvi); \
}
#else // __REACTOS__
#define TreeList_SetItemText(hwndLV,hItem_,iSubItem_,pszText_) \
{ \
    TV_ITEM _ms_tvi; \
    LPWSTR _my_pszText = (pszText_); \
    _ms_tvi.mask        = TVIF_SUBITEM | TVIF_TEXT; \
    _ms_tvi.hItem       = (hItem_); \
    _ms_tvi.stateMask   = 0; \
    _ms_tvi.pszText     = _my_pszText; \
    _ms_tvi.cchTextMax  = _my_pszText ? (int)wcslen(_my_pszText) : 0; \
    _ms_tvi.cChildren   = (iSubItem_); \
    TreeList_SetItem((hwndLV), &_ms_tvi); \
}
#endif // __REACTOS__

#endif
