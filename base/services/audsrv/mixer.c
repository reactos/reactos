/*
 * PROJECT:          ReactOS kernel
 * LICENSE:          GPL - See COPYING in the top level directory
 * FILE:             services/mixer.c
 * PURPOSE:          Audio Server
 * COPYRIGHT:        Copyright 2011 Neeraj Yadav

 */

#include "audsrv.h"
void * MixS8(MixerEngine * mixer,
             int buffer)
{
    return NULL;
}
void * MixS16(MixerEngine * mixer,
              int buffer)
{
    int length=0;
    short minsamplevalue,maxsamplevalue;
    float coefficient = 1.0;
    int streamcount = 0,i;
    PSHORT localsinkbuf,localsrcbuf;
    ServerStream * stream = mixer->serverstreamlist;

    /*TODOAssert(mixer->serverstreamlist == NULL)*/

    /*Find the Longest Buffer within all ServerStreams*/
    length = stream->length_filtered;
    while(stream->next != NULL)
    {
        if(stream->length_filtered > length && stream->ready == TRUE )
            length = stream->length_filtered;
        stream = stream->next;
    }

    /*Allocate MasterBuffer*/
    mixer->masterbuf[buffer] = HeapAlloc(GetProcessHeap(), 0, length);
    localsinkbuf = mixer->masterbuf[buffer];
    mixer->bytes_to_play = length;

    /*Perform Actual Mixing*/
    stream = mixer->serverstreamlist;
    minsamplevalue = *(short *) stream->minsamplevalue;
    maxsamplevalue = *(short *) stream->maxsamplevalue;

    while( stream != NULL)
    {
        localsrcbuf = stream->filteredbuf;
        if(stream->ready == TRUE )
        {
		    coefficient = 1.0;

            if( *(short *)stream->maxsamplevalue != maxsamplevalue ||
                *(short *)stream->minsamplevalue != minsamplevalue  )
            {
                if( (float) maxsamplevalue / (float)*(short *)stream->maxsamplevalue < 
                    (float) minsamplevalue / (float)*(short *)stream->minsamplevalue )
                    coefficient = (float) maxsamplevalue / (float)*(short *)stream->maxsamplevalue;
                else
                    coefficient = (float) minsamplevalue / (float)*(short *)stream->minsamplevalue;
            }
			for(i=0;i<stream->length_filtered/sizeof(short);i++)
            {
                localsinkbuf[i] = (short) (( (localsinkbuf[i] * streamcount) + ((short)((float)  localsrcbuf[i] ) * coefficient) ) / (streamcount +1));
			}
        }
        //stream->ready = 0;  /*TODO Enable it when actual filter thread starts working*/
        streamcount++;
        stream = stream->next;
    }

    return NULL;
}
void * MixS32(MixerEngine * mixer,
              int buffer)
{
    return NULL;
}
void * MixS64(MixerEngine * mixer,
              int buffer)
{
    return NULL;
}
void * MixU8(MixerEngine * mixer,
             int buffer)
{
    return NULL;
}
void * MixU16(MixerEngine * mixer,
              int buffer)
{
    return NULL;
}
void * MixU32(MixerEngine * mixer,
              int buffer)
{
    return NULL;
}
void * MixU64(MixerEngine * mixer,
              int buffer)
{
    return NULL;
}
void * MixFL32(MixerEngine * mixer,
               int buffer)
{
    return NULL;
}
void * MixFL64(MixerEngine * mixer,
               int buffer)
{
    return NULL;
}