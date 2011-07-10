/*
 * PROJECT:          ReactOS kernel
 * LICENSE:          GPL - See COPYING in the top level directory
 * FILE:             dll\win32\audsrvapi\audsrvapi.c
 * PURPOSE:          Audio Server
 * COPYRIGHT:        Copyright 2011 Neeraj Yadav

 */

#include "audsrvapi.h"

/*All the wrappers for Remote Function should be here*/
int status = 0;
/*Initialize an audio stream
 *Return -1 if callbacks are NULL pointers
 */
int
WINAPI
InitStream (ClientStream * clientstream,
                       LONG frequency,
                       int channels,
                       int bitspersample,
                       int datatype, 
                       ULONG channelmask,
                       int volume,
                       int mute,
                       float balance)
{
    long streamid;

    if (clientstream == NULL )
        return -1;

    if (clientstream->callbacks.OpenComplete == NULL || clientstream->callbacks.BufferCopied == NULL || clientstream->callbacks.PlayComplete == NULL)
        return -2;

    /*Validity of all other data will be checked at server*/
    /*Check Connection Status If not connected call Connect()*/
    /*If connected Properly call the remote audsrv_initstream() function*/

    RpcTryExcept  
    {
        streamid = AUDInitStream (audsrv_v0_0_c_ifspec,
                                  frequency,
                                  channels,
                                  bitspersample,
                                  datatype,
                                  channelmask,
                                  volume,
                                  mute,
                                  balance);

        if(streamid != 0)
            clientstream->stream = streamid;
    }
    RpcExcept(1)
    {
        status = RpcExceptionCode();
    }
    RpcEndExcept

    /*Analyse the return by the function*/
    /*Currently Suppose the return is 0 and a valid streamid is returned*/
    clientstream->ClientEventPool[0]=CreateEvent(NULL,
                                                 FALSE,
                                                 FALSE,
                                                 NULL);

    clientstream->dead = 0;

    return 0;
}

int
WINAPI
PlayAudio ( ClientStream * clientstream )
{
    /*This is an ActiveScheduler*/
    clientstream->callbacks.OpenComplete(0);

    while(TRUE)
    {
        while(WaitForSingleObject(clientstream->ClientEventPool[0],
                                  100)!=0)
        {
            if(clientstream->dead)
                break;
        }

        if(clientstream->dead)
            break;

            /*Check Connection Status If not connected call Connect()*/
            /*If connected Properly call the remote audsrv_play() function,This will be a blocking call, placing a dummy wait function here is a good idea.*/
            Sleep(1000);
            clientstream->callbacks.BufferCopied(0);
    }
    clientstream->callbacks.PlayComplete(0);

/*Audio Thread Ended*/
    return 0;
}

int
WINAPI
StopAudio (ClientStream * clientstream )
{
    /*Server Side termination is remaining*/
    /*If connected Properly call the remote audsrv_stop() function*/
    clientstream->dead = 1;  /*Client Side termination*/
    
    return 0;
}

int
WINAPI
Volume(ClientStream * clientstream,
                  int * volume )
{
    return 0;
}

int
WINAPI
SetVolume(ClientStream * clientstream ,
                     const int newvolume)
{
    return 0;
}

int
WINAPI
Write(ClientStream * clientstream ,
                 const char * aData)
{
    if(clientstream->dead)
        return -1;

    SetEvent(clientstream->ClientEventPool[0]);
    
    return 0;
}

int
WINAPI
SetBalance(ClientStream * clientstream ,
                      float balance)
{
    return 0;
}

int
WINAPI
GetBalance(ClientStream * clientstream ,
                      float * balance)
{
    return 0;
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