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
	<linkerflag>-disable-stdcall-fixup</linkerflag>

</project>
