/* $Id: Pattern.h,v 1.1 2001/01/10 01:25:29 narnaoud Exp $ */

// Pattern.h: decalration for pattern functions

#if !defined(PATTERN_H__INCLUDED_)
#define PATTERN_H__INCLUDED_

#define PATTERN_MATCH_ALL  _T("*")

BOOL PatternMatch(const TCHAR *pszPattern, const TCHAR *pszTry);

#endif
