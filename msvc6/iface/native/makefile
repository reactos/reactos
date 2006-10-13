# $Id$
#
# ReactOS Operating System - MSVC6 Zw functions stubs to call generator
#

SYSTEM_CALLS_DB = ..\..\..\reactos\iface\native\sysfuncs.lst
KERNEL_ZW_CALLS =..\..\ntoskrnl\nt_zw_msvc.c

all: genntdll.exe $(KERNEL_ZW_CALLS)

$(KERNEL_ZW_CALLS) : $(SYSTEM_CALLS_DB)
	genntdll.exe $(SYSTEM_CALLS_DB) foo1 foo2 $(KERNEL_ZW_CALLS)
	-@del foo1
	-@del foo2

genntdll.exe : genntdll.c

clean:
	-@del *.exe
	-@del *.obj
	-@del $(KERNEL_ZW_CALLS)
	
