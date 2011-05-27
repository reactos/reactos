#define WIN32_NO_STATUS
#define _KSDDK_

#include <audsrvapi.h>
#include <stdio.h>

void opencomplete (int error );
void buffercopied (int error );
void playcomplete (int error );

ClientStream clientstream = {NULL,0,{NULL},{opencomplete,buffercopied,playcomplete}};/*This initialization should not be necessary for a typical client*/

DWORD WINAPI RunAudioThread(LPVOID param)
{
	ClientStream * localstream = (ClientStream *) param;
	printf("Starting Audio Thread \n");
	playaudio(localstream);
}
void opencomplete (int error )
{
	/*Copy First Buffer and write*/
	Write(&clientstream,"HELLO_RANDOM_STRING");
}
void buffercopied (int error )
{
	Write(&clientstream,"HELLO_RANDOM_STRING");
}
void playcomplete (int error )
{
	printf("Play Complete Code %d\n",error);
}
int
__cdecl
wmain(int argc, char* argv[])
{
	int error;
	DWORD dwID;
	HANDLE audiothread;
	char input='\0';
	printf("ReactOS Audio Mixer Sample Client.Enter 'a' to Stop.\n");
	//if (clientstream->callbacks.OpenComplete == NULL || clientstream->callbacks.BufferCopied == NULL || clientstream->callbacks.PlayComplete == NULL) printf("");
	error = initstream ( &clientstream , 44100 , 2 , 16 , KSAUDIO_SPEAKER_STEREO , 1000 , 0, 0.0 ); /*[out]HANDLE * streamhandle,[in] long frequency,[in] int number of channels,[in] int bitspersample,[in]ULONG channelmask,[in] int volume,[in] int mute,[in] float balance*/
	if ( error )
		printf("Failed to Initialize Stream.Error %d\n", error);
	else
	{
		printf("StreamID : %d\n",*((int *)clientstream.stream));
		audiothread = CreateThread(NULL,0,RunAudioThread,&clientstream,0,&dwID);
	}
	while ( input != 'a' )
	scanf("%c",&input);
	printf("Stoping Audio Stream.\n");
	stopaudio(&clientstream);
	WaitForSingleObject(audiothread,INFINITE);
    return 0;
}