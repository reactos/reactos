<?xml version="1.0"?>
<!DOCTYPE project SYSTEM "tools/rbuild/project.dtd">
<project name="ReactOS" makefile="makefile-i386.auto" xmlns:xi="http://www.w3.org/2001/XInclude">
	<xi:include href="config.rbuild">
		<xi:fallback>
			<xi:include href="config.template.rbuild" />
		</xi:fallback>
	</xi:include>

	<xi:include href="ReactOS-generic.rbuild" />

	<define name="_M_IX86" />
	<define name="_X86_" />
	<define name="__i386__" />
	<define name="TARGET_i386" host="true" />

	<define name="USE_COMPILER_EXCEPTIONS" />
	<define name="_USE_32BIT_TIME_T" />

	<property name="PLATFORM" value="PC"/>

	<group compilerset="gcc">
		<if property="OPTIMIZE" value="1">
			<compilerflag>-ftracer</compilerflag>
			<compilerflag>-momit-leaf-frame-pointer</compilerflag>
		</if>
		<compilerflag>-fms-extensions</compilerflag>
		<compilerflag>-mpreferred-stack-boundary=2</compilerflag>
		<compilerflag compiler="midl">-m32 --win32</compilerflag>
		<compilerflag compiler="cc,cxx">-gstabs+</compilerflag>
		<compilerflag compiler="cc,cxx">-fno-set-stack-executable</compilerflag>
		<compilerflag compiler="cc,cxx">-fno-optimize-sibling-calls</compilerflag>
		<compilerflag compiler="as">-gstabs+</compilerflag>
	</group>

	<group linkerset="ld">
		<linkerflag>-disable-stdcall-fixup</linkerflag>
		<linkerflag>-file-alignment=0x1000</linkerflag>
		<linkerflag>-section-alignment=0x1000</linkerflag>
	</group>

	<directory name="sdk">
		<xi:include href="sdk/sdk.rbuild" />
	</directory>
</project>
