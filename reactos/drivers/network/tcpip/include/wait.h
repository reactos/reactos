#ifndef _TCPIP_WAIT_H
#define _TCPIP_WAIT_H

NTSTATUS TcpipWaitForSingleObject( PVOID Object,
				   KWAIT_REASON Reason,
				   KPROCESSOR_MODE WaitMode,
				   BOOLEAN Alertable,
				   PLARGE_INTEGER Timeout );

#endif/*_TCPIP_WAIT_H*/
