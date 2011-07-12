/*
 * PROJECT:          ReactOS kernel
 * LICENSE:          GPL - See COPYING in the top level directory
 * FILE:             services/audsrv/audsrv.h
 * PURPOSE:          Audio Service
 * COPYRIGHT:        Copyright 2011 Neeraj Yadav
 */

#ifndef __AUDSRV_H__
#define __AUDSRV_H__

#define NDEBUG
#define WIN32_NO_STATUS

#include <windows.h>
#include <wincon.h>
#include <memory.h>
#include <mmsystem.h>
#include <stdio.h>
#include <math.h>
#include <setupapi.h>
#include <ndk/ntndk.h>
#include <ks.h>
#include <ksmedia.h>
#include <ks.h>
#include <debug.h>
#include "audsrvrpc_s.h"


typedef struct ServerStream
{
    long streamid;
    int volume;
    LONG freq;
    int bitspersample;

    /*0=signed int,1=unsigned int,2=float*/
    int datatype;

    int channels;

    /*Standard channelmasks from WINAPI/ksmeida.h*/
    ULONG channelmask;

    /*Balance from -1.0 to 1.0*/
    float balance;
    /*state cycle   0->buffer write -> 1 -> filtering -> 2 ->playback -> 0*/
	char state;
    /*This buffer is filled by the client using RPC calls*/
    PVOID genuinebuf;
    int length_genuine;
    /*This Buffer is filled by Stream-Specific Server Thread, This Buffer's matches masterbuffer specifications*/
    PVOID filteredbuf;
    int length_filtered;
    /*These values must be filled by Stream-Specific Server Thread,these are helful for Mixer Thread*/
    PVOID minsamplevalue;
    PVOID maxsamplevalue;

    HANDLE stream_played_event;
    HANDLE buffer_write_event;
    HANDLE threadready;
    HANDLE thread;
    CRITICAL_SECTION CriticalSection;

    struct ServerStream * next;
} ServerStream;

typedef struct MixerEngine
{
    /*Should be Initialized at Server Start*/
    char dead;
    long streamidpool;
    HANDLE played;
    HANDLE filled;
    HANDLE newStreamEvent;
    HANDLE mixerthread;
    HANDLE playerthread;
    HANDLE rpcthread;
    int playcurrent;
    /*Should be Initialized at Server Start from configuration file,Currently there is no configuration file so initialized to a fixed value at start*/
    int mastervolume;
    BOOL mute;
    /*Should be Initialized before playing First Stream*/
    long masterfreq;
    int masterchannels;
    unsigned long masterchannelmask;
    int masterbitspersample;
    int masterdatatype;
    PVOID masterbuf[2];
    /*Currently don't know the future of following variables*/
    long bytes_to_play;
    HANDLE FilterHandle;
    HANDLE PinHandle;
    PKSPROPERTY Property;
    PKSSTREAM_HEADER Packet;
    ServerStream * serverstreamlist;
} MixerEngine;

extern MixerEngine engine,*pengine;

/* rpc.c */
DWORD WINAPI RunRPCThread(LPVOID lpParameter);

/* audsrv.c*/
void MixAndFill(MixerEngine * mixer,
                int buffer);
void PlayBuffer(MixerEngine * mixer,
                int buffer);

/*stream.c*/
long GetNewStreamID();
long AddStream(LONG frequency,
               int channels,
               int bitspersample,
               int datatype,
               ULONG channelmask,
               int volume,
               int mute,
               float balance);

long WriteBuffer(LONG streamid,
                 LONG length,
                 LPVOID buffer);
/*mixer.c*/
void * MixS8(MixerEngine * mixer,
             int buffer);

void * MixS16(MixerEngine * mixer,
              int buffer);

void * MixS32(MixerEngine * mixer,
              int buffer);

void * MixS64(MixerEngine * mixer,
              int buffer);

void * MixU8(MixerEngine * mixer,
             int buffer);

void * MixU16(MixerEngine * mixer,
              int buffer);

void * MixU32(MixerEngine * mixer,
              int buffer);

void * MixU64(MixerEngine * mixer,
              int buffer);

void * MixFL32(MixerEngine * mixer,
               int buffer);

void * MixFL64(MixerEngine * mixer,
               int buffer);

#endif  /* __AUDSRV_H__ */
