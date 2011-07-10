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

DWORD WINAPI RunStreamThread(LPVOID param)
{
    UINT i = 0;
    ServerStream * localstream = (ServerStream *) param;

/*UGLY HACK--WILL be removed soon-- fill filtered buffer (1 second duration in the master stream format) directly until we are in a condition to get buffer directly from the client*/
/******************************************************/
    BOOL initmin=FALSE,initmax = FALSE;
    short minimum=0,maximum = 0;
    PSHORT tempbuf;


    localstream->length_filtered = localstream->freq * localstream->channels * localstream->bitspersample / 8;
    tempbuf = (PSHORT) HeapAlloc(GetProcessHeap(),
                                0,
                                localstream->length_filtered);

    while (i < localstream->length_filtered / 2)
    {
        tempbuf[i] = 0x7FFF * sin(0.5 * i * 500 * 6.28 / 48000);

        if((localstream->streamid %2) == 0)
            tempbuf[i] = 0x7FFF * sin(0.5 * i * 500 * 6.28 / 24000);

        if(initmin)
        {
            if(tempbuf[i]<minimum)
                minimum = tempbuf[i];
        }else
            minimum = tempbuf[i];
        if(initmax)
        {
            if(tempbuf[i]>maximum)
                maximum = tempbuf[i];
        }else
            maximum = tempbuf[i];

        if(initmin == FALSE || initmax == FALSE )
        {
            initmin = TRUE;
            initmax = TRUE;
        }
        i++;

        tempbuf[i] = 0x7FFF * sin(0.5 * i * 500 * 6.28 / 48000);

        if((localstream->streamid %2) == 0)
            tempbuf[i] = 0x7FFF * sin(0.5 * i * 500 * 6.28 / 24000);


        if(initmin)
        {
            if(tempbuf[i]<minimum)
                minimum = tempbuf[i];
        }
        else
            minimum = tempbuf[i];

        if(initmax)
        {
            if(tempbuf[i]>maximum)
                maximum = tempbuf[i];
        }else
            maximum = tempbuf[i];
        i++;
    }

    *((int *)localstream->minsamplevalue) = minimum;
    *((int *)localstream->maxsamplevalue) = maximum;
    localstream->filteredbuf = tempbuf;
    localstream->ready =TRUE;

/******************************************************/
/*Do Some Initialization If needed.Only After these Initialization remaining system will be told that stream is ready*/
    SetEvent(localstream->threadready);

    while (TRUE)
    {
        /*Wait For Data Write Event,currently NO Wait considering Data has always been written*/

		EnterCriticalSection(&(localstream->CriticalSection));

		LeaveCriticalSection(&(localstream->CriticalSection));
		/*Wait For Stream Played Event*/
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

    newstream->ready = FALSE;
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

/*Dont forget to clean ServerStream's minsamplevalue and maxsamplevalue while removing the stream*/
/*Delete Critical Section while cleaning Stream*/