<?xml version="1.0"?>
<!DOCTYPE project SYSTEM "tools/rbuild/project.dtd">
<project name="ReactOS" makefile="makefile-amd64.auto" xmlns:xi="http://www.w3.org/2001/XInclude" allowwarnings="true">
	<xi:include href="config-amd64.rbuild">
		<xi:fallback>
			<xi:include href="config-amd64.template.rbuild" />
		</xi:fallback>
	</xi:include>

	<xi:include href="ReactOS-generic.rbuild" />

	<define name="_M_AMD64" />
	<define name="_AMD64_" />
	<define name="_M_AXP64" />
	<define name="__x86_64__" />
	<!-- define name="_X86AMD64_" / FIXME: what is this used for? -->
	<define name="_WIN64" />
	<define name="TARGET_amd64" host="true" />

	<define name="USE_COMPILER_EXCEPTIONS" />

	<property name="PLATFORM" value="PC"/>
	<property name="usewrc" value="false"/>
	<property name="WINEBUILD_FLAGS" value="--kill-at"/>

	<group compilerset="gcc">
		<if property="OPTIMIZE" value="1">
			<compilerflag>-ftracer</compilerflag>
			<compilerflag>-momit-leaf-frame-pointer</compilerflag>
		</if>
		<compilerflag>-fms-extensions</compilerflag>
		<compilerflag>-mpreferred-stack-boundary=4</compilerflag>
		<compilerflag compiler="midl">-m64 --win64</compilerflag>
		<!-- compilerflag compiler="cc,cxx">-gstabs+</compilerflag -->
		<!-- compilerflag compiler="as">-gstabs+</compilerflag -->
		<compilerflag>-U_X86_</compilerflag>
		<compilerflag>-Wno-format</compilerflag>
		<compilerflag>-fno-leading-underscore</compilerflag>
	</group>

	<group linkerset="ld">
		<linkerflag>-disable-stdcall-fixup</linkerflag>
		<linkerflag>-file-alignment=0x1000</linkerflag>
		<linkerflag>-section-alignment=0x1000</linkerflag>
		<linkerflag>--unique=.eh_frame</linkerflag>
		<linkerflag>-static</linkerflag>
		<linkerflag>-fno-leading-underscore</linkerflag>
		<linkerflag>-shared</linkerflag>
		<linkerflag>--exclude-all-symbols</linkerflag>
	</group>

	<if property="USERMODE" value="1">
		<directory name="base">
			<xi:include href="base/base.rbuild" />
		</directory>
		<directory name="dll">
			<xi:include href="dll/dll.rbuild" />
		</directory>
	</if>

	<directory name="boot">
		<xi:include href="boot/boot.rbuild" />
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
