/* Copyright (C) 2000 CW Sandmann (sandmann@clio.rice.edu) 1206 Braelinn, Sugar Land, TX 77479 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#ifdef GO32
#include <unistd.h>
#else
#include <io.h>
#endif

char view_only = 0;
const char *client_patch_code;
char buffer[20480];
unsigned long search_base = 0x4c800L;
int f;

char oldpatch[] = {0x3b, 0x05, 0xac, 0xe6 };
char newpatch[] = {0x3b, 0x05, 0x58, 0x5e };

void patch_image(char *filename)
{
  int i,size;
  
  view_only = 0;
  f = open(filename, O_RDWR | O_BINARY);
  if (f < 0) {
    f = open(filename, O_RDONLY | O_BINARY);
    if (f < 0) {
      perror(filename);
      return;
    }
    view_only = 1; 
  }

  lseek(f, search_base, SEEK_SET);
  size = read(f, buffer, sizeof(buffer));

  client_patch_code = NULL;
  for(i=0; i<size && !client_patch_code; i++)
    if(!memcmp(buffer+i,oldpatch,sizeof(oldpatch)))
      client_patch_code = (buffer+i);

  if(!client_patch_code) {
    printf("Old patch string not found in %s!\n",filename);
  } else {
    lseek(f, search_base+i-1, SEEK_SET);	/* Ready to update */
    if(!view_only) {
      write(f, newpatch, sizeof(newpatch));
      printf("%s patched\n",filename);
    } else
      printf("%s patchable (not changed, readonly)\n",filename);
  }
  close(f);
  return;
}

int main(int argc, char **argv)
{
  int i;
  char filename[256];
  char buf1[256];
  char file2[256];
  
  if (argc != 1) {		/* If they specify names, patch them, exit */
    for(i=1; i<argc; i++)
     patch_image(argv[i]);
    return 0;
  }

  fprintf(stderr, "This image patches Windows 2000 NTVDM to fix nesting DPMI bug.\n");

  strcpy(filename,getenv("SYSTEMROOT"));
  strcpy(file2,filename);
  strcat(filename,"\\system32\\ntvdm.exe");
  strcat(file2,"\\system32\\dllcache\\ntvdm.exe");
  
  sprintf(buf1,"copy %s %s\\system32\\ntvdm.ori",filename,getenv("SYSTEMROOT"));
  printf("%s\n",buf1);
  system(buf1);
  
  patch_image(file2);
  patch_image(filename);
  return 0;
}
