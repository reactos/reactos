/*
 * FILE       : man.c
 * NATIVE NAME: ReactOS manual browser
 * AUTHOR     : Semyon Novikov (tappak)
 * PROJECT    : ReactOS Operating System
 * DESCRIPTION: manual file browser (Use Linux man file syntax)
 * DATE       : 2004-03-29
 * LICENSE    : GPL
 */

 /*  Known issues.
  *  1. Scroll screen
  *  2. Non ENVIRONMENT manpath varrible
  *  3. At the end of man page must be ./" tag!
  */


#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <wincon.h>

#define MAXLINE 256
#define BOLD 7|9
#define ITALIC 3|9
#define NORMAL 7|0

/*===[functions]===*/
void SetCl(WORD cl);
int  OpenF(char* name);
int  CloseF(void);
void Usage(void);
int AnalyzeArgv(char *);
int AnalyzeFile();
/*=================*/

/*====[Globals]====*/
FILE* manfile;
char OpenFlag=0;
char manpath[MAXLINE]="c:\\man\\";
/*=================*/

void
SetCl(WORD cl)
{
 SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),cl);
}

int
OpenF(char* name)
{
    int retval=0;
    char *manpath_local=(char*)malloc(sizeof(char)*MAXLINE);

    strcpy(manpath_local, manpath); //save mandir value

    if((manfile=fopen((strcat(manpath_local,name)),"r"))!=NULL)
     {
      OpenFlag=1;
      AnalyzeFile();
     }
    else
     retval=-1;

 strcpy(manpath_local, manpath);
 return retval;
}

int
CloseF()
{
    int retval=0;

    if(fclose(manfile))
     OpenFlag=0;
    else retval=-1;

    return retval;
}

void
Usage()
{
    puts("usage: man [command]");
    puts("see \"man man\" for details");
}

int
AnalyzeArgv(char *argument)
{
    int element=0;
    char HelpFlag=0;
    char *keys[]={"--help","/h","/?","-h"};
    char *sections[]={".1",".2",".3",".4",".5",".6",".7",".8",".9"};
    char *filename=(char*)malloc(sizeof(char)*MAXLINE);

    strcpy(filename,argument); //save argument value

    for(element=0; element < 5;element++)
     if(!strcmp(keys[element],argument))
     {
      Usage();
      HelpFlag=1;
     }

   element = 0;

   if(!HelpFlag)

   if(OpenF(filename))
    while(OpenF(strcat(filename,sections[element])) && (element<9))
    {
     strcpy(filename,argument);
     element++;
    }

    if(element>8) printf("No manual for %s\n",argument);

    return element;
}

int
AnalyzeFile()
{
    char *cur_string=(char*)malloc(sizeof(char)*MAXLINE);
    int symbol=0;
    char *THtag=(char*)malloc(sizeof(char)*MAXLINE);


    void sh_outp(void)
     {
      putchar('\n');
      putchar('\n');
      for(symbol=3;putchar(cur_string[symbol]); symbol++);
     }

    void th_outp(void)
     {
      putchar('\n');
      putchar('\n');
      putchar('\t');
      putchar('\t');
      SetCl(ITALIC);
       for(symbol=3;putchar(THtag[symbol]); symbol++);
      putchar('\n');
      SetCl(NORMAL);
     }

    void text_outp(void)
     {
      char TagFlag=0;
      symbol=0;

      if(cur_string[0]=='.')
       while(cur_string[symbol]!=' ')
        symbol++;

     if(symbol) TagFlag=1;

     for(;cur_string[symbol]!='\n'; symbol++)
        putchar(cur_string[symbol]);
        putchar(' ');
     }

    while(fgets(cur_string,MAXLINE,manfile))

/* TAGs processing */
     if((cur_string[0]=='.')&&(cur_string[1]=='S')&&
       (cur_string[2]=='H')) // .SH tag
      {
       SetCl(BOLD);
       sh_outp();
       SetCl(NORMAL);
      }
     else
     if((cur_string[0]=='.')&&(cur_string[1]=='I')&&
       (cur_string[2]==' ')) // .I tag
      {
       SetCl(ITALIC);
       text_outp();
       SetCl(NORMAL);
      }
     else
     if((cur_string[0]=='.')&&(cur_string[1]=='/')&&
       (cur_string[2]=='\"')); // ./" tag (comment)

     else
     if((cur_string[0]=='.')&&(cur_string[1]=='T')&&
       (cur_string[2]=='H')) // .TH tag
      {
       strcpy(THtag,cur_string);
      }
     else
     if((cur_string[0]=='.')&&(cur_string[1]=='B')&&
       (cur_string[2]==' ')) // .B tag
      {
       SetCl(BOLD);
       text_outp();
       SetCl(NORMAL);
      }
      else
      if((cur_string[0]=='.')&&(cur_string[1]=='N')&&
        (cur_string[2]=='L'))
       {
        putchar('\n');
        putchar(' ');
       }

     else text_outp(); // print plane text
     th_outp();
/* END of TAGs processing */
     return 0;
}


int
main(int argc, char *argv[])
{

  switch(argc)
    {
     case 1:  Usage(); break;
     case 2:  AnalyzeArgv(argv[1]);break;
     default: Usage();break;
    }

   if(OpenFlag)CloseF();
   SetCl(NORMAL);
   return 0;
}

