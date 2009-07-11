<?xml version="1.0"?>
<!DOCTYPE project SYSTEM "tools/rbuild/project.dtd">
<project name="ReactOS" makefile="makefile.auto" xmlns:xi="http://www.w3.org/2001/XInclude">
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

	<property name="NTOSKRNL_SHARED" value="-file-alignment=0x1000 -section-alignment=0x1000 -shared"/>
	<property name="PLATFORM" value="PC"/>

	<group compilerset="gcc">
		<if property="OPTIMIZE" value="1">
			<compilerflag>-ftracer</compilerflag>
			<compilerflag>-momit-leaf-frame-pointer</compilerflag>
		</if>
        <compilerflag>-mpreferred-stack-boundary=2</compilerflag>
    </group>

	<group linkerset="ld">
		<linkerflag>-disable-stdcall-fixup</linkerflag>
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
