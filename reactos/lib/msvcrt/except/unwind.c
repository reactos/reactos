#include <windows.h>

void __cdecl
_global_unwind2(PEXCEPTION_REGISTRATION RegistrationFrame)
{
   RtlUnwind(RegistrationFrame, &&__ret_label, NULL, 0);
__ret_label:
   // return is important
   return;
}
