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
		<if property="BUILD_MP" value="1">
			<xi:include href="ntoskrnl/ntkrnlmp.rbuild" />
		</if>
	</directory>
	<directory name="subsystems">
		<xi:include href="subsystems/subsystems.rbuild" />
	</directory>
	<directory name="tools">
		<xi:include href="tools/tools.rbuild" />
	</directory>
</project>
