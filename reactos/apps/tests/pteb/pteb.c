#include <stdio.h>


int main(int argc, char* argv[])
{  
   int x;
   
   printf("TEB dumpper\n");
   __asm__("movl %%fs:0x18, %0\n\t"
	   : "=a" (x)
	   : /* no inputs */);
   printf("fs[0x18] %x\n", x);
   return(0);
}
