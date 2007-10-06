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

	<property name="NTOSKRNL_SHARED" value="-Wl,--file-alignment,0x1000 -Wl,--section-alignment,0x1000 -nostartfiles -shared"/>

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

</project>
