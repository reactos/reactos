VOID NdrClientInitializeNew(PRPC_MESSAGE Message,
			    PMIDL_STUB_MESSAGE StubMsg,
			    PMIDL_STUB_DESC StubDesc,
			    ULONG a)
{
   NdrClientInitialize(Message,
		       StubMsg,
		       StubDesc,
		       a);
   if (StubDe)
     {
	NdrpSetRpcSsDefaults([eax], [eax+4])
     }
}
