<?xml version="1.0"?>
<!DOCTYPE group SYSTEM "../../../tools/rbuild/project.dtd">
<group>
	<module name="hal_generic_pc" type="objectlibrary">
		<include>include</include>
		<include base="ntoskrnl">include</include>
		<define name="_NTHAL_" />
		<directory name="generic">
			<directory name="bus">
				<file>pcibus.c</file>
			</directory>
		</directory>
	</module>
</group>
