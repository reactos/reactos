/* $Id: tchar.h,v 1.1 2003/07/06 23:04:19 hyperion Exp $
 */

#ifndef __TCHAR_INC_S__
#define __TCHAR_INC_S__

#ifdef _UNICODE

#define _tcscat _wcscat
#define _tcschr _wcschr
#define _tcscmp _wcscmp
#define _tcscpy _wcscpy
#define _tcslen _wcslen
#define _tcsncat _wcsncat
#define _tcsncmp _wcsncmp
#define _tcsncpy _wcsncpy
#define _tcsnlen _wcsnlen
#define _tcsrchr _wcsrchr

#define _tscas scasw
#define _tlods lodsw
#define _tstos stosw

#define _tsize $2

#define _treg(_O_) _O_ ## x

#define _tdec(_O_) sub $2, _O_
#define _tinc(_O_) add $2, _O_

#else

#define _tcscat _strcat
#define _tcschr _strchr
#define _tcscmp _strcmp
#define _tcscpy _strcpy
#define _tcslen _strlen
#define _tcsncat _strncat
#define _tcsncmp _strncmp
#define _tcsncpy _strncpy
#define _tcsnlen _strnlen
#define _tcsrchr _strrchr

#define _tscas scasb
#define _tlods lodsb
#define _tstos stosb

#define _tsize  $1

#define _treg(_O_) _O_ ## l

#define _tdec(_O_) dec _O_
#define _tinc(_O_) inc _O_

#endif

#endif

/* EOF */
