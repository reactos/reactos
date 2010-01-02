<?xml version="1.0"?>
<!DOCTYPE group SYSTEM "../../../tools/rbuild/project.dtd">
<group>
	<module name="hal_generic_mp" type="objectlibrary">
		<include>include</include>
		<include base="ntoskrnl">include</include>
		<define name="_NTHAL_" />
		<define name="CONFIG_SMP" />
		<directory name="generic">
			<file>spinlock.c</file>
		</directory>
		<directory name="mp">
			<if property="ARCH" value="i386">
				<file>irq.S</file>
			</if>
		</directory>
	</module>
</group>
