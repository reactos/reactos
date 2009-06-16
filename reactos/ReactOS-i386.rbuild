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

		<compilerflag>-mpreferred-stack-boundary=2</compilerflag>
		<compilerflag>-fno-strict-aliasing</compilerflag>
		<compilerflag>-Wno-strict-aliasing</compilerflag>
		<compilerflag>-Wpointer-arith</compilerflag>
		<compilerflag>-Wno-multichar</compilerflag>
		<!-- compilerflag>-H</compilerflag>    enable this for header traces -->
	</group>

	<group compilerset="msc">
		<if property="OPTIMIZE" value="1">
			<compilerflag>/O1</compilerflag>
		</if>
		<if property="OPTIMIZE" value="2">
			<compilerflag>/O2</compilerflag>
		</if>
		<if property="OPTIMIZE" value="3">
			<compilerflag>/Ox /GS-</compilerflag>
			<compilerflag>/Ot</compilerflag>
		</if>
		<if property="OPTIMIZE" value="4">
			<compilerflag>/Ox /GS-</compilerflag>
			<compilerflag>/Os</compilerflag>
		</if>
		<if property="OPTIMIZE" value="5">
			<compilerflag>/Ox /GS-</compilerflag>
			<compilerflag>/Os</compilerflag>
			<compilerflag>/Ob2</compilerflag>
			<compilerflag>/GF</compilerflag>
			<compilerflag>/Gy</compilerflag>
		</if>

		<compilerflag>/GS-</compilerflag>
		<compilerflag>/Zl</compilerflag>
		<compilerflag>/Zi</compilerflag>
		<compilerflag>/Wall</compilerflag>
	</group>

	<group linkerset="ld">
		<linkerflag>-disable-stdcall-fixup</linkerflag>
	</group>

	<define name="_USE_32BIT_TIME_T" />
</project>
