/*
 * file.c - File routines module.
 */


/* Headers
 **********/

#include "project.h"
#pragma hdrstop


/* Constants
 ************/

/* size of file comparison buffer in bytes */

#define COMP_BUF_SIZE      (16U * 1024U)


/* Module Variables
 *******************/

#pragma data_seg(DATA_SEG_SHARED)

/* lock count for file comparison buffer */

PRIVATE_DATA ULONG MulcCompBufLock = 0;

/* buffers for file comparison */

PRIVATE_DATA PBYTE MrgbyteCompBuf1 = NULL;
PRIVATE_DATA PBYTE MrgbyteCompBuf2 = NULL;

/* length of file comparison buffers in bytes */

PRIVATE_DATA UINT MucbCompBufLen = 0;

#pragma data_seg()


/****************************** Public Functions *****************************/


/*
** BeginComp()
**
** Increments file comparison buffers' lock count.
**
** Arguments:     void
**
** Returns:       TWINRESULT
**
** Side Effects:  none
*/
PUBLIC_CODE void BeginComp(void)
{
   ASSERT((MrgbyteCompBuf1 && MrgbyteCompBuf2 && MucbCompBufLen > 0) ||
          (! MrgbyteCompBuf1 && ! MrgbyteCompBuf2 && ! MucbCompBufLen));

   ASSERT(MulcCompBufLock < ULONG_MAX);
   MulcCompBufLock++;

   return;
}


/*
** EndComp()
**
** Decrements file comparison buffers' lock count.
**
** Arguments:     void
**
** Returns:       void
**
** Side Effects:  Frees file comparison buffers if lock count goes to 0.
*/
PUBLIC_CODE void EndComp(void)
{
   ASSERT((MrgbyteCompBuf1 && MrgbyteCompBuf2 && MucbCompBufLen > 0) ||
          (! MrgbyteCompBuf1 && ! MrgbyteCompBuf2 && ! MucbCompBufLen));
   ASSERT(MulcCompBufLock > 0 || (! MrgbyteCompBuf1 && ! MrgbyteCompBuf2 && ! MucbCompBufLen));

   if (EVAL(MulcCompBufLock > 0))
      MulcCompBufLock--;

   /* Are the comparison buffers still locked? */

   if (! MulcCompBufLock && MrgbyteCompBuf1 && MrgbyteCompBuf2)
   {
      /* No.  Free them. */

      FreeMemory(MrgbyteCompBuf1);
      MrgbyteCompBuf1 = NULL;

      FreeMemory(MrgbyteCompBuf2);
      MrgbyteCompBuf2 = NULL;

      TRACE_OUT((TEXT("EndComp(): Two %u byte file comparison buffers freed."),
                 MucbCompBufLen));

      MucbCompBufLen = 0;
   }

   return;
}


/*
** CompareFilesByHandle()
**
** Determines whether or not two files are the same.
**
** Arguments:     h1 - DOS file handle to first open file
**                h2 - DOS file handle to second open file
**                pbIdentical - pointer to BOOL to be filled in with value
**                               indicating whether or not the files are
**                               identical
**
** Returns:       TWINRESULT
**
** Side Effects:  Changes the position of the file pointer of each file.
*/
PUBLIC_CODE TWINRESULT CompareFilesByHandle(HANDLE h1, HANDLE h2,
                                              PBOOL pbIdentical)
{
   TWINRESULT tr;

   ASSERT(IS_VALID_HANDLE(h1, FILE));
   ASSERT(IS_VALID_HANDLE(h2, FILE));
   ASSERT(IS_VALID_WRITE_PTR(pbIdentical, BOOL));

   ASSERT((MrgbyteCompBuf1 && MrgbyteCompBuf2 && MucbCompBufLen > 0) ||
          (! MrgbyteCompBuf1 && ! MrgbyteCompBuf2 && ! MucbCompBufLen));
   ASSERT(MulcCompBufLock || (! MrgbyteCompBuf1 && ! MrgbyteCompBuf2 && ! MucbCompBufLen));

   /* Have the comparison buffers already been allocated? */

   if (MrgbyteCompBuf1)
      tr = TR_SUCCESS;
   else
   {
      /* No.  Allocate them. */

      tr = TR_OUT_OF_MEMORY;

      if (AllocateMemory(COMP_BUF_SIZE, &MrgbyteCompBuf1))
      {
         if (AllocateMemory(COMP_BUF_SIZE, &MrgbyteCompBuf2))
         {
            /* Success! */

            MucbCompBufLen = COMP_BUF_SIZE;
            tr = TR_SUCCESS;

            TRACE_OUT((TEXT("CompareFilesByHandle(): Two %u byte file comparison buffers allocated."),
                       MucbCompBufLen));
         }
         else
         {
            FreeMemory(MrgbyteCompBuf1);
            MrgbyteCompBuf1 = NULL;
         }
      }
   }

   if (tr == TR_SUCCESS)
   {
      DWORD dwcbLen1;

      BeginComp();

      /* Get file lengths to compare. */

      tr = TR_SRC_READ_FAILED;

      dwcbLen1 = SetFilePointer(h1, 0, NULL, FILE_END);

      if (dwcbLen1 != INVALID_SEEK_POSITION)
      {
         DWORD dwcbLen2;

         dwcbLen2 = SetFilePointer(h2, 0, NULL, FILE_END);

         if (dwcbLen2 != INVALID_SEEK_POSITION)
         {
            /* Are the files the same length? */

            if (dwcbLen1 == dwcbLen2)
            {
               /* Yes.  Move to the beginning of the files. */

               if (SetFilePointer(h1, 0, NULL, FILE_BEGIN) != INVALID_SEEK_POSITION)
               {
                  if (SetFilePointer(h2, 0, NULL, FILE_BEGIN) != INVALID_SEEK_POSITION)
                  {
                     tr = TR_SUCCESS;

                     do
                     {
                        DWORD dwcbRead1;

                        if (ReadFile(h1, MrgbyteCompBuf1, MucbCompBufLen, &dwcbRead1, NULL))
                        {
                           DWORD dwcbRead2;

                           if (ReadFile(h2, MrgbyteCompBuf2, MucbCompBufLen, &dwcbRead2, NULL))
                           {
                              if (dwcbRead1 == dwcbRead2)
                              {
                                 /* At EOF? */

                                 if (! dwcbRead1)
                                 {
                                    /* Yes. */

                                    *pbIdentical = TRUE;
                                    break;
                                 }
                                 else if (MyMemComp(MrgbyteCompBuf1, MrgbyteCompBuf2, dwcbRead1) != CR_EQUAL)
                                 {
                                    /* Yes. */

                                    *pbIdentical = FALSE;
                                    break;
                                 }
                              }
                              else
                                 tr = TR_SRC_READ_FAILED;
                           }
                           else
                              tr = TR_SRC_READ_FAILED;
                        }
                        else
                           tr = TR_SRC_READ_FAILED;
                     } while (tr == TR_SUCCESS);
                  }
               }
            }
            else
            {
               /* No.  Files different lengths. */

               *pbIdentical = FALSE;

               tr = TR_SUCCESS;
            }
         }
      }

      EndComp();
   }

   return(tr);
}


/*
** CompareFilesByName()
**
**
**
** Arguments:
**
** Returns:       TWINRESULT
**
** Side Effects:  none
*/
PUBLIC_CODE TWINRESULT CompareFilesByName(HPATH hpath1, HPATH hpath2,
                                          PBOOL pbIdentical)
{
   TWINRESULT tr;

   ASSERT(IS_VALID_HANDLE(hpath1, PATH));
   ASSERT(IS_VALID_HANDLE(hpath2, PATH));
   ASSERT(IS_VALID_WRITE_PTR(pbIdentical, BOOL));

   /* Only verify source and destination volumes once up front. */

   if (IsPathVolumeAvailable(hpath1) &&
       IsPathVolumeAvailable(hpath2))
   {
      HANDLE h1;
      TCHAR rgchFile1[MAX_PATH_LEN];

      /* Try to open files.  Assume sequential reads. */

      GetPathString(hpath1, 0, rgchFile1);

      h1 = CreateFile(rgchFile1, GENERIC_READ, FILE_SHARE_READ, NULL,
                      OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);

      if (h1 != INVALID_HANDLE_VALUE)
      {
         HANDLE h2;
         TCHAR rgchFile2[MAX_PATH_LEN];

         GetPathString(hpath2, 0, rgchFile2);

         h2 = CreateFile(rgchFile2, GENERIC_READ, FILE_SHARE_READ, NULL,
                         OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);

         if (h2 != INVALID_HANDLE_VALUE)
         {
            TRACE_OUT((TEXT("CompareFilesByHandle(): Comparing files %s and %s."),
                       DebugGetPathString(hpath1),
                       DebugGetPathString(hpath2)));

            tr = CompareFilesByHandle(h1, h2, pbIdentical);

#ifdef DEBUG

            if (tr == TR_SUCCESS)
            {
               if (*pbIdentical)
                  TRACE_OUT((TEXT("CompareFilesByHandle(): %s and %s are identical."),
                             DebugGetPathString(hpath1),
                             DebugGetPathString(hpath2)));
               else
                  TRACE_OUT((TEXT("CompareFilesByHandle(): %s and %s are different."),
                             DebugGetPathString(hpath1),
                             DebugGetPathString(hpath2)));
            }

#endif

            /*
             * Failing to close the file properly is not a failure condition here.
             */

            CloseHandle(h2);
         }
         else
            tr = TR_DEST_OPEN_FAILED;

          /*
           * Failing to close the file properly is not a failure condition here.
           */

         CloseHandle(h1);
      }
      else
         tr = TR_SRC_OPEN_FAILED;
   }
   else
      tr = TR_UNAVAILABLE_VOLUME;

   return(tr);
}

