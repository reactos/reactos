bits 32
section .text

DECLARE_GLOBAL_SYMBOL InterlockedIncrement
       push ebp
       mov  ebp,esp

       push eax
       push ebx

       mov eax,1
       mov ebx,[ebp+8]
       xadd [ebx],eax

       pop ebx
       pop eax

       mov esp,ebp
       pop ebp

       ret
       
       
DECLARE_GLOBAL_SYMBOL InterlockedDecrement
       mov eax,0xffffffff
       mov ebx,[esp+4]
       xadd [ebx],eax
       dec eax
       ret
       
DECLARE_GLOBAL_SYMBOL InterlockedExchange       
       push ebp
       mov  ebp,esp

       push eax
       push ebx

       mov eax,[ebp+12]
       mov ebx,[ebp+8]
       xchg [ebx],eax
       
       pop ebx
       pop eax
       
       mov esp,ebp
       pop ebp
       ret

DECLARE_GLOBAL_SYMBOL InterlockedExchangeAdd
       mov eax,[esp+8]
       mov ebx,[esp+4]
       xadd [ebx],eax
       ret

DECLARE_GLOBAL_SYMBOL InterlockedCompareExchange
       mov eax,[esp+12]
       mov edx,[esp+8]
       mov ebx,[esp+4]
       cmpxchg [ebx],edx
       mov eax,edx
       ret 
