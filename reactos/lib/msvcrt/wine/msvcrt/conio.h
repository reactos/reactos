/*
 * Console I/O definitions
 *
 * Derived from the mingw header written by Colin Peters.
 * Modified for Wine use by Jon Griffiths and Francois Gouget.
 * This file is in the public domain.
 */
#ifndef __WINE_CONIO_H
#define __WINE_CONIO_H
#ifndef __WINE_USE_MSVCRT
#define __WINE_USE_MSVCRT
#endif

#ifdef __cplusplus
extern "C" {
#endif

char*       _cgets(char*);
int         _cprintf(const char*,...);
int         _cputs(const char*);
int         _cscanf(const char*,...);
int         _getch(void);
int         _getche(void);
int         _kbhit(void);
int         _putch(int);
int         _ungetch(int);

#ifdef _M_IX86
int         _inp(unsigned short);
unsigned long _inpd(unsigned short);
unsigned short _inpw(unsigned short);
int         _outp(unsigned short, int);
unsigned long _outpd(unsigned short, unsigned long);
unsigned short _outpw(unsigned short, unsigned short);
#endif

#ifdef __cplusplus
}
#endif


#ifndef USE_MSVCRT_PREFIX
static inline char* cgets(char* str) { return _cgets(str); }
static inline int cputs(const char* str) { return _cputs(str); }
static inline int getch(void) { return _getch(); }
static inline int getche(void) { return _getche(); }
static inline int kbhit(void) { return _kbhit(); }
static inline int putch(int c) { return _putch(c); }
static inline int ungetch(int c) { return _ungetch(c); }
#ifdef _M_IX86
static inline int inp(unsigned short i) { return _inp(i); }
static inline unsigned short inpw(unsigned short i) { return _inpw(i); }
static inline int outp(unsigned short i, int j) { return _outp(i, j); }
static inline unsigned short outpw(unsigned short i, unsigned short j) { return _outpw(i, j); }
#endif

#ifdef __GNUC__
extern int cprintf(const char*,...) __attribute__((alias("_cprintf"),format(printf,1,2)));
extern int cscanf(const char*,...) __attribute__((alias("_cscanf"),format(scanf,1,2)));
#else
#define cprintf _cprintf
#define cscanf _cscanf
#endif /* __GNUC__ */

#endif /* USE_MSVCRT_PREFIX */

#endif /* __WINE_CONIO_H */
