/* $Id: Completion.h,v 1.1 2001/01/10 01:25:29 narnaoud Exp $ */

// Completion.h - declaration for completion related functions

#if !defined(PATTERN_H__INCLUDED_)
#define PATTERN_H__INCLUDED_

typedef const TCHAR * (*ReplaceCompletionCallback)(unsigned __int64& rnIndex, const BOOL *pblnForward,
												   const TCHAR *pchContext, const TCHAR *pchBegin);

extern const TCHAR * CompletionCallback(unsigned __int64 & rnIndex,
                                        const BOOL *pblnForward,
                                        const TCHAR *pchContext,
                                        const TCHAR *pchBegin);

extern void InvalidateCompletion();
#endif
