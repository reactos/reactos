#include "audsrv.h"

long getnewstreamid()
{
	long streamid= pengine->streamidpool;
	pengine->streamidpool+=1;
	return streamid;
}
DWORD WINAPI RunStreamThread(LPVOID param)
{
	UINT i = 0;
	ServerStream * localstream = (ServerStream *) param;


	printf("Signaling Mixer Thread For First Stream\n");
/*HACK fill filtered buffer (1 second duration in the master stream format) directly until we are in a condition to get buffer directly from the client*/
/******************************************************/
PSHORT tempbuf;
localstream->ready =TRUE;
localstream->length_filtered = localstream->freq * localstream->channels * localstream->bitspersample / 8;
tempbuf = (PSHORT)HeapAlloc(GetProcessHeap(), 0, localstream->length_filtered);
    while (i < localstream->length_filtered / 2)
    {
        tempbuf[i] = 0x7FFF * sin(0.5 * (i - 1) * 500 * 6.28 / 48000);
        i++;
        tempbuf[i] = 0x7FFF * sin(0.5 * (i - 2) * 500 * 6.28 / 48000);
        i++;
    }
localstream->filteredbuf = tempbuf;
/******************************************************/
	SetEvent(localstream->threadready);

	while (1){OutputDebugStringA("Stream Thread Running.");Sleep(100);};
	/*Clean Stream's data*/
}

long addstream(LONG frequency,int channels,int bitspersample,int datatype, ULONG channelmask,int volume,int mute,float balance)
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
	if(datatype==0 || datatype==1 || datatype==2){newstream->datatype=datatype;}else goto error;
	if	((datatype==0 && (bitspersample == 8 || bitspersample == 16 || bitspersample == 32 || bitspersample == 64 )) ||
		(datatype==1 && (bitspersample == 8 || bitspersample == 16 || bitspersample == 32 || bitspersample == 64)) ||
		(datatype==2 && (bitspersample == 32 || bitspersample == 64)) )
	newstream->bitspersample = bitspersample; /*TODO bitspersample validation*/
	else goto error;

	newstream->channels = channels; /*TODO validation*/
	newstream->channelmask = channelmask; /*TODO validation*/

	newstream->ready = FALSE;
	newstream->length_genuine = 0;
	newstream->genuinebuf = NULL;
	newstream->length_filtered = 0;
	newstream->filteredbuf = NULL;
	newstream->minsamplevalue = NULL;
	newstream->maxsamplevalue = NULL;
	
	newstream->next = NULL;
	newstream->played = CreateEvent(NULL,FALSE,FALSE,NULL);
	newstream->threadready = CreateEvent(NULL,FALSE,FALSE,NULL);

	if(newstream->played == NULL || newstream->threadready == NULL) {goto error;}
	newstream->thread=CreateThread(NULL,0,RunStreamThread,newstream,0,&dwID);
	if(newstream->thread == NULL) {goto error;}


	WaitForSingleObject(newstream->threadready,INFINITE);

	newstream->streamid=getnewstreamid();
	if(localstream == NULL)
	{
	pengine->serverstreamlist = newstream;

	pengine->masterfreq=frequency;
	pengine->masterchannels=channels;
	pengine->masterchannelmask=channelmask;
	pengine->masterbitspersample=bitspersample;
	pengine->masterdatatype = datatype;
	}
	else
	{
		while(localstream->next != NULL){localstream = localstream->next;}
		localstream->next = newstream;
	}
	SetEvent(pengine->streampresent);
	return newstream->streamid;

error:
	printf("Stream Rejected \n");
	HeapFree(GetProcessHeap(), 0, newstream);
	return 0;
}
