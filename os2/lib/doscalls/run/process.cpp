
/* Terminates the current thread or the current Process.
	Decission is made by action 
	FIXME:	move this code to OS2.EXE */
VOID APIENTRY Dos32Exit(ULONG action, ULONG result)
{
	// decide what to do
	if( action == EXIT_THREAD)
	{
		NtTerminateThread( NULL, result );
	}
	else	// EXIT_PROCESS
	{
		NtTerminateProcess( NULL, result );
	}
}

