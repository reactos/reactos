/*
 */

#define NTOS_MODE_USER
#include <ntos.h>
#include <windows.h>

//#define NDEBUG
#include <debug.h>


HANDLE PortThreadHandle = NULL;
HANDLE ConnectPortHandle = NULL;
HANDLE MessagePortHandle = NULL;


static NTSTATUS
InitializeLsaPort(VOID)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING PortName;
  LPC_MAX_MESSAGE Request;
  NTSTATUS Status;

  ConnectPortHandle = NULL;
  MessagePortHandle = NULL;

  RtlInitUnicodeString(&PortName,
                       L"\\SeLsaCommandPort");

  InitializeObjectAttributes(&ObjectAttributes,
			     &PortName,
			     0,
			     NULL,
			     NULL);

  Status = NtCreatePort(&ConnectPortHandle,
			&ObjectAttributes,
			0,
			0x100,
			0x2000);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("NtCreatePort() failed (Status %lx)\n", Status);
      goto ByeBye;
    }

  Status = NtListenPort(ConnectPortHandle,
			&Request.Header);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("NtListenPort() failed (Status %lx)\n", Status);
      goto ByeBye;
    }

  Status = NtAcceptConnectPort(&MessagePortHandle,
			       ConnectPortHandle,
			       NULL,
			       TRUE,
			       NULL,
			       NULL);
  if (!NT_SUCCESS (Status))
    {
      DPRINT1("NtAcceptConnectPort() failed (Status %lx)\n", Status);
      goto ByeBye;
    }

  Status = NtCompleteConnectPort (MessagePortHandle);
  if (!NT_SUCCESS (Status))
    {
      DPRINT1("NtCompleteConnectPort() failed (Status %lx)\n", Status);
      goto ByeBye;
    }

ByeBye:
  if (!NT_SUCCESS (Status))
    {
      if (ConnectPortHandle != NULL)
	NtClose (ConnectPortHandle);

      if (MessagePortHandle != NULL)
	NtClose (MessagePortHandle);
    }

  return Status;
}


static NTSTATUS
ProcessPortMessage(VOID)
{
  LPC_MAX_MESSAGE Request;
//  LPC_MAX_MESSAGE Reply;
  NTSTATUS Status;


  DPRINT1("ProcessPortMessage() called\n");

  Status = STATUS_SUCCESS;

  for (;;)
    {
      Status = NtReplyWaitReceivePort(MessagePortHandle,
				      0,
				      NULL,
				      &Request.Header);
      if (!NT_SUCCESS(Status))
	{
	  DPRINT1("NtReplyWaitReceivePort() failed (Status %lx)\n", Status);
	  break;
	}

      DPRINT("Received message\n");

      if (Request.Header.MessageType == LPC_PORT_CLOSED)
	{
	  DPRINT("Port closed\n");

//	  return STATUS_UNSUCCESSFUL;
	}
      if (Request.Header.MessageType == LPC_REQUEST)
	{
	  DPRINT("Received request\n");

	}
      else if (Request.Header.MessageType == LPC_DATAGRAM)
	{
	  DPRINT("Received datagram\n");

//	  Message = (PIO_ERROR_LOG_MESSAGE)&Request.Data;

	}
    }

  return Status;
}


static NTSTATUS STDCALL
PortThreadRoutine(PVOID Param)
{
  NTSTATUS Status = STATUS_SUCCESS;

  Status = InitializeLsaPort();
  if (!NT_SUCCESS(Status))
    return Status;

  while (NT_SUCCESS(Status))
    {
      Status = ProcessPortMessage();
    }

  if (ConnectPortHandle != NULL)
    NtClose (ConnectPortHandle);

  if (MessagePortHandle != NULL)
    NtClose (MessagePortHandle);

  return Status;
}


BOOLEAN
StartLsaPortThread(VOID)
{
  DWORD ThreadId;

  PortThreadHandle = CreateThread(NULL,
				  0x1000,
				  (LPTHREAD_START_ROUTINE)PortThreadRoutine,
				  NULL,
				  0,
				  &ThreadId);

  return (PortThreadHandle != NULL);
}

/* EOF */
