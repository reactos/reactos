/* $Id: stubs.c 12852 2005-01-06 13:58:04Z mf $
 *
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock DLL
 * FILE:        stubs.c
 * PURPOSE:     WSAIoctl wrappers for Microsoft extensions to Winsock
 * PROGRAMMERS: KJK::Hyperion <hackbunny@reactos.com>
 * REVISIONS:
 */

#include <windows.h>
#include <winsock2.h>
#include <mswsock.h>

/*
 * @implemented
 */
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

/* EOF */
