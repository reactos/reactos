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
#include <tchar.h>
#include <iostream>

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
