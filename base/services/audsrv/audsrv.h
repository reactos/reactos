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
	long streamid;
	int volume;
	LONG freq;
	int bitspersample;
	int datatype;  /*0=signed int,1=unsigned int,2=float*/
	int channels;
	ULONG channelmask;
	HANDLE played;
	HANDLE threadready;
	HANDLE thread;
	float balance;
	BOOL ready;
	PVOID genuinebuf;
	int length_genuine;
	PVOID filteredbuf;
	int length_filtered;
	PVOID minsamplevalue;
	PVOID maxsamplevalue;
	struct ServerStream * next;
} ServerStream;

typedef struct MixerEngine
{
/*Should be Initialized at Server Start*/
	char dead;
	long streamidpool;
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
void mixandfill(MixerEngine * mixer,int buffer);
void playbuffer(MixerEngine * mixer,int buffer);
/*stream.c*/
long getnewstreamid();
long addstream(LONG frequency,int channels,int bitspersample,int datatype, ULONG channelmask,int volume,int mute,float balance);
/*mixer.c*/
void * mixs8(MixerEngine * mixer,int buffer);
void * mixs16(MixerEngine * mixer,int buffer);
void * mixs32(MixerEngine * mixer,int buffer);
void * mixs64(MixerEngine * mixer,int buffer);
void * mixu8(MixerEngine * mixer,int buffer);
void * mixu16(MixerEngine * mixer,int buffer);
void * mixu32(MixerEngine * mixer,int buffer);
void * mixu64(MixerEngine * mixer,int buffer);
void * mixfl32(MixerEngine * mixer,int buffer);
void * mixfl64(MixerEngine * mixer,int buffer);
/********************************/

#endif  /* __AUDSRV_H__ */
