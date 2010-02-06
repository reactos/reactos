/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Winsock 2 SPI
 * FILE:        lib/mswsock/lib/init.c
 * PURPOSE:     DLL Initialization
 */

/* INCLUDES ******************************************************************/
#include "msafd.h"

/* DATA **********************************************************************/

PVOID SockBufferKeyTable;
ULONG SockBufferKeyTableSize;

/* FUNCTIONS *****************************************************************/
BOOL
WINAPI
TransmitFile(SOCKET Socket,
             HANDLE File,
             DWORD NumberOfBytesToWrite,
             DWORD NumberOfBytesPerSend,
             LPOVERLAPPED Overlapped,
             LPTRANSMIT_FILE_BUFFERS TransmitBuffers,
             DWORD Flags)
{
  static GUID TransmitFileGUID = WSAID_TRANSMITFILE;
  LPFN_TRANSMITFILE pfnTransmitFile;
  DWORD cbBytesReturned;

  if (WSAIoctl(Socket,
               SIO_GET_EXTENSION_FUNCTION_POINTER,
               &TransmitFileGUID,
               sizeof(TransmitFileGUID),
               &pfnTransmitFile,
               sizeof(pfnTransmitFile),
               &cbBytesReturned,
               NULL,
               NULL) == SOCKET_ERROR)
  {
    return FALSE;
  }

  return pfnTransmitFile(Socket,
                         File,
                         NumberOfBytesToWrite,
                         NumberOfBytesPerSend,
                         Overlapped,
                         TransmitBuffers,
                         Flags);
}

