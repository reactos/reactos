<?xml version="1.0"?>
<!DOCTYPE project SYSTEM "tools/rbuild/project.dtd">
<project name="ReactOS" makefile="makefile.ppc" xmlns:xi="http://www.w3.org/2001/XInclude">
	<xi:include href="config-ppc.rbuild">
		<xi:fallback>
			<xi:include href="config-ppc.template.rbuild" />
		</xi:fallback>
	</xi:include>

	<xi:include href="ReactOS-generic.rbuild" />

	<property name="MKHIVE_OPTIONS" value="-be" />
	<property name="OFWLDR_LINKFORMAT" value="-L$(INTERMEDIATE)/lib/ppcmmu -lppcmmu_code -nostdlib -nostartfiles -lgcc -Wl,-e,__start -Wl,-Ttext,0xe00000 -N"/>
	<property name="NTOSKRNL_SHARED" value="-Wl,--file-alignment,0x1000 -Wl,--section-alignment,0x1000 -nostartfiles"/>

	<define name="__MSVCRT__"/>
	<compilerflag>-fshort-wchar</compilerflag>
	<compilerflag>-fsigned-char</compilerflag>
	<compilerflag>-mfull-toc</compilerflag>
	<compilerflag>-meabi</compilerflag>
	<compilerflag>-O2</compilerflag>
	<compilerflag>-Wno-strict-aliasing</compilerflag>

</project>
