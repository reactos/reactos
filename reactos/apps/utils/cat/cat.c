#include <stdlib.h>
#include <stdio.h>

int main(int argc, char* argv[])
{
   int i;
   FILE* in;
   char ch;
   
   for (i=1; i<argc; i++)
     {
	in = fopen(argv[i],"r");
	if (in == NULL)
	  {
	     printf("Failed to open file %s\n", argv[i]);
	     return(0);
	  }
	
	while ((ch = fgetc(in)) != EOF)
	  {
	     putchar(ch);
	  }
	fclose(in);
     }
   return 0;
}
