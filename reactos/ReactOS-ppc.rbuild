<?xml version="1.0"?>
<!DOCTYPE project SYSTEM "tools/rbuild/project.dtd">
<project name="ReactOS" makefile="makefile.ppc" xmlns:xi="http://www.w3.org/2001/XInclude">
	<xi:include href="config-ppc.rbuild">
		<xi:fallback>
			<xi:include href="config-ppc.template.rbuild" />
		</xi:fallback>
	</xi:include>

	<xi:include href="ReactOS-generic.rbuild" />

	<property name="OFWLDR_LINKFORMAT" value="-L$(INTERMEDIATE)/lib/ppcmmu -lppcmmu_code -lppcmmu -nostdlib -nostartfiles"/>

	<define name="_M_PPC" />
	<define name="_PPC_" />
	<define name="__PowerPC__" />
	<define name="_REACTOS_" />
	<define name="stdcall" empty="true" />
	<define name="__stdcall__" empty="true" />
	<define name="fastcall" empty="true" />
	<define name="cdecl" empty="true" />
	<define name="__cdecl__" empty="true" />
	<define name="dllimport" empty="true" />
	<define name="WORDS_BIGENDIAN" empty="true" />
	<define name="MB_CUR_MAX">1</define>
	<define name="_BSDTYPES_DEFINED"/>
	<compilerflag>-fshort-wchar</compilerflag>
	<compilerflag>-fsigned-char</compilerflag>
	<compilerflag>-mfull-toc</compilerflag>
	<compilerflag>-meabi</compilerflag>
	<compilerflag>-O2</compilerflag>
	<compilerflag>-Wno-strict-aliasing</compilerflag>
	<compilerflag>-Wno-trampolines</compilerflag>

</project>
