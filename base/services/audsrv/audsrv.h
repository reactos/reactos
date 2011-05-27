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


typedef struct PortStream
{
	int volume;
	LONG freq;
	int bitspersample;
	int channels;
	ULONG channelmask;
	HANDLE thread;
	struct PortStream * next;
} PortStream;

typedef struct MixerEngine
{
/*Should be Initialized at Server Start*/
	char dead;
	HANDLE EventPool[2];//0=Played,1=Ready
	HANDLE mixerthread;
	HANDLE playerthread;
	HANDLE rpcthread;
/*Should be Initialized at Server Start from configuration file,Currently there is no configuration file so initialized to a fixed value at start*/
	int mastervolume;
	BOOL mute;
/*Should be Initialized before playing First Stream*/
	long masterfreq;
	int masterchannels;
	unsigned long masterchannelmask;
	int masterbitspersample;
	PSHORT masterbuf;
/*Currently don't know the future of following variables*/
	HANDLE FilterHandle;
    HANDLE PinHandle;
	PKSPROPERTY Property;
	PKSSTREAM_HEADER Packet;
} MixerEngine;

extern MixerEngine engine;

/* rpc.c */
DWORD WINAPI RunRPCThread(LPVOID lpParameter);
/********************************/

#endif  /* __AUDSRV_H__ */
