<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="wintrust_winetest" type="win32cui" installbase="bin" installname="wintrust_winetest.exe" allowwarnings="true">
	<include base="wintrust_winetest">.</include>
	<define name="__ROS_LONG64__" />
	<file>asn.c</file>
	<file>crypt.c</file>
	<file>register.c</file>
	<file>softpub.c</file>
	<file>testlist.c</file>
	<library>wine</library>
	<library>wintrust</library>
	<library>crypt32</library>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
</group>
