/*	File: C:|WACKER\TDLL\BV_TEXT.HH (Created: 11-JAN-1994)
 *	Created from:
 *	File: C:\HA5G\ha5g\s_text.h (Created: 27-SEP-1991)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:37p $
 */

typedef VOID (CALLBACK *STXT_OWNERDRAW)(HWND, HDC);

struct s_text
	{
	ULONG	ulCheck;			/* Validity check field */
	ULONG	cBackGround;		/* Fill color for background */
	ULONG	cTextColor; 		/* Color to use for text display */
	ULONG	cUpperEdge; 		/* Upper and left edge 3D border color */
	ULONG	cLowerEdge; 		/* Lower and right edge 3D border color */
	USHORT	usDepth;
	LPTSTR	pszText;			/* Text to be displayed */
	HFONT	hFont;				/* Text font used to draw text */
	INT 	iFontHeight;		/* Used for vertical centering */
	STXT_OWNERDRAW fpOwnerDraw; /* pointer to ownerdraw proc. */
	};

#define STEXT_VALID 	0x78745374

typedef struct s_text STEXT;
typedef STEXT *LPSTEXT;

#define STEXT_OK(x) 	((x!=NULL)&&(x->ulCheck==STEXT_VALID))

extern VOID stxtDrawBeveledText(
								HDC 	   hdc,
								HFONT	   hNewFont,
								LPRECT	   lpRc,
								USHORT	   usWidth,
								DWORD	   dwStyle,
								ULONG FAR *pulColors,
								LPTSTR	   pszText
							   );

