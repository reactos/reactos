/* $Id: devices.cpp,v 1.3 2002/09/04 22:19:47 robertk Exp $
*/
/*
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS OS/2 sub system
 * PART:			 doscalls.dll
 * FILE:             devices.cpp
 * CONTAINS:		 device io ctl main function.
 * PURPOSE:          Kernelservices for OS/2 apps
 * PROGRAMMER:       Robert K. nonvolatil@yahoo.de
 * REVISION HISTORY:
 *  13-03-2002  Created
 *	25-07-2002	Work to make it compile	
 *	10-11-2002	Done som little things
 */

#define INCL_DOSDEVICES
#define INCL_DOSERRORS
#include "ros2.h"


/*******************************************/
/* DosDevIOCtl performs control functions  */
/* on a device specified by an opened      */
/* device handle.                          */
/*******************************************/
/*HFILE     hDevice;         Device handle returned by DosOpen, or a standard (open) device handle. */
/*ULONG     category;        Device category. */
/*ULONG     function;        Device-specific function code. */
/*PVOID     pParams;         Address of the command-specific argument list. */
/*ULONG     cbParmLenMax;    Length, in bytes, of pParams. */
/*PULONG    pcbParmLen;      Pointer to the length of parameters. */
/*PVOID     pData;           Address of the data area. */
/*ULONG     cbDataLenMax;    Length, in bytes, of pData. */
/*PULONG    pcbDataLen;      Pointer to the length of data. */
/*APIRET    ulrc;            Return Code. 
 
 ulrc (APIRET) - returns 
    Return Code. 

    DosDevIOCtl returns one of the following values: 

      0         NO_ERROR 
      1         ERROR_INVALID_FUNCTION 
      6         ERROR_INVALID_HANDLE 
      15        ERROR_INVALID_DRIVE 
      31        ERROR_GEN_FAILURE 
      87        ERROR_INVALID_PARAMETER 
      111       ERROR_BUFFER_OVERFLOW 
      115       ERROR_PROTECTION_VIOLATION 
      117       ERROR_INVALID_CATEGORY 
      119       ERROR_BAD_DRIVER_LEVEL 
      163       ERROR_UNCERTAIN_MEDIA 
      165       ERROR_MONITORS_NOT_SUPPORTED 
 
*/
APIRET STDCALL Dos32DevIOCtl(HFILE hDevice, ULONG category, ULONG function,
        PVOID pParams,ULONG cbParmLenMax,PULONG pcbParmLen,
        PVOID pData,ULONG cbDataLenMax,PULONG pcbDataLen)
{
	return ERROR_CALL_NOT_IMPLEMENTED;
}



/* EOF */