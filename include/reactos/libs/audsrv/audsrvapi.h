#ifndef _AUDSRVAPI_H
#define _AUDSRVAPI_H

#include <windows.h>
#include <ks.h>
#include <ksmedia.h>
#include <stdio.h>

/********************Structures*********************/

typedef struct CallBacks
{
    void (*OpenComplete) (int error );
    void (*BufferCopied) (int error );
    void (*PlayComplete) (int error );
} CallBacks;

typedef struct ClientStream
{
    long stream;
    int dead;
    HANDLE ClientEventPool[1];
    struct CallBacks callbacks;

    /*Just for the time being when we dont have any audio source*/
	long wavefreq;
} ClientStream;

/********************API Functions******************/
int
WINAPI
InitStream (ClientStream * clientstream,
                       LONG frequency,
                       int channels,
                       int bitspersample,
                       int datatype,    /*0=signed int,1=unsigned int,2=float*/
                       ULONG channelmask,
                       int volume,
                       int mute,
                       float balance);

int
WINAPI
PlayAudio ( ClientStream * clientstream);

int
WINAPI
StopAudio (ClientStream * clientstream );

int
WINAPI
Volume(ClientStream * clientstream,
                  int * volume );

int
WINAPI
SetVolume(ClientStream * clientstream ,
                     const int newvolume);
int
WINAPI
Write(ClientStream * clientstream ,
                 const char * aData);

int
WINAPI
SetBalance(ClientStream * clientstream ,
                      float balance);
int
WINAPI
GetBalance(ClientStream * clientstream ,
                      float * balance);
#endif