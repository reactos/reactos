/* $Id: genntdll.c,v 1.9 2000/01/23 15:14:42 hochoa Exp $
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

#define VERBOSE

#define INPUT_BUFFER_SIZE 255

/* FUNCTIONS ****************************************************************/

int makeSystemServiceTable(FILE *in, FILE *out)
{
char    line [INPUT_BUFFER_SIZE];
char    *s;
char    *name;
char    *name2;
int     sys_call_idx;
char    *nr_args;
char    *stmp;

	/*
     * Main SSDT Header
	 */
    fprintf(out,"// Machine generated, don't edit\n");
    fprintf(out,"\n\n");

    /*
     * First we build the Main SSDT
     *
     */
    fprintf(out,"\n\n\n");
    fprintf(out,"SSDT MainSSDT[] = {\n");

	for (	/* First system call has index zero */
		sys_call_idx = 0;
		/* Go on until EOF or read zero bytes */
		(	(!feof(in))
			&& (fgets(line, sizeof line, in) != NULL)
			);
		/* Next system call index */
		sys_call_idx++
		)
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
			/* Extract the ZwXXX name */
			name2 = (char *)strtok(NULL," \t");
			//value = strtok(NULL," \t");
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
			printf("%3d \"%s\"\n",sys_call_idx,name);
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

 		   }
    }
    /* Close the service table (C syntax) */
    fprintf(out,"\n};\n");

    /*
     * Now we build the Main SSPT
     *
     */
    rewind(in);
    fprintf(out,"\n\n\n");
    fprintf(out,"SSPT MainSSPT[] = {\n");

	for (	/* First system call has index zero */
		sys_call_idx = 0;
		/* Go on until EOF or read zero bytes */
		(	(!feof(in))
			&& (fgets(line, sizeof line, in) != NULL)
			);
		/* Next system call index */
		sys_call_idx++
		)
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
			/* Extract the ZwXXX name */
			name2 = (char *)strtok(NULL," \t");
			//value = strtok(NULL," \t");
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
			printf("%3d \"%s\"\n",sys_call_idx,name);
#endif

			if (sys_call_idx > 0)
			{
                fprintf(out,",\n");
			}
			/*
             * Now write the current system call's ID
             * in the service table along with its Parameters Size.
			 */
             fprintf(out,"\t\t{ %s }",nr_args);

 		   }
    }
    /* Close the service table (C syntax) */
    fprintf(out,"\n};\n");

    // We write some useful defines
    fprintf(out, "\n\n#define MIN_SYSCALL_NUMBER    0\n");
    fprintf(out, "#define MAX_SYSCALL_NUMBER    %d\n", sys_call_idx-1);
    fprintf(out, "#define NUMBER_OF_SYSCALLS    %d\n", sys_call_idx);
    fprintf(out, "#define SSDTSHADOW_MAX_ENTRIES    2\n");

    // Now write the KeServiceDescriptorTable
    fprintf(out, "\n\n// These Service Descriptor Table its exported by NTOSKRNL and can be used\n");
    fprintf(out, "// by Device Drivers to hook system services\n");
    fprintf(out, "\n\nKE_SERVICE_DESCRIPTOR_TABLE_ENTRY KeServiceDescriptorTable = {\n"
                            "\t\tMainSSDT,\n"
                            "\t\tNULL,\n"
                            "\t\tNUMBER_OF_SYSCALLS,\n"
                            "\t\tMainSSPT\n\t\t};\n");

   // Now write the KeServiceDescriptorTableShadow
   // This would be used when win32k.sys gets implemented
   fprintf(out, "\n\n// These Service Descriptor Table will be used by the win32k.sys driver\n");
   fprintf(out, "\n\nKE_SERVICE_DESCRIPTOR_TABLE_ENTRY KeServiceDescriptorTableShadow[SSDTSHADOW_MAX_ENTRIES] = {\n"
                            "\t\t{ MainSSDT, NULL, NUMBER_OF_SYSCALLS, MainSSPT },\n"
                            "\t\t{ NULL,    NULL,   0,   NULL   }\n"
                            "};\n");



    return(0);


}


int
process(
	FILE	* in,
	FILE	* out,
	FILE	* out2,
	FILE	* out3
	)
{
	char		line [INPUT_BUFFER_SIZE];
	char		* s;
	char		* name;		/* NtXXX name */
	char		* name2;	/* ZwXXX name */
	int		sys_call_idx;	/* NtXXX index number in the service table */
	char		* nr_args;	/* stack_size / machine_word_size */
	char		* stmp;

	/*
	 * NTDLL stubs file header
	 */
	fprintf(out,"// Machine generated, don't edit\n");
	fprintf(out,"\n\n");
	/*
	 * Service table header
	 */
    //Hfprintf(out2,"// Machine generated, don't edit\n");
    //Hfprintf(out2,"\n\n");

	//fprintf(out2,"#include <ntddk.h>");

    //H fprintf(out2,"\n\n\n");
    //H fprintf(out2,"SERVICE_TABLE _SystemServiceTable[256] = {\n");

 	/*
	 * NTOSKRNL Zw functions stubs header
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
	for (	/* First system call has index zero */
		sys_call_idx = 0;
		/* Go on until EOF or read zero bytes */
		(	(!feof(in))
			&& (fgets(line, sizeof line, in) != NULL)
			);
		/* Next system call index */
		sys_call_idx++
		)
	{
		//fgets(line,255,in);
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
			/* Extract the ZwXXX name */
			name2 = (char *)strtok(NULL," \t");
			//value = strtok(NULL," \t");
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
			printf("%3d \"%s\"\n",sys_call_idx,name);
#endif
			/*
			 * Write the NTDLL stub for the current
			 * system call: NtXXX and ZwXXX symbols
			 * are aliases.
			 */
#ifdef PARAMETERIZED_LIBS
			fprintf(out,"__asm__(\"\\n\\t.global _%s@%s\\n\\t\"\n",name,nr_args);
			fprintf(out,"\".global _%s@%s\\n\\t\"\n",name2,nr_args);
			fprintf(out,"\"_%s@%s:\\n\\t\"\n",name,nr_args);
			fprintf(out,"\"_%s@%s:\\n\\t\"\n",name2,nr_args);
#else
			fprintf(out,"__asm__(\"\\n\\t.global _%s\\n\\t\"\n",name);
			fprintf(out,"\".global _%s\\n\\t\"\n",name2);
			fprintf(out,"\"_%s:\\n\\t\"\n",name);
			fprintf(out,"\"_%s:\\n\\t\"\n",name2);
#endif
			fprintf(out,"\t\"mov\t$%d,%%eax\\n\\t\"\n",sys_call_idx);
			fprintf(out,"\t\"lea\t4(%%esp),%%edx\\n\\t\"\n");
			fprintf(out,"\t\"int\t$0x2E\\n\\t\"\n");
			fprintf(out,"\t\"ret\t$%s\\n\\t\");\n\n",nr_args);

            /*
			 * Mark the end of the previous service
			 * table's element (C array syntax). When
			 * the current function is the n-th, we
			 * close the (n - 1)-th entry.
			 */
            //Hif (sys_call_idx > 0)
            //H{
            //H    fprintf(out2,",\n");
            //H}

			/*
			 * Now write the current system call's name
			 * in the service table.
			 */
            //Hfprintf(out2,"\t\t{ %s, (ULONG)%s }",nr_args,name);

            /*
			 * Now write the NTOSKRNL stub for the
			 * current system call. ZwXXX does NOT
			 * alias the corresponding NtXXX call.
			 */
			fprintf(out3,"__asm__(\n");
			fprintf(out3,"\".global _%s@%s\\n\\t\"\n",name2,nr_args);
			fprintf(out3,"\"_%s@%s:\\n\\t\"\n",name2,nr_args);
			fprintf(out3,"\t\"mov\t$%d,%%eax\\n\\t\"\n",sys_call_idx);
			fprintf(out3,"\t\"lea\t4(%%esp),%%edx\\n\\t\"\n");
			fprintf(out3,"\t\"int\t$0x2E\\n\\t\"\n");
			fprintf(out3,"\t\"ret\t$%s\\n\\t\");\n\n",nr_args);
		}
	}
	/* Close the service table (C syntax) */
    //Hfprintf(out2,"\n};\n");

	return(0);
}

void usage(char * argv0)
{
	printf("\
Usage: %s sysfuncs.lst napi.c napi.h zw.c\n\
  sysfuncs.lst  system functions database\n\
  napi.c        NTDLL stubs\n\
  napi.h        NTOSKRNL service table\n\
  zw.c          NTOSKRNL Zw stubs\n",
		argv0
		);
}

int main(int argc, char* argv[])
{
	FILE	* in;	/* System calls database */
	FILE	* out;	/* NTDLL stubs */
	FILE	* out2;	/* SERVICE_TABLE */
	FILE	* out3;	/* NTOSKRNL Zw stubs */
	int	ret;

	if (argc != 5)
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

	out = fopen(argv[2],"wb");
	if (out == NULL)
	{
		perror("Failed to open output file (NTDLL stubs)");
		return(1);
	}


	out2 = fopen(argv[3],"wb");
	if (out2 == NULL)
	{
		perror("Failed to open output file (NTOSKRNL service table)");
		return(1);
	}

	out3 = fopen(argv[4],"wb");
	if (out3 == NULL)
	{
		perror("Failed to open output file (NTOSKRNL Zw stubs)");
		return(1);
	}

	ret = process(in,out,out2,out3);
    rewind(in);
    ret = makeSystemServiceTable(in, out2);

	fclose(in);
	fclose(out);
	fclose(out2);
	fclose(out3);

	return(ret);
}
