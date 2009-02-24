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
	HANDLE hBatchFile;
	TCHAR BatchFilePath[MAX_PATH];
	LPTSTR params;
	LPTSTR raw_params;   /* Holds the raw params given by the input */
	INT    shiftlevel;
	BOOL   bEcho;        /* Preserve echo flag across batch calls */
	REDIRECTION *RedirList;
	TCHAR forvar;
	UINT   forvarcount;
	LPTSTR *forvalues;
} BATCH_CONTEXT, *LPBATCH_CONTEXT;


/*  The stack of current batch contexts.
 * NULL when no batch is active
 */
extern LPBATCH_CONTEXT bc;

extern BOOL bEcho;       /* The echo flag */

#define BATCH_BUFFSIZE  8192

extern TCHAR textline[BATCH_BUFFSIZE]; /* Buffer for reading Batch file lines */


LPTSTR FindArg (INT);
LPTSTR BatchParams (LPTSTR, LPTSTR);
VOID   ExitBatch (LPTSTR);
BOOL   Batch (LPTSTR, LPTSTR, LPTSTR, BOOL);
LPTSTR ReadBatchLine();
VOID AddBatchRedirection(REDIRECTION **);

#endif /* _BATCH_H_INCLUDED_ */
