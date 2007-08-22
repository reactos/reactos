<?xml version="1.0"?>
<!DOCTYPE project SYSTEM "tools/rbuild/project.dtd">
<project name="ReactOS" makefile="makefile.ppc" xmlns:xi="http://www.w3.org/2001/XInclude">
  <xi:include href="config-ppc.rbuild">
    <xi:fallback>
      <xi:include href="config-ppc.template.rbuild" />
    </xi:fallback>
  </xi:include>

  <xi:include href="ReactOS-generic.rbuild" />

  <property name="BOOTPROG_PREPARE" value="ppc-le2be" />
  <property name="BOOTPROG_FLATFORMAT" value="-O elf32-powerpc -B powerpc:common" />
  <property name="BOOTPROG_LINKFORMAT" value="-melf32ppc --no-omagic -Ttext 0xe00000 -Tdata 0xe10000" />
  <property name="BOOTPROG_COPYFORMAT" value="--only-section=.text --only-section=.data --only-section=.bss -O aixcoff-rs6000" />

  <define name="_M_PPC" />
  <define name="_PPC_" />
  <define name="__PowerPC__" />
  <define name="__MINGW_IMPORT" empty="true" />
  <define name="stdcall" empty="true" />	
  <define name="__stdcall__" empty="true" />
  <define name="fastcall" empty="true" />
  <define name="cdecl" empty="true" />
  <define name="__cdecl__" empty="true" />
  <define name="dllimport" empty="true" />
  <compilerflag>-v</compilerflag>
  <compilerflag>-Wpointer-arith</compilerflag>

</project>
