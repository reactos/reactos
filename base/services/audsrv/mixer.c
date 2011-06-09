/*
 * PROJECT:          ReactOS kernel
 * LICENSE:          GPL - See COPYING in the top level directory
 * FILE:             services/mixer.c
 * PURPOSE:          Audio Server
 * COPYRIGHT:        Copyright 2011 Neeraj Yadav

 */

#include "audsrv.h"
void * MixS8(MixerEngine * mixer,int buffer)
{
    return NULL;
}
void * MixS16(MixerEngine * mixer,int buffer)
{
    mixer->masterbuf[buffer] = HeapAlloc(GetProcessHeap(), 0, mixer->serverstreamlist->length_filtered);
    CopyMemory(mixer->masterbuf[buffer],mixer->serverstreamlist->filteredbuf,mixer->serverstreamlist->length_filtered);
    mixer->bytes_to_play = mixer->serverstreamlist->length_filtered;

    return NULL;
}
void * MixS32(MixerEngine * mixer,int buffer)
{
    return NULL;
}
void * MixS64(MixerEngine * mixer,int buffer)
{
    return NULL;
}
void * MixU8(MixerEngine * mixer,int buffer)
{
    return NULL;
}
void * MixU16(MixerEngine * mixer,int buffer)
{
    return NULL;
}
void * MixU32(MixerEngine * mixer,int buffer)
{
    return NULL;
}
void * MixU64(MixerEngine * mixer,int buffer)
{
    return NULL;
}
void * MixFL32(MixerEngine * mixer,int buffer)
{
    return NULL;
}
void * MixFL64(MixerEngine * mixer,int buffer)
{
    return NULL;
}