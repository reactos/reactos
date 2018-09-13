/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    mibcc.h

Abstract:

    mibcc.h contains the definitions common to the MIB compiler.

Environment:

    User Mode - Win32

Revision History:

    10-May-1996 DonRyan
        Removed banner from Technology Dynamics, Inc.

--*/

//--------------------------- VERSION INFO ----------------------------------

//--------------------------- PUBLIC CONSTANTS ------------------------------

//--------------------------- PUBLIC STRUCTS --------------------------------

#define UINT unsigned int
#define LPSTR char *

#define BOOL int
#define FALSE 0
#define TRUE 1

//--------------------------- PUBLIC VARIABLES --(same as in module.c file)--

extern int lineno;

/* command line switches */
extern BOOL fTreePrint;		/* -p : Print the tree when it is all parsed */
extern BOOL fNodePrint;		/* -n : Print each node as it is added */
extern unsigned int nWarningLevel;
extern unsigned int nStopAfterErrors;

extern LPSTR lpOutputFileName;	/* Global pointer to output file name */

extern FILE *yyin, *yyout;	/* where lex will read its input from */

//--------------------------- PUBLIC PROTOTYPES -----------------------------

extern int yyparse ();
extern void mark_flex_to_init (void);

#define error_out	stdout

//--------------------------- END -------------------------------------------

