<?xml version="1.0"?>
<!DOCTYPE group SYSTEM "../../../tools/rbuild/project.dtd">
<group>
	<module name="hal_generic_up" type="objectlibrary">
		<include>include</include>
		<include base="ntoskrnl">include</include>
		<define name="_DISABLE_TIDENTS" />
		<define name="_NTHAL_" />
		<directory name="generic">
			<file>irq.S</file>
			<file>processor.c</file>
			<file>spinlock.c</file>
		</directory>
	</module>
</group>
