#include <stdarg.h>
#include <windows.h>

HANDLE stdin;
HANDLE stdout;


void Console_puts(char* str)
{
        ULONG nchar;

        WriteConsole(stdout,
                     str,
                     strlen(str),
                     &nchar,
                     NULL);
}

void Console_printf(char* fmt, ...)
{
        char buffer[255];
        va_list vargs;

        va_start(vargs,fmt);
        vsprintf(buffer,fmt,vargs);
        Console_puts(buffer);
        va_end(vargs);
}

void Console_getline(PCH Prompt, PCH Output, DWORD OutputLength)
{
        char ch;
        DWORD nbytes;

        Console_puts(Prompt);

        ReadConsole(stdin,
                    Output,
                    OutputLength,
                    &nbytes,
                    NULL);
        Output[nbytes-2]=0;
}

void func_cd(char* s)
{
        Console_printf("Changing directory to %s\n",s);
        if (!SetCurrentDirectory(s))
        {
                Console_puts("Failed to change to directory\n");
        }
}

void func_dir(char* s)
{
        HANDLE shandle;
        WIN32_FIND_DATA FindData;

        shandle = FindFirstFile("*.*",&FindData);

        if (shandle==INVALID_HANDLE_VALUE)
        {
                return;
        }
        do
        {
                Console_printf("Scanning %s\n",FindData.cFileName);
        } while(FindNextFile(shandle,&FindData));
}

int is_builtin(char* name, char* args)
{
        if (strcmp(name,"dir")==0)
        {
                func_dir(args);
                return(1);
        }
        if (strcmp(name,"cd")==0)
        {
                func_cd(args);
                return(1);
        }
        return(0);
}

int process_command(char* str)
{
        char* name;
        char* args;
        PROCESS_INFORMATION pi;
        STARTUPINFO si;
        char process_arg[255];

        if (strcmp(str,"exit")==0)
        {
                return(1);
        }

        name = strtok(str," \t");
        args = strtok(NULL,"");

        if (is_builtin(name,args))
        {
                return(0);
        }
        memset(&si,0,sizeof(STARTUPINFO));
        si.cb=sizeof(STARTUPINFO);
        si.lpTitle=strdup(name);

        strcpy(process_arg,name);
        strcat(process_arg," ");
        if(args!=NULL)
        {
                strcat(process_arg,args);
        }
        Console_printf("name '%s' process_arg '%s'\n",name,process_arg);
        if (!CreateProcess(NULL,process_arg,NULL,NULL,FALSE,
                     CREATE_NEW_CONSOLE,
                      NULL,NULL,&si,&pi))
                      {
                        Console_printf("Failed to execute process\n");
                      }
        return(0);
}

void build_prompt(char* prompt)
{
        int len;

        len = GetCurrentDirectory(255,prompt);
        strcat(prompt,">");
}

void command_loop()
{
        char line[255];
        char prompt[255];
        int do_exit = 0;

        while (!do_exit)
        {
                build_prompt(prompt);
                Console_getline(prompt,line,255);
                Console_printf("Processing command '%s'\n",line);
                do_exit = process_command(line);
        }        
}

int STDCALL WinMain (HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmd, int nShow)
{
        AllocConsole();
        stdin = GetStdHandle(STD_INPUT_HANDLE);
        stdout = GetStdHandle(STD_OUTPUT_HANDLE);

        command_loop();

	return 0;
}
