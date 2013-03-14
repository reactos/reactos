/* $Id: Completion.h 15091 2005-05-07 21:24:31Z sedwards $ */

// Completion.h - declaration for completion related functions

#if !defined(COMLPETION_H__INCLUDED_)
#define COMPLETION_H__INCLUDED_

typedef const TCHAR * (*ReplaceCompletionCallback)(unsigned __int64& rnIndex, const BOOL *pblnForward,
												   const TCHAR *pchContext, const TCHAR *pchBegin);

extern const TCHAR * CompletionCallback(unsigned __int64 & rnIndex,
                                        const BOOL *pblnForward,
                                        const TCHAR *pchContext,
                                        const TCHAR *pchBegin);

extern void InvalidateCompletion();
#endif
