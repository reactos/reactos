/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include <pseh/pseh.h>

int test(int x)
{
    return x+1;
}

void execute(char* message, int(*func)(int))
{
   ULONG status = 0;
   ULONG result;

   printf("%s ... ", message);

   _SEH_TRY
   {
      result = func(1);
   }
   _SEH_HANDLE
   {
      status = _SEH_GetExceptionCode();
   }
   _SEH_END;
   if (status == 0)
   {
       printf("OK.\n");
   }
   else
   {
       printf("Error, status=%lx.\n", status);
   }
}

char data[100];

int main(void)
{
    unsigned char stack[100];
    void* heap;
    ULONG protection;

    printf("NoExecute\n");

    execute("Executing within the code segment", test);
    memcpy(data, test, 100);
    execute("Executing within the data segment", (int(*)(int))data);
    memcpy(stack, test, 100);
    execute("Executing on stack segment", (int(*)(int))stack);
    heap = VirtualAlloc(NULL, 100, MEM_COMMIT, PAGE_READWRITE);
    memcpy(heap, test, 100);
    execute("Executing on the heap with protection PAGE_READWRITE", (int(*)(int))heap);
    VirtualProtect(heap, 100, PAGE_EXECUTE, &protection);
    execute("Executing on the heap with protection PAGE_EXECUTE", (int(*)(int))heap);

    return 0;
}
