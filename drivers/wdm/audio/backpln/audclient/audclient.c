/*
 * PROJECT:          ReactOS kernel
 * LICENSE:          GPL - See COPYING in the top level directory
 * FILE:             services/audsrv.c
 * PURPOSE:          Audio Server
 * COPYRIGHT:        Copyright 2011 Neeraj Yadav

 */

#define WIN32_NO_STATUS
#define _KSDDK_

#include <audsrvapi.h>
#include <stdio.h>
#include <debug.h>
#include <windows.h>
#include <winbase.h>

void OpenComplete (int error );
void BufferCopied (int error );
void PlayComplete (int error );

ClientStream clientstream = {0,
                             0,
                             {NULL},
                                {OpenComplete,
                                 BufferCopied,
                                 PlayComplete}
                            };

DWORD WINAPI RunAudioThread(LPVOID param)
{
    ClientStream * localstream = (ClientStream *) param;
    PlayAudio(localstream);

    return 0;
}
void OpenComplete (int error )
{
    /*Copy First Buffer and write*/
    Write(&clientstream,
          "HELLO_RANDOM_STRING");
}
void BufferCopied (int error )
{
    Write(&clientstream,
          "HELLO_RANDOM_STRING");
}
void PlayComplete (int error )
{
}
int
__cdecl
wmain(int argc,
      char* argv[])
{
    int error;
    DWORD dwID;
    HANDLE audiothread = NULL;
    char input='\0';
    printf("ReactOS Audio Mixer Sample Client.Enter 'a' to Stop.\nPlease Enter the frequency of the sinusoidal wave [Hz] : ");
	scanf("%ld",&(clientstream.wavefreq));

    /*[out]HANDLE * streamhandle,[in] long frequency,[in] int number of channels,[in] int bitspersample,[in]ULONG channelmask,[in] int volume,[in] int mute,[in] float balance*/
    error = InitStream ( &clientstream ,
                         44100 ,
                         2 ,
                         16 ,
                         0,
                         KSAUDIO_SPEAKER_STEREO ,
                         1000 ,
                         0,
                         0.0 );

    if ( error )
    {
        goto error;
    }
    else
    {
        audiothread = CreateThread(NULL,0,RunAudioThread,&clientstream,0,&dwID);
    }

    while ( input != 'a' )
        scanf("%c",&input);

    StopAudio(&clientstream);
    WaitForSingleObject(audiothread,INFINITE);

    return 0;

error:
    return -1;
}