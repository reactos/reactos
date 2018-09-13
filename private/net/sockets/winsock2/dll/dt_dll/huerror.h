/*++
  
  Copyright (c) 1995 Intel Corp
  
  File Name:
  
    huerror.cpp
  
  Abstract:
  
    Header file for error functions.
  
  Author:
    
    Mark Hamilton
  
--*/

#ifndef _ERRORH_
#define _ERRORH_

#include <stdarg.h>

const int ERRORSTART = 0x100;

typedef enum {  ENONE     = ERRORSTART,
                OTHERERROR,
                ALLOCERROR,
                INVALIDARG,
                OBJNOTINIT,
		OBJEFFERROR,  // Object effeciency error.
		ALREADYCONN,
		ALREADYLIST
             } ErrorCode_e;

void        HUSetLastError(ErrorCode_e ErrorCode);
ErrorCode_e HUGetLastError();
void        HUPrintError(char *func,ErrorCode_e ErrorCode = ENONE);

#endif
