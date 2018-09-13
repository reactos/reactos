/*++
  
  Copyright (c) 1995 Intel Corp
  
  Module Name:
  
    cstack.h
  
  Abstract:
  
    Creates a subclass of Stack_c that can push and pop an integer
    counter onto or off a stack.
  
  Author:
    
    Michael A. Grafton 
  
--*/
#ifndef _CSTACKH_
#define _CSTACKH_

#include "nowarn.h"  /* turn off benign warnings */
#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_   /* Prevent inclusion of winsock.h in windows.h */
#endif
#include <windows.h>
#include "nowarn.h"  /* some warnings may have been turned back on */
#include "nideque.h"
#include "stack.h"

class Cstack_c : private Stack_c<int> {
    
public: 
    Cstack_c() : Stack_c<int>() { counter = 0; }
    ~Cstack_c()                 {}
    inline BOOL CPush()         {return Stack_c<int>::Push(counter++);}
    inline BOOL CPop(int &Data) {return Stack_c<int>::Pop(Data);}
    inline int  CGetCounter()   {return counter;}
    inline BOOL IsEmpty()       {return Stack_c<int>::IsEmpty();}
    
private:
    int counter;
};

#endif
