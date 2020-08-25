/*
 *  BATCH.H - A structure to preserve the context of a batch file
 */

#pragma once

typedef struct _BATCH_CONTEXT
{
    struct _BATCH_CONTEXT *prev;
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
} BATCH_CONTEXT, *PBATCH_CONTEXT;

typedef struct _FOR_CONTEXT
{
    struct _FOR_CONTEXT *prev;
    TCHAR firstvar;
    UINT   varcount;
    LPTSTR *values;
} FOR_CONTEXT, *PFOR_CONTEXT;


/*
 * The stack of current batch contexts.
 * NULL when no batch is active.
 */
extern PBATCH_CONTEXT bc;
extern PFOR_CONTEXT fc;

extern BOOL bEcho;       /* The echo flag */

#define BATCH_BUFFSIZE  8192

extern TCHAR textline[BATCH_BUFFSIZE]; /* Buffer for reading Batch file lines */


LPTSTR FindArg(TCHAR, BOOL *);
VOID   ExitBatch(VOID);
VOID   ExitAllBatches(VOID);
INT    Batch(LPTSTR, LPTSTR, LPTSTR, PARSED_COMMAND *);
BOOL   BatchGetString(LPTSTR lpBuffer, INT nBufferLength);
LPTSTR ReadBatchLine(VOID);
VOID   AddBatchRedirection(REDIRECTION **);
