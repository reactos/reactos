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
 *  DISCLAIMED. This includes but is not limited to warranties of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * $Revision: 1.4 $
 * $Author: robd $
 * $Date: 2002/11/24 18:06:00 $
 *
 */

#ifndef __STRICT_ANSI__

#ifndef _CONIO_H_
#define _CONIO_H_

#ifdef  __cplusplus
extern "C" {
#endif


char* _cgets(char*);
int _cprintf(const char*, ...);
int _cputs(const char*); 
int _cscanf(char*, ...);
int _getch(void);
int _getche(void);
int _kbhit(void);
int _putch(int);
int _ungetch(int);

#ifndef _NO_OLDNAMES
#define getch           _getch
#define getche          _getche
#define kbhit           _kbhit
#define putch(cPut)     _putch(cPut)
#define ungetch(cUnget) _ungetch(cUnget)
#endif  /* Not _NO_OLDNAMES */


#ifdef  __cplusplus
}
#endif

#endif  /* Not _CONIO_H_ */

#endif  /* Not __STRICT_ANSI__ */
