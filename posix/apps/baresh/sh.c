/* $Id: sh.c,v 1.1 2002/01/20 21:22:29 ea Exp $
 *
 * baresh - Bare Shell for the PSX subsystem.
 * Copyright (c) 2002 Emanuele Aliberti
 * License: GNU GPL v2
 */
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>

#define INPUT_BUFFER_SIZE 128

int run=1;

void cmd_exit(char*buf)
{
  run=0;
}

void cmd_pwd(char * buf)
{
  char pwd[1024];

  getcwd(pwd,sizeof pwd);
  printf("%s\n",pwd);
}

void cmd_ls(char*buf)
{
  char pwd[1024];
  DIR * dir;
  struct dirent * entry;

  getcwd(pwd,sizeof pwd);
  dir=opendir(pwd);
  while (NULL!=(entry=readdir(dir)))
  {
    printf("%s\n",entry->d_name);
  }
  closedir(dir);
}

int main(int argc,char*argv[])
{
  char buf[INPUT_BUFFER_SIZE];

  while (run)
  {
    printf("# ");
    if (gets(buf))
    {
      if (!strcmp("exit",buf))    cmd_exit(buf);
      else if (!strcmp("pwd",buf)) cmd_pwd(buf);
      else if (!strcmp("ls",buf)) cmd_ls(buf);
      else printf("%s: unknown command\n",argv[0]);
    }
  }
  return 0;
}
/* EOF */
