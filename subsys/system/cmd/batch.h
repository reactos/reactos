/*
 *  BATCH.H - A structure to preserve the context of a batch file
 *
 *
 */

#ifndef _BATCH_H_INCLUDED_
#define _BATCH_H_INCLUDED_


typedef struct tagBATCHCONTEXT
{
	struct tagBATCHCONTEXT *prev;
	LPWIN32_FIND_DATA ffind;
	HANDLE hBatchFile;
	LPTSTR forproto;
	LPTSTR params;
	INT    shiftlevel;
	BOOL   bEcho;        /* Preserve echo flag across batch calls */
	HANDLE hFind;        /* Preserve find handle when doing a for */
	TCHAR forvar;
} BATCH_CONTEXT, *LPBATCH_CONTEXT;


/*  The stack of current batch contexts.
 * NULL when no batch is active
 */
extern LPBATCH_CONTEXT bc;

extern BOOL bEcho;       /* The echo flag */

#define BATCH_BUFFSIZE  2048

extern TCHAR textline[BATCH_BUFFSIZE]; /* Buffer for reading Batch file lines */


LPTSTR FindArg (INT);
LPTSTR BatchParams (LPTSTR, LPTSTR);
VOID   ExitBatch (LPTSTR);
BOOL   Batch (LPTSTR, LPTSTR, LPTSTR);
LPTSTR ReadBatchLine (LPBOOL);

#endif /* _BATCH_H_INCLUDED_ */
