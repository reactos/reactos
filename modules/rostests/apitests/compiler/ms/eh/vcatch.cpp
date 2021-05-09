// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for
// full license information.

// Write to volatiles in the catch to try and verify that we have
// a correct catch frame prolog and epilog.

#include <stdio.h>
#include <malloc.h>

__declspec(align(64)) struct X
{
   X() : x(3) { }
   ~X() { }
   volatile int x;
};

void t(char * c)
{
   c[4] = 'a';
   throw 123;
}

bool f()
{
   char * buf = (char *) _alloca(10);
   X x;
   volatile bool caught = false;

   try 
   {
      t(buf);
   }
   catch(int)
   {
      caught = true;
      x.x = 2;
   }

   return caught;
}

int main()
{
   bool result = false;
   
   __try {
      result = f();
   }
   __except(1)
   {
      printf("ERROR - Unexpectedly caught an exception\n");
   }

   printf(result ? "passed\n" : "failed\n");
   return result ? 0 : -1;
}
