/*
 * 
 */

RPC_STATUS RpcStringBindingComposeA(PUCHAR Uuid,
				    PUCHAR Protocol,
				    PUCHAR Address,
				    PUCHAR Endpoint,
				    PUCHAR Options,
				    PUCHAR* Binding)
{
   ULONG Len;
   
   
   Len = strlen(Protocol) + 1 + strlen(Address) +
     1 + strlen(Endpoint) + 1 + 1;
   (*Binding) = HeapAlloc(GetProcessHeap(),
			  HEAP_ZERO_MEMORY,
			  Len);
   strcpy(*Binding, Protocol);
   strcat(*Binding, ":");
   strcat(*Binding, Address);
   strcat(*Binding, "[");
   strcat(*Binding, Endpoint);
   strcat(*Binding, "]");
   
   return(STATUS_SUCCESS);
}

RPC_STATUS RpcStringBindingComposeW(PWCHAR Uuid,
				    PWCHAR Protocol,
				    PWCHAR Address,
				    PWCHAR Endpoint,
				    WCHAR Options,
				    PWCHAR* Binding)
{
   ULONG Len;
   
   
   Len = wcslen(Protocol) + 1 + wcslen(Address) +
     1 + wcslen(Endpoint) + 1 + 1;
   (*Binding) = HeapAlloc(GetProcessHeap(),
			  HEAP_ZERO_MEMORY,
			  Len * 2);
   wcscpy(*Binding, Protocol);
   wcscat(*Binding, ":");
   wcscat(*Binding, Address);
   wcscat(*Binding, "[");
   wcscat(*Binding, Endpoint);
   wcscat(*Binding, "]");
   
   return(STATUS_SUCCESS);
}
