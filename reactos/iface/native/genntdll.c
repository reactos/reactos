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
#include <string.h>

#define TRUE  1
#define FALSE 0

#define PARAMETERIZED_LIBS

/* FUNCTIONS ****************************************************************/

int process(FILE* in, FILE* out, FILE *out2)
{
   char line[255];
   char* s;
   char* name;
   char* name2;
   int value;
   char* nr_args;
   char* stmp;
   
   unsigned char first1 = TRUE;
   
   fprintf(out,"// Machine generated, don't edit\n");
   fprintf(out,"\n\n");
   
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
	     
	     if ((stmp=strchr(nr_args,'\n'))!=NULL)
	       {
		  *stmp=0;
	       }
	     
//	     printf("name %s value %d\n",name,value);
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
         fprintf(out,"\t\"mov\t$%d,%%eax\\n\\t\"\n",value);
         fprintf(out,"\t\"lea\t4(%%esp),%%edx\\n\\t\"\n");
         fprintf(out,"\t\"int\t$0x2E\\n\\t\"\n");
         fprintf(out,"\t\"ret\t$%s\\n\\t\");\n\n",nr_args);
		 

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
