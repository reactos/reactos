/* $Id: genw32k.c,v 1.4 2003/07/09 20:41:35 hyperion Exp $
 *
 * COPYRIGHT:             See COPYING in the top level directory
 * PROJECT:               ReactOS version of ntdll
 * FILE:                  iface/native/genntdll.c
 * PURPOSE:               Generates the system call stubs in ntdll
 * CHANGE HISTORY:	  Added a '@xx' to deal with stdcall [ Ariadne ]
 * 	19990616 (ea)
 * 		Four arguments now required; 4th is the file
 * 		for ntoskrnl ZwXXX functions (which are merely calls
 * 		to twin NtXXX calls, via int 0x2e (x86).
 * 	19990617 (ea)
 * 		Fixed a bug in function numbers in kernel ZwXXX stubs.
 *
 */

/* INCLUDE ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PARAMETERIZED_LIBS

#define INPUT_BUFFER_SIZE 255

#define INDEX  0x1000		/* SSDT index 1 */


/* FUNCTIONS ****************************************************************/

int makeSystemServiceTable(FILE *in, FILE *out)
{
char    line [INPUT_BUFFER_SIZE];
char    *s;
char    *name;
int     sys_call_idx;
char    *nr_args;
char    *stmp;

	/*
	 * Main SSDT Header
	 */
	fprintf(out,"// Machine generated, don't edit\n");
	fprintf(out,"\n\n");

	/*
	 * First we build the Win32k SSDT
	 */
	fprintf(out,"SSDT Win32kSSDT[] = {\n");

	/* First system call has index zero */
	sys_call_idx = 0;

	/* Go on until EOF or read zero bytes */
	while ((!feof(in))&& (fgets(line, sizeof line, in) != NULL))
	{
		if ((s = (char *) strchr(line,'\r')) != NULL)
		{
			*s = '\0';
		}
		/*
		 * Skip comments (#) and empty lines.
		 */
		s = & line[0];
		if ((*s) != '#' && (*s) != '\0')
		{
			/* Extract the NtXXX name */
			name = (char *)strtok(s," \t");
			/* Extract the stack size */
			nr_args = (char *)strtok(NULL," \t");
			/*
			 * Remove, if present, the trailing LF.
			 */
			if ((stmp = strchr(nr_args, '\n')) != NULL)
			{
				*stmp = '\0';
			}
#ifdef VERBOSE
			printf("%3d \"%s\"\n",sys_call_idx | INDEX,name);
#endif

			if (sys_call_idx > 0)
			{
				fprintf(out,",\n");
			}
			/*
			 * Now write the current system call's name
			 * in the service table.
			 */
			fprintf(out,"\t\t{ (ULONG)%s }",name);

			/* Next system call index */
			sys_call_idx++;
		}
	}
	/* Close the service table (C syntax) */
	fprintf(out,"\n};\n");

	/*
	 * Now we build the Win32k SSPT
	 */
	rewind(in);
	fprintf(out,"\n\n");
	fprintf(out,"SSPT Win32kSSPT[] = {\n");

	/* First system call has index zero */
	sys_call_idx = 0;

	/* Go on until EOF or read zero bytes */
	while ((!feof(in))&& (fgets(line, sizeof line, in) != NULL))
	{
		if ((s = (char *) strchr(line,'\r')) != NULL)
		{
			*s = '\0';
		}
		/*
		 * Skip comments (#) and empty lines.
		 */
		s = & line[0];
		if ((*s) != '#' && (*s) != '\0')
		{
			/* Extract the NtXXX name */
			name = (char *)strtok(s," \t");
			/* Extract the stack size */
			nr_args = (char *)strtok(NULL," \t");
			/*
			 * Remove, if present, the trailing LF.
			 */
			if ((stmp = strchr(nr_args, '\n')) != NULL)
			{
				*stmp = '\0';
			}
#ifdef VERBOSE
			printf("%3d \"%s\"\n",sys_call_idx|INDEX,name);
#endif

			if (sys_call_idx > 0)
			{
				fprintf(out,",\n");
			}
			/*
			 * Now write the current system call's ID
			 * in the service table along with its Parameters Size.
			 */
			fprintf(out,"\t\t{ %d }",atoi(nr_args) * sizeof(void*));

			/* Next system call index */
			sys_call_idx++;
		}
	}
	/*
	 * Close the service table (C syntax)
	 */
	fprintf(out,"\n};\n");

	/*
	 * We write some useful defines
	 */
	fprintf(out, "\n\n#define MIN_SYSCALL_NUMBER    0\n");
	fprintf(out, "#define MAX_SYSCALL_NUMBER    %d\n", sys_call_idx-1);
	fprintf(out, "#define NUMBER_OF_SYSCALLS    %d\n", sys_call_idx);
	fprintf(out, "ULONG Win32kNumberOfSysCalls = %d;\n", sys_call_idx);

	return(0);
}


int
process(
	FILE	* in,
	FILE	* out1,
	FILE	* out2,
	FILE    * out3
	)
{
	char		line [INPUT_BUFFER_SIZE];
	char		* s;
	char		* name;		/* NtXXX name */
	int		sys_call_idx;	/* NtXXX index number in the service table */
	char		* nr_args;	/* stack_size / machine_word_size */
	char		* stmp;
	int		stacksize;

	/*
	 * GDI32 stubs file header
	 */
	fprintf(out1,"// Machine generated, don't edit\n");
	fprintf(out1,"\n\n");

	/*
	 * USER32 stubs file header
	 */
	fprintf(out2,"// Machine generated, don't edit\n");
	fprintf(out2,"\n\n");

	/*
	 * CSRSS stubs file header
	 */
	fprintf(out3,"// Machine generated, don't edit\n");
	fprintf(out3,"\n\n");

	/*
	 * Scan the database. DB is a text file; each line
	 * is a record, which contains data for one system
	 * function. Each record has three columns:
	 *
	 * NT_NAME	(e.g. NtCreateProcess)
	 * ZW_NAME	(e.g. ZwCreateProcess)
	 * STACK_SIZE	(in machine words: for x[3456]86
	 * 		processors a machine word is 4 bytes)
	 */

	/* First system call has index zero */
	sys_call_idx = 0;
	while (
		/* Go on until EOF or read zero bytes */
		(	(!feof(in))
			&& (fgets(line, sizeof line, in) != NULL)
			)
		)
	{
		/*
		 * Remove, if present, the trailing CR.
		 * (os specific?)
		 */
		if ((s = (char *) strchr(line,'\r')) != NULL)
		{
			*s = '\0';
		}
		/*
		 * Skip comments (#) and empty lines.
		 */
		s = & line[0];
		if ((*s) != '#' && (*s) != '\0')
		{
			/* Extract the NtXXX name */
			name = (char *)strtok(s," \t");
			/* Extract the stack size */
			nr_args = (char *)strtok(NULL," \t");
			stacksize = atoi(nr_args)*sizeof(void*);
			/*
			 * Remove, if present, the trailing LF.
			 */
			if ((stmp = strchr(nr_args, '\n')) != NULL)
			{
				*stmp = '\0';
			}
#ifdef VERBOSE
			printf("%3d \"%s\"\n",sys_call_idx | INDEX,name);
#endif

			/*
			 * Write the GDI32 stub for the current system call.
			 */
#ifdef PARAMETERIZED_LIBS
			fprintf(out1,"__asm__(\"\\n\\t.global _%s@%d\\n\\t\"\n",name,stacksize);
			fprintf(out1,"\"_%s@%d:\\n\\t\"\n",name,stacksize);
#else
			fprintf(out1,"__asm__(\"\\n\\t.global _%s\\n\\t\"\n",name);
			fprintf(out1,"\"_%s:\\n\\t\"\n",name);
#endif
			fprintf(out1,"\t\"mov\t$%d,%%eax\\n\\t\"\n",sys_call_idx | INDEX);
			fprintf(out1,"\t\"lea\t4(%%esp),%%edx\\n\\t\"\n");
			fprintf(out1,"\t\"int\t$0x2E\\n\\t\"\n");
			fprintf(out1,"\t\"ret\t$%d\\n\\t\");\n\n",stacksize);

			/*
			 * Write the USER32 stub for the current system call
			 */
#ifdef PARAMETERIZED_LIBS
			fprintf(out2,"__asm__(\"\\n\\t.global _%s@%d\\n\\t\"\n",name,stacksize);
			fprintf(out2,"\"_%s@%d:\\n\\t\"\n",name,stacksize);
#else
			fprintf(out2,"__asm__(\"\\n\\t.global _%s\\n\\t\"\n",name);
			fprintf(out2,"\"_%s:\\n\\t\"\n",name);
#endif
			fprintf(out2,"\t\"mov\t$%d,%%eax\\n\\t\"\n",sys_call_idx | INDEX);
			fprintf(out2,"\t\"lea\t4(%%esp),%%edx\\n\\t\"\n");
			fprintf(out2,"\t\"int\t$0x2E\\n\\t\"\n");
			fprintf(out2,"\t\"ret\t$%d\\n\\t\");\n\n",stacksize);

			/*
			 * Write the CSRSS stub for the current system call
			 */
#ifdef PARAMETERIZED_LIBS
			fprintf(out3,"__asm__(\"\\n\\t.global _%s@%d\\n\\t\"\n",name,stacksize);
			fprintf(out3,"\"_%s@%d:\\n\\t\"\n",name,stacksize);
#else
			fprintf(out3,"__asm__(\"\\n\\t.global _%s\\n\\t\"\n",name);
			fprintf(out3,"\"_%s:\\n\\t\"\n",name);
#endif
			fprintf(out3,"\t\"mov\t$%d,%%eax\\n\\t\"\n",sys_call_idx | INDEX);
			fprintf(out3,"\t\"lea\t4(%%esp),%%edx\\n\\t\"\n");
			fprintf(out3,"\t\"int\t$0x2E\\n\\t\"\n");
			fprintf(out3,"\t\"ret\t$%d\\n\\t\");\n\n",stacksize);

			/* Next system call index */
			sys_call_idx++;
		}
	}

	return(0);
}

void usage(char * argv0)
{
	printf("Usage: %s w32k.lst ssdt.h win32k.c win32k.c\n"
	       "  w32k.lst      system functions database\n"
	       "  ssdt.h        WIN32K service table\n"
	       "  win32k.c      GDI32 stubs\n"
	       "  win32k.c      USER32 stubs\n",
		argv0
		);
}

int main(int argc, char* argv[])
{
	FILE	* in;	/* System calls database */
	FILE	* out1;	/* SERVICE_TABLE */
	FILE	* out2;	/* GDI32 stubs */
	FILE	* out3;	/* USER32 stubs */
	FILE    * out4; /* CSRSS stubs */
	int	ret;

	if (argc != 6)
	{
		usage(argv[0]);
		return(1);
	}

	in = fopen(argv[1],"rb");
	if (in == NULL)
	{
		perror("Failed to open input file (system calls database)");
		return(1);
	}

	out1 = fopen(argv[2],"wb");
	if (out1 == NULL)
	{
		perror("Failed to open output file (WIN32K service table)");
		return(1);
	}

	out2 = fopen(argv[3],"wb");
	if (out2 == NULL)
	{
		perror("Failed to open output file (GDI32 stubs)");
		return(1);
	}

	out3 = fopen(argv[4],"wb");
	if (out3 == NULL)
	{
		perror("Failed to open output file (USER32 stubs)");
		return(1);
	}

	out4 = fopen(argv[5],"wb");
	if (out4 == NULL)
	  {
	    perror("Failed to open output file (CSRSS stubs)");
	    return(1);
	  }

	ret = process(in,out2,out3,out4);
	rewind(in);
	ret = makeSystemServiceTable(in, out1);

	fclose(in);
	fclose(out1);
	fclose(out2);
	fclose(out3);

	return(ret);
}
