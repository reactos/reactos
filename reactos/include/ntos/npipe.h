#ifndef __INCLUDE_NAPI_NPIPE_H
#define __INCLUDE_NAPI_NPIPE_H

#include <windef.h>
#include <ntdef.h>

/*
 * FUNCTION: ZwCreateNamedPipeFile creates named pipe
 * ARGUMENTS:
 *        NamedPipeFileHandle (OUT) = Caller supplied storage for the 
 *                                    resulting handle
 *        DesiredAccess = Specifies the type of access that the caller 
 *                        requires to the file boject
 *        ObjectAttributes = Points to a structure that specifies the
 *                           object attributes.
 *        IoStatusBlock = Points to a variable that receives the final
 *                        completion status and information
 *        ShareAccess = Specifies the limitations on sharing of the file.
 *                      This parameter can be zero or any compatible 
 *                      combination of the following flags
 *                         FILE_SHARE_READ
 *                         FILE_SHARE_WRITE
 *        CreateDisposition = Specifies what to do depending on whether
 *                            the file already exists. This must be one of
 *                            the following values
 *                                  FILE_OPEN
 *                                  FILE_CREATE
 *                                  FILE_OPEN_IF
 *        CreateOptions = Specifies the options to be applied when
 *                        creating or opening the file, as a compatible
 *                        combination of the following flags
 *                            FILE_WRITE_THROUGH
 *                            FILE_SYNCHRONOUS_IO_ALERT
 *                            FILE_SYNCHRONOUS_IO_NONALERT
 *        TypeMessage = Specifies whether the data written to the pipe is
 *                      interpreted as a sequence of messages or as a 
 *                      stream of bytes
 *        ReadModeMessage = Specifies whether the data read from the pipe
 *                          is interpreted as a sequence of messages or as
 *                          a stream of bytes
 *        NonBlocking = Specifies whether non-blocking mode is enabled
 *        MaxInstances = Specifies the maximum number of instancs that can
 *                       be created for this pipe
 *        InBufferSize = Specifies the number of bytes to reserve for the
 *                       input buffer
 *        OutBufferSize = Specifies the number of bytes to reserve for the
 *                        output buffer
 *        DefaultTimeout = Optionally points to a variable that specifies
 *                         the default timeout value in units of 
 *                         100-nanoseconds.
 * REMARKS: This funciton maps to the win32 function CreateNamedPipe
 * RETURNS:
 *	Status
 */

NTSTATUS STDCALL NtCreateNamedPipeFile(OUT PHANDLE NamedPipeFileHandle,
				       IN ACCESS_MASK DesiredAccess,
				       IN POBJECT_ATTRIBUTES ObjectAttributes,
				       OUT PIO_STATUS_BLOCK IoStatusBlock,
				       IN ULONG ShareAccess,
				       IN ULONG CreateDisposition,
				       IN ULONG CreateOptions,
				       IN BOOLEAN WriteModeMessage,
				       IN BOOLEAN ReadModeMessage,
				       IN BOOLEAN NonBlocking,
				       IN ULONG MaxInstances,
				       IN ULONG InBufferSize,
				       IN ULONG OutBufferSize,
				       IN PLARGE_INTEGER DefaultTimeOut);
NTSTATUS STDCALL ZwCreateNamedPipeFile(OUT PHANDLE NamedPipeFileHandle,
				       IN ACCESS_MASK DesiredAccess,
				       IN POBJECT_ATTRIBUTES ObjectAttributes,
				       OUT PIO_STATUS_BLOCK IoStatusBlock,
				       IN ULONG ShareAccess,
				       IN ULONG CreateDisposition,
				       IN ULONG CreateOptions,
				       IN BOOLEAN WriteModeMessage,
				       IN BOOLEAN ReadModeMessage,
				       IN BOOLEAN NonBlocking,
				       IN ULONG MaxInstances,
				       IN ULONG InBufferSize,
				       IN ULONG OutBufferSize,
				       IN PLARGE_INTEGER DefaultTimeOut);

#define FSCTL_PIPE_ASSIGN_EVENT \
        CTL_CODE(FILE_DEVICE_NAMED_PIPE, 0, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define FSCTL_PIPE_DISCONNECT \
        CTL_CODE(FILE_DEVICE_NAMED_PIPE, 1, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define FSCTL_PIPE_LISTEN \
        CTL_CODE(FILE_DEVICE_NAMED_PIPE, 2, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define FSCTL_PIPE_PEEK \
        CTL_CODE(FILE_DEVICE_NAMED_PIPE, 3, METHOD_BUFFERED, FILE_READ_DATA)

#define FSCTL_PIPE_QUERY_EVENT \
        CTL_CODE(FILE_DEVICE_NAMED_PIPE, 4, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define FSCTL_PIPE_TRANSCEIVE \
        CTL_CODE(FILE_DEVICE_NAMED_PIPE, 5, METHOD_NEITHER,  FILE_READ_DATA | FILE_WRITE_DATA)

#define FSCTL_PIPE_WAIT \
        CTL_CODE(FILE_DEVICE_NAMED_PIPE, 6, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define FSCTL_PIPE_IMPERSONATE \
        CTL_CODE(FILE_DEVICE_NAMED_PIPE, 7, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define FSCTL_PIPE_SET_CLIENT_PROCESS \
        CTL_CODE(FILE_DEVICE_NAMED_PIPE, 8, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define FSCTL_PIPE_QUERY_CLIENT_PROCESS \
        CTL_CODE(FILE_DEVICE_NAMED_PIPE, 9, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define FSCTL_PIPE_GET_STATE \
        CTL_CODE(FILE_DEVICE_NAMED_PIPE, 10, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define FSCTL_PIPE_SET_STATE \
        CTL_CODE(FILE_DEVICE_NAMED_PIPE, 11, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define FSCTL_PIPE_INTERNAL_READ \
        CTL_CODE(FILE_DEVICE_NAMED_PIPE, 2045, METHOD_BUFFERED, FILE_READ_DATA)

#define FSCTL_PIPE_INTERNAL_WRITE \
        CTL_CODE(FILE_DEVICE_NAMED_PIPE, 2046, METHOD_BUFFERED, FILE_WRITE_DATA)

#define FSCTL_PIPE_INTERNAL_TRANSCEIVE \
        CTL_CODE(FILE_DEVICE_NAMED_PIPE, 2047, METHOD_NEITHER, FILE_READ_DATA | FILE_WRITE_DATA)

#define FSCTL_PIPE_INTERNAL_READ_OVFLOW \
        CTL_CODE(FILE_DEVICE_NAMED_PIPE, 2048, METHOD_BUFFERED, FILE_READ_DATA)


typedef struct _NPFS_WAIT_PIPE
{
   LARGE_INTEGER Timeout;
} NPFS_WAIT_PIPE, *PNPFS_WAIT_PIPE;

typedef struct _NPFS_LISTEN
{
} NPFS_LISTEN, *PNPFS_LISTEN;

typedef struct _NPFS_SET_STATE
{
   BOOLEAN WriteModeMessage;
   BOOLEAN ReadModeMessage;
   BOOLEAN NonBlocking;
   ULONG InBufferSize;
   ULONG OutBufferSize;
   LARGE_INTEGER Timeout;
} NPFS_SET_STATE, *PNPFS_SET_STATE;

typedef struct _NPFS_GET_STATE
{
   BOOLEAN WriteModeMessage;
   BOOLEAN ReadModeMessage;
   BOOLEAN NonBlocking;
   ULONG InBufferSize;
   ULONG OutBufferSize;
   LARGE_INTEGER Timeout;
} NPFS_GET_STATE, *PNPFS_GET_STATE;

typedef struct _FILE_PIPE_PEEK_BUFFER
{
  ULONG NamedPipeState;
  ULONG ReadDataAvailable;
  ULONG NumberOfMessages;
  ULONG MessageLength;
  CHAR Data[1];
} FILE_PIPE_PEEK_BUFFER, *PFILE_PIPE_PEEK_BUFFER;

#define FILE_PIPE_BYTE_STREAM_TYPE	0x00000000
#define FILE_PIPE_MESSAGE_TYPE		0x00000001

#define FILE_PIPE_BYTE_STREAM_MODE	0x00000000
#define FILE_PIPE_MESSAGE_MODE		0x00000001

#define FILE_PIPE_QUEUE_OPERATION	0x00000000
#define FILE_PIPE_COMPLETE_OPERATION	0x00000001

#define FILE_PIPE_INBOUND		0x00000000
#define FILE_PIPE_OUTBOUND		0x00000001
#define FILE_PIPE_FULL_DUPLEX		0x00000002

#define FILE_PIPE_DISCONNECTED_STATE	0x00000001
#define FILE_PIPE_LISTENING_STATE	0x00000002
#define FILE_PIPE_CONNECTED_STATE	0x00000003
#define FILE_PIPE_CLOSING_STATE		0x00000004

#define FILE_PIPE_CLIENT_END		0x00000000
#define FILE_PIPE_SERVER_END		0x00000001

#define FILE_PIPE_READ_DATA		0x00000000
#define FILE_PIPE_WRITE_SPACE		0x00000001


#endif /* __INCLUDE_NAPI_NPIPE_H */
