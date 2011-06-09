/*
 * PROJECT:          ReactOS kernel
 * LICENSE:          GPL - See COPYING in the top level directory
 * FILE:             services/mixer.c
 * PURPOSE:          Audio Server
 * COPYRIGHT:        Copyright 2011 Neeraj Yadav

 */

#include "audsrv.h"
void * mixs8(MixerEngine * mixer,int buffer)
{
    return NULL;
}
void * mixs16(MixerEngine * mixer,int buffer)
{
    mixer->masterbuf[buffer] = HeapAlloc(GetProcessHeap(), 0, mixer->serverstreamlist->length_filtered);
    CopyMemory(mixer->masterbuf[buffer],mixer->serverstreamlist->filteredbuf,mixer->serverstreamlist->length_filtered);
    mixer->bytes_to_play = mixer->serverstreamlist->length_filtered;

    return NULL;
}
void * mixs32(MixerEngine * mixer,int buffer)
{
    return NULL;
}
void * mixs64(MixerEngine * mixer,int buffer)
{
    return NULL;
}
void * mixu8(MixerEngine * mixer,int buffer)
{
    return NULL;
}
void * mixu16(MixerEngine * mixer,int buffer)
{
    return NULL;
}
void * mixu32(MixerEngine * mixer,int buffer)
{
    return NULL;
}
void * mixu64(MixerEngine * mixer,int buffer)
{
    return NULL;
}
void * mixfl32(MixerEngine * mixer,int buffer)
{
    return NULL;
}
void * mixfl64(MixerEngine * mixer,int buffer)
{
    return NULL;
}