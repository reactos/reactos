#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

extern FILE* yyin;
extern int yyparse(void);
extern int yydebug;
extern int nr_errors;
extern char* current_file;

int main(int argc, char* argv[])
{
   int fd;
   char* tempname;
   int n, p, status;
   char* args[9];
   
   if (argc == 1)
     {
	printf("Not enough arguments\n");
	exit(1);
     }
   
   tempname = tempnam(NULL, "midl");
   
   args[0] = strdup("/usr/bin/gcc");
   args[1] = strdup("-x");
   args[2] = strdup("c");
   args[3] = strdup("-P");
   args[4] = strdup("-E");
   args[5] = strdup(argv[1]);
   args[6] = strdup("-o");
   args[7] = strdup(tempname);
   args[8] = NULL;
   
   if ((n = fork()) == 0)
     {
	execv("/usr/bin/gcc", args);
	perror("gcc");
	exit(1);
     }
   else if (n == -1)
     {
	perror("midl");
	exit(1);
     }
   
   p = waitpid(n, &status, WUNTRACED);
   if (p == -1 || p == 0 || !WIFEXITED(status))
     {
	perror("midl");
	exit(1);
     }
   if (WEXITSTATUS(status) != 0)
     {
	printf("midl: the preprocessor %s failed\n");
	exit(1);
     }
   
//   yydebug = 1;
   
   yyin = fopen(tempname, "r+b");
   if (yyin == NULL)
     {
	perror(argv[1]);
	exit(1);
     }
   
   current_file = strdup(argv[1]);
   
   if (yyparse() != 0 || nr_errors > 0)
     {
	exit(1);
     }
   
   unlink(tempname);
}
