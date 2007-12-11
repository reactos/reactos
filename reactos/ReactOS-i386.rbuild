<?xml version="1.0"?>
<!DOCTYPE project SYSTEM "tools/rbuild/project.dtd">
<project name="ReactOS" makefile="makefile.auto" architecture="i386" xmlns:xi="http://www.w3.org/2001/XInclude">
	<xi:include href="config.rbuild">
		<xi:fallback>
			<xi:include href="config.template.rbuild" />
		</xi:fallback>
	</xi:include>

	<xi:include href="ReactOS-generic.rbuild" />

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


	<overridemodule name="ntoskrnl" allowwarnings="true">
		<define name="SILLY_DEFINE" />
	</overridemodule>

</project>
