#include <windows.h>

extern int main(int args, char* argv[], char* environ[]);

static unsigned int _argc = 0;
static char** _argv = NULL;
static char** _environ = NULL;

int mainCRTStartup(PWSTR args)
{
   int nRet;
   
//   SetUnhandledExceptionFilter(NULL);
   
//   _fpreset();
   
//   __GetMainArgs(&_argc, &_argv, &_environ, 0);
   
   nRet = main(_argc, _argv, _environ);
   
//   _cexit();
   
   ExitProcess(nRet);
}

int WinMainCRTStartup()
{
   return mainCRTStartup(NULL);
}

void __main(void)
{
}
