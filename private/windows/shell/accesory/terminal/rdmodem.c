/*===========================================================================*/
/*          Copyright (c) 1987 - 1988, Future Soft Engineering, Inc.         */
/*                              Houston, Texas                               */
/*===========================================================================*/

#define NOLSTRING    TRUE  /* jtf win3 mod */
#include <windows.h>
#include "port1632.h"
#include "dcrc.h"
#include "dynacomm.h"
#include "task.h"


BREAKCOND breakcondition_rdModem;
                        

/*---------------------------------------------------------------------------*/
/* rdModem() - Read characters from system modem into local IT buffer. [scf] */
/*---------------------------------------------------------------------------*/

VOID rdModem(BOOL rFlag)
{
   INT  i;
   INT  yieldCount;
   MSG  msg;

   if(modemBytes())
   {
      if(answerMode)
      {
         theChar = getMdmChar(TRUE);         /* mbbx 1.06A: ics new xlate */
         return;
      }

      later = FALSE;
      hideTermCursor();
      yieldCount = 0;
      getPort();

      repeat
      {
         theChar = (the8Char = getMdmChar(TRUE));  /* mbbx 1.06A: ics new xlate... */

         if(rFlag)
         {
            modemInp(the8Char, TRUE);        /* mbbx: make this optional ??? */

            if(mdmResult[0] < MDMRESLEN)
               mdmResult[++(*mdmResult)] = theChar;
            if(theChar != LF)
            {
               if(mdmResult[0] == 1)
                  mdmResult[0] = 0;          /* throw it away!!! */
            }
            else if(mdmResult[0] > 2)
               mdmValid = TRUE;
            later = TRUE;
         }
         else
         {
            if(xferWaitEcho)
               if(xferTxtType == XFRLINE)    /* (jtfx) */
               {
                  if(theChar == xferLinStr[xferWaitEcho])
                     if(xferWaitEcho == xferLinStr[0])
                        xferWaitEcho = FALSE;
                     else
                        xferWaitEcho++;
               }
               else if((xferTxtType == XFRCHAR) && (theChar == xferCharEcho))    /* (jtfx) */
                  xferWaitEcho = FALSE;

            modemInp(the8Char, TRUE);        /* mbbx 1.10 */
            if(theChar == CR)
               if(trmParams.inpCRLF)         /* mbbx 1.10: CUA */
                  modemInp(LF, TRUE);

            if(!modemBytes())
               later = TRUE;

            if(++yieldCount == YIELDCHARS)
            {
               termCleanUp();                /* mbbx: per mac version */
               yieldCount = 0;
               if(yield((LPMSG) &msg, (HWND) NULL))   /* mbbx */
                  later = TRUE;
            }
         }
      } until(later);

      activSelect = FALSE;    /* rjs bugs 020 */
      termCleanUp();
      showTermCursor();
      releasePort();
   }
}


