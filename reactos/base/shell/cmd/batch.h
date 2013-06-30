/*
 *  BATCH.H - A structure to preserve the context of a batch file
 */

#pragma once

typedef struct tagBATCHCONTEXT
{
    struct tagBATCHCONTEXT *prev;
    char    *mem;       /* batchfile content in memory */
    DWORD   memsize;    /* size of batchfile */
    DWORD   mempos;     /* current position to read from */
    BOOL    memfree;    /* true if it need to be freed when exitbatch is called */	
    TCHAR BatchFilePath[MAX_PATH];
    LPTSTR params;
    LPTSTR raw_params;  /* Holds the raw params given by the input */
    INT    shiftlevel[10];
    BOOL   bEcho;       /* Preserve echo flag across batch calls */
    REDIRECTION *RedirList;
    PARSED_COMMAND *current;
    struct _SETLOCAL *setlocal;
} BATCH_CONTEXT, *LPBATCH_CONTEXT;

typedef struct tagFORCONTEXT
{
    struct tagFORCONTEXT *prev;
    TCHAR firstvar;
    UINT   varcount;
    LPTSTR *values;
} FOR_CONTEXT, *LPFOR_CONTEXT;


/*
 * The stack of current batch contexts.
 * NULL when no batch is active
 */
extern LPBATCH_CONTEXT bc;

extern LPFOR_CONTEXT fc;

extern BOOL bEcho;       /* The echo flag */

#define BATCH_BUFFSIZE  8192

extern TCHAR textline[BATCH_BUFFSIZE]; /* Buffer for reading Batch file lines */


LPTSTR FindArg (TCHAR, BOOL *);
LPTSTR BatchParams (LPTSTR, LPTSTR);
VOID   ExitBatch (VOID);
INT    Batch (LPTSTR, LPTSTR, LPTSTR, PARSED_COMMAND *);
BOOL   BatchGetString (LPTSTR lpBuffer, INT nBufferLength);
LPTSTR ReadBatchLine(VOID);
VOID   AddBatchRedirection(REDIRECTION **);
