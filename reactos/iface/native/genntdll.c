/*
 * COPYRIGHT:             See COPYING in the top level directory
 * PROJECT:               ReactOS version of ntdll
 * FILE:                  iface/native/genntdll.c
 * PURPOSE:               Generates the system call stubs in ntdll
 * CHANGE HISTORY:	  Added a '@xx' to deal with stdcall [ Ariadne ]
 */

/* INCLUDE ******************************************************************/

#include <stdio.h>
#include <stdlib.h>

#define TRUE  1
#define FALSE 0

/* FUNCTIONS ****************************************************************/

int process(FILE* in, FILE* out, FILE *out2)
{
   char line[255];
   char* s;
   char* name;
   char* name2;
   int value;
   char* nr_args;

   unsigned char first1 = TRUE;
   
   fprintf(out,"; Machine generated, don't edit\n");
   fprintf(out,"\n\n");
   fprintf(out,"SECTION .text\n\n");
   fprintf(out,"BITS 32\n\n");
   
   fprintf(out2,"// Machine generated, don't edit\n");
   fprintf(out2,"\n\n");
   //fprintf(out2,"#include <ntddk.h>");
   fprintf(out2,"\n\n\n");
   fprintf(out2,"SERVICE_TABLE _SystemServiceTable[256] = {\n");
 

   value = 0;
   while (!feof(in) && fgets(line,255,in) != NULL)
     {
//	fgets(line,255,in);
	if ((s=(char *)strchr(line,13))!=NULL)
	  {
	     *s=0;
	  }
	s=&line[0];
	if ((*s)!='#' && (*s) != 0)
	  {
	     name = (char *)strtok(s," \t");
	     name2 = (char *)strtok(NULL," \t");
//	     value = strtok(NULL," \t");
	     nr_args = (char *)strtok(NULL," \t");
	     
//	     printf("name %s value %d\n",name,value);
	     fprintf(out,"GLOBAL _%s@%s\n",name,nr_args);
	     fprintf(out,"GLOBAL _%s@%s\n",name2,nr_args);
	     fprintf(out,"_%s@%s:\n",name,nr_args);
	     fprintf(out,"_%s@%s:\n",name2,nr_args);
	     fprintf(out,"\tmov\teax,%d\n",value);
	     fprintf(out,"\tlea\tedx,[esp+4]\n");
	     fprintf(out,"\tint\t2Eh\n");
	     fprintf(out,"\tret\t%s\n\n",nr_args);
		 

	     value++;
	     
	     if ( first1 == TRUE ) 
			first1 = FALSE;
	     else
			fprintf(out2,",\n");

	     fprintf(out2,"\t\t{ %s, (ULONG)%s }",nr_args,name);
		 
	  }
     }

     fprintf(out2,"\n};\n");
	 
   return(0);
}

void usage(void)
{
   printf("Usage: genntdll sysfuncs.lst outfile.asm outfile.h\n");
}

int main(int argc, char* argv[])
{
   FILE* in;
   FILE* out;
   FILE *out2;
   int ret;
   
   if (argc!=4)
     {
	usage();
	return(1);
     }
   
   in = fopen(argv[1],"rb");
   if (in==NULL)
     {
	perror("Failed to open input file");
	return(1);
     }
   
   out = fopen(argv[2],"wb");
   if (out==NULL)
     {
	perror("Failed to open output file");
	return(1);
     }
   out2 = fopen(argv[3],"wb");
   if (out2==NULL)
     {
	perror("Failed to open output file");
	return(1);
     }
   
   ret = process(in,out,out2);
   
   fclose(in);
   fclose(out);
   fclose(out2);
   
   return(ret);
}
