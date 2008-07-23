<?xml version="1.0"?>
<!DOCTYPE project SYSTEM "tools/rbuild/project.dtd">
<project name="ReactOS" makefile="makefile.auto" xmlns:xi="http://www.w3.org/2001/XInclude">
	<xi:include href="config-amd64.rbuild">
		<xi:fallback>
			<xi:include href="config-amd64.template.rbuild" />
		</xi:fallback>
	</xi:include>

	<define name="_M_AMD64" />
	<define name="_AMD64_" />
	<define name="_M_AXP64" />
	<define name="__x86_64__" />
	<define name="_X86AMD64_" />
	<define name="_WIN64" />

	<property name="PLATFORM" value="PC"/>
	<property name="usewrc" value="false"/>
	<property name="WINEBUILD_FLAGS" value="--kill-at"/>
	<property name="NTOSKRNL_SHARED" value="-Wl,--file-alignment,0x1000 -Wl,--section-alignment,0x1000 -nostartfiles -shared"/>
	<linkerflag>-enable-stdcall-fixup</linkerflag>

	<if property="OPTIMIZE" value="1">
		<compilerflag>-Os</compilerflag>
		<compilerflag>-ftracer</compilerflag>
		<compilerflag>-momit-leaf-frame-pointer</compilerflag>
	</if>
	<if property="OPTIMIZE" value="2">
		<compilerflag>-Os</compilerflag>
	</if>
	<if property="OPTIMIZE" value="3">
		<compilerflag>-O1</compilerflag>
	</if>
	<if property="OPTIMIZE" value="4">
		<compilerflag>-O2</compilerflag>
	</if>
	<if property="OPTIMIZE" value="5">
		<compilerflag>-O3</compilerflag>
	</if>

	<compilerflag>-mpreferred-stack-boundary=4</compilerflag>
	<compilerflag>-fno-strict-aliasing</compilerflag>
	<compilerflag>-Wno-strict-aliasing</compilerflag>
	<compilerflag>-Wpointer-arith</compilerflag>
	<compilerflag>-Wno-uninitialized</compilerflag>
	<linkerflag>-enable-stdcall-fixup</linkerflag>
	<linkerflag>-s</linkerflag>
	<linkerflag>-static</linkerflag>

<!-- Here starts <xi:include href="ReactOS-generic.rbuild" /> -->

	<xi:include href="baseaddress.rbuild" />

	<define name="__REACTOS__" />
	<if property="DBG" value="1">
		<define name="DBG">1</define>
		<define name="_SEH_ENABLE_TRACE" />
		<property name="DBG_OR_KDBG" value="true" />
	</if>
	<if property="KDBG" value="1">
		<define name="KDBG">1</define>
		<property name="DBG_OR_KDBG" value="true" />
	</if>

	<include>.</include>
	<include>include</include>
	<include root="intermediate">include</include>
	<include>include/psdk</include>
	<include root="intermediate">include/psdk</include>
	<include>include/dxsdk</include>
	<include root="intermediate">include/dxsdk</include>
	<include>include/crt</include>
	<include>include/crt/mingw32</include>
	<include>include/ddk</include>
	<include>include/GL</include>
	<include>include/ndk</include>
	<include>include/reactos</include>
	<include root="intermediate">include/reactos</include>
	<include root="intermediate">include/reactos/mc</include>
	<include>include/reactos/libs</include>

	<!-- directory name="base">
		<xi:include href="base/base.rbuild" />
	</directory -->

	<directory name="boot">
		<xi:include href="boot/boot.rbuild" />
	</directory>

	<!-- directory name="dll">
		<xi:include href="dll/dll.rbuild" />
	</directory -->
<!--
	<directory name="drivers">
		<directory name="base">
			<directory name="bootvid">
				<xi:include href="drivers/base/bootvid/bootvid.rbuild" />
			</directory>
			<directory name="kdcom">
				<xi:include href="drivers/base/kdcom/kdcom.rbuild" />
			</directory>
		</directory>
	</directory>

	<directory name="hal">
		<xi:include href="hal/hal.rbuild" />
	</directory>
-->
	<directory name="include">
		<xi:include href="include/directory.rbuild" />
	</directory>
	<directory name="lib">
		<xi:include href="lib/lib.rbuild" />
	</directory>

	<!-- directory name="media">
		<xi:include href="media/media.rbuild" />
	</directory -->

	<directory name="ntoskrnl">
		<xi:include href="ntoskrnl/ntoskrnl.rbuild" />
	</directory>

	<!-- directory name="subsystems">
		<xi:include href="subsystems/subsystems.rbuild" />
	</directory -->

	<directory name="tools">
		<xi:include href="tools/tools.rbuild" />
	</directory>

<!-- Here ends <xi:include href="ReactOS-generic.rbuild" /> -->

</project>
