/*++
  
  Copyright (c) 1995 Intel Corp
  
  File Name:
  
    huerror.cpp
  
  Abstract:
  
    Error functions.
  
  Author:
    
    Mark Hamilton
  
--*/

#include "nowarn.h"  /* turn off benign warnings */
#include <stdio.h>
#include "huerror.h"

static ErrorCode_e HULastError	= ENONE;

void HUSetLastError(ErrorCode_e ErrorCode)
{
  HULastError = ErrorCode;
}

ErrorCode_e HUGetLastError()
{
  return HULastError;
}

void HUPrintError(char *func,ErrorCode_e ErrorCode)
{
  if(func == NULL){
    printf("Error: - - ");
  }else{
    printf("Error: %s - - ",func);
  }
  switch(ErrorCode){
    case ENONE:
	printf("No error\n");
	break;
    case ALLOCERROR:
	printf("Allocation error\n");
	break;
    case INVALIDARG:
	printf("Invalid arguement passed in\n");
	break;
    case OBJNOTINIT:
	printf("Object not initialized\n");
	break;
    case OBJEFFERROR:
	printf("Object becoming ineffecient. Trying making it large\n");
	break;
    case ALREADYCONN:
	printf("Already connected\n");
	break;
    default:
      printf("ErrorCode not defined\n");
  }
}
