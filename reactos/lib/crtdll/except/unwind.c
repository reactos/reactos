void _global_unwind2( PEXCEPTION_FRAME frame )
{
	//RtlUnwind( frame, 0, NULL, 0 );
}
 

void _local_unwind2( PEXCEPTION_FRAME endframe, DWORD nr )
{
     //TRACE(crtdll,"(%p,%ld)\n",endframe,nr);
	return;
}
