#include <windows.h>
#include <stdio.h>

HANDLE events[2];

DWORD WINAPI thread( LPVOID crap )
{
	SetEvent( events[0] );
	if( crap )
		SetEvent( events[1] );
	return 1;
}

int main()
{
	DWORD id, Status;
	printf( "Creating events\n" );
	events[0] = CreateEvent( 0, TRUE, FALSE, 0 );
	events[1] = CreateEvent( 0, TRUE, FALSE, 0 );
	printf( "Created events\n" );
	CreateThread( 0, 0, thread, 0, 0, &id );
	printf( "WaitForSingleObject %s\n", ( WaitForSingleObject( events[0], INFINITE ) == WAIT_OBJECT_0 ? "worked" : "failed" ) );
	ResetEvent( events[0] );
	CreateThread( 0, 0, thread, 0, 0, &id );
	printf( "WaitForMultipleObjects with waitall = FALSE %s\n", ( WaitForMultipleObjects( 2, events, FALSE, INFINITE ) == WAIT_OBJECT_0 ? "worked" : "failed" ) );
	ResetEvent( events[0] );
	CreateThread( 0, 0, thread, (void *)1, 0, &id );
	Status = WaitForMultipleObjects( 2, events, TRUE, INFINITE );
	printf( "WaitForMultipleObjects with waitall = TRUE %s\n", ( Status == WAIT_OBJECT_0 || Status == WAIT_OBJECT_0 + 1 ? "worked" : "failed" ) );
	ResetEvent( events[0] );
	printf( "WaitForSingleObject with timeout %s\n", ( WaitForSingleObject( events[0], 100 ) == WAIT_TIMEOUT ? "worked" : "failed" ) );
	return 0;
}
