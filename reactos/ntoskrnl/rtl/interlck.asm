bits 32
section .text

global _InterlockedIncrement
_InterlockedIncrement
       mov eax,1
       mov ebx,[esp+4]
       xadd [ebx],eax
       ret

global _InterlockedDecrement
_InterlockedDecrement:       
       mov eax,0xffffffff
       mov ebx,[esp+4]
       xadd [ebx],eax
       dec eax
       ret
       
global _InterlockedExchange
_InterlockedExchange:
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
       
global _InterlockedExchangeAdd
_InterlockedExchangeAdd:
       mov eax,[esp+8]
       mov ebx,[esp+4]
       xadd [ebx],eax
       ret
       
global _InterlockedCompareExchange
_InterlockedCompareExchange:
       mov eax,[esp+12]
       mov edx,[esp+8]
       mov ebx,[esp+4]
       cmpxchg [ebx],edx
       mov eax,edx
       ret 
