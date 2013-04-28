;
; wmilib.def - export definition file for ReactOS
;
@ stdcall WmiCompleteRequest(ptr ptr long long long)
@ stdcall WmiFireEvent(ptr ptr long long ptr)
@ stdcall WmiSystemControl(ptr ptr ptr ptr)
