/*
 * COPYRIGHT:  See COPYING in the top level directory
 * PROJECT:    DCE/RPC for Reactos
 * FILE:       lib/rpcrt4/srv/listen.c
 * PURPOSE:    Listener functions
 * PROGRAMMER: David Welch <welch@cwcom.net>
 */

/* INCLUDES ******************************************************************/

/* TYPES *********************************************************************/

typedef struct
{
} listener_socket;

/* GLOBALS *******************************************************************/

#define NR_MAX_LISTENERS              (255)

static HANDLE ListenerMutex;
static ULONG InServerListen;

/* FUNCTIONS *****************************************************************/

static DWORD RpcListenerThread(PVOID Param)
{
   
}

BOOL RpcInitListenModule(VOID)
{
   ListenerMutex = CreateMutex(NULL,
			       FALSE,
			       NULL);
   if (ListenMutex == NULL)
     {
	return(FALSE);
     }
   
   InServerListen = 0;
}

RPC_STATUS RpcServerListen(unsigned int MinimumCallThreads,
			   unsigned int MaxCalls,
			   unsigned int DontWait)
{
}
