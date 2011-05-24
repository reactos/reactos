
#include <audiosrv.h>

static MixerEngine * lpvMem = NULL;
static HANDLE hMapObject = NULL;

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
BOOL fInit, fIgnore;
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		hMapObject = CreateFileMapping( 
                INVALID_HANDLE_VALUE,   // use paging file
                NULL,                   // default security attributes
                PAGE_READWRITE,         // read/write access
                0,                      // size: high 32-bits
                sizeof(MixerEngine),TEXT("MixerEngine"));              // size: low 32-bits
            if (hMapObject == NULL) 
                return FALSE; 

            fInit = (GetLastError() != ERROR_ALREADY_EXISTS);

			lpvMem = (MixerEngine *)MapViewOfFile( 
                hMapObject,     // object to map view of
                FILE_MAP_WRITE, // read/write access
                0,              // high offset:  map from
                0,              // low offset:   beginning
                0);             // default: map entire file
            if (lpvMem == NULL)
                return FALSE;

            if (fInit)
			lpvMem->dead=0;
			lpvMem->masterchannels=0;
			lpvMem->masterbitspersample=0;
			lpvMem->masterchannelmask=0;
			lpvMem->masterdoublebuf[0]=NULL;
			lpvMem->masterdoublebuf[1]=NULL;
			lpvMem->masterfreq=0;
			lpvMem->mastervolume=0;
			lpvMem->mute=0;
			lpvMem->portstream=NULL;
			lpvMem->workingbuffer=0;
			lpvMem->mixerthread=NULL;
			lpvMem->playerthread=NULL;
			lpvMem->EventPool[0]=CreateEvent(NULL,FALSE,FALSE,NULL);
			lpvMem->EventPool[1]=CreateEvent(NULL,FALSE,FALSE,NULL);
 
            break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
            fIgnore = UnmapViewOfFile(lpvMem);
            fIgnore = CloseHandle(hMapObject);
		break;
	}
	return TRUE;
}

WINAPI MixerEngine * getmixerengine()
{
return lpvMem;
}
