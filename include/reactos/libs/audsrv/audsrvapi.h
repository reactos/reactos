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
} ClientStream;

/********************API Functions******************/
WINAPI int initstream (ClientStream * clientstream,
                       LONG frequency,
                       int channels,
                       int bitspersample,
                       int datatype,    /*0=signed int,1=unsigned int,2=float*/
                       ULONG channelmask,
                       int volume,
                       int mute,
                       float balance);

WINAPI int playaudio ( ClientStream * clientstream);
WINAPI int stopaudio (ClientStream * clientstream );

WINAPI int Volume(ClientStream * clientstream,
                  int * volume );

WINAPI int SetVolume(ClientStream * clientstream ,
                     const int newvolume);
WINAPI int Write(ClientStream * clientstream ,
                 const char * aData);
WINAPI int SetBalance(ClientStream * clientstream ,
                      float balance);
WINAPI int GetBalance(ClientStream * clientstream ,
                      float * balance);
#endif