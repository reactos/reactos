/*
 * PROJECT:          ReactOS kernel
 * LICENSE:          GPL - See COPYING in the top level directory
 * FILE:             services/audsrv/audsrv.h
 * PURPOSE:          Event logging service
 * COPYRIGHT:        Copyright 2011 Pankaj Yadav
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
	int volume;
	LONG freq;
	int bitspersample;
	int channels;
	ULONG channelmask;
	HANDLE played;
	HANDLE streamready;
	HANDLE thread;
	float balance;
	struct ServerStream * next;
} ServerStream;

typedef struct MixerEngine
{
/*Should be Initialized at Server Start*/
	char dead;
	HANDLE played;
	HANDLE filled;
	HANDLE streampresent;
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
	PSHORT masterbuf[2];
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
void fill(MixerEngine * mixer,int buffer);
void playbuffer(MixerEngine * mixer,int buffer);
/*stream.c*/
HANDLE addstream(LONG frequency,int channels,int bitspersample, ULONG channelmask,int volume,int mute,float balance);
/********************************/

#endif  /* __AUDSRV_H__ */
