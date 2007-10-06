<?xml version="1.0"?>
<!DOCTYPE project SYSTEM "tools/rbuild/project.dtd">
<project name="ReactOS" makefile="makefile.ppc" xmlns:xi="http://www.w3.org/2001/XInclude">
	<xi:include href="config-ppc.rbuild">
		<xi:fallback>
			<xi:include href="config-ppc.template.rbuild" />
		</xi:fallback>
	</xi:include>

	<xi:include href="ReactOS-generic.rbuild" />

	<property name="OFWLDR_LINKFORMAT" value="-L$(INTERMEDIATE)/lib/ppcmmu -lppcmmu_code -nostdlib -nostartfiles -lgcc -Wl,-e,__start -Wl,-Ttext,0xe00000 -N"/>
        <property name="NTOSKRNL_SHARED" value="-Wl,--file-alignment,0x1000 -Wl,--section-alignment,0x1000 -nostartfiles"/>

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
  <define name="__MSVCRT__" empty="true" />
  <define name="__NO_CTYPE_INLINES" />
  <define name="__DECLSPEC_SUPPORTED" />
  <define name="__MINGW_IMPORT">extern</define>
  <define name="_CRTIMP" empty="true" />
  <define name="'__declspec(x)'" empty="true" />
  <compilerflag>-fshort-wchar</compilerflag>
  <compilerflag>-fsigned-char</compilerflag>
  <compilerflag>-mfull-toc</compilerflag>
  <compilerflag>-meabi</compilerflag>
  <compilerflag>-O2</compilerflag>
  <compilerflag>-Wno-strict-aliasing</compilerflag>

</project>
