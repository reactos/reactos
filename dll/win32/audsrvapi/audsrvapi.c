#include "audsrvapi.h"

/*All the wrappers for Remote Function should be here*/
int status = 0;
/*Initialize an audio stream
 *Return -1 if callbacks are NULL pointers
 */
WINAPI int initstream (ClientStream * clientstream,LONG frequency,int channels,int bitspersample, ULONG channelmask,int volume,int mute,float balance)
{
	RPC_STATUS status;
    unsigned char * pszStringBinding    = NULL;


	if(clientstream == NULL ) return -1;
	if (clientstream->callbacks.OpenComplete == NULL || clientstream->callbacks.BufferCopied == NULL || clientstream->callbacks.PlayComplete == NULL) return -2;
	/*Validity of all other data will be checked at server*/
	/*Check Connection Status If not connected call Connect()*/
	/*If connected Properly call the remote audsrv_initstream() function*/
		status = RpcStringBindingCompose(NULL,L"ncacn_np",NULL,L"\\pipe\\audsrv", NULL,&pszStringBinding);

		status = RpcBindingFromStringBinding(pszStringBinding, &audsrv_v0_0_c_ifspec);
 
		if (status) printf("Connection Problem p %d \n",status);

		status = RpcStringFree(&pszStringBinding);
 
		if (status) printf("Problem Freeing String : %d \n",status);


	RpcTryExcept  
    {
		AUDInitStream (&audsrv_v0_0_c_ifspec);
    }
    RpcExcept(1) 
    {
        status = RpcExceptionCode();
        printf("Runtime reported exception 0x%lx = %ld\n", status, status);
    }
    RpcEndExcept
		    status = RpcBindingFree(&audsrv_v0_0_c_ifspec);
 		if (status == RPC_S_INVALID_BINDING) printf("Error : %d Invalid RPC S HANDLE\n",status);
	/*Analyse the return by the function*/
	/*Currently Suppose the return is 0 and a valid streamid is returned*/
	clientstream->stream = &status;

	clientstream->ClientEventPool[0]=CreateEvent(NULL,FALSE,FALSE,NULL);
	clientstream->dead = 0;

	return 0;
}

WINAPI int playaudio ( ClientStream * clientstream )
{
	/*This is an ActiveScheduler*/
	clientstream->callbacks.OpenComplete(0);
	while(1)
	{
		while(WaitForSingleObject(clientstream->ClientEventPool[0],100)!=0){if(clientstream->dead)goto DEAD;}
			/*Check Connection Status If not connected call Connect()*/
			/*If connected Properly call the remote audsrv_play() function,This will be a blocking call, placing a dummy wait function here is a good idea.*/
			Sleep(1000);
			printf("Played a virtual buffer on Virtual Audio Server :) %d\n",clientstream->dead);
			clientstream->callbacks.BufferCopied(0);
	}
	clientstream->callbacks.PlayComplete(0);

DEAD:
printf("\nAudio Thread Ended\n");

	return 0;
}
WINAPI int stopaudio (ClientStream * clientstream )
{
	/*Server Side termination is remaining*/
	/*If connected Properly call the remote audsrv_stop() function*/
	clientstream->dead = 1;  /*Client Side termination*/
}
WINAPI int Volume(ClientStream * clientstream, int * volume )
{
}
WINAPI int SetVolume(ClientStream * clientstream ,const int newvolume)
{
}
WINAPI int Write(ClientStream * clientstream ,const char * aData)
{
	if(clientstream->dead) return -1;
	SetEvent(clientstream->ClientEventPool[0]);
}
WINAPI int SetBalance(ClientStream * clientstream ,float balance)
{
}
WINAPI int GetBalance(ClientStream * clientstream ,float * balance)
{
}


/******************************************************/
/*         MIDL allocate and free                     */
/******************************************************/
 
void __RPC_FAR * __RPC_USER midl_user_allocate(SIZE_T len)
{
    return(malloc(len));
}
 
void __RPC_USER midl_user_free(void __RPC_FAR * ptr)
{
    free(ptr);
}