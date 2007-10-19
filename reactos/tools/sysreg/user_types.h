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
#define popen _popen
#define pclose _pclose
#endif

typedef std::basic_string<char> string;
typedef std::basic_istringstream<char> istringstream;

using std::cout;
using std::cerr;
using std::endl;


#endif // end of USER_TYPES_H__
