file ntoskrnl/ntoskrnl.nostrip.exe
add-symbol-file lib/ntdll/ntdll.dll 0x77f61000
add-symbol-file subsys/smss/smss.exe 0x401000
#add-symbol-file subsys/csrss/csrss.exe 0x401000
break exp.c:156
