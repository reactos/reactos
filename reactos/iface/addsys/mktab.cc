/* $Id: mktab.cc,v 1.1 2000/02/20 22:52:47 ea Exp $
 *
 * COPYRIGHT: See COPYING in the top level directory
 * PROJECT  : ReactOS 
 * FILE     : iface/addsys/mktab.c
 * PURPOSE  : Generating any files required for a kernel module 
 *            to register an additional system calls table in
 *            NTOSKRNL.
 * REVISIONS: 
 * 	2000-02-13 (ea)
 * 		Derived from genntdll.c mainly to build
 * 		win32k.sys functions table.
 */

/* INCLUDE ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VERBOSE

#define INPUT_BUFFER_SIZE 255

#define DB_RECORD_NAME_SIZE 64

#define STACK_ENTRY_SIZE (sizeof(void*))

#define FALSE 0
#define TRUE  1

typedef unsigned int 	DWORD;
typedef int		INT;
typedef char		* LPSTR, CHAR, * PCHAR;
typedef int		BOOL;
typedef void		VOID, * PVOID;

typedef
struct _DB_RECORD
{
	CHAR	Name [DB_RECORD_NAME_SIZE];
	INT	ArgumentCount;
	INT	StackSize;
	
} DB_RECORD, * PDB_RECORD;

typedef
enum 
{
	SELECT_OK,
	SELECT_EOF,
	SELECT_ERROR

} SELECT_TYPE;


/* FUNCTIONS ****************************************************************/


void usage (char * argv0)
{
	printf (
"\nUsage: %s api.db mask table.c apistubs.c\n\n"
"  api.db     additional system functions database (NAME, ARG_COUNT)\n"
"  mask       service table mask (in hex; e.g. 0x1000000)\n"
"  table.c    service table file to be linked in the kernel module\n"
"  apistubs.c stubs for user mode clients to call the API\n\n"
"NOTE: NAME must be an ASCII string and ARG_COUNT a decimal number;\n"
"between NAME and ARG_COUNT there must be at least one space or tab.\n",
		argv0
		);
}


void
CloseAllFiles (FILE * f0, FILE * f1, FILE * f2)
{
	if (f0) fclose (f0);
	if (f1) fclose (f1);
	if (f2) fclose (f2);
}


SELECT_TYPE
GetNextRecord (
	FILE		* InputFile,
	PDB_RECORD	Dbr
	)
{
	CHAR		InputBuffer [INPUT_BUFFER_SIZE];
	PCHAR		s;
	static INT	LineNumber = 0;
	BOOL		BadData = TRUE;

	if (	(NULL == InputFile) 
		|| (NULL == Dbr)
		)
	{
		fprintf (stderr, "GetNextRecord: bad argument!\n");
		return SELECT_ERROR;
	}
	while (TRUE == BadData)
	{
		if (	feof (InputFile)
			|| (NULL == fgets (
					InputBuffer,
					sizeof InputBuffer,
					InputFile
					)
				)
			)
		{
#ifdef VERBOSE
			fprintf (
				stderr,
				"GetNextRecord: EOF at line %d\n",
				LineNumber
				);
#endif
			return SELECT_EOF;
		}
		++ LineNumber;
		/*
		 * Remove, if present, the trailing CR.
		 * (os specific?)
		 */
		if (NULL != (s = (char *) strchr (InputBuffer,'\r')))
		{
			*s = '\0';
		}
		/*
		 * Skip comments (#) and empty lines.
		 */
		s = & InputBuffer [0];
		if (	('#' != (*s))
			&& ('\0' != (*s))
			)
		{
			BadData = FALSE;
		}
	}
	if (2 != sscanf (
			InputBuffer,
			"%s%d",
			Dbr->Name,
			& Dbr->ArgumentCount
			)
		)
	{
		fprintf (
			stderr,
			"GetNextRecord: line %d: syntax error!\n",
			LineNumber
			);
		return SELECT_ERROR;
	}
	Dbr->StackSize = (Dbr->ArgumentCount * sizeof (void*));
	return SELECT_OK;
}


/* User mode service stubs file generation */


void
OutputStubPrologue (FILE * of)
{
	fprintf (
		of,
		"/* Machine generated. Don't edit! */\n\n"
		"__asm (\n"
		);
}


BOOL
OutputStub (FILE * of, PDB_RECORD Dbr, INT Id, DWORD Mask)
{
	DWORD	CallId = (Mask | (DWORD) Id);
	CHAR	DecoratedName [DB_RECORD_NAME_SIZE];
	
	sprintf (
		DecoratedName,
		"_%s@%d",
		Dbr->Name,
		Dbr->StackSize
		);
	fprintf (
		of,
		"\t\".global %s\\n\\t\"\n"
		"\"%s:\\n\\t\"\n"
		"\t\"mov\t$0x%08x,%%eax\\n\\t\"\n"
		"\t\"lea\t4(%%esp),%%edx\\n\\t\"\n"
		"\t\"int\t$0x2E\\n\\t\"\n"
		"\t\"ret\t$%d\\n\\t\"\n",
		DecoratedName,
		DecoratedName,
		CallId,
		Dbr->StackSize
		);
	return TRUE;
}


void
OutputStubEpilog (FILE * of)
{
	fprintf (
		of,
		");\n\n/* EOF */\n"
		);
}


/* Service table file generation (used by the kernel module) */


void
OutputTablePrologue (FILE * of)
{
	fprintf (
		of,
		"/* Machine generated. Don't edit! */\n\n"
		"SERVICE_TABLE W32kServiceTable [] =\n{\n"
		);
}


BOOL
OutputTable (FILE * of, PDB_RECORD Dbr)
{
	static BOOL First = TRUE;
	
	if (TRUE == First)
	{
		fprintf (
			of,
			"{%d,(ULONG)%s}",
			Dbr->StackSize,
			Dbr->Name
			);
		First = FALSE;
	}
	else
	{
		fprintf (
			of,
			",\n{%d,(ULONG)%s}",
			Dbr->StackSize,
			Dbr->Name
			);
	}
	return TRUE;
}


void
OutputTableEpilog (FILE * of)
{
	fprintf (
		of,
		"\n};\n/* EOF */"
		);
}


/* MAIN */

int
main (int argc, char * argv [])
{
	FILE		* ApiDb = NULL;
	FILE		* Table = NULL;
	FILE		* Stubs = NULL;

	DWORD		Mask = 0;
	DB_RECORD	Dbr;
	INT		Id = 0;

	SELECT_TYPE	ReturnValue;
	
	/* --- Check arguments --- */
	
	if (argc != 5)
	{
		usage (argv[0]);
		return (1);
	}

	/* --- Create files --- */

	ApiDb = fopen (argv[1], "rb");
	if (NULL == ApiDb)
	{
		fprintf (
			stderr,
			"%s: fatal: could not open the file \"%s\".\n",
			argv [0],
			argv [1]
			);
		return (1);
	}
	printf ("< %s\n", argv[1]);

	Stubs = fopen (argv[3], "wb");
	if (NULL == Stubs)
	{
		fprintf (
			stderr,
			"%s: fatal: could not open the file \"%s\".\n",
			argv [0],
			argv [3]
			);
		CloseAllFiles (ApiDb, Table, Stubs);
		return (1);
	}
	printf ("> %s\n", argv[3]);

	Table = fopen (argv[4], "wb");
	if (NULL == Table)
	{
		fprintf (
			stderr,
			"%s: fatal: could not open the file \"%s\".\n",
			argv [0],
			argv [4]
			);
		CloseAllFiles (ApiDb, Table, Stubs);
		return (1);
	}
	printf ("> %s\n", argv[4]);
	
	/* --- Convert the mask value --- */

	if (1 != sscanf (argv[2], "%x", & Mask))
	{
		fprintf (
			stderr,
			"%s: fatal: could not convert the mask \"%s\".\n",
			argv [0],
			argv [2]
			);
		CloseAllFiles (ApiDb, Table, Stubs);
		return (1);
	}
	printf ("& 0x%08x\n", Mask);
	
	/* --- Process data --- */

	printf ("Processing data...\n");
	OutputStubPrologue (Stubs);
	OutputTablePrologue (Table);
	while (SELECT_OK == (ReturnValue = GetNextRecord (ApiDb, & Dbr)))
	{
		if (TRUE == OutputTable (Table, & Dbr))
		{
			if (FALSE == OutputStub (Stubs, & Dbr, Id, Mask))
			{
				fprintf (
					stderr,
					"%s: WARNING: %s has no user mode stub\n",
					argv [0],
					Dbr.Name
					);
			}
		}
		else
		{
			fprintf (
				stderr,
				"%s: WARNING: %s skipped on I/O error\n",
				argv [0],
				Dbr.Name
				);
		}
		++ Id;
#ifdef VERBOSE
		printf (
			"%3d: _%s@%d\n",
			Id,
			Dbr.Name,
			Dbr.StackSize
			);
#endif
	}
	if (SELECT_EOF == ReturnValue)
	{
		OutputStubEpilog (Stubs);
		OutputTableEpilog (Table);
	}
	else
	{
		fprintf (
			stderr,
			"%s: generated files may be incomplete!\n",
			argv [0]
			);
	}

	/* --- Close files --- */

	CloseAllFiles (ApiDb, Table, Stubs);

	printf ("Done\n");

	return (0);
}


/* EOF */
