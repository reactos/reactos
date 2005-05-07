/**
 * Test case for PostThreadMessage
 * (C) 2003 ReactOS
 * License: LGPL
 * See: LGPL.txt in top directory.
 * Author: arty
 *
 * Windows thread message queue test case.
 * Derived from ../event/event.c in part.
 */

#include <windows.h>
#include <stdio.h>
#include <assert.h>

HANDLE hWaitForFailure;
HANDLE hOkToPostThreadMessage;
HANDLE hOkToTerminate;

DWORD WINAPI thread( LPVOID crap )
{
	MSG msg;

	/* Failure case ... Wait for the parent to try to post a message
	   before queue creation */
	printf( "Waiting to create the message queue.\n" );

        WaitForSingleObject(hWaitForFailure,INFINITE);

	printf( "Creating message queue.\n" );

	/* "Create" a message queue */
	PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE );

	printf( "Signalling the parent that we're ready.\n" );

	/* Signal that it's ok to post */
	SetEvent( hOkToPostThreadMessage );

	printf( "Listening messages.\n" );

	/* Now read some messages */
	while( GetMessage( &msg, 0,0,0 ) ) {
		printf( "Received message: %04x %04x %08lx\n",
		        (msg.message & 0xffff),
			(msg.wParam & 0xffff),
			msg.lParam );
		assert( !msg.hwnd );
	}

	printf( "Finished receiving messages.\n" );
	SetEvent( hOkToTerminate );

	return 0;
}

int main( int argc, char **argv )
{
	DWORD id;

	printf( "Creating events\n" );

	hOkToPostThreadMessage = CreateEvent( NULL, FALSE, FALSE, NULL );
	hOkToTerminate = CreateEvent( NULL, FALSE, FALSE, NULL );
        hWaitForFailure = CreateEvent( NULL, FALSE, FALSE, NULL );

	printf( "Created events\n" );

	if( CreateThread( 0, 0, thread, 0, 0, &id ) == NULL ) {
		printf( "Couldn't create one thread.\n" );
		return 0;
        }

        printf( "Posting to non-existent queue\n" );

	/* Check failure case */
	assert( PostThreadMessage( id, WM_USER + 0, 1, 2 ) == FALSE );

	printf( "Signalling thread to advance.\n" );

        SetEvent( hWaitForFailure );

	printf( "Waiting for signal from thread.\n" );
	WaitForSingleObject( hOkToPostThreadMessage, INFINITE );

	printf( "Sending three messages, then quit.\n" );
	assert( PostThreadMessage( id, WM_USER + 0, 1, 2 ) );
	assert( PostThreadMessage( id, WM_USER + 1, 3, 4 ) );
	Sleep( 500 ); /* Sleep a bit, so that the queue is empty for a bit. */
	assert( PostThreadMessage( id, WM_USER + 2, 5, 6 ) );
	assert( PostThreadMessage( id, WM_QUIT, 0,0 ) );

	WaitForSingleObject( hOkToTerminate, INFINITE );
	printf( "Test complete.\n" );

	return 0;
}
