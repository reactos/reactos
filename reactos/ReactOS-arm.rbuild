<?xml version="1.0"?>
<!DOCTYPE project SYSTEM "tools/rbuild/project.dtd">
<project name="ReactOS" makefile="makefile.auto" xmlns:xi="http://www.w3.org/2001/XInclude">
	<xi:include href="config-arm.rbuild">
		<xi:fallback>
			<xi:include href="config-arm.template.rbuild" />
		</xi:fallback>
	</xi:include>

	<xi:include href="ReactOS-generic.rbuild" />

	<define name="_ARM_" />
	<define name="__arm__" />

	<include>include/reactos/arm</include>

	<property name="WINEBUILD_FLAGS" value="--kill-at"/>
	<property name="NTOSKRNL_SHARED" value="-Wl,--file-alignment,0x1000 -Wl,--section-alignment,0x1000 -nostartfiles -shared"/>

	<if property="SARCH" value="versatile">
		<define name="BOARD_CONFIG_VERSATILE"/>
	</if>

	<if property="OPTIMIZE" value="1">
		<compilerflag>-Os</compilerflag>
		<compilerflag>-ftracer</compilerflag>
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

	<compilerflag>-Wno-attributes</compilerflag>
	<compilerflag>-fno-strict-aliasing</compilerflag>
	<linkerflag>-s</linkerflag>
	<linkerflag>-lgcc</linkerflag>
	<linkerflag>-static</linkerflag>
</project>
