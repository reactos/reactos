#ifndef _TDI_PROTO_H
#define _TDI_PROTO_H

NTSTATUS TdiConnect( PIRP *PendingIrp,
		     PFILE_OBJECT ConnectionObject,
		     PTRANSPORT_ADDRESS RemoteAddress,
		     PIO_COMPLETION_ROUTINE CompletionRoutine,
		     PVOID CompletionContext );

NTSTATUS TdiOpenConnectionEndpointFile(PUNICODE_STRING DeviceName,
				       PHANDLE ConnectionHandle,
				       PFILE_OBJECT *ConnectionObject);

NTSTATUS TdiCloseDevice(HANDLE Handle,
			PFILE_OBJECT FileObject);

#endif/*_TDI_PROTO_H*/
