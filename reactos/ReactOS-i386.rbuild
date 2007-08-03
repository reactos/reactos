<?xml version="1.0"?>
<!DOCTYPE project SYSTEM "tools/rbuild/project.dtd">
<project name="ReactOS" makefile="makefile.auto" xmlns:xi="http://www.w3.org/2001/XInclude">
  <xi:include href="config.rbuild">
    <xi:fallback>
      <xi:include href="config.template.rbuild" />
    </xi:fallback>
  </xi:include>

  <xi:include href="baseaddress.rbuild" />

  <define name="_M_IX86" />
  <define name="_X86_" />
  <define name="__i386__" />
  <define name="_REACTOS_" />
  <if property="MP" value="1">
    <define name="CONFIG_SMP" value="1" />
  </if>
  <if property="DBG" value="1">
    <define name="DBG" value="1" />
	<define name="_SEH_ENABLE_TRACE" />
    <property name="DBG_OR_KDBG" value="true" />
  </if>
  <if property="KDBG" value="1">
    <define name="KDBG" value="1" />
    <property name="DBG_OR_KDBG" value="true" />
  </if>

  <if property="GDB" value="0">
    <if property="OPTIMIZE" value="1">
        <compilerflag>-Os</compilerflag>
        <compilerflag>-ftracer</compilerflag>
        <compilerflag>-momit-leaf-frame-pointer</compilerflag>
        <compilerflag>-mpreferred-stack-boundary=2</compilerflag>
    </if>
    <if property="OPTIMIZE" value="2">
        <compilerflag>-Os</compilerflag>
        <compilerflag>-mpreferred-stack-boundary=2</compilerflag>
    </if>
    <if property="OPTIMIZE" value="3">
        <compilerflag>-O1</compilerflag>
        <compilerflag>-mpreferred-stack-boundary=2</compilerflag>
    </if>
    <if property="OPTIMIZE" value="4">
        <compilerflag>-O2</compilerflag>
        <compilerflag>-mpreferred-stack-boundary=2</compilerflag>
    </if>
    <if property="OPTIMIZE" value="5">
        <compilerflag>-O3</compilerflag>
        <compilerflag>-mpreferred-stack-boundary=2</compilerflag>
    </if>
  </if>

  <compilerflag>-Wno-strict-aliasing</compilerflag>
  <compilerflag>-Wpointer-arith</compilerflag>
  <linkerflag>-enable-stdcall-fixup</linkerflag>

  <include>.</include>
  <include>include</include>
  <include root="intermediate">include</include>
  <include>include/psdk</include>
  <include root="intermediate">include/psdk</include>
  <include>include/dxsdk</include>
  <include>include/crt</include>
  <include>include/ddk</include>
  <include>include/GL</include>
  <include>include/ndk</include>
  <include>include/reactos</include>
  <include root="intermediate">include/reactos</include>
  <include>include/reactos/libs</include>

  <directory name="base">
    <xi:include href="base/base.rbuild" />
  </directory>
  <directory name="boot">
    <xi:include href="boot/boot.rbuild" />
  </directory>
  <directory name="dll">
    <xi:include href="dll/dll.rbuild" />
  </directory>
  <directory name="drivers">
    <xi:include href="drivers/drivers.rbuild" />
  </directory>
  <directory name="hal">
    <xi:include href="hal/hal.rbuild" />
  </directory>
  <directory name="include">
    <xi:include href="include/directory.rbuild" />
  </directory>
  <directory name="lib">
    <xi:include href="lib/lib.rbuild" />
  </directory>
  <directory name="media">
    <xi:include href="media/media.rbuild" />
  </directory>
  <directory name="modules">
    <xi:include href="modules/directory.rbuild" />
  </directory>
  <directory name="ntoskrnl">
    <xi:include href="ntoskrnl/ntoskrnl.rbuild" />
  </directory>
  <directory name="subsystems">
    <xi:include href="subsystems/subsystems.rbuild" />
  </directory>

</project>
