/*
 *  BATCH.H - A structure to preserve the context of a batch file
 *
 *
 */


typedef struct tagBATCHCONTEXT
{
	struct tagBATCHCONTEXT *prev;
	LPWIN32_FIND_DATA ffind;
	HANDLE hBatchFile;
	LPTSTR forproto;
	LPTSTR params;
	INT    shiftlevel;
	BOOL   bEcho;        /* Preserve echo flag across batch calls [HBP_001] */
	TCHAR forvar;
} BATCH_CONTEXT, *LPBATCH_CONTEXT;

/* HBP_002 } */


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
