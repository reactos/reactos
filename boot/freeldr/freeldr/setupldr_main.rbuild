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
	<directory name="windows">
		<file>setupldr.c</file>
	</directory>
</module>
