/*
 *  BATCH.H - A structure to preserve the context of a batch file
 */

#pragma once

/*
 * This batch type enumeration allows us to adjust the behaviour of some commands
 * depending on whether they are run from within a .BAT or a .CMD file.
 * The behaviour is selected when the top-level batch file is loaded,
 * and it remains the same for any child batch file that may be loaded later.
 *
 * See https://ss64.com/nt/errorlevel.html for more details.
 */
typedef enum _BATCH_TYPE
{
    NONE,
    BAT_TYPE,   /* Old-style DOS batch file */
    CMD_TYPE    /* New-style NT OS/2 batch file */
} BATCH_TYPE;


/* Enable this define for Windows' CMD batch-echo behaviour compatibility */
#define MSCMD_BATCH_ECHO

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
#ifndef MSCMD_BATCH_ECHO
    BOOL   bEcho;       /* Preserve echo flag across batch calls */
#endif
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
extern BATCH_TYPE BatType;
extern PBATCH_CONTEXT bc;
extern PFOR_CONTEXT fc;

#ifdef MSCMD_BATCH_ECHO
extern BOOL bBcEcho;
#endif

extern BOOL bEcho;       /* The echo flag */

#define BATCH_BUFFSIZE  8192

extern TCHAR textline[BATCH_BUFFSIZE]; /* Buffer for reading Batch file lines */


BOOL
FindArg(
    IN TCHAR Char,
    OUT PCTSTR* ArgPtr,
    OUT BOOL* IsParam0);

VOID   ExitBatch(VOID);
VOID   ExitAllBatches(VOID);
INT    Batch(LPTSTR, LPTSTR, LPTSTR, PARSED_COMMAND *);
BOOL   BatchGetString(LPTSTR lpBuffer, INT nBufferLength);
LPTSTR ReadBatchLine(VOID);
VOID   AddBatchRedirection(REDIRECTION **);
