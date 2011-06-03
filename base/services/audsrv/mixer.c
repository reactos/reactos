
#include "audsrv.h"
void * mixs8(MixerEngine * mixer,int buffer)
{
}
void * mixs16(MixerEngine * mixer,int buffer)
{
	mixer->masterbuf[buffer] = HeapAlloc(GetProcessHeap(), 0, mixer->serverstreamlist->length_filtered);
	CopyMemory(mixer->masterbuf[buffer],mixer->serverstreamlist->filteredbuf,mixer->serverstreamlist->length_filtered);
	mixer->bytes_to_play = mixer->serverstreamlist->length_filtered;
}
void * mixs32(MixerEngine * mixer,int buffer)
{
}
void * mixs64(MixerEngine * mixer,int buffer)
{
}
void * mixu8(MixerEngine * mixer,int buffer)
{
}
void * mixu16(MixerEngine * mixer,int buffer)
{
}
void * mixu32(MixerEngine * mixer,int buffer)
{
}
void * mixu64(MixerEngine * mixer,int buffer)
{
}
void * mixfl32(MixerEngine * mixer,int buffer)
{
}
void * mixfl64(MixerEngine * mixer,int buffer)
{
}