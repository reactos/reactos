#ifndef __XPLTFRM_H__
#define __XPLTFRM_H__

#include <platform.h>

#ifdef unix
#define LONGLONG_ZERO 0LL
#define __int8 char
#define DIR_SEPARATOR_CHAR TEXT('/')
#define DIR_SEPARATOR_STRING TEXT("/")
#define WEBDIR_STRING "Web/"
// Follwing is workaround for MainWin Registry API bug 2053.
#else
#define LONGLONG_ZERO 0i64
#define DIR_SEPARATOR_CHAR TEXT('\\')
#define DIR_SEPARATOR_STRING TEXT("\\")
#define WEBDIR_STRING "Web\\" 
#endif /* unix */

#endif /* __XPLTFRM_H__ */

