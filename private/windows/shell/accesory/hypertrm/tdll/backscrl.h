/*	File: D:\WACKER\tdll\backscrl.h (Created: 10-Dec-1993)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:34p $
 */

#if !defined(INCL_HBACKSCRL)
#define INCL_HBACKSCRL

// Defines

#define BKPOS_THUMBPOS	1
#define BKPOS_ABSOLUTE	2

#define BKSCRL_USERLINES_DEFAULT_MAX	500
#define BKSCRL_USERLINES_DEFAULT_MIN	0

// This macro use to calculate bytes needed to save ul lines.
// The value is approximate.
//
#define BK_LINES_TO_BYTES(i)   (i * 80)

/* --- Function prototypes --- */

void		backscrlDestroy 	(const HBACKSCRL hBackscrl);

BOOL		backscrlAdd 		(const HBACKSCRL hBackscrl,
								 const ECHAR	 *pachBuf,
								 const int		 iLen
								);

HBACKSCRL	backscrlCreate		(const HSESSION hSession, const int iBytes);

BOOL		backscrlGetBkLines	(const HBACKSCRL hBackscrl,
								 const int yBeg,
								 const int iWant,
								 int	  *piGot,
								 ECHAR    **lpstrTxt,
								 int	  *piOffset
								);

int 		backscrlGetNumLines (const HBACKSCRL hBackscrl);
void 		backscrlSave(const HBACKSCRL hBackscrl);
void		backscrlFlush(const HBACKSCRL hBackscrl);
BOOL		backscrlChanged(const HBACKSCRL hBackscrl);
void		backscrlResetChangedFlag(const HBACKSCRL hBackscrl);
void 		backscrlRead(const HBACKSCRL hBackscrl);
int 		backscrlSetUNumLines (const HBACKSCRL hBackscrl, const int iUserLines);
int 		backscrlGetUNumLines (const HBACKSCRL hBackscrl);
void		backscrlSetShowFlag(const HBACKSCRL hBackscrl, const int fFlag);
#endif
