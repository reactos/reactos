<?xml version="1.0"?>
<!DOCTYPE group SYSTEM "../../../tools/rbuild/project.dtd">
<group>
	<module name="hal_generic_up" type="objectlibrary">
		<include>include</include>
		<include base="ntoskrnl">include</include>
		<define name="_NTHAL_" />
		<directory name="generic">
			<file>spinlock.c</file>
		</directory>
		<directory name="up">
			<file>processor.c</file>
			<if property="ARCH" value="i386">
				<file>irq.S</file>
			</if>
		</directory>
	</module>
</group>
