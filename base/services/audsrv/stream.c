#include "audsrv.h"

DWORD WINAPI RunStreamThread(LPVOID param)
{
	ServerStream * localstream = (ServerStream *) param;

	SetEvent(localstream->streamready);
	printf("Signaling Mixer Thread For First Stream\n");
	SetEvent(pengine->streampresent);
	while (1){OutputDebugStringA("Stream Thread Running.");Sleep(100);};
	/*Clean Stream's data*/
}

HANDLE addstream(LONG frequency,int channels,int bitspersample, ULONG channelmask,int volume,int mute,float balance)
{
	ServerStream * newstream,*localstream;
	DWORD dwID;

	/*Add Data to Linked list*/
	localstream = pengine->serverstreamlist;
	newstream = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(ServerStream));
	if(newstream == NULL) {goto error;}
	if(volume < 0) {newstream->volume = 0;}else if (volume > 1000) {newstream->volume = 1000;}else {newstream->volume = volume;}
	if(volume < -1.0) {newstream->volume = -1.0;}else if (volume > 1.0) {newstream->volume = 1.0;}else {newstream->volume = volume;}
	newstream->freq = frequency;  /*TODO frequency validation required*/
	newstream->bitspersample = bitspersample; /*TODO bitspersample validation*/
	newstream->channels = channels; /*TODO validation*/
	newstream->channelmask = channelmask; /*TODO validation*/

	newstream->next = NULL;
	newstream->played = CreateEvent(NULL,FALSE,FALSE,NULL);
	newstream->streamready = CreateEvent(NULL,FALSE,FALSE,NULL);

	if(newstream->played == NULL || newstream->streamready == NULL) {goto error;}
	newstream->thread=CreateThread(NULL,0,RunStreamThread,newstream,0,&dwID);
	if(newstream->thread == NULL) {goto error;}

	WaitForSingleObject(newstream->streamready,INFINITE);

	if(localstream == NULL)
	{
	pengine->serverstreamlist = localstream;

	pengine->masterfreq=frequency;
	pengine->masterchannels=channels;
	pengine->masterchannelmask=channelmask;
	pengine->masterbitspersample=bitspersample;
	}
	else
	{
		while(localstream->next != NULL){localstream = localstream->next;}
		localstream->next = newstream;
	}
	return newstream->thread;

error:
	HeapFree(GetProcessHeap(), 0, newstream);
	return NULL;
}
