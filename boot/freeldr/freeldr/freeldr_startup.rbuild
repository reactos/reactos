<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="freeldr_startup" type="objectlibrary">
	<include base="freeldr_startup">include</include>
	<include base="ntoskrnl">include</include>
	<compilerflag>-fno-inline</compilerflag>
	<compilerflag>-fno-zero-initialized-in-bss</compilerflag>
	<directory name="arch">
		<if property="ARCH" value="i386">
			<directory name="i386">
				<file first="true">fathelp.asm</file>
				<file>arch.S</file>
			</directory>
		</if>
	</directory>
</module>
