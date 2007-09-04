#ifndef USER_TYPES_H__
#define USER_TYPES_H__

/* $Id$
 *
 * PROJECT:     System regression tool for ReactOS
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        tools/sysreg/user_types.h
 * PURPOSE:     user types
 * PROGRAMMERS: Johannes Anderwald (johannes.anderwald at sbox tugraz at)
 */

#include <string>
#include <iostream>

#ifndef __LINUX__
 #include <tchar.h>
#else
 #define TCHAR char
 #define tstrcpy strcpy
 #define _tcstok strtok
 #define _tcschr strchr
 #define _tcscat strcat
 #define _tcscpy(str1, str2) strcpy(str1, str2)
 #define _tcslen(str1) strlen(str1)
 #define _tcstod strtod
 #define _tcscmp strcmp
 #define _tcstoul strtoul
 #define _tcsncmp strncmp
 #define _tremove remove
 #define _ttoi atoi
 #define _T(x) x
 #define _tfopen fopen
 #define _tcsstr strstr
 #define _fgetts fgets
 #define _tgetenv getenv
 #define _tmain main
 #define _tcstol strtol
#endif

	typedef std::basic_string<TCHAR> string;
	typedef std::basic_istringstream<TCHAR> istringstream;


#ifdef UNICODE

	using std::wcout;
	using std::wcerr;
	using std::endl;

#define cout wcout
#define cerr wcerr

#else

	using std::cout;
	using std::cerr;
	using std::endl;

#endif


#endif // end of USER_TYPES_H__ 
