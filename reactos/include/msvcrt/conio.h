/*
 * conio.h
 *
 * Low level console I/O functions. Pretty please try to use the ANSI
 * standard ones if you are writing new code.
 *
 * This file is part of the Mingw32 package.
 *
 * Contributors:
 *  Created by Colin Peters <colin@bird.fu.is.saga-u.ac.jp>
 *
 *  THIS SOFTWARE IS NOT COPYRIGHTED
 *
 *  This source code is offered for use in the public domain. You may
 *  use, modify or distribute it freely.
 *
 *  This code is distributed in the hope that it will be useful but
 *  WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 *  DISCLAMED. This includes but is not limited to warranties of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * $Revision: 1.3 $
 * $Author: chorns $
 * $Date: 2002/09/08 10:22:30 $
 *
 */

#ifndef	__STRICT_ANSI__

#ifndef	_CONIO_H_
#define	_CONIO_H_

#ifdef	__cplusplus
extern "C" {
#endif


char*	_cgets (char* szBuffer);
int	_cprintf (const char* szFormat, ...);
int	_cputs (const char* szString);
int	_cscanf (char* szFormat, ...);

int	_getch (void);
int	_getche (void);
int	_kbhit (void);
int	_putch (int cPut);
int	_ungetch (int cUnget);


#ifndef	_NO_OLDNAMES

#define	getch 			_getch
#define	getche  		_getche
#define	kbhit 			_kbhit
#define	putch(cPut)		_putch(cPut)
#define	ungetch(cUnget)		_ungetch(cUnget)

#endif	/* Not _NO_OLDNAMES */


#ifdef	__cplusplus
}
#endif

#endif	/* Not _CONIO_H_ */

#endif	/* Not __STRICT_ANSI__ */
