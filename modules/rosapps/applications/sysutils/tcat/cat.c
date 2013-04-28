/*
 * FILE       : cat.c
 * NATIVE NAME: tcat "tappak's cat" :)
 * AUTHOR     : Semyon Novikov (tappak)
 * PROJECT    : ReactOS Operating System
 * DESCRIPTION: file concatenation tool
 * DATE       : 2004-01-21
 * LICENSE    : GPL
 */

#include <stdio.h>
#include <string.h>
#define F_O_ERR "can not open file"

void help(void)
{
 puts("File concatenation tool");
 puts("Usage: cat [file]");
}

int main(int argc, char *argv[])
{
 FILE *srcf;
 char *keys[]={"--help","/help"};
 int i=0,ret=0;
 switch(argc)
  {
    case 1:puts("Usage: cat [file]");break;
    case 2:
     if ((!strcmp(argv[1],keys[0]))||(!strcmp(argv[1],keys[1])))
      help();
     else
      {
      if((srcf=fopen(argv[1],"r"))!=NULL)
      {
       while(i!=EOF)
        { i=fgetc(srcf);
          putchar(i);
        }
       fclose(srcf);
      }
      else
      {
       printf("%s %s %s\n",argv[0],F_O_ERR,argv[1]);
       ret=-1;
      }
     }
    break;
   }
 return ret;
}

