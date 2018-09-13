/*++
  
  Copyright (c) 1995 Intel Corp
  
  File Name:
  
    stack.h
  
  Abstract:
  
    Implements stack structure.
  
  Author:
    
    Mark Hamilton
  
--*/

#ifndef _STACKH_
#define _STACKH_

#include "nowarn.h"  /* turn off benign warnings */
#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_   /* Prevent inclusion of winsock.h in windows.h */
#endif
#include <windows.h>
#include "nowarn.h"  /* some warnings may have been turned back on */
#include "nideque.h"

// Name:     Stack_c   
// Purpose:  A general purpose stack.                                   
// Context:  Can be used anywhere.
template<class T> class Stack_c : private NIDeque_c<T> {

    public: 
                Stack_c() : NIDeque_c<T>() {}
        	    ~Stack_c()       {}
        inline BOOL Push(T Data) {return NIDeque_c<T>::InsertIntoFront(Data);}
        inline BOOL	Pop(T &Data) {return
                                  NIDeque_c<T>::RemoveFromFront(Data);}     
	    inline BOOL IsEmpty()    {return NIDeque_c<T>::IsEmpty();}
};

#endif
