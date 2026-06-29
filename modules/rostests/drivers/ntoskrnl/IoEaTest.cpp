// IoEaTest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <stdio.h>
#include <windows.h>

typedef struct _FILE_FULL_EA_INFORMATION {
    ULONG NextEntryOffset;
    UCHAR Flags;
    UCHAR EaNameLength;
    USHORT EaValueLength;
    CHAR EaName[1];
} FILE_FULL_EA_INFORMATION, *PFILE_FULL_EA_INFORMATION;

#define ULONG_PTR unsigned char*

#define NTSTATUS unsigned int

#define STATUS_EA_LIST_INCONSISTENT 0x80000014L
#define STATUS_SUCCESS              0x00000000L

/*
 * @implemented
 */
NTSTATUS
NTAPI
IoCheckEaBufferValidityROS(IN PFILE_FULL_EA_INFORMATION EaBuffer,
                           IN ULONG EaLength,
                           OUT PULONG ErrorOffset)
{
   PFILE_FULL_EA_INFORMATION EaBufferEnd;
   ULONG NextEaBufferOffset;
   UINT IntEaLength;

   /* Length of the rest. Initialize it to EaLength */
   IntEaLength = EaLength;
   /* Initialize EaBuffer to EaBuffer */
   EaBufferEnd = EaBuffer;

   /* The rest length of the buffer */
   /* 8 = sizeof(ULONG) + sizeof(UCHAR) + sizeof(UCHAR) + sizeof(USHORT) */
   while (IntEaLength >= 8)
   {
      /* The rest of the buffer must be greater than sizeof(FILE_FULL_EA_INFORMATION) + buffer */
      NextEaBufferOffset = EaBufferEnd->EaNameLength+EaBufferEnd->EaValueLength + 9;
      if (IntEaLength >= NextEaBufferOffset)
      {
         /* is the EaBufferName terminated with zero? */
         if (EaBufferEnd->EaName[EaBufferEnd->EaNameLength]==0)
         {
            /* more EaBuffers ahead */
            if (EaBufferEnd->NextEntryOffset == 0)
            {
               /* test the rest buffersize */
               IntEaLength = IntEaLength - NextEaBufferOffset;
               if (IntEaLength>=0)
               {
                  return STATUS_SUCCESS;
               }
            }
            else
            {
               /*
                  From MSDN (http://msdn2.microsoft.com/en-us/library/ms795740.aspx (DEAD_LINK)).
                  For all entries except the last, the value of NextEntryOffset must be greater
                  than zero and must fall on a ULONG boundary.
               */
               NextEaBufferOffset = ((NextEaBufferOffset + 3) & 0xFFFFFFFC);
               if ((EaBufferEnd->NextEntryOffset == NextEaBufferOffset) && (EaBufferEnd->NextEntryOffset>0))
               {
                  /* The rest of the buffer must be greater than the next offset */
                  IntEaLength = IntEaLength - EaBufferEnd->NextEntryOffset;
                  if (IntEaLength>=0)
                  {
                     EaBufferEnd = (PFILE_FULL_EA_INFORMATION)((ULONG_PTR)EaBufferEnd + EaBufferEnd->NextEntryOffset);
                     continue;
                  }
               }
            }
         }
      }
      break;
   }

   if (ErrorOffset != NULL)
   {
      /* calculate the error offset. */
      *ErrorOffset = (ULONG)((ULONG_PTR)EaBufferEnd - (ULONG_PTR)EaBuffer);
   }

   return STATUS_EA_LIST_INCONSISTENT;
}





void CheckROSAgainstWinAndPrintResult(PFILE_FULL_EA_INFORMATION WinEaBuffer,PFILE_FULL_EA_INFORMATION ROSEaBuffer,NTSTATUS WinStatus,NTSTATUS ROSStatus,ULONG WinErrorOffset,ULONG ROSErrorOffset,int iBufferLength,int iTestCount,ULONG TestEaLength)
{
   printf("Subtest:%i Status:%x EaErrorOffset:%x TestEaLength:%i passed - ",iTestCount,WinStatus,WinErrorOffset,TestEaLength);
   if (memcmp(WinEaBuffer,ROSEaBuffer,iBufferLength)==0)
   {
      if (WinStatus == ROSStatus)
      {
         if (WinErrorOffset == ROSErrorOffset)
         {
            printf("okay\n");
            return;
         }
      }
   }
   printf("*failed*\n");
}

typedef NTSTATUS (*NTAPI pIoCheckEaBufferValidity) (IN PFILE_FULL_EA_INFORMATION EaBuffer,IN ULONG EaLength,OUT PULONG ErrorOffset);

typedef PVOID (*NTAPI pMmPageEntireDriver) (IN PVOID AddressWithinSection);

#define RANDOM_INIT_ERROR 0xDEADBAD0
#define TEST_BUFFER_LEN 256

int _tmain(int argc, _TCHAR* argv[])
{
   void *pFunction;
   pIoCheckEaBufferValidity IoCheckEaBufferValidity;

   HMODULE hKrnlMod = LoadLibrary(L"ntoskrnl.exe");
   if (hKrnlMod)
   {
      pFunction = GetProcAddress(hKrnlMod,"IoCheckEaBufferValidity");
      IoCheckEaBufferValidity = (pIoCheckEaBufferValidity)pFunction;
      if (IoCheckEaBufferValidity!=NULL)
      {
         /* Check tes Windows Function */
         ULONG ulWinError;
         ULONG ulROSError;
         NTSTATUS WinStatus;
         NTSTATUS ROSStatus;
         PFILE_FULL_EA_INFORMATION WinEaBuffer;
         PFILE_FULL_EA_INFORMATION ROSEaBuffer;
         char szTest[100] = "FltMgr";
         int iTestCount,i;
         ULONG TestEaLength;
         UCHAR TestEaBufferFlags;

         // Test the flag
         TestEaBufferFlags = 0;

         iTestCount = 1;
         WinEaBuffer = (PFILE_FULL_EA_INFORMATION)malloc(TEST_BUFFER_LEN);
         ROSEaBuffer = (PFILE_FULL_EA_INFORMATION)malloc(TEST_BUFFER_LEN);


         printf("1.) Test : *********************\n");

         /* Check EaLength calculation */
         /* Here all zero : only i>9 pass the test with STATUS_SUCCESS */

         for (i=0;i<TEST_BUFFER_LEN;i++)
         {
            TestEaLength = i;
            // Windows
            memset(WinEaBuffer,0,TEST_BUFFER_LEN);
            ulWinError = RANDOM_INIT_ERROR;
            WinEaBuffer->Flags = TestEaBufferFlags;
            WinStatus = IoCheckEaBufferValidity(WinEaBuffer,TestEaLength,&ulWinError);

            // ROS
            memset(ROSEaBuffer,0,TEST_BUFFER_LEN);
            ulROSError = RANDOM_INIT_ERROR;
            ROSEaBuffer->Flags = TestEaBufferFlags;
            ROSStatus = IoCheckEaBufferValidityROS(ROSEaBuffer,TestEaLength,&ulROSError);

            CheckROSAgainstWinAndPrintResult(WinEaBuffer,ROSEaBuffer,WinStatus,ROSStatus,ulWinError,ulWinError,TEST_BUFFER_LEN,iTestCount,TestEaLength);
            iTestCount++;
         }

         printf("2.) Test : *********************\n");

         /* Here all zero but EaBuffer::EaName is set : will always end in STATUS_EA_LIST_INCONSISTENT */
         /* There must a link to EaBuffer::EaName */
         for (i=0;i<TEST_BUFFER_LEN;i++)
         {
            TestEaLength = i;
            // Windows
            memset(WinEaBuffer,0,TEST_BUFFER_LEN);
            sprintf(WinEaBuffer->EaName,"%x",RANDOM_INIT_ERROR);
            ulWinError = RANDOM_INIT_ERROR;
            WinEaBuffer->Flags = TestEaBufferFlags;
            WinStatus = IoCheckEaBufferValidity(WinEaBuffer,TestEaLength,&ulWinError);

            // ROS
            memset(ROSEaBuffer,0,TEST_BUFFER_LEN);
            sprintf(ROSEaBuffer->EaName,"%x",RANDOM_INIT_ERROR);
            ulROSError = RANDOM_INIT_ERROR;
            ROSStatus = IoCheckEaBufferValidityROS(ROSEaBuffer,TestEaLength,&ulROSError);

            CheckROSAgainstWinAndPrintResult(WinEaBuffer,ROSEaBuffer,WinStatus,ROSStatus,ulWinError,ulWinError,TEST_BUFFER_LEN,iTestCount,TestEaLength);
            iTestCount++;
         }

         printf("3.) Test : *********************\n");

         /* Here EaBuffer::EaName is set and EaBuffer::EaNameLength is count up. EaLength is maxbuffer: STATUS_SUCCESS when EaBuffer::EaNameLength>strlen(EaBuffer::EaName) */
         TestEaLength = TEST_BUFFER_LEN;
         for (i=0;i<TEST_BUFFER_LEN;i++)
         {

            // Windows
            memset(WinEaBuffer,0,TEST_BUFFER_LEN);
            sprintf(WinEaBuffer->EaName,"%x",RANDOM_INIT_ERROR);
            WinEaBuffer->EaNameLength = i;
            ulWinError = RANDOM_INIT_ERROR;
            WinEaBuffer->Flags = TestEaBufferFlags;
            WinStatus = IoCheckEaBufferValidity(WinEaBuffer,TestEaLength,&ulWinError);

            // ROS
            memset(ROSEaBuffer,0,TEST_BUFFER_LEN);
            sprintf(ROSEaBuffer->EaName,"%x",RANDOM_INIT_ERROR);
            ROSEaBuffer->EaNameLength = i;
            ulROSError = RANDOM_INIT_ERROR;
            ROSEaBuffer->Flags = TestEaBufferFlags;
            ROSStatus = IoCheckEaBufferValidityROS(ROSEaBuffer,TestEaLength,&ulROSError);

            CheckROSAgainstWinAndPrintResult(WinEaBuffer,ROSEaBuffer,WinStatus,ROSStatus,ulWinError,ulWinError,TEST_BUFFER_LEN,iTestCount,TestEaLength);
            iTestCount++;
         }

         printf("4.) Test : *********************\n");

         /* Here EaBuffer::EaName is set and EaBuffer::EaNameLength is strlen(EaBuffer::EaName). EaLength is count: STATUS_SUCCESS when EaLength>=17 (EaBuffer::EaNameLength+9) */
         for (i=0;i<TEST_BUFFER_LEN;i++)
         {
            TestEaLength = i;
            // Windows
            memset(WinEaBuffer,0,TEST_BUFFER_LEN);
            sprintf(WinEaBuffer->EaName,"%x",RANDOM_INIT_ERROR);
            WinEaBuffer->EaNameLength = (UCHAR)strlen(WinEaBuffer->EaName);
            ulWinError = RANDOM_INIT_ERROR;
            WinEaBuffer->Flags = TestEaBufferFlags;
            WinStatus = IoCheckEaBufferValidity(WinEaBuffer,TestEaLength,&ulWinError);

            // ROS
            memset(ROSEaBuffer,0,TEST_BUFFER_LEN);
            sprintf(ROSEaBuffer->EaName,"%x",RANDOM_INIT_ERROR);
            ROSEaBuffer->EaNameLength = (UCHAR)strlen(ROSEaBuffer->EaName);
            ulROSError = RANDOM_INIT_ERROR;
            ROSEaBuffer->Flags = TestEaBufferFlags;
            ROSStatus = IoCheckEaBufferValidityROS(ROSEaBuffer,TestEaLength,&ulROSError);

            CheckROSAgainstWinAndPrintResult(WinEaBuffer,ROSEaBuffer,WinStatus,ROSStatus,ulWinError,ulWinError,TEST_BUFFER_LEN,iTestCount,TestEaLength);
            iTestCount++;
         }

         printf("5.) Test : *********************\n");

         /* Here EaBuffer::EaName is set and EaBuffer::EaNameLength is strlen(EaBuffer::EaName) EaBuffer::EaValueLength is strlen(EaBuffer::EaName)+1. EaLength is count: STATUS_SUCCESS when EaLength>=26 (EaBuffer::EaNameLength+EaBuffer::EaValueLength+9) */
         for (i=0;i<TEST_BUFFER_LEN;i++)
         {
            TestEaLength = i;
            // Windows
            memset(WinEaBuffer,0,TEST_BUFFER_LEN);
            sprintf(WinEaBuffer->EaName,"%x",RANDOM_INIT_ERROR);
            WinEaBuffer->EaNameLength = (UCHAR)strlen(WinEaBuffer->EaName);
            WinEaBuffer->EaValueLength = (UCHAR)strlen(WinEaBuffer->EaName)+1;
            ulWinError = RANDOM_INIT_ERROR;
            WinEaBuffer->Flags = TestEaBufferFlags;
            WinStatus = IoCheckEaBufferValidity(WinEaBuffer,TestEaLength,&ulWinError);

            // ROS
            memset(ROSEaBuffer,0,TEST_BUFFER_LEN);
            sprintf(ROSEaBuffer->EaName,"%x",RANDOM_INIT_ERROR);
            ROSEaBuffer->EaNameLength = (UCHAR)strlen(ROSEaBuffer->EaName);
            ROSEaBuffer->EaValueLength = (UCHAR)strlen(ROSEaBuffer->EaName)+1;
            ulROSError = RANDOM_INIT_ERROR;
            ROSEaBuffer->Flags = TestEaBufferFlags;
            ROSStatus = IoCheckEaBufferValidityROS(ROSEaBuffer,TestEaLength,&ulROSError);

            CheckROSAgainstWinAndPrintResult(WinEaBuffer,ROSEaBuffer,WinStatus,ROSStatus,ulWinError,ulWinError,TEST_BUFFER_LEN,iTestCount,TestEaLength);
            iTestCount++;
         }


         printf("6.) Test : *********************\n");

         /* The same test like 5.) but more data in the buffer*/
         /* Here EaBuffer::EaName is set and EaBuffer::EaNameLength is strlen(EaBuffer::EaName) EaBuffer::EaValueLength is strlen(EaBuffer::EaName)+1. EaLength is count: STATUS_SUCCESS when EaLength>=26 (EaBuffer::EaNameLength+EaBuffer::EaValueLength+9) */

         for (i=0;i<TEST_BUFFER_LEN;i++)
         {
            TestEaLength = i;
            // Windows
            memset(WinEaBuffer,0,TEST_BUFFER_LEN);
            sprintf(WinEaBuffer->EaName,"%x%x%x",RANDOM_INIT_ERROR,RANDOM_INIT_ERROR,RANDOM_INIT_ERROR);
            sprintf(WinEaBuffer->EaName,"%x",RANDOM_INIT_ERROR);
            WinEaBuffer->EaNameLength = (UCHAR)strlen(WinEaBuffer->EaName);
            WinEaBuffer->EaValueLength = (UCHAR)strlen(WinEaBuffer->EaName)+1;
            ulWinError = RANDOM_INIT_ERROR;
            WinEaBuffer->Flags = TestEaBufferFlags;
            WinStatus = IoCheckEaBufferValidity(WinEaBuffer,TestEaLength,&ulWinError);

            // ROS
            memset(ROSEaBuffer,0,TEST_BUFFER_LEN);
            sprintf(ROSEaBuffer->EaName,"%x%x%x",RANDOM_INIT_ERROR,RANDOM_INIT_ERROR,RANDOM_INIT_ERROR);
            sprintf(ROSEaBuffer->EaName,"%x",RANDOM_INIT_ERROR);
            ROSEaBuffer->EaNameLength = (UCHAR)strlen(ROSEaBuffer->EaName);
            ROSEaBuffer->EaValueLength = (UCHAR)strlen(ROSEaBuffer->EaName)+1;
            ulROSError = RANDOM_INIT_ERROR;
            ROSEaBuffer->Flags = TestEaBufferFlags;
            ROSStatus = IoCheckEaBufferValidityROS(ROSEaBuffer,TestEaLength,&ulROSError);

            CheckROSAgainstWinAndPrintResult(WinEaBuffer,ROSEaBuffer,WinStatus,ROSStatus,ulWinError,ulWinError,TEST_BUFFER_LEN,iTestCount,TestEaLength);
            iTestCount++;
         }

         printf("7.) Test : *********************\n");

         /* The same test like 6.) but wrong strlen */
         /* Here EaBuffer::EaName is set and EaBuffer::EaNameLength is strlen(EaBuffer::EaName) EaBuffer::EaValueLength is strlen(EaBuffer::EaName)+1. EaLength is count: will always end in STATUS_EA_LIST_INCONSISTENT */
         for (i=0;i<TEST_BUFFER_LEN;i++)
         {
            TestEaLength = i;
            // Windows
            memset(WinEaBuffer,0,TEST_BUFFER_LEN);
            sprintf(WinEaBuffer->EaName,"%x%x%x",RANDOM_INIT_ERROR,RANDOM_INIT_ERROR,RANDOM_INIT_ERROR);
            sprintf(WinEaBuffer->EaName,"%x",RANDOM_INIT_ERROR);
            WinEaBuffer->EaNameLength = (UCHAR)strlen(WinEaBuffer->EaName)-1;
            WinEaBuffer->EaValueLength = (UCHAR)strlen(WinEaBuffer->EaName)+2;
            ulWinError = RANDOM_INIT_ERROR;
            WinEaBuffer->Flags = TestEaBufferFlags;
            WinStatus = IoCheckEaBufferValidity(WinEaBuffer,TestEaLength,&ulWinError);

            // ROS
            memset(ROSEaBuffer,0,TEST_BUFFER_LEN);
            sprintf(ROSEaBuffer->EaName,"%x%x%x",RANDOM_INIT_ERROR,RANDOM_INIT_ERROR,RANDOM_INIT_ERROR);
            sprintf(ROSEaBuffer->EaName,"%x",RANDOM_INIT_ERROR);
            ROSEaBuffer->EaNameLength = (UCHAR)strlen(ROSEaBuffer->EaName)-1;
            ROSEaBuffer->EaValueLength = (UCHAR)strlen(ROSEaBuffer->EaName)+2;
            ulROSError = RANDOM_INIT_ERROR;
            ROSEaBuffer->Flags = TestEaBufferFlags;
            ROSStatus = IoCheckEaBufferValidityROS(ROSEaBuffer,TestEaLength,&ulROSError);

            CheckROSAgainstWinAndPrintResult(WinEaBuffer,ROSEaBuffer,WinStatus,ROSStatus,ulWinError,ulWinError,TEST_BUFFER_LEN,iTestCount,TestEaLength);
            iTestCount++;
         }


         printf("8.) Test : *********************\n");

         /* Here WinEaBuffer->NextEntryOffset test : STATUS_SUCCESS when NextEntryOffset=0 else STATUS_EA_LIST_INCONSISTENT when NextEntryOffset = 28 = 8+8+9 ((WinEaBuffer->EaNameLength+WinEaBuffer->EaNameLength+9)+3)&0xFFFFFFFC then ErrorOffset 28 */
         /* From the MSDN (http://msdn2.microsoft.com/en-us/library/ms795740.aspx (DEAD_LINK)). For all entries except the last, the value of NextEntryOffset must be greater than zero and must fall on a ULONG boundary.*/
         for (i=0;i<TEST_BUFFER_LEN;i++)
         {
            TestEaLength = TEST_BUFFER_LEN;
            // Windows
            memset(WinEaBuffer,0,TEST_BUFFER_LEN);
            sprintf(WinEaBuffer->EaName,"%x%x%x",RANDOM_INIT_ERROR,RANDOM_INIT_ERROR,RANDOM_INIT_ERROR);
            sprintf(WinEaBuffer->EaName,"%x",RANDOM_INIT_ERROR);
            WinEaBuffer->EaNameLength = (UCHAR)strlen(WinEaBuffer->EaName);
            WinEaBuffer->EaValueLength = (UCHAR)strlen(WinEaBuffer->EaName);
            ulWinError = RANDOM_INIT_ERROR;
            WinEaBuffer->Flags = TestEaBufferFlags;
            WinEaBuffer->NextEntryOffset = i;
            WinStatus = IoCheckEaBufferValidity(WinEaBuffer,TestEaLength,&ulWinError);

            // ROS
            memset(ROSEaBuffer,0,TEST_BUFFER_LEN);
            sprintf(ROSEaBuffer->EaName,"%x%x%x",RANDOM_INIT_ERROR,RANDOM_INIT_ERROR,RANDOM_INIT_ERROR);
            sprintf(ROSEaBuffer->EaName,"%x",RANDOM_INIT_ERROR);
            ROSEaBuffer->EaNameLength = (UCHAR)strlen(ROSEaBuffer->EaName);
            ROSEaBuffer->EaValueLength = (UCHAR)strlen(ROSEaBuffer->EaName);
            ulROSError = RANDOM_INIT_ERROR;
            ROSEaBuffer->Flags = TestEaBufferFlags;
            ROSEaBuffer->NextEntryOffset = i;
            ROSStatus = IoCheckEaBufferValidityROS(ROSEaBuffer,TestEaLength,&ulROSError);

            printf("%i-",ROSEaBuffer->NextEntryOffset);
            CheckROSAgainstWinAndPrintResult(WinEaBuffer,ROSEaBuffer,WinStatus,ROSStatus,ulWinError,ulWinError,TEST_BUFFER_LEN,iTestCount,TestEaLength);
            iTestCount++;
         }

         printf("9.) Test : *********************\n");

         /* Here WinEaBuffer->NextEntryOffset test wrong strlen: STATUS_SUCCESS NextEntryOffset=0 & NextEntryOffset = 28 = 8+8+9 ((WinEaBuffer->EaNameLength+WinEaBuffer->EaNameLength+9)+3)&0xFFFFFFFC */
         /* From the MSDN (http://msdn2.microsoft.com/en-us/library/ms795740.aspx (DEAD_LINK)). For all entries except the last, the value of NextEntryOffset must be greater than zero and must fall on a ULONG boundary.*/
         for (i=0;i<TEST_BUFFER_LEN;i++)
         {
            TestEaLength = TEST_BUFFER_LEN;
            // Windows
            memset(WinEaBuffer,0,TEST_BUFFER_LEN);
            sprintf(WinEaBuffer->EaName,"%x",RANDOM_INIT_ERROR);

            WinEaBuffer->EaNameLength = (UCHAR)strlen(WinEaBuffer->EaName)-1;
            WinEaBuffer->EaValueLength = (UCHAR)strlen(WinEaBuffer->EaName);
            ulWinError = RANDOM_INIT_ERROR;
            WinEaBuffer->Flags = TestEaBufferFlags;
            WinEaBuffer->NextEntryOffset = i;
            WinStatus = IoCheckEaBufferValidity(WinEaBuffer,TestEaLength,&ulWinError);

            // ROS
            memset(ROSEaBuffer,0,TEST_BUFFER_LEN);
            sprintf(ROSEaBuffer->EaName,"%x",RANDOM_INIT_ERROR);

            ROSEaBuffer->EaNameLength = (UCHAR)strlen(ROSEaBuffer->EaName)-1;
            ROSEaBuffer->EaValueLength = (UCHAR)strlen(ROSEaBuffer->EaName);
            ulROSError = RANDOM_INIT_ERROR;
            ROSEaBuffer->Flags = TestEaBufferFlags;
            ROSEaBuffer->NextEntryOffset = i;
            ROSStatus = IoCheckEaBufferValidityROS(ROSEaBuffer,TestEaLength,&ulROSError);

            printf("%i-",ROSEaBuffer->NextEntryOffset);
            CheckROSAgainstWinAndPrintResult(WinEaBuffer,ROSEaBuffer,WinStatus,ROSStatus,ulWinError,ulWinError,TEST_BUFFER_LEN,iTestCount,TestEaLength);
            iTestCount++;
         }

         printf("10.) Test : *********************\n");

         /* Here WinEaBuffer->NextEntryOffset test wrong strlen: STATUS_SUCCESS NextEntryOffset=0 & NextEntryOffset = 28 = 8+8+9 ((WinEaBuffer->EaNameLength+WinEaBuffer->EaNameLength+9)+3)&0xFFFFFFFC */
         /* From the MSDN (http://msdn2.microsoft.com/en-us/library/ms795740.aspx (DEAD_LINK)). For all entries except the last, the value of NextEntryOffset must be greater than zero and must fall on a ULONG boundary.*/
         for (i=0;i<TEST_BUFFER_LEN;i++)
         {
            TestEaLength = TEST_BUFFER_LEN;
            // Windows
            memset(WinEaBuffer,0,TEST_BUFFER_LEN);
            sprintf(WinEaBuffer->EaName,"%x",RANDOM_INIT_ERROR);

            WinEaBuffer->EaNameLength = (UCHAR)strlen(WinEaBuffer->EaName)+1;
            WinEaBuffer->EaValueLength = (UCHAR)strlen(WinEaBuffer->EaName)+1;
            ulWinError = RANDOM_INIT_ERROR;
            WinEaBuffer->Flags = TestEaBufferFlags;
            WinEaBuffer->NextEntryOffset = i;
            WinStatus = IoCheckEaBufferValidity(WinEaBuffer,TestEaLength,&ulWinError);

            // ROS
            memset(ROSEaBuffer,0,TEST_BUFFER_LEN);
            sprintf(ROSEaBuffer->EaName,"%x",RANDOM_INIT_ERROR);

            ROSEaBuffer->EaNameLength = (UCHAR)strlen(ROSEaBuffer->EaName)+1;
            ROSEaBuffer->EaValueLength = (UCHAR)strlen(ROSEaBuffer->EaName)+1;
            ulROSError = RANDOM_INIT_ERROR;
            ROSEaBuffer->Flags = TestEaBufferFlags;
            ROSEaBuffer->NextEntryOffset = i;
            ROSStatus = IoCheckEaBufferValidityROS(ROSEaBuffer,TestEaLength,&ulROSError);

            printf("%i-",ROSEaBuffer->NextEntryOffset);
            CheckROSAgainstWinAndPrintResult(WinEaBuffer,ROSEaBuffer,WinStatus,ROSStatus,ulWinError,ulWinError,TEST_BUFFER_LEN,iTestCount,TestEaLength);
            iTestCount++;
         }

         printf("11.) Test : *********************\n");

         /* Here WinEaBuffer->NextEntryOffset :  */
         for (i=0;i<TEST_BUFFER_LEN;i++)
         {
            TestEaLength = TEST_BUFFER_LEN;
            // Windows
            memset(WinEaBuffer,0,TEST_BUFFER_LEN);
            sprintf(WinEaBuffer->EaName,"%x",RANDOM_INIT_ERROR,RANDOM_INIT_ERROR);

            WinEaBuffer->EaNameLength = (UCHAR)strlen(WinEaBuffer->EaName);
            WinEaBuffer->EaValueLength = (UCHAR)strlen(WinEaBuffer->EaName);
            ulWinError = RANDOM_INIT_ERROR;
            WinEaBuffer->Flags = TestEaBufferFlags;
            WinEaBuffer->NextEntryOffset = ((WinEaBuffer->EaNameLength+WinEaBuffer->EaNameLength+9)+3)&0xFFFFFFFC;
            WinStatus = IoCheckEaBufferValidity(WinEaBuffer,TestEaLength,&ulWinError);

            // ROS
            memset(ROSEaBuffer,0,TEST_BUFFER_LEN);
            sprintf(ROSEaBuffer->EaName,"%x",RANDOM_INIT_ERROR,RANDOM_INIT_ERROR);

            ROSEaBuffer->EaNameLength = (UCHAR)strlen(ROSEaBuffer->EaName);
            ROSEaBuffer->EaValueLength = (UCHAR)strlen(ROSEaBuffer->EaName);
            ulROSError = RANDOM_INIT_ERROR;
            ROSEaBuffer->Flags = TestEaBufferFlags;
            ROSEaBuffer->NextEntryOffset = ((ROSEaBuffer->EaNameLength+ROSEaBuffer->EaNameLength+9)+3)&0xFFFFFFFC;
            ROSStatus = IoCheckEaBufferValidityROS(ROSEaBuffer,TestEaLength,&ulROSError);

            printf("%i-",ROSEaBuffer->NextEntryOffset);
            CheckROSAgainstWinAndPrintResult(WinEaBuffer,ROSEaBuffer,WinStatus,ROSStatus,ulWinError,ulWinError,TEST_BUFFER_LEN,iTestCount,TestEaLength);
            iTestCount++;
         }


         free(WinEaBuffer);
         free(ROSEaBuffer);
      }

      FreeLibrary(hKrnlMod);
   }
   else
   {
      DWORD dwLastError = GetLastError();
      switch (dwLastError)
      {
      case ERROR_MOD_NOT_FOUND:
         printf("ERROR_MOD_NOT_FOUND\n");
         break;
      case ERROR_BAD_EXE_FORMAT:
         printf("ERROR_BAD_EXE_FORMAT\n");
         break;
      }
   }
   return 0;
}

