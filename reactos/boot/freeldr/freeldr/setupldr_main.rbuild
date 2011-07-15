<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="setupldr_main" type="objectlibrary" crt="static">
	<include base="setupldr_main">include</include>
	<include base="ntoskrnl">include</include>
	<define name="_NTHAL_" />
	<define name="FREELDR_REACTOS_SETUP" />
	<file>bootmgr.c</file>
	<directory name="inffile">
		<file>inffile.c</file>
	</directory>
	<if property="ARCH" value="i386">
		<directory name="windows">
			<file>setupldr2.c</file>
		</directory>
	</if>
	<if property="ARCH" value="amd64">
		<directory name="windows">
			<file>setupldr2.c</file>
		</directory>
	</if>
</module>
