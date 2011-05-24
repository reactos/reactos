#include <windows.h>
#include <memory.h>

#ifndef _PORTINTERFACE_H
#define _PORTINTERFACE_H

#define MIXER getmixerengine()

typedef struct PortStream
{
	int volume;
	double freq;
	int channels;
	int bitspersample;
	DWORD channelmask;
	HANDLE thread;
	struct PortStream * next;
} PortStream;

typedef struct MixerEngine
{
	int mastervolume;
	int mute;
	char dead;
	double masterfreq;
	int masterchannels;
	DWORD masterchannelmask;
	int masterbitspersample;
	int workingbuffer;
	PSHORT masterdoublebuf[2];
	HANDLE mixerthread;
	HANDLE playerthread;
	HANDLE EventPool[2];//0=Played,1=Ready
	PortStream * portstream;
} MixerEngine;

#ifdef __cplusplus
extern "C" {
#endif

WINAPI MixerEngine * getmixerengine();

#ifdef __cplusplus 
}
#endif

#endif