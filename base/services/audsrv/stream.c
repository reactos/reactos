/*
 * PROJECT:          ReactOS kernel
 * LICENSE:          GPL - See COPYING in the top level directory
 * FILE:             services/stream.c
 * PURPOSE:          Audio Server
 * COPYRIGHT:        Copyright 2011 Neeraj Yadav
 */

#include "audsrv.h"

long GetNewStreamID()
{
    long streamid= pengine->streamidpool;
    pengine->streamidpool += 1;
    return streamid;
}
BOOL FilterAudio(LPVOID param)
{
    ServerStream * localstream = (ServerStream *) param;

    EnterCriticalSection(&(localstream->CriticalSection));

    if(localstream->state == 1)
    {
	/*Fake Filter,Simply gives the genuine buffer as filtered buffer*/
	/*minsamplevalue and maxsamplevalue must be calculated during filtering*/
	/*Filter must ensure that all the samples are distributed evenly about 0 in case of signed samples*/
    localstream->length_filtered = localstream->length_genuine;
    localstream->filteredbuf = HeapAlloc(GetProcessHeap(),
                                         0,
                                         localstream->length_filtered);

    memcpy(localstream->filteredbuf,localstream->genuinebuf,localstream->length_filtered);

    HeapFree(GetProcessHeap(),
              0,
              localstream->genuinebuf);

    *((int *)localstream->minsamplevalue) = -32766;
    *((int *)localstream->maxsamplevalue) = 32766;

    localstream->state = 2;

    LeaveCriticalSection(&(localstream->CriticalSection));
    return TRUE;
    }
    else
	{
	    LeaveCriticalSection(&(localstream->CriticalSection));
		return FALSE;
	}
}

DWORD WINAPI RunStreamThread(LPVOID param)
{
    ServerStream * localstream = (ServerStream *) param;

    SetEvent(localstream->threadready);

    while (TRUE)
    {
        if(FilterAudio(param) == TRUE )
            WaitForSingleObject(localstream->stream_played_event,INFINITE);
    }
    /*Clean Stream's data*/
}

long AddStream(LONG frequency,
               int channels,
               int bitspersample,
               int datatype,
               ULONG channelmask,
               int volume,
               int mute,
               float balance )
{
    ServerStream * newstream,*localstream;
    DWORD dwID;

    /*Add Data to Linked list*/
    localstream = pengine->serverstreamlist;
    newstream = HeapAlloc(GetProcessHeap(),
                          HEAP_ZERO_MEMORY,
                          sizeof(ServerStream));

    if(newstream == NULL)
        goto error;

    if(volume < 0)
        newstream->volume = 0;
    else if (volume > 1000)
        newstream->volume = 1000;
    else
        newstream->volume = volume;

    if(volume < -1.0)
        newstream->volume = -1.0;
    else if (volume > 1.0) 
        newstream->volume = 1.0;
    else 
        newstream->volume = volume;

    newstream->freq = frequency;  /*TODO frequency validation required*/

    if(datatype==0 || datatype==1 || datatype==2)
        newstream->datatype=datatype;
    else
        goto error;

    if    ((datatype==0 && (bitspersample == 8 || bitspersample == 16 || bitspersample == 32 || bitspersample == 64 )) ||
        (datatype==1 && (bitspersample == 8 || bitspersample == 16 || bitspersample == 32 || bitspersample == 64)) ||
        (datatype==2 && (bitspersample == 32 || bitspersample == 64)) )
    newstream->bitspersample = bitspersample; /*TODO bitspersample validation*/
    else
        goto error;

    newstream->channels = channels; /*TODO validation*/
    newstream->channelmask = channelmask; /*TODO validation*/

    newstream->state = 0;
    newstream->length_genuine = 0;
    newstream->genuinebuf = NULL;
    newstream->length_filtered = 0;
    newstream->filteredbuf = NULL;
    newstream->minsamplevalue = HeapAlloc(GetProcessHeap(),
                                          HEAP_ZERO_MEMORY,
                                          bitspersample/8);

    newstream->maxsamplevalue = HeapAlloc(GetProcessHeap(),
                                          HEAP_ZERO_MEMORY,
                                          bitspersample/8);
    
    newstream->next = NULL;

    newstream->stream_played_event = CreateEvent(NULL,
                                    FALSE,
                                    FALSE,
                                    NULL);

    newstream->buffer_write_event = CreateEvent(NULL,
                                    FALSE,
                                    FALSE,
                                    NULL);

    newstream->threadready = CreateEvent(NULL,
                                         FALSE,
                                         FALSE,
                                         NULL);

    if(newstream->stream_played_event == NULL || newstream->threadready == NULL)
        goto error;

    newstream->streamid=GetNewStreamID();

    if (!InitializeCriticalSectionAndSpinCount(&(newstream->CriticalSection), 
                                               0x00000400) ) 
        goto error;

    newstream->thread=CreateThread(NULL,
                                   0,
                                   RunStreamThread,
                                   newstream,
                                   0,
                                   &dwID);

    if(newstream->thread == NULL)
        goto error;

    WaitForSingleObject(newstream->threadready,
                        INFINITE);



    if(localstream == NULL)
    {
        pengine->serverstreamlist = newstream;
        pengine->masterfreq=frequency;
        pengine->masterchannels=channels;
        pengine->masterchannelmask=channelmask;
        pengine->masterbitspersample=bitspersample;
        pengine->masterdatatype = datatype;
    }
    else
    {
        while(localstream->next != NULL)
            localstream = localstream->next;
        localstream->next = newstream;
    }
    SetEvent(pengine->newStreamEvent);
    return newstream->streamid;

error:
    HeapFree(GetProcessHeap(),
             0,
			 newstream);
    return 0;
}

long WriteBuffer(LONG streamid,
                 LONG length,
                 char * buffer)
{
    ServerStream * localstream = pengine->serverstreamlist;
    while(localstream!=NULL)
    {
        if(localstream->streamid == streamid) break;
        localstream = localstream->next;
    }

    if(localstream == NULL)
        return -1;

    EnterCriticalSection(&(localstream->CriticalSection));

    if(localstream->state == 0)
	{
        localstream->length_genuine = length;
        localstream->genuinebuf = (PSHORT) HeapAlloc(GetProcessHeap(),
                                                               0,
                                                               length);

        memcpy(localstream->genuinebuf,buffer,length);

        localstream->state = 1;
    }

	LeaveCriticalSection(&(localstream->CriticalSection));

    return 0;
}
/*Dont forget to clean ServerStream's minsamplevalue and maxsamplevalue while removing the stream*/
/*Delete Critical Section while cleaning Stream*/