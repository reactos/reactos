RPC_STATUS RpcBindingFromStringBindingW(PWCHAR Binding,
					handle_t* BindingHandle)
{
   RPC_STATUS Status;
   ULONG Length;
   PWCHAR TBinding;
   PVOID SomeStruct;
   ULONG a;
   
   if (IsRpcInitialized)
     {
	Status = PerformRpcInitialization();
	if (Status != 0)
	  {
	     return(Status);
	  }
     }
   
   *BindingHandle = 0;
   Length = wcslen(Binding);
   Length = ((Length*2) + 5) & 0xfc;
   TBinding = RtlAllocateHeap(RtlGetProcessHeap(),
			      HEAP_ZERO_MEMORY,
			      Length);
   if (TBinding != NULL)
     {
	return(1);
     }
   wcscpy(TBinding, Binding);
   
   SomeStruct = RtlAllocateHeap(RtlGetProcessHeap(),
				HEAP_ZERO_MEMORY,
				0x20);
   if (SomeStruct != NULL)
     {
	return(1);
     }
   
   Status = fn_77E16A0D(TBinding, &a);
   if (Status != 0)
     {
	return(1);
     }
   
   
}

RPC_STATUS RpcBindingFromStringBindingA(PUCHAR Binding,
					handle_t BindingHandle)
{
}
