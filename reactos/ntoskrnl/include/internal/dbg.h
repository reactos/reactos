#ifndef __INCLUDE_INTERNAL_DBG_H
#define __INCLUDE_INTERNAL_DBG_H

#include <napi/dbg.h>
#include <internal/port.h>

NTSTATUS STDCALL 
LpcSendDebugMessagePort(PEPORT Port,
			PLPC_DBG_MESSAGE Message);

#endif /* __INCLUDE_INTERNAL_DBG_H */
