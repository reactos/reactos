/*
 * Copyright (C) 2004 ReactOS Team
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS diskspeed.exe
 * FILE:            apps/tests/diskspeed/diskspeed.c
 * PURPOSE:         Determines disk transfer rates
 * PROGRAMMER:
 */

#include <stdio.h>
#include <stdlib.h>
#define WIN32_NO_STATUS
#include <windows.h>
#define NTOS_MODE_USER
#include <ndk/ntndk.h>

#define _NTSCSI_USER_MODE_
#include <ntddscsi.h>
#include <scsi.h>

BOOL GetInquiryData(HANDLE hDevice, PINQUIRYDATA InquiryData)
{
  BOOL Result;
  DWORD dwReturned;
  SCSI_ADDRESS ScsiAddress;
  PSCSI_ADAPTER_BUS_INFO AdapterInfo;
  PSCSI_INQUIRY_DATA InquiryBuffer;
  BYTE Buffer[4096];
  int i;

  Result = DeviceIoControl(hDevice,
	                   IOCTL_SCSI_GET_ADDRESS,
			   NULL,
			   0,
			   &ScsiAddress,
			   sizeof(ScsiAddress),
			   &dwReturned,
			   FALSE);
  if (Result == FALSE)
    {
      return FALSE;
    }
  Result = DeviceIoControl(hDevice,
	                   IOCTL_SCSI_GET_INQUIRY_DATA,
			   NULL,
			   0,
			   Buffer,
			   sizeof(Buffer),
			   &dwReturned,
			   FALSE);
  if (Result)
    {
      AdapterInfo = (PSCSI_ADAPTER_BUS_INFO)Buffer;
      for (i = 0; i < AdapterInfo->NumberOfBuses; i++)
	{
	  InquiryBuffer = (PSCSI_INQUIRY_DATA) (Buffer + AdapterInfo->BusData[i].InquiryDataOffset);
	  if (AdapterInfo->BusData[i].InquiryDataOffset)
	    {
	       while (1)
	         {
		   if (InquiryBuffer->PathId == ScsiAddress.PathId &&
		       InquiryBuffer->TargetId == ScsiAddress.TargetId &&
		       InquiryBuffer->Lun == ScsiAddress.Lun)
		     {
		       memcpy(InquiryData, InquiryBuffer->InquiryData, sizeof(INQUIRYDATA));
		       return TRUE;
		     }
		   if (InquiryBuffer->NextInquiryDataOffset == 0)
		     {
		       break;
		     }
		   InquiryBuffer = (PSCSI_INQUIRY_DATA) (Buffer + InquiryBuffer->NextInquiryDataOffset);
		 }
	    }
	}
    }
  return FALSE;
}



int main(void)
{
    HANDLE hDevice;
    OVERLAPPED ov;

    PBYTE Buffer = NULL ;
    DWORD Start;
    DWORD dwReturned;
    DWORD dwReadTotal;
    DWORD Size;
    BOOL Result;
    ULONG Drive;
    CHAR Name[20];

    INQUIRYDATA InquiryData;


    Drive = 0;
    while (1)
      {
        sprintf(Name, "\\\\.\\PHYSICALDRIVE%ld", Drive);
	hDevice = CreateFile(Name,
	                     GENERIC_READ,
			     FILE_SHARE_READ,
			     NULL,
			     OPEN_EXISTING,
			     0,
			     NULL);
	if (hDevice == INVALID_HANDLE_VALUE)
	  {
	    if (Drive > 0)
	      {
	        VirtualFree(Buffer, 512 * 1024, MEM_RELEASE);
	      }
	    else
	      {
	        printf("Cannot open '%s'\n", Name);
	      }
	    break;
	  }
        if (Drive == 0)
	  {
            printf("Transfer Size (kB)           1     2     4     8    16    32    64   128   256\n");
            printf("Transfer Rate (MB/s)\n");
            printf("-------------------------------------------------------------------------------\n");

	    Buffer = VirtualAlloc(NULL, 512 * 1024, MEM_COMMIT, PAGE_READWRITE);
	  }
        Result = GetInquiryData(hDevice, &InquiryData);
        if (Result)
	  {
            printf("%.24s ", InquiryData.VendorId);
	  }
	else
	  {
	    printf("Disk %ld                   ", Drive + 1);
	  }
        Size = 1024;
        memset(&ov, 0, sizeof(OVERLAPPED));
	while (Size <= 256 * 1024)
	  {
	    memset(Buffer, 0, Size);
	    dwReadTotal = 0;

            Start = GetTickCount() + 2000;
	    while (Start > GetTickCount())
	      {
	        Result = ReadFile(hDevice, Buffer, Size, &dwReturned, &ov);
		if (Result)
		  {
		    dwReadTotal += dwReturned;
		    ov.Offset += dwReturned;
		  }
	      }
	    dwReadTotal /= 2048;
            printf("%3ld.%ld ", dwReadTotal / 1024, (dwReadTotal % 1024) * 10 / 1024);
	    Size *= 2;
	  }
        printf("\n");
	CloseHandle(hDevice);
	Drive++;
      }
    printf("\n");


    return 0;
}
