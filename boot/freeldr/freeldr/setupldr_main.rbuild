<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="setupldr_main" type="objectlibrary">
	<include base="setupldr_main">include</include>
	<include base="ntoskrnl">include</include>
	<define name="_NTHAL_" />
	<define name="FREELDR_REACTOS_SETUP" />
	<group compilerset="gcc">
		<compilerflag>-ffreestanding</compilerflag>
		<compilerflag>-fno-builtin</compilerflag>
		<compilerflag>-fno-inline</compilerflag>
		<compilerflag>-fno-zero-initialized-in-bss</compilerflag>
		<compilerflag>-Os</compilerflag>
	</group>
	<file>bootmgr.c</file>
	<directory name="inffile">
		<file>inffile.c</file>
	</directory>
	<directory name="reactos">
		<file>setupldr.c</file>
	</directory>
	<if property="ARCH" value="i386">
    	<directory name="windows">
            <file>setupldr2.c</file>
        </directory>
    </if>
</module>
