#include <stdio.h>
#include <windows.h>

/* This tests the ability of the target win32 to duplicate a process handle,
 * spawn a child, and have the child dup it's own handle back into the parent
 * using the duplicated handle.
 */

int main( int argc, char **argv ) {
  HANDLE h_process;
  HANDLE h_process_in_parent;

  fprintf( stderr, "%d: Starting\n", GetCurrentProcessId() );

  if( argc == 2 ) {
    h_process = atoi(argv[1]);
  } else {
    if( !DuplicateHandle( GetCurrentProcess(),
			  GetCurrentProcess(),
			  GetCurrentProcess(),
			  &h_process,
			  0,
			  TRUE,
			  DUPLICATE_SAME_ACCESS) ) {
      fprintf( stderr, "%d: Could not duplicate my own process handle.\n",
	       GetCurrentProcessId() );
      return 101;
    }
  }

  if( argc == 1 ) {
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    char cmdline[1000];

    memset( &si, 0, sizeof( si ) );
    memset( &pi, 0, sizeof( pi ) );

    sprintf( cmdline, "%s %d", argv[0], h_process );
    if( !CreateProcess(NULL, cmdline, NULL, NULL, TRUE, 0, NULL, NULL,
		       &si, &pi ) ) {
      fprintf( stderr, "%d: Could not create child process.\n",
	       GetCurrentProcessId() );
      return 5;
    }

    if( WaitForSingleObject( pi.hThread, INFINITE ) != WAIT_OBJECT_0 ) {
      fprintf( stderr, "%d: Failed to wait for child process to terminate.\n",
	       GetCurrentProcessId() );
      return 6;
    }
  } else {
    if( !DuplicateHandle( GetCurrentProcess(),
			  GetCurrentProcess(),
			  h_process,
			  &h_process_in_parent,
			  0,
			  TRUE,
			  DUPLICATE_SAME_ACCESS) ) {
      fprintf( stderr, "%d: Could not duplicate my handle into the parent.\n",
	       GetCurrentProcessId() );
      return 102;
    }
  }

  return 0;
}
