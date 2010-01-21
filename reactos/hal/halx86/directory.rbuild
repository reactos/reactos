<?xml version="1.0"?>
<!DOCTYPE group SYSTEM "../tools/rbuild/project.dtd">
<group xmlns:xi="http://www.w3.org/2001/XInclude">

	<xi:include href="hal_generic.rbuild" />
	<xi:include href="hal_generic_up.rbuild" />
	<xi:include href="hal_generic_mp.rbuild" />

	<if property="ARCH" value="i386">
		<xi:include href="hal.rbuild" />
		<xi:include href="halxbox.rbuild" />
		<if property="BUILD_MP" value="1">
			<xi:include href="halmps.rbuild" />
		</if>
	</if>

	<if property="ARCH" value="amd64">
		<xi:include href="halamd64.rbuild" />
	</if>

</group>
