#include <stdio.h>
#include <string.h>

int main()
{
   char msg1[] = "testing _write\n";
   char msg2[] = "testing putchar.";
   char msg3[] = "testing printf.";
   char tmpbuf[255];
   FILE* f1;
   
   write(1, msg1, strlen(msg1));
   
   write(1, msg2, strlen(msg2));
   putchar('o'); putchar('k'); putchar('\n');
   
   write(1, msg3, strlen(msg3));
   printf("ok\n");
   
   printf("Testing fopen\n");
   f1 = fopen("tmp.txt","w+b");
   if (f1 == NULL)
     {
	printf("fopen failed\n");
	return(1);
     }
   
   printf("Testing fwrite\n");
   if (fwrite(msg1, 1, strlen(msg1)+1, f1) != (strlen(msg1)+1))
     {
	printf("fwrite failed\n");
	return(1);
     }
   
   printf("Testing fread\n");
   fseek(f1, 0, SEEK_SET);
   if (fread(tmpbuf, 1, strlen(msg1)+1, f1) != (strlen(msg1)+1))
     {
	printf("fread failed\n");
	return(1);
     }
   if (strcmp(tmpbuf,msg1) != 0)
     {
	printf("fread failed, data corrupt\n");
	return(1);
     }
   
   printf("Testing fclose\n");
   if (fclose(f1) != 0)
     {
	printf("fclose failed\n");
	return(1);
     }
   return(0);
}
