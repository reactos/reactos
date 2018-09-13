/************************************************************************
 *                                                                      *
 *  Copyright (c) 1991                                                  *
 *  by Future Soft Engineering, Inc.,  Houston, Texas                   *
 *                                                                      *
 *  The information in this software  is  subject  to  change  without  *
 *  notice  and  should not be construed as a commitment by Future Soft *
 *  Engineering Incorporated.                                           *
 *                                                                      *
 ************************************************************************
 * 
 * ABSTRACT:   About dialog box processing
 *
 * AUTHOR:     BJW
 *
 * CREATION DATE: 91/10/05
 *
 * REVISION HISTORY: $Log:	asm2c_32.c $
 * Revision 1.1  92/04/06  16:10:25  rjs
 * Initial revision
 * 
 *
 */

#include "winrev.h"
#include <windows.h>
#include "port1632.h"
#include <dos.h>
#include <stdlib.h>
#include "dcrc.h"
#include "dynacomm.h"                  // dwb KtoA   STRING + ?
#include "fileopen.h"                   // dwb KtoA

LONG fileLength(INT hFile)
{
   LONG lPointerNow;
   LONG lFileLength;

   lPointerNow = _llseek(hFile, 0L, 1);
   lFileLength = _llseek(hFile, 0L, 2);

   _llseek(hFile, lPointerNow, 0);

   return(lFileLength);
}


