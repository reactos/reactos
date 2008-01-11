<?xml version="1.0"?>
<!DOCTYPE group SYSTEM "../tools/rbuild/project.dtd">
<group xmlns:xi="http://www.w3.org/2001/XInclude">
	<directory name="hal">
		<xi:include href="hal/hal.rbuild" />
	</directory>
	<if property="ARCH" value="i386">
		<directory name="halx86">
			<xi:include href="halx86/directory.rbuild" />
		</directory>
	</if>
	<if property="ARCH" value="powerpc">
		<directory name="halppc">
			<xi:include href="halppc/directory.rbuild" />
		</directory>
	</if>
</group>
