/*
 * COPYRIGHT:             See COPYING in the top level directory
 * PROJECT:               ReactOS version of ntdll
 * FILE:                  lib/ntdll/genntdll.c
 * PURPOSE:               Generates the system call stubs in ntdll
 * PROGRAMMER:            David Welch (welch@mcmail.com)
 */

/* INCLUDE ******************************************************************/

#include <stdio.h>
#include <stdlib.h>

/* FUNCTIONS ****************************************************************/

int process(FILE* in, FILE* out)
{
   char line[255];
   char* s;
   char* name;
   char* value;
   char* nr_args;
   
   fprintf(out,"; Machine generated, don't edit\n");
   fprintf(out,"\n\n");
   
   while (!feof(in) && fgets(line,255,in)!=NULL)
     {
	fgets(line,255,in);
	if ((s=strchr(line,'\n'))!=NULL)
	  {
	     *s=0;
	  }
	s=&line[0];
	if ((*s)!='#')
	  {
	     name = strtok(s," \t");
	     value = strtok(NULL," \t");
	     nr_args = strtok(NULL," \t");
	     
//	     printf("name %s value %s\n",name,value);
	     
	     fprintf(out,"%s:\n",name);
	     fprintf(out,"\tmov\teax,%s\n",value);
	     fprintf(out,"\tlea\tedx,[esp+4]\n");
	     fprintf(out,"\tint\t2Eh\n");
	     fprintf(out,"\tret\t%s\n\n",nr_args);
	  }
     }
}

void usage(void)
{
   printf("Usage: genntdll infile.cll outfile.c\n");
}

int main(int argc, char* argv[])
{
   FILE* in;
   FILE* out;
   int ret;
   
   if (argc!=3)
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
   
   ret = process(in,out);
   
   fclose(in);
   fclose(out);
   
   return(ret);
}
